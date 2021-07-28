/* 

IOT Temp - a somewhat basic temperature and humidity logger. 

Featuring the LOLIN D1 ESP 8266 and associated shields.

You will need a file "data.h" which looks like this
-----------------------
#define NODENAME "<Your NodeName - Kitchen for example";
#define LOCALSSID "<Your WiFi SSID>";    Can't use plain SSID as the WiFi library now defines it.
#define PASSWORD "<Your WiFI Password>";
#define HOST "<Your emoncms host - most likely emoncms.org>";  Note:just the host not the protocol
#define MYAPIKEY "<Your API write key for emoncms>";
#define BRFACTOR 1;  See code below this sets to 44 as there are more of these at Lilliput
#define HEADLESS;    Shove this in if you don't have a display.
#define SHT30;       Add this if you are running the SHT30 temp/%RH sensor otherwise will default to DHT12
-------------------------------------

Trying to do this in both Arduino IDE and PlatformIO is too hard - Stick to Arduino

Additional Libraries for DHT12 and SHT30
https://github.com/wemos

*/

#define VERSION 1.25            // 1.25 WebClient
                                // 1.24 Pressure and headless operation 
                                // 1.23 Bushfire danger feeds - now defaults to 44
                                // edit bushFireRatingFactor to taste

#warning Setup your data.h
#include "data.h"               // Means I don't keep uploading my API key to GitHub

#undef FIXED_IP

#define WIFI

#ifdef WIFI
 #include <ESP8266WiFi.h>
 #include <WiFiClient.h>
 #include <WiFiUdp.h>
 #include <ESP8266mDNS.h>
 #include <ESP8266WebServer.h>   // Include the WebServer library
#endif

//BushFire Rating Factor (amount to times the result by for logging purposes)
// Tony = 1
// PB = 44 as this gives granularity for graph results out of 100

#ifndef BRFACTOR
int brFactor = 44;
#else
int brFactor = BRFACTOR;
#endif

#ifdef BMP    // Barometric Pressure
 #include <LOLIN_HP303B.h>
#endif
 
#ifdef SHT30
 #include <WEMOS_SHT3X.h>
#else
 #include <WEMOS_DHT12.h>      // Mighty LOLIN DHT12 temperature and humidity sensor
#endif

#include <Adafruit_GFX.h>      // Core graphics library
#include <Adafruit_ST7735.h>  // Hardware-specific library

#ifdef RTC
 #include <Wire.h>
 #include "RTClib.h"
 
#endif

#define CONNECTOR_110      // the v1.1.0 connector board has different CS and DC values
                           // comment out if you have a v1.0 board
#ifdef CONNECTOR_110
 #define TFT_CS     D4
 #define TFT_DC     D3
#else                    
 #define TFT_CS     D0
 #define TFT_DC     D8
#endif

#define TFT_RST    -1       // you can also connect this to the Arduino reset
                            // in which case, set this #define pin to -1!

#define CELSIUS             // Comment out if you prefer Fahrenheit
#define DEBUG
//
// If we did then DEBUG_LOG will log a string, otherwise
// it will be ignored as a comment.
//
#ifdef DEBUG
#  define DEBUG_LOG(x) Serial.print(x)
#else
#  define DEBUG_LOG(x)
#endif

const char* ssid = LOCALSSID;
const char* password = PASSWORD;
const char* host = HOST;
const char* APIKEY = MYAPIKEY;
const char* nodeName = NODENAME;

#ifdef FIXED_IP
 IPAddress staticIP(192,168,1,22);
 IPAddress gateway(192,168,1,1);
 IPAddress subnet(255,255,255,0);
 IPAddress dns1(8,8,8,8);

#endif
#ifndef HEADLESS
Adafruit_ST7735 tft = Adafruit_ST7735( TFT_CS, TFT_DC, TFT_RST);    // Instance of tft
#endif

#ifdef SHT30
SHT3X sht30(0x45);            
#else
DHT12 dht12;                  // Instance of dht12
#endif

#ifdef BMP
 LOLIN_HP303B HP303B;         // HP303B BMP instance
 int32_t pressure;
 int16_t bmpRet;

#endif

#ifdef WIFI
 WiFiClient client;              // Instance of WiFi Client
 ESP8266WebServer server(80);    // Create a webserver object that listens for HTTP request on port 80
 void handleRoot();              // function prototypes for HTTP handlers
 void handleNotFound();

#endif

#ifdef RTC
RTC_DS1307 rtc;
WiFiUDP Udp;
unsigned int localPort = 2390;
static const char ntpServerName[] = "time.nist.gov";
const long timeZoneOffset = 36000L; // + 10 hours in seco
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
String timeStr; 

#endif

int waitForWiFi = 20000 ;     // How long to wait for the WiFi to connect - 10 Seconds should be enough 
int startWiFi;
int connectMillis = millis();     // this gets reset after every successful data push

int poll = 60000;           // Poll the sensor every 60 seconds (or so)
int lastRun = millis() - (poll + 1);

float TempC;
float TempF;
float Humidity;

void setup()
{
  Serial.begin(115200);
  Serial.println();
  
#ifdef RTC
 if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
#endif 

#ifdef BMP
  HP303B.begin(); // I2C address = 0x77
#endif

void SetRTC();

#ifndef HEADLESS
  tft.initR(INITR_144GREENTAB);
  tft.setTextWrap(false);     // Allow text to run off right edge
  tft.setRotation( 1 );     // Portrait mode
#endif

#ifdef WIFI
if (MDNS.begin( nodeName )) {              // Start the mDNS responder for <nodeName>.local
    Serial.println("mDNS responder started");
  } else {
    Serial.println("Error setting up MDNS responder!");
  }

server.on("/", handleRoot);               // Call the 'handleRoot' function when a client requests URI "/"
server.onNotFound(handleNotFound);        // When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"

server.begin();                           // Actually start the server
Serial.println("HTTP server started");
#endif
}

void loop() {

 if ( millis() > 14400000) {   // Reboot every 4 hours - I have crappy internet. You may not need this
      Serial.println("Rebooting");
      ESP.restart();           // Kick it over and try from the beginning
  }

#ifdef WIFI 
 server.handleClient();                    // Listen for HTTP requests from clients
#endif

// Serial.println( millis() );
// Serial.println( lastRun+poll);

if ( millis() > lastRun + poll ) {        // only want this happening every so often - see Poll value

#ifdef SHT30
 if( !(sht30.get() == 0 ) ){
#else  
 if( !(dht12.get() == 0 ) ){
#endif
  Serial.println("Cannot read Sensor");
  tft.println("Sensor Error");
  
 }
 else
 {
  

#ifdef RTC
  DateTime now = rtc.now();
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" (");
  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
  Serial.print(") ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
  timeStr = String( now.hour(), DEC );
  timeStr.concat( ":" ); 
  timeStr.concat( String( now.minute(), DEC ));
  timeStr.concat( ":" );
  timeStr.concat( String( now.second(), DEC ));
#endif  

#ifdef SHT30
  TempC = sht30.cTemp;
  TempF = sht30.fTemp;
  Humidity = sht30.humidity;
#else
  TempC = dht12.cTemp;
  TempF = dht12.fTemp;
  Humidity = dht12.humidity;
#endif  

  Serial.println();
  Serial.print("Temperature in Celsius : ");
  Serial.println(TempC);
  Serial.print("Temperature in Fahrenheit : ");
  Serial.println(TempF);
  Serial.print("Relative Humidity : ");
  Serial.println(Humidity);
#ifdef BMP
    bmpRet = HP303B.measurePressureOnce(pressure, 7);  
    pressure = pressure/100;
#endif

#ifdef RTC
  Serial.print("Time : ");
  Serial.println(timeStr);

#endif

#ifndef HEADLESS
  tft.fillScreen(ST7735_BLACK);
  tft.setCursor(0, 0);
  tft.setTextSize(2);
  tft.setTextColor(0x006F);
  tft.println(" IoT Temp");
  tft.println("");
  tft.setTextColor(ST7735_WHITE);
  tft.print(" Tmp " );
  tft.setTextColor(ST7735_GREEN);
  #ifdef CELSIUS
  tft.println(TempC);
  #else
  tft.println(TempF);
  #endif
  tft.setTextColor(ST7735_WHITE);
  tft.print(" R/H ");
  tft.setTextColor(ST7735_GREEN);
  tft.println(Humidity);
#ifdef BMP
  tft.setTextColor(ST7735_WHITE);
  tft.print("mBar ");
  tft.setTextColor(ST7735_GREEN);
  tft.println(pressure);
#endif    
  tft.setTextSize(1);
  tft.setTextColor(ST7735_WHITE);
#ifdef RTC
  tft.print( " Time:" );
  tft.println( timeStr );
#endif
  tft.print(" Node:");
  tft.setTextColor(ST7735_GREEN);
  tft.println(nodeName);

#endif

#ifdef WIFI

  if (WiFi.status() != WL_CONNECTED){
    connectWiFi();

  }
  if (WiFi.status() != WL_CONNECTED ) {
#ifndef HEADLESS
   tft.setTextColor(ST7735_RED);
   tft.println("");
   tft.print(" Error:");
   tft.setTextColor(ST7735_GREEN);
   tft.println("No Wifi Conn");
#endif  
  }
  else
  {
#ifndef HEADLESS
   tft.setTextColor(ST7735_WHITE);
   tft.print(" SSID:" );
   tft.setTextColor(ST7735_GREEN);
   tft.println( ssid );
   tft.setTextColor(ST7735_WHITE);
   tft.print("   IP:" );
   tft.setTextColor(ST7735_GREEN);
   tft.println( WiFi.localIP() );

#endif
    Serial.printf("\n[Connecting to %s ... ", host, "\n");
      
    if (client.connect(host, 80))     {
     Serial.println("Connected]");
    Serial.println("[Sending a request]");

    String request  = "GET " ;
           request += "/input/post?node=";
           request += nodeName;
           request += "&fulljson={\"temp\":";
  #ifdef CELSIUS
           request += TempC ;
  #else
           request += TempF ;
  #endif         
           request += ",\"humidity\":" ;
           request += Humidity ;
           request += ",\"BFD\":" ;
           request += ((1/Humidity)*TempC*brFactor) ;
 #ifdef BMP
           request += ",\"Pressure\":" ;
           request += pressure ;
 #endif
           request += "}&apikey=";
           request += APIKEY; 

    Serial.println( request );
    client.println( request );

    Serial.println("[Response:]");

    while (client.connected()) {
     if (client.available()) {
      String resp = "Null";
      resp = client.readStringUntil('\n');  // See what the host responds with.
      Serial.println( resp );
#ifndef HEADLESS
      tft.println();
      tft.println( resp );
#endif
     }
    }
    // client.stop();
    // Serial.println("\n[Disconnected]");
 //   }     // lastRun
   }
   else
   {
    // client.stop();
    Serial.println("Connection failed!]");
 #ifndef HEADLESS
    tft.setTextColor(ST7735_RED);
    tft.print( " " );  
    tft.println( host );
    tft.print(" Connection failed");
 #endif
   }
#endif  // def WIFI
  }  
     lastRun = millis();
  
 }    // Wifi Status 
 }   // Sensor Read
 // delay( poll ); // Set this to whatever you think is OK

}  // Loop

#ifndef HEADLESS
void tftPrint ( char* value, bool newLine, int color ) {
  tft.setTextColor( color );
  if (newLine) {
    tft.println( value );
  }
  else
  {
  tft.print( value);
  }
}
#endif

void connectWiFi() {

#ifdef FIXED_IP  
  WiFi.config(staticIP, gateway, subnet, dns1);
#endif
  WiFi.begin(ssid, password);
//  WiFi.hostname( nodeName );

  String strDebug = ssid ;
  strDebug += "  ";
  strDebug +=  password;
  Serial.println( strDebug );
  
  startWiFi = millis() ;        // When we started waiting
  // Loop and wait 
  while ((WiFi.status() != WL_CONNECTED) && ( (millis() - startWiFi) < waitForWiFi ))
  {
    delay(500);
    Serial.print(".");
  }

#ifndef HEADLESS
  tft.print("");
  Serial.println("");
  Serial.print("IP Address: ");
  Serial.println( WiFi.localIP());
  Serial.printf("Connection status: %d\n", WiFi.status());
#endif

}


#ifdef RTC
const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
    IPAddress ntpServerIP;

    // discard any previously received packets
    while (Udp.parsePacket() > 0) ;

    DEBUG_LOG("Initiating NTP sync\n");

    // get a random server from the pool
    WiFi.hostByName(ntpServerName, ntpServerIP);

    DEBUG_LOG(ntpServerName);
    DEBUG_LOG(" -> ");
    DEBUG_LOG(ntpServerIP);
    DEBUG_LOG("\n");

    sendNTPpacket(ntpServerIP);

    delay(50);
    uint32_t beginWait = millis();

    while ((millis() - beginWait) < 5000)
    {
        DEBUG_LOG("#");
        int size = Udp.parsePacket();

        if (size >= NTP_PACKET_SIZE)
        {

            DEBUG_LOG("Received NTP Response\n");
            Udp.read(packetBuffer, NTP_PACKET_SIZE);

            unsigned long secsSince1900;

            // convert four bytes starting at location 40 to a long integer
            secsSince1900 = (unsigned long)packetBuffer[40] << 24;
            secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
            secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
            secsSince1900 |= (unsigned long)packetBuffer[43];

            // Now convert to the real time.
            unsigned long now = secsSince1900 - 2208988800UL;

#ifdef TIME_ZONE
            DEBUG_LOG("Adjusting time : ");
            DEBUG_LOG(TIME_ZONE);
            DEBUG_LOG("\n");

            now += (TIME_ZONE * SECS_PER_HOUR);
#endif

            return (now);
        }

        delay(50);
    }

    DEBUG_LOG("NTP-sync failed\n");
    return 0;
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
    // set all bytes in the buffer to 0
    memset(packetBuffer, 0, NTP_PACKET_SIZE);

    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    packetBuffer[0] = 0b11100011;   // LI, Version, Mode
    packetBuffer[1] = 0;     // Stratum, or type of clock
    packetBuffer[2] = 6;     // Polling Interval
    packetBuffer[3] = 0xEC;  // Peer Clock Precision

    // 8 bytes of zero for Root Delay & Root Dispersion
    packetBuffer[12] = 49;
    packetBuffer[13] = 0x4E;
    packetBuffer[14] = 49;
    packetBuffer[15] = 52;

    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp:
    Udp.beginPacket(address, 123); //NTP requests are to port 123
    Udp.write(packetBuffer, NTP_PACKET_SIZE);
    Udp.endPacket();
}


void SetRTC() {
//  tft.setCursor( 15, 0 );
  Serial.println( "Setting RTC" );
  unsigned long fromNTP = getNtpTime();   // number of seconds since 1/1/1900
  if ( fromNTP > 0UL  ) {
    unsigned long UnixTime = fromNTP - 2208988800UL;
#ifndef HEADLESS
    rtc.adjust( UnixTime + timeZoneOffset );
    tft.print("*" );
#endif    
  }
  else {
  }
#ifndef HEADLESS    
    tft.print("X");
#endif
  }
}

#endif

#ifdef WIFI
void handleRoot() {
  String response = "<h1>Welcome to Iot Temp </h1>";
         response += "Node Name <b>" + String(nodeName) + "</b>"; 
         response += "   Local IP is: <b>" + WiFi.localIP().toString() + "</b><br>";
         response += "Temperature <b>" + String(TempC) + "C</b><br>";
         response += "Humidity <b>" + String(Humidity) + " %RH</b><br>";
         response += "Air Pressure <b>" + String(pressure) + " millibars</b><br>";
         
  server.send(200, "text/html", response );   // Send HTTP status 200 (Ok) and send some text to the browser/client
}

void handleNotFound(){
  server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}
#endif
