#include "RF24d.h"

#include "RF24Relay.h"

using namespace std;

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

int main(int argc, char *argv[])
{
 
  //variables to store cmdline params
  string this_node_id_arg;
  int channel_arg = -1;
  int dataRate_arg = -1;
  int paLevel_arg = -1;
  
  int option_char = 0;
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
  
  uint16_t this_node_id_oct = 00;
  // set non-default nodeId
  if(this_node_id_arg.size() > 0){
    std::istringstream(this_node_id_arg) >> std::oct >> this_node_id_oct; // make sure node id is octal, sent as string 
  }
  
  uint8_t channel = 115;
  // set non-default channel
  if(channel_arg > 0 && channel_arg <= 125){
    channel = (uint8_t)channel_arg;
  } else if(channel_arg > 125 ) {
    cout << "Invalid Channel - enter channel from 1-125" << endl;
    return 0;
  } 

  rf24_pa_dbm_e paLevel = (rf24_pa_dbm_e)3;
  // set non-default PaLevel
  if(paLevel_arg > 0){
    paLevel = (rf24_pa_dbm_e)paLevel_arg;
  }

   rf24_datarate_e dataRate = (rf24_datarate_e)0;
  // set non-default dataRate
   if(dataRate_arg > 0){
    dataRate = (rf24_datarate_e)dataRate_arg;
  } 

  setlogmask(LOG_UPTO(LOG_NOTICE));
  openlog(DAEMON_NAME, LOG_CONS | LOG_NDELAY | LOG_PERROR | LOG_PID, LOG_USER);
  
  // pid_t pid, sid;
   //Fork the Parent Process
  // pid = fork();

   // if (pid < 0) { exit(EXIT_FAILURE); }

    //We got a good pid, Close the Parent Process
  //  if (pid > 0) { exit(EXIT_SUCCESS); }

    //Change File Mask
  //  umask(0);

    //Create a new Signature Id for our child
  //  sid = setsid();
 //   if (sid < 0) { exit(EXIT_FAILURE); }

    //Change Directory
    //If we cant find the directory we exit with failure.
  //  if ((chdir("/")) < 0) { exit(EXIT_FAILURE); }

    //Close Standard File Descriptors
  //  close(STDIN_FILENO);
 //   close(STDOUT_FILENO);
 //   close(STDERR_FILENO);

    //logMsg("fork", "process_forked" , to_string(pid));
  
    // start loop
    Rf24Relay(this_node_id_oct, channel, dataRate, paLevel);
}



