#ifndef CSrf_H
#define CSrf_H

#include <string>
#include <sstream>
#include <unistd.h>
#include <stdint.h>
#include <queue>
#include "CLogging.h"

class CSrf
{
  public:
    CSrf(std::string spi_device, int irq);
    ~CSrf();
    
    bool receive(std::string &data);
    int transmit(std::string data);
    
  private:
    bool setup_io();
    uint8_t spi_wr_1b(uint8_t tx, int delay);
    bool spi_open();
    void processReceivedChar(uint8_t c);
    void SPItransfer();
    void startTransfer();
    
    std::string _spi_device;
    std::queue<uint8_t> _rx_buffer;
    std::queue<uint8_t> _tx_buffer;
    
    std::queue<std::string> _rx_message_line; // 
    
    bool _init_success;
    int  _mem_fd;
    int _spi_fd; //global spi file descriptor
    char *_gpio_map;    
    // I/O access
    volatile unsigned *_gpio; 

    int _handle;  
    bool _device_open;
    int _irq_pin;
    bool _bTransfering;
    
};
  
#endif
  