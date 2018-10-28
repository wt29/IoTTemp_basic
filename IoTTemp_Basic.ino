/* 
IOT Temp - a somewhat basic temperature and humidity logger. 

Featuring the LOLON D1 ESP 8266 and associated shields.

You will need a file "data.h" which looks like this
-----------------------
#define SSID "<Your WiFi SSID>";
#define PASSWORD "<Your WiFI Password>";
#define HOST "<Your emoncms host - most likely emoncms.org>";
#define MYAPIKEY "<Your API write key for emoncms>";
#define NODENAME "<Your NodeName - Kitchen for example";
-------------------------------------
*/
#warning Setup your data.h

#include <ESP8266WiFi.h>
#include <WEMOS_DHT12.h>

#include <Adafruit_GFX.h>    	// Core graphics library
#include <Adafruit_ST7735.h>	// Hardware-specific library

#define CONNECTOR_10      // the v1.1.0 connector board has different CS and DC values
                          // comment out if you have a v1.1.0 board

#ifdef CONNECTOR_10
 #define TFT_CS     D4
 #define TFT_DC     D3
#else                    
 #define TFT_CS     D0
 #define TFT_DC     D8
#endif

#define TFT_RST    -1  			// you can also connect this to the Arduino reset
					// in which case, set this #define pin to -1!

#define CELSIUS             		// Comment out if you prefer Fahrenheit

#include "data.h"           		// Means I don't keep uploading my API key to GitHub

const char* ssid = SSID;
const char* password = PASSWORD;
const char* host = HOST;
const char* APIKEY = MYAPIKEY;
const char* nodeName = NODENAME;

Adafruit_ST7735 tft = Adafruit_ST7735( TFT_CS, TFT_DC, TFT_RST);    // Instance of tft
DHT12 dht12;                                                        // Instance of dht12
WiFiClient client;                                                  // Instance of WiFi Client

float TempC;
float TempF;
float Humidity;

int waitForWiFi = 10000 ;  		// How long to wait for the WiFi to connect - 10 Seconds should be enough 
int startWiFi;
int connectMillis = millis(); 		// this gets reset after every successful data push

int poll = 60000;     			// Poll the sensor every 60 seconds (or so)

void setup()
{
  Serial.begin(115200);
  Serial.println();

  tft.initR(INITR_144GREENTAB);
  tft.setTextWrap(false); 		// Allow text to run off right edge
  tft.setRotation( 1 );			// Portrait mode
  tft.fillScreen(ST7735_BLACK);

  WiFi.begin(ssid, password);

  Serial.print("Connecting");
  tft.setTextSize(2);
  tft.setCursor(0,0);
  tft.setTextColor(ST7735_BLUE);
  tft.println( "Connecting" );

  startWiFi = millis() ;    		// When we started waiting
  // Loop and wait 
  while ((WiFi.status() != WL_CONNECTED) && ( (millis() - startWiFi) < waitForWiFi ))
  {
    delay(500);
    Serial.print(".");
    tft.print(".");    			// Show that it is trying
  }
  
}

void loop() {

 if ( millis() > 14400000) {   // Reboot every 4 hours - I have crappy internet. You may not need this
      Serial.println("Rebooting");
      ESP.restart();           // Kick it over and try from the beginning
  }

 if( !(dht12.get() == 0 ) ){
  Serial.println("Cannot read DHT12 Sensor");
  tft.println("DHT12 Error");
  
 }
 else
 {
  TempC = dht12.cTemp;
  TempF = dht12.fTemp;
  Humidity = dht12.humidity;

  Serial.print("Temperature in Celsius : ");
  Serial.println(TempC);
  Serial.print("Temperature in Fahrenheit : ");
  Serial.println(TempF);
  Serial.print("Relative Humidity : ");
  Serial.println(Humidity);
  Serial.println();
  
  tft.fillScreen(ST7735_BLACK);
  tft.setCursor(0, 0);
  tft.setTextSize(2);
  tft.setTextColor(ST7735_BLUE);
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
  tft.println();

  tft.setTextSize(1);
  tft.setTextColor(ST7735_WHITE);
  tft.print(" Node:");
  tft.setTextColor(ST7735_GREEN);
  tft.println(nodeName);

  if (WiFi.status() != WL_CONNECTED ) {
   tft.setTextColor(ST7735_RED);
   tft.println("");
   tft.print(" Error:");
   tft.setTextColor(ST7735_GREEN);
   tft.println("No Connection");
  
  }
  else
  {
   tft.setTextColor(ST7735_WHITE);
   tft.print(" SSID:" );
   tft.setTextColor(ST7735_GREEN);
   tft.println( ssid );
   tft.setTextColor(ST7735_WHITE);
   tft.print("   IP:" );
   tft.setTextColor(ST7735_GREEN);
   tft.println( WiFi.localIP() );

   Serial.printf("\n[Connecting to %s ... ", host, "\n");
      
   if (client.connect(host, 80))     {
    Serial.println("Connected]");
    Serial.println("[Sending a request]");

    String request  = "GET " ;
           request += "/input/post?node=";
           request += nodeName;
           request += "&fulljson={\"temp\":";
  #ifdef CELSIUS
           request += dht12.cTemp ;
  #else
           request += dht12.fTemp ;
  #endif         
           request += ",\"humidity\":" ;
           request += dht12.humidity ;
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
      tft.println( resp );
     }
    }
    client.stop();
    Serial.println("\n[Disconnected]");

   }
   else
   {
    client.stop();
    Serial.println("Connection failed!]");
    tft.setTextColor(ST7735_RED);
    tft.print( " " );  
    tft.println( host );
    tft.print(" Connection failed");
   }
  }  
 }
  
 delay( poll ); // Set this to whatever you think is OK

}

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


