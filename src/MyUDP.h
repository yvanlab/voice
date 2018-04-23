#ifndef MYUDP_H
#define MYUDP_H

#include <WiFiUdp.h>
#include <baseManager.h>
#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

class MyUDP : public WiFiUDP
{
public:
  MyUDP():WiFiUDP() {} ;
  void handleClient() {
    int packetSize = parsePacket();
    if (packetSize) {
        // receive incoming UDP packets
        DEBUGLOGF("Received %d bytes from %s, port %d\n", packetSize, remoteIP().toString().c_str(),remotePort());
        int len = read(m_incomingPacket, 255);
        if (len > 0) {
          m_incomingPacket[len] = 0;
        }
        DEBUGLOGF("UDP packet contents: %s\n", m_incomingPacket);
        if (m_callBack)
          m_callBack(m_incomingPacket);
    };
  }
  typedef void udpCallback(char *packet);
  //typedef std::function<void(void *timer_arg)
  void onReceivePacket(void (*reveiveParcketCallback)(char *)) {
    m_callBack = reveiveParcketCallback;
  };

  void send(const char* content_type = NULL) {
    beginPacket(remoteIP(), remotePort());
    write(content_type);
    endPacket();
  }

public:
  void (*m_callBack)(char *);
  char m_incomingPacket[255];
};


#endif
