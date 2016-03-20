#include "RF24Relay.h"

using namespace rapidjson;
using namespace std;

void Rf24Relay(uint16_t this_node_id, uint8_t channel, rf24_datarate_e dataRate, rf24_pa_dbm_e paLevel )
{
  signal(SIGTERM, signalHandler);
  signal(SIGINT, signalHandler);
  string ipc_addr = "ipc:///tmp/rf24d.ipc";

  logMsg("dataRate", to_string(dataRate), "config");
  logMsg("paLevel", to_string(paLevel), "config");
  logMsg("this_node_id", to_string(this_node_id), "config");
  logMsg("channel", to_string(channel), "config");

  logMsg("OK", "starting RF24d", "start_up");
  
  // start radio
  RF24 radio(22, 0);
  RF24Network network(radio);
  
  // create and connect nanomsg pair socket
  nnxx::socket nn_sock { nnxx::SP, nnxx::PAIR };
  nn_sock.connect(ipc_addr.c_str());

  if(radio.begin()){
    radio.setDataRate(dataRate);
    radio.setPALevel(paLevel);
    radio.setChannel(channel);
    delay(250);
    if(network.is_valid_address(this_node_id)){
      logMsg("network_begin", "with_node_id", to_string(this_node_id));
      network.begin(this_node_id);
      delay(100);
      //start looping
      while (1){
        network.update(); //keep network up
        // handing incoming messages from nn socket
        handleIncomingNNMsg(nn_sock, network);
  
        // handing incoming messages from RF24 network
        if( network.available() ){   // we have a message from the sensor network
           handleIncomingRF24Msg(nn_sock, network);
        } 
      } // restart loop
    } else {
       logMsg("network_begin", "invalid node address");
    }
  } else {
     logMsg("radio_begin", "rf24 radio begin, false");
  }
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

void sendBackErr(nnxx::socket &to, string error_for_ex)
{
  Document nn_out_err;
  nn_out_err.SetObject();
  Document::AllocatorType& allocator = nn_out_err.GetAllocator();
  nn_out_err.AddMember("error",Value(error_for_ex.c_str(), error_for_ex.size(), allocator).Move(), allocator);
  StringBuffer out_s_buf;
  Writer<StringBuffer> writer(out_s_buf);
  nn_out_err.Accept(writer);
  to.send(out_s_buf.GetString(), nnxx::DONTWAIT);
}


void handleIncomingRF24Msg(nnxx::socket &nn_socket, RF24Network &net) //pass socket and network through as reference
{
  RF24NetworkHeader header; // create header
  Document in_rf_msg;       // create document for incoming message
  in_rf_msg.SetObject();  
  Document::AllocatorType& allocator = in_rf_msg.GetAllocator();  // grab allocator
  char payloadJ[144]; //max payload for rf24 network fragment, could be sent over multiple packets
  size_t readP = net.read(header,&payloadJ, sizeof(payloadJ));
  string payloadStr(payloadJ); // create string object to strip nonsense off buffer
  if(payloadStr.size() != readP){ payloadStr.resize(readP); } // resize payloadStr 
  //cout << payloadStr << endl; 
  //in_rf_msg.AddMember("msg_id", Value().SetInt(header.id) , allocator); 
  in_rf_msg.AddMember("from_node", Value().SetInt(header.from_node) , allocator); // add integer to incoming msg json: from_node
  in_rf_msg.AddMember("type", Value().SetInt(header.type), allocator);            // add integer to incoming msg json: type 
  in_rf_msg.AddMember("msg", Value(payloadStr.c_str(), payloadStr.size(), allocator).Move(), allocator);
  //write json to string
  StringBuffer buffer;
  Writer<StringBuffer> writer(buffer);
  in_rf_msg.Accept(writer);
  // send encoded string over nn socket
  int sentBytes = nn_socket.send(buffer.GetString(), nnxx::DONTWAIT);
  if(sentBytes > 0){ //log
     logMsg("sent", buffer.GetString(), "to_nn");
  }
}

void handleIncomingNNMsg(nnxx::socket &nn_socket, RF24Network &net) //pass socket and network through as reference
{
  try {
    stringstream errMsg;
    nnxx::message_istream msg { nn_socket.recv(nnxx::DONTWAIT) }; //try and grab message
    std::string msg_str(std::istream_iterator<char>(msg), {});  // turn whatever we got into a string
    if(msg_str.size() > 0 ){  // if we acually got something, size > 0
      Document nn_in_msg; // create doc to store message if its a json
      try {
        if (nn_in_msg.Parse(msg_str.c_str()).HasParseError() == false){ // we have a json    
          uint16_t to_node_id_oct;      
          std::istringstream(nn_in_msg["to_node"].GetString()) >> std::oct >> to_node_id_oct; // make sure node id is octal, sent as string 
          if(net.is_valid_address(to_node_id_oct)){   // only send if we have a valid address
            string msgPayload(nn_in_msg["msg"].GetString());      //grab payload put it in a string
            unsigned char msg_type = nn_in_msg["type"].GetInt();  // grap message type as unsigned char 
            char payload[144]; // max payload for rf24 network fragment, could be sent over multiple packets
            strcpy(payload, msgPayload.data()); // copy data into payload char, will probably segfault if over 144 char...
            RF24NetworkHeader header( to_node_id_oct , msg_type ); //create RF24Network header
            size_t bytesSent = net.write(header, &payload, sizeof(payload));
            if (bytesSent > 0) {
              logMsg(to_string(msg_type), to_string(to_node_id_oct), "to_rf24");
            } else { 
              errMsg << "FAILED_TO_NODE: " << to_node_id_oct;
              throw errMsg.str();
            }      
          } else {
            errMsg << "INVALID_RF24_ADDR " << to_node_id_oct;
            throw errMsg.str();
          }  
        } else {
          errMsg << "RAPID_JSON_PARSE_ERROR: " << msg_str;
          throw errMsg.str();
        }      
      } catch(string &e){ // catch RF24d errors
        sendBackErr(nn_socket, e);
        logMsg("RF24d_ERR", e);
      }   
    } 
  } catch(const std::system_error &e) { // catch nn errors
    std::cerr << e.what() << std::endl;
  }
}


void signalHandler( int signum )
{
  syslog (LOG_NOTICE, "Closing RF24d!");
  closelog ();
  exit(signum);  
}
