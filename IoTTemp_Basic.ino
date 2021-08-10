/* 
IOT Temp - the little weather station that could.  Read/Display/Log. 

Featuring the LOLIN D1 ESP 8266,  and associated shields as desired.
https://lolin.aliexpress.com/store/1331105?spm=a2g0o.detail.1000007.1.277c6380JG6A1m

You will need a file "data.h" - copy the template below.
if you have lots of these keep the individual configs in myDeviceName.h for easy reference to save remembering config and board versions.
Occasionaly you may need to add an extra setting from the template below if you are enabling new features.
e.g. create a "kitchen.h" and change the local code to just include kitchen.h?

Also add that file.h or *.h to the .gitignore so you dont upload your wifi password to github!

-------------------------------------
//template for data.h

//** Node and Network Setup
#define NODENAME "<Your NodeName>";                 // eg "Kitchen"  Required and UNIQUE per site.  Also used to find mdns eg NODENAME.local

#define WIFI                                        // Default is true to enable WiFi
#define LOCALSSID "<Your WiFi SSID>";               // eg "AwesomeWifi"  Required Can't use plain SSID as the WiFi library now defines it.
#define PASSWORD "<Your WiFI Password>";            // eg "HorseStapleSomething"  Required

#define HOST "<Your emoncms host fqdn>";            // eg  "emoncms.org" Required for logging. Note:just the host not the protocol
#define MYAPIKEY "<Your emoncms API write key>";    // Required Get it from your MyAccount details in your emoncms instance

//Enable the following block to your data.h to set fixed IP addresses. Configure as required
//#define STATIC_IP
//IPAddress staticIP( 192,168,1,22 );
//IPAddress gateway( 192,168,1,1 );
//IPAddress subnet(255,255,255,0 );
//IPAddress dns1( 8,8,8,8 );


// **Configuration and Shield Options

//#define CONNECTOR_100 // 100 series shield otherwise defaults to 1.1.0.  
                      // Note that some shields show 1.1.0 but are really version 1.0.0.  
                      // If your TFT stays "white" or is blank on bootup then you probably have a 1.0.0 regardless of branding.

//#define HEADLESS      // Define if you don't have a display. Defaults to true


// **Sensors

//- Air Quality
//#define AIRQUALITY    // enable SGP30 Shield 
                        // TVOC: (Total Volatile Organic Compound) concentration within a range of 0 to 60,000 parts per billion (ppb)
                        // Note: Note that TVOC in ppm is  EU standard.  https://help.atmotube.com/faq/5-iaq-standards/
                        // [] Action: Discuss - best way.  We can report as ppm but emon rounds to 2decimal places so we lose granularity...?
                        // eCO2: (equivalent calculated carbon-dioxide) concentration  400-60000 ppm
                        //
                        // While this shield can be run in isolation its better to get the actual temp and humidity inputs
                        // for more accurate calculations - in our case we feed it data from our temp/humidity shield.
                        // https://www.wemos.cc/en/latest/d1_mini_shield/sgp30.html
                        // Library Manager should find it but...  https://github.com/adafruit/Adafruit_SGP30
                        // docs: https://adafruit.github.io/Adafruit_SGP30/html/class_adafruit___s_g_p30.html#a3cea979c8b14138cef092f13102b0e22

                        
//- temperature and humidity
//#define HASDHT12        // If you have the older DHT12 otherwise will default to SHT30
                          // DHT12 temperature and humidity sensor was originally used but humidity not accurate. Deprecated!
                          // SHT30 is default and best bang for buck at present.

//- Barometer
//#define BMP             // Define to enable Barometric Air Pressure Shield Libraries and Logging 
//#define LOCALALTITUDE 300;  // Required if using BMP.  Enter your local altitude in meters eg 300
                              // The reported pressure is corrected  by currentSensorReading + ((117/1000)*YourLocalAltitude).  
                          // Notes Barometric Pressure readings need to be calibrated by 117 for every rise of 1000m above sea level.  
                          // eg for 300m abobe sea level, the calc is 0.117 * 300 = 35

//- Light meter
//placeHolder for now

//- water
//placeHolder for now

//- wind speed
//- wind angle 
//placeHolders for now

//#define BFDLOGGING    // Define to enable logging BushFireFactor to the server. 
                      // Non Scientific but useful enough.  Plan to incorporate wind speed/rainfall in future.

//#define BRFACTOR 1;   // Bushfire Rating Factor (Multiplier).  Default is 44 (for granularity/graphing purposes/100).  Define (uncomment) your own value.
// --end of data.h

-------------------------------------

Trying to do this in both Arduino IDE and PlatformIO is too hard - Stick to Arduino

Additional Libraries for DHT12 and SHT30 etc.  Download and save to user documents
They show a warning on compile but are fine.
https://github.com/wemos

*/
#define VERSION 1.31            // 1.31 Enable SGP30 Shield V1.0.0 AIR QUALITY SENSOR
                                // 1.30 Change to BMPaltitude to calibrate from defined LOCALALTITUDE in data.h              
                                // 1.29 Add BMPaltitude to data.h.  Tweak to comments for consistency.  Expanded notes in template and cleaned up code a little more.
                                // 1.28 Moved the Static IP options to the data.h. Sensor now defaults to SHT30
                                // 1.27 Removed Real Time Clock (RTC) routines. Only useful if RTC and SD Card logging available.
                                // 1.26 Tweaks to wording for data.h.  New Defaults for CONNECTOR and BFD logging.  Prep for new shields. Mild code refactor.
                                // 1.25 WebClient
                                // 1.24 Pressure and headless operation 
                                // 1.23 Bushfire danger feeds - now defaults to 44
                                // edit bushFireRatingFactor to taste

#warning Setup your data.h.  Refer to template in code.

//debug mode
#define DEBUG
// If we define then DEBUG_LOG will log a string, otherwise
// it will be ignored as a comment.
#ifdef DEBUG
#  define DEBUG_LOG(x) Serial.print(x)
#else
#  define DEBUG_LOG(x)
#endif


#define WIFI

//Node and Network Setup
#ifdef WIFI
  #include <ESP8266WiFi.h>
  #include <WiFiClient.h>
  #include <WiFiUdp.h>
  #include <ESP8266mDNS.h>
  #include <ESP8266WebServer.h>   // Include the WebServer library
#endif

// Needed to move this here as the IPAddress types aren't declared until the WiFi libs are loaded
// #include "data.h"             // Create this file from template above.  
//                                  Update here if you changed the name.
//                                  This means we dont keep uploading API key+password to GitHub. (data.h should be ignored in repository)
#include "Outside.h"


#ifndef HEADLESS                 // no screen
 #include <Adafruit_GFX.h>       // Core graphics library
 #include <Adafruit_ST7735.h>    // Hardware-specific library
#endif

const char* nodeName = NODENAME;
const char* ssid = LOCALSSID;
const char* password = PASSWORD;
const char* host = HOST;
const char* APIKEY = MYAPIKEY;


//Configuration and Shield Options

//connector shield version (Load Library and Instantiate)
#ifndef CONNECTOR_100    // the v1.1.0 connector board has different CS and DC values
  #define TFT_CS     D4
  #define TFT_DC     D3
#else                    // Otherwise use the 1.0.0 values
  #define TFT_CS     D0
  #define TFT_DC     D8
#endif

//headless vs TFT?
#ifndef HEADLESS
  #define TFT_RST    -1       // you can also connect this to the Arduino reset. in which case, set this #define pin to -1!
  Adafruit_ST7735 tft = Adafruit_ST7735( TFT_CS, TFT_DC, TFT_RST);    // Instance of tft
#endif

//air quality shield 
#ifdef AIRQUALITY //Air Quality Shield
  #include "Adafruit_SGP30.h"
  Adafruit_SGP30 sgp30;         //SPG3030 Air quality instance
  float AQ_TVOC;    //  ppb Total Volatile Organic Components
  float AQ_eCO2;    //  ppm estimated concentration of carbon dioxide calculated from known TVOC concentration
#endif

#ifdef BMP    // Barometric Pressure shield
  #include <LOLIN_HP303B.h>
  LOLIN_HP303B HP303B;         // HP303B BMP instance
  int32_t pressure;
  int16_t bmpRet;

  //setup and calculate the correct barometer pressure for your altitude based on data.h
  int localAltitude = LOCALALTITUDE;
  float BMPCorrection = ( localAltitude * 0.117 );
  float pressureMSL;
#endif

//temperature and humidity shield
#ifndef HASDHT12
  #include <WEMOS_SHT3X.h>      // SHT30 Current best bang for back in typical human/environment temp and humidity ranges
  SHT3X sht30(0x45);
#else
  #include <WEMOS_DHT12.h>      // depreciated DHT12 temperature and humidity sensor. 
  DHT12 dht12;                  
#endif

//water logging placeholder
//wind speed logging placeholder
//wind angle logging placeholder

//Do we want to log BushFire Danger Stuff? 
#ifndef BFDLOGGING      // All defines should be UC
  //nothing needed for now
#else
  //logging so we need to setup BRFactor etc.
  //BushFire Rating Factor (amount to multiply the result by for logging purposes)
  #ifndef BRFACTOR
    int brFactor = 44;
  #else
    int brFactor = BRFACTOR;
  #endif
#endif
 
//Working Variables etc.
#define CELSIUS             // Comment out if you prefer Fahrenheit
float TempC;
float TempF;
float Humidity;

#ifdef WIFI
  WiFiClient client;              // Instance of WiFi Client
  ESP8266WebServer server(80);    // Create a webserver object that listens for HTTP request on port 80
  void handleRoot();              // function prototypes for HTTP handlers
  void handleNotFound();
#endif

int waitForWiFi = 20000 ;         // How long to wait for the WiFi to connect - 10 Seconds should be enough 
int startWiFi;
int connectMillis = millis();     // this gets reset after every successful data push

int poll = 60000;           // Poll the sensor every 60 seconds (or so)
int lastRun = millis() - (poll + 1);

void setup()
{
  Serial.begin(115200); //baud rate
  Serial.println();
  
#ifdef AIRQUALITY
  sgp30.begin(); // startup/calibrate the air quality shield
  sgp30.IAQinit();
#endif

#ifdef BMP
  HP303B.begin(); // I2C address = 0x77
#endif

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
}       // Setup

void loop() {

 if ( millis() > 14400000) {   // Reboot every 4 hours - I have crappy internet. You may not need this
      Serial.println("Rebooting");
      ESP.restart();           // Kick it over and try from the beginning
  }

#ifdef WIFI 
 server.handleClient();                    // Listen for HTTP requests from clients
#endif

if ( millis() > lastRun + poll ) {        // only want this happening every so often - see Poll value

#ifndef HASDHT12
  Serial.println("Reading SHT30 Temperature/Humidity Shield");
 if( !(sht30.get() == 0 ) ){
#else  
  Serial.println("Reading DHT12 Temperature/Humidity Shield");
 if( !(dht12.get() == 0 ) ){
#endif
  Serial.println("Cannot read Temperature/Humidity Shield");
  #ifndef HEADLESS
    tft.println("Temp/Humidity Shield Error");
  #endif
  
 }
 else
 {
  

#ifndef HASDHT12
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

#ifdef AIRQUALITY
  Serial.println("Reading sgp30 Air Quality Shield");

  //set the absolute humidity to enable humidity compensation for the air quality signals
  sgp30.setHumidity(Humidity);
  
  if (sgp30.IAQmeasure())
  {
    AQ_eCO2 = sgp30.eCO2;
    AQ_TVOC = (sgp30.TVOC); // [] Do we end up converting from ppb to ppm  ??
    Serial.print("TVOC "); Serial.print(AQ_TVOC); Serial.print(" ppb\t");
    Serial.print("eCO2 "); Serial.print(AQ_eCO2); Serial.println(" ppm");
    Serial.print("Raw H2 "); Serial.print(sgp30.rawH2); Serial.print(" \t");
    Serial.print("Raw Ethanol "); Serial.print(sgp30.rawEthanol); Serial.println("");
   }
  else
  {
  Serial.println("Cannot read AQ Sensor");
  #ifndef HEADLESS
    tft.println("AQ  Sensor Error");
  #endif
  }
#endif

#ifdef BMP
  Serial.println("Reading HP303B Barometric Pressure Shield");
  bmpRet = HP303B.measurePressureOnce(pressure, 7);  
  pressure = pressure/100;                    // only interested in millbars not pascals
  pressureMSL = ( pressure + BMPCorrection ); // adjust for altitude defined in data.h
  Serial.print("Pressure mBar : ");
  Serial.println(pressureMSL);
#endif

  Serial.print("Free Heap : ");
  Serial.println(ESP.getFreeHeap());

#ifndef HEADLESS
  tft.fillScreen(ST7735_BLACK);
  tft.setCursor(0, 0);
  tft.setTextSize(2);
  tft.setTextColor(ST7735_ORANGE);   // Can't read the "Dark Blue"
  tft.println(" IoT Temp");
  //tft.println(""); //PB Needed an extra line on the screen
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
    int intMSL = pressureMSL;
    tft.println( intMSL );
  #endif    

  #ifdef AIRQUALITY
    tft.setTextSize(1); //doesnt need to be as large as other key data.
    tft.setTextColor(ST7735_WHITE);
    tft.print("  eCO2 ");
    tft.setTextColor(ST7735_GREEN);
    int inteCO2 = AQ_eCO2;
    tft.print( inteCO2 );
    tft.setTextColor(ST7735_WHITE);
    tft.print("  TVOC ");
    tft.setTextColor(ST7735_GREEN);
    int intTVOC = AQ_TVOC;
    tft.println( intTVOC );
    tft.println("");
    tft.setTextSize(2); //back to defaults
  #endif 
     
  tft.setTextSize(1);
  tft.setTextColor(ST7735_WHITE);
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

  #ifdef BFDLOGGING
           request += ",\"BFD\":" ;
           request += ((1/Humidity)*TempC*brFactor) ;
  #endif

  #ifdef AIRQUALITY
          request += ",\"eCO2\":" ;
          request += inteCO2 ;
          request += ",\"TVOC\":" ;
          request += intTVOC ;          
  #endif
  
  #ifdef BMP
           request += ",\"Pressure\":" ;
           request += pressureMSL ;
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
   }
   else
   {
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
 }    // Sensor Read

}     // Loop

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

#ifdef STATIC_IP  
 WiFi.config( staticIP, gateway, subnet, dns1 );
#endif
  WiFi.begin(ssid, password);
  WiFi.hostname( nodeName );     // This will show up in your DHCP server

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

#ifdef WIFI
void handleRoot() {
  String response = "<h1>Welcome to IoT Temp </h1>";
         response += "Temperature <b>" + String(TempC) + "C</b><br>";
         response += "Humidity <b>" + String(Humidity) + " %RH</b><br>";
  #ifdef BMP
         response += "Air Pressure local <b>" + String(pressure) + " millibars</b><br>";
         response += "Air Pressure MSL <b>" + String(pressureMSL) + " millibars</b><br>";
  #endif
  #ifdef BFDLOGGING
         response += "Bushfire Rating <b>"+ String((1/Humidity)*TempC*brFactor) + " </b><br>";
  #endif

  #ifdef AIRQUALITY
         response += "<br>";
         response += "Air Quality eCO2 <b>"+ String(AQ_eCO2) + " ppm</b><br>";
         response += "Air Quality TVOC <b>"+ String(AQ_TVOC) + " ppb</b><br>";
  #endif

         response += "<br>";
         response += "Node Name <b>" + String(nodeName) + "</b><br>"; 
         response += "Local IP is: <b>" + WiFi.localIP().toString() + "</b><br>";
         response += "Free Heap Space <b>" + String(ESP.getFreeHeap()) + " bytes</b><br>";
         response += "Software Version <b>" + String(VERSION) + "</b>";
         
  server.send(200, "text/html", response );   // Send HTTP status 200 (Ok) and send some text to the browser/client
}

void handleNotFound(){
  server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}
#endif
