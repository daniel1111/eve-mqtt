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

#include "CNHmqtt.h"
#include "CEve.h"

CLogging *log;

using namespace std;

CEve::CEve(int argc, char *argv[]) : CNHmqtt(argc, argv)
{
  string i2c_device;
  string spi_device;
  int eve_temp_address, srf_irq;
  
  i2c_device = get_str_option("eve", "i2c_device", "/dev/i2c-0");
  _eve_temp_poll_interval = get_int_option("eve_temp", "poll_interval", 10);
  eve_temp_address = get_int_option("eve_temp", "i2c_address", 0x48);
  _eve_temp_topic = get_str_option("eve_temp", "topic", "temp");
  _eve_srf_topic  = get_str_option("eve_srf", "topic", "srf/");
  
  spi_device =  get_str_option("eve_srf", "spi_device", "/dev/spidev0.0");
  srf_irq = get_int_option("eve_srf", "irq", 25);
  _base_topic = get_str_option("eve", "base_topic", "eve/");
  
  _i2c = new CI2C(i2c_device);
  _eve_temp = new Ctmp100(_i2c, eve_temp_address);
  _eve_temp_last_poll = -1;
  
  _srf = new CSrf(spi_device, srf_irq);
}
   
// Process incoming MQTT message
void CEve::process_message(string topic, string message)
{
  // Transmit anything received on the srf tx MQTT topic 
  if (topic == _base_topic + _eve_srf_topic + "tx")
  {
    _srf->transmit(message);
  }
  
  CNHmqtt::process_message(topic, message);
}
    
int CEve::go()
{  
  mosq_connect();   
  CEve::daemonize();  
  
  _eve_temp->set_12bit_resolution();
  subscribe (_base_topic + _eve_srf_topic + "tx");
  
  //process any mqtt messages
  while (!message_loop(50))
  {
    poll_srf();
    poll_eve_temp();
  }
  
  return 0;
}

bool CEve::poll_eve_temp()
{
  double temperature;
  char buf[25];
  
  if (((time(NULL) - _eve_temp_last_poll) > _eve_temp_poll_interval) || (_eve_temp_last_poll == -1))
  {
    _eve_temp_last_poll = time(NULL);
    if (!_eve_temp->get_temperature(&temperature))
      return false;
    
    sprintf(buf, "%.2f", temperature);
    message_send(_base_topic + _eve_temp_topic, buf);
  }
  
  return true;
}
  
bool CEve::poll_srf()
{
  string msg;
  
  if (_srf->receive(msg))
  {
    message_send(_base_topic + _eve_srf_topic + "rx", msg);
    return true;
  } else
    return false;
}

int main(int argc, char *argv[])
{
  log = new CLogging();  
  CEve eve = CEve(argc, argv);
  return eve.go();
}
