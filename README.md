# IotaTemp_basic
Wemos D1 mini and DHT12 Sensor logs to the internet.

Uses a Wemos/Lolin D1 mini hooked up to the Wemos DHT v3.0.0 shield that logs temp and humidity up to an emoncms server.

You can create your own emoncms server, the code is open source or create an account at emoncms.org.

Create a local data.h and substitute your values as shown in the ino file.

Once you have an account, substitute the values in the #data.h recompile and upload. 

Your values should appear in emoncms/inputs almost immediately. 

BOM
WeMOS D1 Mini
TFT 1.4 Shield
SHT 3.0 Temp and Humidity Shield
TFT I2C connection shield - No strictly required but is makes remote connection of the Temp shield possible.
