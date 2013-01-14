/*
 * Module to access the SRF on the EVE board using SPI.
 * 
 * Code is mix of three online examples:
 * 
 * SPI:
 * http://www.cmdrkeen.net/2012/11/06/playing-with-raspberry-pi-issue-2-using-spi-on-the-pi/
 * 
 * GPIO (to read IRQ line from the srf - currently polled, not attached to an interupt handler):
 * http://elinux.org/Rpi_Datasheet_751_GPIO_Registers
 * 
 * SRF:
 * https://github.com/CisecoPlc/SRFSPI/
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include "CLogging.h"
#include "CSrf.h"

// some defines for our SPI config
#define SPI_MODE              SPI_MODE_0  // SPI Mode
#define SPI_BITS_PER_WORD     8           // we're sending bytewise
#define SPI_MAX_SPEED         10000       // maximum speed is .01 Mhz
 
// GPIO Stuff
#define BCM2708_PERI_BASE        0x20000000
#define GPIO_BASE                (BCM2708_PERI_BASE + 0x200000) /* GPIO controller */

#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(g) *(_gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(_gpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(_gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

#define GPIO_SET *(_gpio+7)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR *(_gpio+10) // clears bits which are 1 ignores bits which are 0

#define GPIO_READ(g)    *(_gpio + 13) &= (1<<(g))


extern CLogging *log;

using namespace std;

CSrf::CSrf(string spi_device, int irq)
{  
  char buf[100];
  
  _init_success = false;
  _spi_device = spi_device;
  _irq_pin = irq;
   
  // Set up gpi pointer for direct register access
  _init_success = setup_io();   
 
  sprintf(buf, "CSrf: spi_device=[%s], irq=[%d]. Init %s.",
          spi_device.c_str(), irq, (_init_success ? "succeeded" : "FAILED"));
  log->dbg(buf);
}


 
/* spi_wr_1b:
*    - sending one byte and write the received byte into the send buffer.    
*/
uint8_t CSrf::spi_wr_1b(uint8_t tx, int delay)
{
  if (!_init_success)
    return -1;
  
  struct spi_ioc_transfer spi;
  memset((void *)&spi, 0, sizeof(struct spi_ioc_transfer));
  uint8_t rx;
  
  spi.tx_buf        = (__u64)&tx;
  spi.rx_buf        = (__u64)&rx;
  spi.len           = 1;
  spi.delay_usecs   = delay;
  spi.speed_hz      = SPI_MAX_SPEED;
  spi.bits_per_word = SPI_BITS_PER_WORD;
  spi.cs_change     = 1;

  if (ioctl (_spi_fd, SPI_IOC_MESSAGE(1), &spi) < 0)
  {
    log->dbg("CSrf.spi_wr_1b: ERROR while sending");
  }
  return rx;
}
 
/*spi_open
*      - Open the given SPI channel and configures it.
*      - there are normally two SPI devices on your PI:
*        /dev/spidev0.0: activates the CS0 pin during transfer
*        /dev/spidev0.1: activates the CS1 pin during transfer
*
*/
bool CSrf::spi_open()
{
  if (!_init_success)
    return -1;
  
  int mode  = SPI_MODE; 
  int bpw   = SPI_BITS_PER_WORD;
  int speed = SPI_MAX_SPEED;
 
  if ((_spi_fd = open(_spi_device.c_str(), O_RDWR)) < 0)
  {
    log->dbg("CSrf.spi_open: error opening [" + _spi_device + "]");
    return false;
  }
 
  if (ioctl (_spi_fd, SPI_IOC_WR_MODE, &mode) < 0)
      return false;
  if (ioctl (_spi_fd, SPI_IOC_RD_MODE, &mode) < 0)
      return false ;
 
  if (ioctl (_spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bpw) < 0)
      return false ;
  if (ioctl (_spi_fd, SPI_IOC_RD_BITS_PER_WORD, &bpw) < 0)
      return false ;
 
  if (ioctl (_spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed)   < 0)
      return false ;
  if (ioctl (_spi_fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed)   < 0)
      return false ;
 
  return true;
}

 
bool CSrf::receive(string &data)
{
  if (!_init_success)
    return false;
  
  data = "";
  startTransfer();
  
  if (!_rx_message_line.empty())
  {
    data = _rx_message_line.front();
    _rx_message_line.pop();
    return true;
  } else
    return false;
}

int CSrf::transmit(string data)
{
  if (!_init_success)
    return -1;
  
  unsigned int i;
  log->dbg("CSrf.transmit: > " + data);
  for (i=0; i < data.length(); i++)
  {
    _tx_buffer.push(data[i]);
  }
  startTransfer();
  return 1;
}

 
void CSrf::startTransfer()
{
  if (!_bTransfering)
  {
    _bTransfering = true;
    SPItransfer();
  }
} 
 
void CSrf::SPItransfer()
{
  uint8_t c,d;
  uint8_t FEflag=0;
  int irq;

  if (spi_open() < 0)
  {
    log->dbg("CSrf.SPItransfer: spi_open failed"); 
    return;
  }

  while (_bTransfering)
  {
    irq = GPIO_READ(_irq_pin);
    if (_tx_buffer.empty() && (irq == 0)) // nothing left to do
    {
      _bTransfering = false; // say we have done
      close(_spi_fd); 
      return;
    }        
    
    // do we have anything to send?
    if (!_tx_buffer.empty())
    {
      c = _tx_buffer.front();
      _tx_buffer.pop();
      if ((c & 0xFE) == 0xFE) 
        FEflag=1;
    }
    else c = 0xFF;

    if (FEflag)
    {
      d = spi_wr_1b(0xFE,  1); // send FE flag
      processReceivedChar(d);
      FEflag=0;
    }
  
    d = spi_wr_1b(c,  1); // exchange a byte
    processReceivedChar(d);
    irq = GPIO_READ(_irq_pin);
    if (_tx_buffer.empty() && (irq == 0)) // nothing left to do
    {
      _bTransfering = false; // say we have done
    }
  }
  
  // close SPI channel
  close(_spi_fd);  
}

void CSrf::processReceivedChar(uint8_t c)
{

  if ((c==0x80) || (c==0xFF) || (c==0xFE))
    return;
  
  if ((c == '\n') || (c == '\r'))
  {
    string msg="";
    
    // Carrage return received, transfer all data recieved so far to message line buffer
    while (!_rx_buffer.empty())
    {
      msg += _rx_buffer.front();
      _rx_buffer.pop();
    }
      
    _rx_message_line.push(msg);
  } else
    _rx_buffer.push(c);
}
  
bool CSrf::setup_io()
{
   /* open /dev/mem */
   if ((_mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) 
   {
      log->dbg("CSrf.setup_io: can't open /dev/mem (non running as root?)");
      return false;
   }

   /* mmap GPIO */
   _gpio_map = (char *)mmap(
      NULL,                 // Any adddress in our space will do
      BLOCK_SIZE,           // Map length
      PROT_READ|PROT_WRITE, // Enable reading & writting to mapped memory
      MAP_SHARED,           // Shared with other processes
      _mem_fd,              // File to map
      GPIO_BASE             // Offset to GPIO peripheral
   );

   close(_mem_fd); //No need to keep mem_fd open after mmap

   if ((long)_gpio_map < 0) 
   {
     char buf[35];
     sprintf(buf, "CSrf.setup_io: mmap error %d", (int)_gpio_map);
     log->dbg(buf);
     return false;
   }

   // Always use volatile pointer!
   _gpio = (volatile unsigned *)_gpio_map;
   return true;
} // setup_io
