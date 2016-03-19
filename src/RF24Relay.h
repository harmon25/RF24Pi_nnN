#ifndef RF24RELAY_H
  #define RF24RELAY_H
  

  #include <system_error>
  #include <nnxx/message.h>
  #include <nnxx/message_istream.h>
  #include <nnxx/pair.h>
  #include <nnxx/socket.h>

  #include "rapidjson/document.h"
  #include "rapidjson/writer.h"
  #include "rapidjson/stringbuffer.h"
  #include "rapidjson/error/en.h"

  #include <RF24/RF24.h>
  #include <RF24Network/RF24Network.h>
  
  #include <csignal>
  #include <syslog.h>
  #include <errno.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include <iostream>
  #include <sstream>  

  void Rf24Relay(uint16_t, uint8_t , rf24_datarate_e, rf24_pa_dbm_e );
  void logMsg(std::string = "", std::string = "", std::string = "error"); 
  void sendBackErr(nnxx::socket &, std::string);
  void signalHandler( int signum );

#endif 