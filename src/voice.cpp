
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
//#include <SD.h>
#include <SDfat.h>
SdFat SD;
#include <WifiManagerV2.h>
#include <WiFiUdp.h>

#include <myTimer.h>
/*
{
  "url": "https://stream.watsonplatform.net/speech-to-text/api",
  "username": "c5eff64e-3b64-4147-9984-fdc461ada7a9",
  "password": "gI8xXWjHSN6G"
}
*/
// ArduinoLog.h is PlatformIO library id=1532
//#include <ArduinoLog.h>
// This ESP_VS1053_Library
//#include <VS1053.h>
#include <ADAFRUIT_VS1053.h>


#ifdef MCPOC_TELNET
#include "RemoteDebug.h"
#endif


#include <WiFiClient.h>
#include "MyUDP.h"

MyUDP Udp;

#define  localUdpPort 4210  // local port to listen on
//char incomingPacket[255];  // buffer for incoming packets


#define VS1053_CS     D1
#define VS1053_DCS    D0
#define VS1053_DREQ   D2

#define VS1053_CS      D1     // VS1053 chip select pin (output)
#define VS1053_DCS     D0    // VS1053 Data/command select pin (output)
#define CARDCS          D4     // Card chip select pin
#define VS1053_DREQ     D2     // VS1053 Data request, ideally an Interrupt pin
#define VS1053_RESET   -1     // VS1053 reset pin (not used!)

int VOLUME = 90; // volume level 0-100

//VS1053 playerVS(VS1053_CS, VS1053_DCS, VS1053_DREQ);
//Adafruit_VS1053 playerVS(D8,VS1053_CS, VS1053_DCS, VS1053_DREQ);
//Adafruit_VS1053_FilePlayer playerVS(D8,VS1053_CS, VS1053_DCS, VS1053_DREQ);
Adafruit_VS1053_FilePlayer playerVS = Adafruit_VS1053_FilePlayer(VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);
#define MODULE_NAME VOICE_NAME
#define MODULE_MDNS VOICE_MDNS
#define MODULE_MDNS_AP VOICE_MDNS_AP
#define MODULE_IP   VOICE_IP

#define  pinLed           D4 //D4

BaseSettingManager          smManager(pinLed);
WifiManager                 wfManager(pinLed,&smManager);
String previousText;
uint8_t maxVolume = 0;

#ifdef MCPOC_TELNET
RemoteDebug             Debug;
#endif

#ifdef MCPOC_TELNET // Not in PRODUCTION
void processCmdRemoteDebug() {
    String lastCmd = Debug.getLastCommand();
    /*if (lastCmd == "next") {
      siInterface.setPagechangeDetected();
    } else if (lastCmd == "on") {
      siInterface.setPersonDetected(true);
    } else if (lastCmd == "off") {
      siInterface.setPersonDetected(false);
    } else if (lastCmd == "cfgon") {
      siInterface.setCfgDetected(true);
    } else if (lastCmd == "cfgoff") {
      siInterface.setCfgDetected(false);
    }*/
}
#endif
unsigned char h2int(char c)
{
    if (c >= '0' && c <='9'){
        return((unsigned char)c - '0');
    }
    if (c >= 'a' && c <='f'){
        return((unsigned char)c - 'a' + 10);
    }
    if (c >= 'A' && c <='F'){
        return((unsigned char)c - 'A' + 10);
    }
    return(0);
}


String urldecode(String str)
{

    String encodedString="";
    char c;
    char code0;
    char code1;
    for (int i =0; i < str.length(); i++){
        c=str.charAt(i);
      if (c == '+'){
        encodedString+=' ';
      }else if (c == '%') {
        i++;
        code0=str.charAt(i);
        i++;
        code1=str.charAt(i);
        c = (h2int(code0) << 4) | h2int(code1);
        encodedString+=c;
      } else{

        encodedString+=c;
      }

      yield();
    }

   return encodedString;
}

String urlencode(String str)
{
    String encodedString="";
    char c;
    char code0;
    char code1;
    char code2;
    for (int i =0; i < str.length(); i++){
      c=str.charAt(i);
      if (c == ' '){
        encodedString+= '+';
      } else if (isalnum(c)){
        encodedString+=c;
      } else{
        code1=(c & 0xf)+'0';
        if ((c & 0xf) >9){
            code1=(c & 0xf) - 10 + 'A';
        }
        c=(c>>4)&0xf;
        code0=c+'0';
        if (c > 9){
            code0=c - 10 + 'A';
        }
        code2='\0';
        encodedString+='%';
        encodedString+=code0;
        encodedString+=code1;
        //encodedString+=code2;
      }
      yield();
    }
    return encodedString;

}



void timerrestartESP(void *pArg) {
    ESP.restart();
}

String getJson()
{

  String tt("{\"module\":{");
    tt += "\"nom\":\"" + String(MODULE_NAME) +"\"," ;
    tt += "\"version\":\"" + String(VOICE_VERSION) +"\"," ;
    tt += "\"status\":\"" + String(STATUS_PERIPHERIC_WORKING) +"\"," ;
    tt += "\"uptime\":\"" + NTP.getUptimeString() +"\"," ;
    tt += "\"build_date\":\""+ String(__DATE__" " __TIME__)  +"\"},";
    tt += "\"datetime\":{" + wfManager.getHourManager()->toDTString(JSON_TEXT) + "},";
    tt += "\"setting\":{";
    tt += "\"text\":\""+ previousText +"\"," ;
    tt += "\"volume\":\""+ String(maxVolume) +"\"}" ;
    //tt += smManager.toString(JSON_TEXT) + "},";
      //  tt += "\"LOG\":[]";*/
    tt += "}";
    return tt;
}



void dataSummaryJson() {
	digitalWrite ( pinLed, LOW );
  wfManager.getServer()->send ( 200, "text/json", getJson() );
  digitalWrite ( pinLed, HIGH );

}
void dataSummaryPage() {
	digitalWrite ( pinLed, LOW );
  wfManager.getServer()->send ( 200, "text/json", getJson() );
  digitalWrite ( pinLed, HIGH );

}

void displayData() {
	digitalWrite ( pinLed, LOW );



  String message =  "<html>\
    <head>\
      <title>message page</title>\
      <style>\
        body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
      </style>\
    </head>\
    <body>";
  message += "<p>";
  message +="<ul>";
  message += "</ul>";
  message += "<form method='get' action='setData'>";
  message += "<label>text:</label><input name='text' length=600 value=\"\"><br>";
  message += "<label>volume:</label><input name='volume' length=30 value=\"\"><br>";
  message += "<label>commande:</label><input name='cmd' length=30 value=\"\"><br>";
  message += "<input type='submit'></form>";
  message += "</body></html>";

  wfManager.getServer()->send ( 200, "text/html", message );

  digitalWrite ( pinLed, HIGH );

}




int8_t convertTTS(char const *str/*, Adafruit_VS1053 *plyr*/) {
  if (strcmp(str,previousText.c_str()) == 0) {
    DEBUGLOG("Text already converted");
    return 0 ;
  }

  HTTPClient http;
  int httpCode = 0;
  String URL = "https://stream.watsonplatform.net/text-to-speech/api/v1/synthesize?accept=audio/mp3&rate=192&voice=fr-FR_ReneeVoice&text="+urlencode(String(str));
  //String URL = "https://stream.watsonplatform.net";

  const char* fingerprint = "76 D3 F9 0A C5 F7 E1 B8 32 F2 61 83 DF 72 E9 6F 42 DE 53 DE"; //"4E 53 E9 C1 29 6F 68 85 F3 1F DA ED 50 9C 91 A9 71 A9 5A E7";//"28 53 07 45 d6 41 f1 16 0d d6 f8 7a 1e 94 19 88 59 d7 83 18";
  DEBUGLOG("url : " + URL);
  if (!http.begin(URL, fingerprint)) {
    DEBUGLOG(F("getLocation: [HTTP] connect failed!"));
    return -1;
  }
  //http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Basic ZTA4NTQ4MTEtYjQ2MS00NDVkLWI1ZjAtMDE1MjBmODg4MjRmOm5lbENuY0FKU0VYYw==");
  //http.addHeader("Postman-Token","cbcaf8d1-32f3-9d94-a882-1a6f33d2db0a");
  http.addHeader("Postman-Token","e8828436-ca87-da3d-58a5-8f6e613b3bc1");

  http.addHeader("Cache-Control","no-cache");
  http.addHeader("Connection","close");
  httpCode =http.GET();
  //httpCode =http.POST("/text-to-speech/api/v1/synthesize?accept=audio/mp3&amp;voice=fr-FR_ReneeVoice&amp;text="+String(incomingPacket));
  DEBUGLOGF("httpcode1 : %d\n",httpCode);

  if(httpCode != HTTP_CODE_OK) {
    http.end();
    DEBUGLOG(F("[HTTP] connection closed or file end."));
    return -2;
  }
  // HTTP header has been send and Server response header has been handled
  DEBUGLOGF("memory 1: %d\n", ESP.getFreeHeap());
  int len = http.getSize();
  DEBUGLOGF("http.getSize(): %d\n", len);
  // create buffer for read

  // delete possible existing file
  SD.remove("tts.mp3");
  SdFile file;
  file.open("tts.mp3",  O_CREAT | O_EXCL | O_WRITE);


  // get tcp stream
  WiFiClient * stream = http.getStreamPtr();
  #define buf_size  512
  static __attribute__((aligned(4))) uint8_t buff[buf_size];

    // read all data from server
  while(http.connected() && (len > 0 || len == -1)) {
    size_t size = stream->available();
    //DEBUGLOGF("size stream %d\n", size);
    // get available data size
  // if (plyr->readyForData()) {
    //  size_t size = stream->available();
      DEBUGLOGF("size stream %d\n", size);
      if(size) {
        // read up to 128 byte
        int c = stream->readBytes(buff, ((size > buf_size) ? buf_size : size));
        // write it to Serial
        //DEBUGLOGF("size %d\n", c);
        //plyr->playData(buff, 32);
        file.write(buff, c);
        if(len > 0) {
          len -= c;
        }
      }else {
        delay(500);
      }
     yield();
  /*  }
    else {
      DEBUGLOG("No data req");
    }*/
  }
  file.close();
  http.end();
  previousText = String(str);
  //delay(1000);

  //plyr->setVolume(100,100);
  //plyr->setVolume(0);
}

void reveiveParcketCallback (char * str) {
  DEBUGLOGF("String to convert: %s\n", str);
  Udp.send("OK");
  playerVS.setVolume(maxVolume,maxVolume);
  if (convertTTS(str) == 0){
    playerVS.playFullFile("tts.mp3");
  } else {
    playerVS.playFullFile("bong.mp3");
  };

}

void setData(){
  String tts;
  String str = wfManager.getServer()->arg("volume");
  if (str.length()>0) {
    maxVolume = str.toInt();
    // smManager.volume = str.toInt();
  }
  str = wfManager.getServer()->arg("text");
  if (str.length()>0) {
    tts = str;
  }
  str = wfManager.getServer()->arg("cmd");
  if (str.length()>0) {
    if (str == "heure") {
      tts += "il est " +  NTP.getTimeStr();
    } else if  (str == "date") {
      tts += "nous sommes le " +  NTP.getDateStr();
    } else tts += "désolé je ne connais pas cette commande";
  }
  if (tts.length() >0){
    playerVS.setVolume(maxVolume,maxVolume);
    if (convertTTS(tts.c_str()) == 0){
      playerVS.playFullFile("tts.mp3");
    } else {
      playerVS.playFullFile("bong.mp3");
    };
  }
  wfManager.getServer()->send ( 200, "text/html", "sent");
}

void startWiFiserver() {
  if (wfManager.begin(IPAddress(MODULE_IP),MODULE_NAME, MODULE_MDNS, MODULE_MDNS_AP)==WL_CONNECTED) {
    wfManager.getServer()->on ( "/", dataSummaryPage );
    wfManager.getServer()->onNotFound ( dataSummaryPage );
  } /*else {
    DEBUGLOG("Not connected");
  }*/

  wfManager.getServer()->on ( "/setting", displayData );
  wfManager.getServer()->on ( "/setData", setData );
  wfManager.getServer()->on ( "/status", dataSummaryJson );

  Serial.println( "HTTP server started" );
  Udp.begin(localUdpPort);
  Udp.onReceivePacket(reveiveParcketCallback);
  DEBUGLOGF("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), localUdpPort);
  Serial.println(wfManager.toString(STD_TEXT));
}


void printDirectory(File dir, int numTabs) {
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}


void setup ( void ) {
  // Iniialise input
  Serial.begin ( 115200 );
//Log.begin   (LOG_LEVEL_VERBOSE, &Serial);
  //pinMode(pin_DETECTION,INPUT);
 smManager.readData();
  DEBUGLOG("");DEBUGLOG(smManager.toString(STD_TEXT));
  startWiFiserver();

#ifdef MCPOC_TELNET
  Debug.begin(MODULE_NAME);
  String helpCmd = "next\n\ron/off\n\rcfgon/cfgoff\n\r";
  Debug.setHelpProjectsCmds(helpCmd);
  Debug.setCallBackProjectCmds(&processCmdRemoteDebug);

#endif


//SPI.begin();
/*pinMode(VS1053_CS,OUTPUT);
digitalWrite(VS1053_CS, HIGH);
SPI.endTransaction();*/

delay(500);
if (!playerVS.begin()) {
  Serial.println("VS1053 not found");
  while (1);  // don't do anything more
}
playerVS.sineTest(0x44, 500);    // Make a tone to indicate VS1053 is working

if (!SD.begin(CARDCS)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");

  //File  root = SD.open("/");

  //printDirectory(root, 0);
  //mtTimer.begin(timerFrequence);
  //mtTimer.setCustomMS(20000);

//digitalWrite(D3, HIGH);


//File currentTrack;
//VS1053_CS
//SD
Serial.println("initialization");


playerVS.useInterrupt(VS1053_FILEPLAYER_PIN_INT);  // DREQ int


// Play a file in the background, REQUIRES interrupts!
Serial.println(F("Playing full track 001"));
playerVS.setVolume(0,0);
playerVS.playFullFile("bong.mp3");
Serial.println(F("Playing full track 001 * done"));
//playerVS.playFullFile("TRACK001.mp3");


}


void loop ( void ) {

	wfManager.handleClient();
  Udp.handleClient();
  /*delay(1000);
  Serial.print("working");*/

}
