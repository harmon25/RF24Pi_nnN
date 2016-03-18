#include <cstring>
#include <istream>
#include <streambuf>
#include <string>
#include <iostream>
#include <csignal>
#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <json/json.h>
#include <nanomsg/nn.h>
#include <nanomsg/pair.h>
#include <RF24/RF24.h>
#include <RF24Network/RF24Network.h>
#include <getopt.h>

using namespace std;

#define DAEMON_NAME "RF24d"

Json::StyledWriter styledWriter;
Json::Reader reader;
Json::FastWriter fastWriter;
Json::CharReaderBuilder builder;

int s;

static void show_usage(string name)
{
    cerr << "Usage: " << name << " <option(s)>\n"
              << "Options:\n"
              << "\t-h,--help - Show this help message\n"
              << "\t-i,--id NODEID - Set Node ID - Parsed as octal: 00\n"
              << "\t-c,--channel CHANNEL# - Set RF24 Channel - 1-255: 115\n"
              << "\t-r,--rate RATE - Set Data Rate [0-2] - 1_MBPS|2_MBPS|250KBPS: 0\n"
              << "\t-p,--level POWER - Set PA Level [0-4] - RF24_PA_MIN|RF24_PA_LOW|RF24_PA_HIGH|RF24_PA_MAX|RF24_PA_ERROR: 3\n"
              << endl;
}

void signalHandler( int );
void logMsg(string = "", string = "", string = "error"); 
void Rf24d(uint16_t, uint8_t , rf24_datarate_e, rf24_pa_dbm_e );
int sendBackErr(int nn_socket, string error_for_ex, Json::Value failed_msg);


int main(int argc, char *argv[])
{
 
  rf24_pa_dbm_e paLevel = (rf24_pa_dbm_e)3;;
  rf24_datarate_e dataRate = (rf24_datarate_e)0;
  int this_node_id = 00;
  uint8_t channel = 115;
  
  int option_char = 0;
  //variables to store cmdline params
  string this_node_id_arg;
  int channel_arg = -1;
  int dataRate_arg = -1;
  int paLevel_arg = -1;
  
  // Invokes member function `int operator ()(void);'
  while ((option_char = getopt(argc, argv,"i:c:p:r:")) != -1) {
    switch (option_char)
      {  
         case 'i': this_node_id_arg = optarg;
                   break;
         case 'c': channel_arg =  atoi (optarg);
                   break;
         case 'p': paLevel_arg = atoi (optarg);
                   break;
         case 'r': dataRate_arg = atoi (optarg);
                   break;
         default: show_usage(argv[0]);
                  return 0;
      }
  }
  
  // set non-default nodeId
  if(this_node_id_arg.size() > 0){
    this_node_id = atoi(this_node_id_arg.c_str());
  } 

  // set non-default channel
  if(channel_arg > 0 && channel_arg <= 125){
    channel = (uint8_t)channel_arg;
  } else if(channel_arg > 125 ) {
    logMsg("invalid_channel" , to_string(channel_arg));
    return 0;
  } 

  // set non-default PaLevel
  if(paLevel_arg > 0){
    paLevel = (rf24_pa_dbm_e)paLevel_arg;
  }

  // set non-default dataRate
  if(dataRate_arg > 0){
    dataRate = (rf24_datarate_e)dataRate_arg;
  } 

  logMsg("config", "dataRate" , to_string(dataRate));
  logMsg("config", "paLevel" , to_string(paLevel));
  logMsg("config", "this_node_id" , to_string(this_node_id));
  logMsg("config", "channel" , to_string(channel));

  setlogmask(LOG_UPTO(LOG_NOTICE));
  openlog(DAEMON_NAME, LOG_CONS | LOG_NDELAY | LOG_PERROR | LOG_PID, LOG_USER);
  
   pid_t pid, sid;
   //Fork the Parent Process
   pid = fork();

    if (pid < 0) { exit(EXIT_FAILURE); }

    //We got a good pid, Close the Parent Process
    if (pid > 0) { exit(EXIT_SUCCESS); }

    //Change File Mask
    umask(0);

    //Create a new Signature Id for our child
    sid = setsid();
    if (sid < 0) { exit(EXIT_FAILURE); }

    //Change Directory
    //If we cant find the directory we exit with failure.
    if ((chdir("/")) < 0) { exit(EXIT_FAILURE); }

    //Close Standard File Descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // register signal handler
    signal(SIGTERM, signalHandler);
    logMsg("fork", "process_forked" , to_string(pid));
    // start loop
    Rf24d((uint16_t)this_node_id, channel, dataRate, paLevel);
}

void signalHandler( int signum )
{
    Json::Value sig_log;
    sig_log["type"] = "signal";
    sig_log["info"] = signum;
    sig_log["msg"] = "Terminating RF24d";
    string sig_log_str = fastWriter.write(sig_log);
    nn_close(s);
    syslog (LOG_NOTICE, sig_log_str.c_str());
    closelog ();
    exit(signum);  
}

void Rf24d(uint16_t this_node_id, uint8_t channel, rf24_datarate_e dataRate, rf24_pa_dbm_e paLevel )
{
  // Address of our node in Octal format (01,021, etc)
  string ipc_addr = "ipc:///tmp/rf24d.ipc";
  // can only recieve max 24 bytes from RF24Network, so cap payloadmsg size at that
  struct payload_json {
     char msg[24];
  };
  logMsg("start_up", "starting RF24d");
  RF24 radio(22, 0);
  RF24Network network(radio);
  // create and connect nanomsg pair socket
  s = nn_socket(AF_SP, NN_PAIR);
  nn_connect(s, ipc_addr.c_str());
  if(radio.begin()){
    radio.setDataRate(dataRate);
    radio.setPALevel(paLevel);
    radio.setChannel(channel);
    delay(100);
    if(network.is_valid_address(this_node_id)){
      logMsg("network_begin", "with_node_id", to_string(this_node_id));
      network.begin(this_node_id);
      delay(50);
      //radio.printDetails();
    } else {
       logMsg("network_begin", "invalid node address");
    }
  } else {
     logMsg("radio_begin", "rf24 radio begin, false");
  }
  
  void* buf = NULL;
  int count;
  while (1){
    network.update();
    count = nn_recv(s, &buf, NN_MSG, NN_DONTWAIT); //check if we have any messages from elixir
    if(count != -1 && count > 0 ){  // if there is not an error and count > 0 we have a message from elixir
      string rawjsonMsg = (const char*)buf; 
      nn_freemsg(buf);
      Json::Value jsonMessage;
      if (reader.parse(rawjsonMsg, jsonMessage)){
        int msgType = jsonMessage.get("type", 1).asInt();
        string to_node_id_str = jsonMessage.get("node_id", "01").asString();
        uint16_t to_node_id_oct;
        std::istringstream(to_node_id_str) >> std::oct >> to_node_id_oct; // make sure node id is octal, sent as string 
        if(network.is_valid_address(to_node_id_oct)){
          string strMsg = jsonMessage.get("msg", "ok").asString();
          payload_json payload;
          strcpy(payload.msg, strMsg.data());
          RF24NetworkHeader header(/*to node*/ to_node_id_oct , /*msg type, 0-255 */ (unsigned char)msgType); //create RF24Network header
          if (network.write(header,&payload,sizeof(payload))){
            logMsg(to_string(msgType), to_node_id_str, "sent_to_node");
          } else { 
            //err
            sendBackErr(s,"failed_to_node", jsonMessage );
            logMsg("failed_to_node", to_node_id_str);
          }      
        } else {
          sendBackErr(s,"invalid_rf24_addr", jsonMessage );
          logMsg("invalid_rf24_addr", to_node_id_str);
        }  
      } else {
         sendBackErr(s,"invalid_nn_msg_json", jsonMessage);
         logMsg("invalid_nn_msg_json", rawjsonMsg);
      }      
    }

    if( network.available() ){   // we have a message from the sensor network
      int payloadSize = 24;
      RF24NetworkHeader header;   
      payload_json payloadJ;
      Json::Value jsonRFM;
      //uint16_t this_msg_type = network.peek(header); 
      network.read(header,&payloadJ, payloadSize);
      jsonRFM["from_node"] = header.from_node;
      jsonRFM["type"] = header.type;
      Json::Value fromArduino; // new json to parse arduino msg into
      istringstream iss; // stream object to temporarily hold buffer
      iss.str (payloadJ.msg); // move the message onto the string stream
      builder["collectComments"] = false;
      string errs;
      bool parsingSuccessful = Json::parseFromStream(builder, iss, &fromArduino, &errs); //place parsed json in parsedFromString
      if (parsingSuccessful)
      {
        jsonRFM["msg"] = fromArduino;
        string str_for_elixir = fastWriter.write(jsonRFM); // write json into string
        int nbytesent = nn_send(s, str_for_elixir.c_str(), str_for_elixir.size(), NN_DONTWAIT);
        // log
        logMsg("elixir", to_string(nbytesent), "sent_bytes");
      } else {
        //err
        logMsg("parse_arduino_msg", errs);
      }
    } 
    delay(50);
  } // restart loop
} 

void logMsg(string info, string msg, string type){
  Json::Value log_msg;
  log_msg["type"] = type;
  log_msg["info"] = info;
  log_msg["msg"] = msg;
  string log_str = fastWriter.write(log_msg);
  syslog (LOG_NOTICE, log_str.c_str());
}



int sendBackErr(int nn_socket, string error_for_ex, Json::Value failed_msg)
{
  Json::Value jsonErrMessage;
  jsonErrMessage["error"] = error_for_ex;
  jsonErrMessage["details"] = failed_msg;
  string err_for_elixir = fastWriter.write(jsonErrMessage); // write json into string
  return nn_send(nn_socket, err_for_elixir.c_str(), err_for_elixir.size(), NN_DONTWAIT);
}