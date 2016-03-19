#include "RF24Relay.h"

using namespace rapidjson;
using namespace std;

int nn_sock;

void Rf24Relay(uint16_t this_node_id, uint8_t channel, rf24_datarate_e dataRate, rf24_pa_dbm_e paLevel )
{
  // register signal handler
  signal(SIGTERM, signalHandler);
  // Address of our node in Octal format (01,021, etc)
  string ipc_addr = "ipc:///tmp/rf24d.ipc";
  // can only recieve max 24 bytes from RF24Network, so cap payloadmsg size at that

  logMsg("dataRate", to_string(dataRate), "config");
  logMsg("paLevel", to_string(paLevel), "config");
  logMsg("this_node_id", to_string(this_node_id), "config");
  logMsg("channel", to_string(channel), "config");

  logMsg("OK", "starting RF24d", "start_up");
  
  // start radio
  RF24 radio(22, 0);
  RF24Network network(radio);
  
  // create and connect nanomsg pair socket
  //nn_sock = nn_socket(AF_SP, NN_PAIR);
  nnxx::socket nn_sock { nnxx::SP, nnxx::PAIR };
  nn_sock.connect(ipc_addr.c_str());

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

  while (1){
    int gotErr = -1;
    stringstream errMsg;
    network.update(); //keep network up
    nnxx::message_istream msg { nn_sock.recv(nnxx::DONTWAIT) }; //try and grab message
    std::string msg_str(std::istream_iterator<char>(msg), {});  // turn whatever we got into a string
    if(msg_str.size() > 0 ){  // if we acually got something, size > 0
      Document nn_in_msg; // create doc to store message if its a json
      if (nn_in_msg.Parse(msg_str.c_str()).HasParseError() == false){ // we have a json    
        uint16_t to_node_id_oct;      
        std::istringstream(nn_in_msg["to_node"].GetString()) >> std::oct >> to_node_id_oct; // make sure node id is octal, sent as string 
        if(network.is_valid_address(to_node_id_oct)){
          string msgPayload(nn_in_msg["msg"].GetString());      //grab payload put it in a string
          unsigned char msg_type = nn_in_msg["type"].GetInt();  // grap message type as unsigned char 
          char payload[24]; // 
          strcpy(payload, msgPayload.data());
          RF24NetworkHeader header(/*to node*/ to_node_id_oct , /*msg type, 0-255 */ msg_type ); //create RF24Network header
          if (network.write(header,&payload, sizeof(payload))) {
            logMsg(to_string(msg_type), to_string(to_node_id_oct), "sent_to_node");
          } else { 
            gotErr = 1;
            errMsg << "failed_to_node ";
            logMsg("failed_to_node", to_string(to_node_id_oct));
          }      
        } else {
          gotErr = 1;
          errMsg << "invalid_rf24_addr ";
          logMsg(errMsg.str(), to_string(to_node_id_oct));
        }  
      } else {
        gotErr = 1;
        errMsg << "RAPID_JSON_PARSE_ERROR ";
        logMsg(errMsg.str(), msg_str);
      }      
    
      if(gotErr > 0){
        cout << errMsg << endl;
        //sendBackErr(nn_sock, errMsg, nn_in_msg);
      }
    }
    
   
  

    if( network.available() ){   // we have a message from the sensor network
      RF24NetworkHeader header; // create header
      Document in_rf_msg;       // create document for incoming message
      in_rf_msg.SetObject();  
      Document::AllocatorType& allocator = in_rf_msg.GetAllocator();  // grab allocator
      char payloadJ[24]; // make 24byte char to store payload
      network.read(header,&payloadJ, sizeof(payloadJ));
      string payloadStr(payloadJ); // create string object to strip nonsense off buffer
      in_rf_msg.AddMember("from_node", Value().SetInt(header.from_node) , allocator); // add integer to incoming msg json: from_node
      in_rf_msg.AddMember("type", Value().SetInt(header.type), allocator);            // add integer to incoming msg json: type 
      in_rf_msg.AddMember("msg", Value(payloadStr.c_str(), payloadStr.size(), allocator).Move(), allocator);
      StringBuffer buffer;
      Writer<StringBuffer> writer(buffer);
      in_rf_msg.Accept(writer);
      int sentBytes = nn_sock.send(buffer.GetString(), nnxx::DONTWAIT);
      if(sentBytes > 0){ //log
          logMsg("sent", buffer.GetString(), "to_elixir");
        }
    } 
    delay(150);
  } // restart loop
} 

void logMsg(string info, string msg, string type){

  Document log_msg;
  log_msg.SetObject();
  Document::AllocatorType& allocator = log_msg.GetAllocator();
  log_msg.AddMember("info", Value(info.c_str(), info.size(), allocator).Move() , allocator);
  log_msg.AddMember("type", Value(type.c_str(), type.size(), allocator).Move(), allocator);
  log_msg.AddMember("msg", Value(msg.c_str(), msg.size(), allocator).Move(), allocator);

  StringBuffer buffer;
  Writer<StringBuffer> writer(buffer);
  log_msg.Accept(writer);


  syslog (LOG_NOTICE, buffer.GetString());
}

/*
void sendBackErr(int &nn_socket, string error_for_ex, rapidjson::Document &failed_msg)
{
  Document nn_out_err;
  nn_out_err.SetObject();
  nn_out_err.AddMember("error", error_for_ex, nn_out_err.GetAllocator());
  nn_out_err.AddMember("details", failed_msg, nn_out_err.GetAllocator());
  StringBuffer out_s_buf;
  Writer<StringBuffer> writer(out_s_buf);
  nn_out_err.Accept(writer);
  nn_send(nn_socket, out_s_buf.GetString() , sizeof(out_s_buf.GetString()), NN_DONTWAIT);
}
*/

void signalHandler( int signum )
{
    syslog (LOG_NOTICE, "Closing RF24d@");

    closelog ();
    exit(signum);  
}