#pragma once
#include <string>
#include <sstream>
#include "mosquitto.h"
#include "CLogging.h"
#include "inireader/INIReader.h"

#define EXIT_TERMINATE 1 
#define EXIT_RESET 2

using namespace std;

extern CLogging *log;

class CNHmqtt
{
  public:
    CNHmqtt(int argc, char *argv[]);
    ~CNHmqtt();
    string get_topic();
    struct mosquitto *mosq; //needs to be public so can be accessed by static callback
    void dbg(string msg);
    int message_loop(void);
    int message_loop(int timeout);
    static int daemonize();
         
    int  mosq_connect();
    virtual void process_message(string topic, string message);
   
    int subscribe(string topic);
    bool mosq_connected;
    static bool debug_mode;
    static bool daemonized;
    static string itos(int n);
    
  protected:
    string mqtt_topic;
    string mqtt_rx;
    string mqtt_tx;
    string status; // returned in response to status mqtt message (so should be short!)
    string mosq_server;
    int mosq_port;

    int message_send(string topic, string message);
    int message_send(string topic, string message, bool no_debug);
    int get_int_option(string section, string option, int def_value);
    string get_str_option(string section, string option, string def_value);
        
  private:
    static void connect_callback(struct mosquitto *mosq, void *obj, int result);
    static void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message);  
    bool terminate;
    bool reset;
    bool config_file_parsed;
    bool no_staus_debug;
    INIReader *reader;
    uid_t uid;
    pthread_mutex_t mosq_mutex;
};


