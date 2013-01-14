#ifndef CI2C_H
#define CI2C_H

#include <string>
#include <sstream>
#include <unistd.h>
#include <stdint.h>
#include "CLogging.h"

#define I2C_MAX_MSG_SIZE 16

struct i2cmsg
{
  int i2c_target;     // target i2c address
  uint8_t msg_type;   // msg_type (first byte to transmit)
  int msg_len;        // number of byte to send, not counting msg type. Can be 0.
                      // After transmitting, contains the number of bytes recieved in reply 
  uint8_t buffer[I2C_MAX_MSG_SIZE]; // msg body to send. After transmitting, contains any reply
};

class CI2C
{
  public:
    CI2C(std::string i2c_device);
    ~CI2C();
    
    int open();
    void close();
    int transmit(struct i2cmsg *msg);
    bool is_open();
    
  private:
    int _handle;  
    std::string _i2c_dev;
    bool _device_open;
    
};
  
#endif
  