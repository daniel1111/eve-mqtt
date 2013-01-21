eve-mqtt  
========  
Eve-mqtt is designed pass messages from/to the SRF on the eve alpha board to an mqtt broker.  
It should work with Raspbian "wheezy" (tested with "2012-12-16"), after mosquitto has been installed.  
  
It's been tested on an Eve alpha board with two radios installed (SRF and RFM11B - kickstarter reward level £39), but needs a jump wire installing between CE1 and SS RFM12 on the eve board, see:  
http://openmicros.org/index.php/component/kunena/12-eve-alpha-contributers-forum/2746-how-to-get-started-with-the-everfm12b?Itemid=0#2833  
(needed to stop the RFM11B module interfering with the SRF?).  
  
Installation  
------------  
* First, mosquitto needs to be installed. So add the mosquitto repository:  
``` 
wget http://repo.mosquitto.org/debian/mosquitto-repo.gpg.key  
sudo apt-key add mosquitto-repo.gpg.key  
cd /etc/apt/sources.list.d/  
sudo wget http://repo.mosquitto.org/debian/mosquitto-repo.list  
sudo apt-get update  
```
  
* Install the required packages:  
`sudo apt-get install i2c-tools libmosquitto1 libmosquitto1-dev mosquitto mosquitto-clients`  
  
* Load the SPI & I2C modules, and give everyone read/write access:  
``` 
sudo modprobe i2c-bcm2708  
sudo modprobe spi-bcm2708  
sudo modprobe i2c-dev  
sudo udevadm trigger  
sudo chmod a+rwx /dev/i2c-0  
sudo chmod a+rwx /dev/spidev0.0  
sudo chmod a+rwx /dev/spidev0.1  
```
It should be possible to get these to load on boot by removing the modules from the blacklist ("/etc/modprobe.d/raspi-blacklist.conf")  
  
* Check the two i2c devices show up: (RTC+Temperature)  
`pi@raspberrypi ~ $ i2cdetect -y 0`  
  
* Build eve-mqtt  
Change to the cpp/ folder, and run make:
```  
pi@raspberrypi ~/eve-mqtt/cpp $ make  
g++ -Wall -c CEve.cpp  
g++ -Wall -c CI2C.cpp  
g++ -Wall -c CLogging.cpp  
g++ -Wall -c Ctmp100.cpp  
g++ -Wall -c CSrf.cpp  
g++ -Wall -c CNHmqtt.cpp  
gcc -c inireader/ini.c  
g++ -c inireader/INIReader.cpp  
g++ -lmosquitto -lrt -o eve CEve.o CI2C.o CLogging.o Ctmp100.o CSrf.o CNHmqtt.o ini.o INIReader.o  
pi@raspberrypi ~/eve-mqtt/cpp $  
```  
  
Usage  
-----  
Start eve-mqtt  
eve-mqtt currently needs to be ran as root, to allow GPIO access.  
Start eve-mqtt, with `-d` for debug (don't daemonize, and print debugging messages to the console) and `-c` to specify the config file:
 
pi@raspberrypi ~/eve-mqtt/cpp $ sudo ./eve -d -c eve.conf  
Jan 14 20:29:38: Starting in debug mode  
Jan 14 20:29:38: mosq_server = 127.0.0.1  
Jan 14 20:29:38: mosq_port = 1883  
Jan 14 20:29:38: mqtt_tx = test/tx  
Jan 14 20:29:38: mqtt_rx = test/rx  
Jan 14 20:29:38: CI2C: Using device [/dev/i2c-0]  
Jan 14 20:29:38: CSrf: spi_device=[/dev/spidev0.0], irq=[25]. Init succeeded.  
Jan 14 20:29:38: Connecting to Mosquitto as [test-2309]  
Jan 14 20:29:38: Ctmp100.set_12bit_resolution: got config register: E0  
Jan 14 20:29:38: Subscribing to topic [eve/communications/srf/tx]  
Jan 14 20:29:38: Connected to mosquitto.  
Jan 14 20:29:38: Ctmp100.get_temperature: got 1D:05 ==> temperature = 29.312500C  
Jan 14 20:29:38: Sending message, topic=[eve/device/temperature/eve], message=[29.31]  
Jan 14 20:29:41: Sending message, topic=[eve/communications/srf/rx], message=[TEST]  
  
Use mosquitto  
In another session, load up `mosquitto_sub`, this should periodically show the temperature, and any messages received by the srf, e.g.:  

pi@raspberrypi ~ $ mosquitto_sub -v -t \#  
eve/device/temperature/eve 29.31  
eve/communications/srf/rx TEST  

To send messages using the srf, use mosquitto_pub:  

pi@raspberrypi ~ $ mosquitto_pub -t "eve/communications/srf/tx" -m "Hello world"  

The eve-mqtt console/log should show:  

Jan 14 20:37:45: Got mqtt message, topic=[eve/communications/srf/tx], message=[Hello world]  
Jan 14 20:37:45: CSrf.transmit: > Hello world  

  
