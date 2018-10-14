#include <ESP8266WiFi.h>
#include <WEMOS_DHT12.h>
#include <SPI.h>

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

#define CELSIUS             // Comment out if you prefer Fahrenheit

#include "data.h"           // Means I don't keep uploading my API key to GitHub
/*
You will need a file "data.h" which looks like this
-----------------------
#define SSID "<Your WiFi SSID>";
#define PASSWORD "<Your WiFI Password>";
#define HOST "<Your emoncms host - most likely emoncms.org>";
#define MYAPIKEY "<Your API write key for emoncms>";
-------------------------------------
*/
const char* ssid = SSID;
const char* password = PASSWORD;
const char* host = HOST;
const char* APIKEY = MYAPIKEY;

const char* nodeName = "External";

Adafruit_ST7735 tft = Adafruit_ST7735( TFT_CS, TFT_DC, TFT_RST);    // Instance of tft
DHT12 dht12;                                                        // Instance of dht12
WiFiClient client;                                                  // Instance of WiFi Client

float TempC;
float TempF;
float Humidity;

int waitForWiFi = 10000 ;  		// How long to wait for the WiFi to connect - 10 Seconds should be enought   
int startWiFi;

int poll = 10000;     			// Poll the sensor every 10 seconds (or so)

void setup()
{
  Serial.begin(115200);
  Serial.println();

  tft.initR(INITR_144GREENTAB);
  tft.setTextWrap(false); // Allow text to run off right edge
  tft.setRotation( 1 );
  tft.fillScreen(ST7735_BLACK);

  WiFi.begin(ssid, password);

  Serial.print("Connecting");
  tft.setTextColor(ST7735_BLUE);
  tft.setTextSize(2);
  tft.setCursor(0,0);
  tft.println("Connecting");
    
  startWiFi = millis() ;     // When we started waiting
  
  while ((WiFi.status() != WL_CONNECTED) && ( (millis() - startWiFi) < waitForWiFi ))
  {
    delay(500);
    Serial.print(".");
	tft.print(".");    		// Show that it is working
  }
  
}

void loop() {

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
  tft.setTextColor(ST7735_BLUE);
  tft.setCursor(0, 0);
  tft.setTextSize(2);
  tft.println(" IoT Temp");
  tft.println("");
  tft.setTextColor(ST7735_WHITE);
  tft.print(" Tmp ");
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
   tft.print(" SSID:");
   tft.setTextColor(ST7735_GREEN);
   tft.println(ssid);
   tft.setTextColor(ST7735_WHITE);
   tft.print("   IP:");
   tft.setTextColor(ST7735_GREEN);
   tft.println(WiFi.localIP());

   Serial.printf("\n[Connecting to %s ... ", host, "\n");
   Serial.println();
      
   if (client.connect(host, 80))     {
    Serial.println("Connected]");
    Serial.println("[Sending a request]");

    String request  = "GET " ;
           request += "/input/post?node=";
           request += nodeName;
           request += "&fulljson={\"temp\":";
           request += dht12.cTemp ;
           request += ",\"humidity\":" ;
           request += dht12.humidity ;
           request += "}&apikey=";
           request += APIKEY; 

    Serial.println( request );
    client.println( request );

    Serial.println("[Response:]");

    while (client.connected()) {
     if (client.available()) {
      String line = client.readStringUntil('\n');  // See what the host responds with.
      Serial.println(line);
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
 delay( poll ); // every 10 seconds should be enough
}
