all: eve

eve: CEve.o CI2C.o CLogging.o Ctmp100.o CSrf.o CNHmqtt.o ini.o INIReader.o
	g++ -lmosquitto -lrt -o eve CEve.o CI2C.o CLogging.o Ctmp100.o CSrf.o CNHmqtt.o ini.o INIReader.o

CEve.o: CEve.cpp CEve.h
	g++ -Wall -c CEve.cpp

CI2C.o: CI2C.cpp CI2C.h
	g++ -Wall -c CI2C.cpp

CLogging.o: CLogging.cpp CLogging.h
	g++ -Wall -c CLogging.cpp 

Ctmp100.o: Ctmp100.cpp Ctmp100.h
	g++ -Wall -c Ctmp100.cpp 

CSrf.o: CSrf.cpp CSrf.h
	g++ -Wall -c CSrf.cpp 

CNHmqtt.o: CNHmqtt.cpp CNHmqtt.h
	g++ -Wall -c CNHmqtt.cpp

ini.o: inireader/ini.c inireader/ini.h
	gcc -c inireader/ini.c

INIReader.o: inireader/INIReader.cpp inireader/INIReader.h
	g++ -c inireader/INIReader.cpp

clean:
	rm -fv eve *.o