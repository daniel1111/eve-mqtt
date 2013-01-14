/* 
 * Copyright (c) 2013, Daniel Swann <hs@dswann.co.uk>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "Ctmp100.h"

using namespace std;

extern CLogging *log;


Ctmp100::Ctmp100(CI2C *i2c, uint8_t address)
{
  _i2c_device = i2c;
  _i2c_address = address;
}

Ctmp100::~Ctmp100()
{


}

bool Ctmp100::set_12bit_resolution()
{
  i2cmsg msg;
  char buf[100];
  bool initally_open;
  
  // Remeber device state, so as to leave it in the same state we found it
  initally_open = _i2c_device->is_open();
  
  if (!initally_open)
  {
    if (_i2c_device->open())
    {
      log->dbg("Ctmp100.set_12bit_resolution: Failed to open i2c device");
      return false;
    }
  }

  msg.i2c_target = _i2c_address;
  msg.msg_type = 0x01; // config register
  msg.msg_len = 0;
  
  if (_i2c_device->transmit(&msg))
  {
    log->dbg("Ctmp100.set_12bit_resolution: Failed to get config register");
    if (!initally_open)
      _i2c_device->close();
    return false;
  }
  
  sprintf(buf, "Ctmp100.set_12bit_resolution: got config register: %2.2hhX", msg.buffer[0]);
  log->dbg(buf);
  
  msg.msg_type = 0x01;
  msg.msg_len = 1;
  msg.buffer[0] |= 0x60; // enable 12bit resolution
  
  if (_i2c_device->transmit(&msg))
  {
    log->dbg("Ctmp100.set_12bit_resolution: Failed to transmit updated config");
    if (!initally_open)
      _i2c_device->close();
    return false;
  }  
  
  usleep(350 * 1000); // wait 350ms for first conversion to happen
  if (!initally_open)
    _i2c_device->close(); 
  return true;
}

// On success, returns true, and puts the temperate into <temperature>
bool Ctmp100::get_temperature(double *temperature)
{
  i2cmsg msg;
  char buf[100];
  bool initally_open = _i2c_device->is_open();
   
  if (!initally_open)
  {
    if (_i2c_device->open())
    {
      log->dbg("Ctmp100.get_temperature: Failed to open i2c device");
      return false;
    }
  }  
  
  msg.i2c_target = _i2c_address;
  msg.msg_type = 0x00; // temperature register
  msg.msg_len = 0;  
  if (_i2c_device->transmit(&msg))
  {
    log->dbg("Ctmp100.get_temperature: Failed to get temperature");
    if (!initally_open)
      _i2c_device->close();     
    return false;
  }    
  
  
  msg.buffer[1] = msg.buffer[1] >> 4;
  *temperature = (double)msg.buffer[0] + ((double)msg.buffer[1]/16);
  sprintf(buf, "Ctmp100.get_temperature: got %2.2hhX:%2.2hhX ==> temperature = %fC", 
          msg.buffer[0], msg.buffer[1], *temperature);
  log->dbg(buf);
  
  if (!initally_open)
    _i2c_device->close();   
  return true;
}
