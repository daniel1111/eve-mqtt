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

#include <stdio.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <string.h>
#include "CI2C.h"

extern CLogging *log;

using namespace std;

CI2C::CI2C(string i2c_device)
{
  _i2c_dev = i2c_device;
  _handle = -1;
  _device_open = false;
  log->dbg("CI2C: Using device [" + i2c_device + "]");
  
  return;
}

CI2C::~CI2C()
{
  close();
  return;
}

int CI2C::open()
{
  _handle = ::open(_i2c_dev.c_str(), O_RDWR);
  if (_handle == -1)
  {
    _device_open = false;
    return -1;
  }
  else
  {
   _device_open = true;
   return 0;
  }
}

void CI2C::close()
{
  if (_handle != -1)
  {
    ::close(_handle);
    _handle = -1;
  }
  _device_open = false;
}


int CI2C::transmit(struct i2cmsg *msg)
{
  char *buf;
  int ret;
  
  if (_handle==-1)
    return -1;
  
  if (!_device_open)
    return -1;
  
  ioctl(_handle, I2C_SLAVE, msg->i2c_target);
  
  buf = (char*)malloc(msg->msg_len + 1);
  if (buf==NULL)
    return -1;
  
  buf[0] = msg->msg_type;
  memcpy(buf+1, msg->buffer, msg->msg_len);
  
  ret = write(_handle, buf, msg->msg_len + 1);
  free(buf);
  if (ret < msg->msg_len + 1)
    return -1;
  
  usleep(10000);
  
  // Get reply
  memset(msg->buffer, 0xFF, I2C_MAX_MSG_SIZE); // I2C_MAX_MSG_SIZE);
  msg->msg_len = read(_handle, &msg->buffer[0], I2C_MAX_MSG_SIZE); //I2C_MAX_MSG_SIZE);
  if (msg->msg_len < 0)
    return -1;
  else  
    return 0;
}

bool CI2C::is_open()
{
  return _device_open;
}
    
    