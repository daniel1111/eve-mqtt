bool CNHmqtt::debug_mode = false;
bool CNHmqtt::daemonized = false;

#include "CI2C.h"
#include "Ctmp100.h"
#include "CSrf.h"

class CEve : public CNHmqtt
{

  public:
    CEve(int argc, char *argv[]);
    
    // Process incoming MQTT message
    void process_message(std::string topic, std::string message);
    
    int go();
    
  private:
    bool poll_eve_temp();
    bool poll_srf();
    
    CI2C *_i2c;
    int _eve_temp_poll_interval;
    time_t _eve_temp_last_poll;
    
    Ctmp100 *_eve_temp;
    CSrf *_srf;
    
    std::string _base_topic;
    std::string _eve_temp_topic;
    std::string _eve_srf_topic;
    
  
    
};
  
