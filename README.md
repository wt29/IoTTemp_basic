# IotaTemp_basic
Wemos D1 mini and DHT12 Sensor log to internets

Uses a Wemos/Lolin D1 mini hooked up to the Wemos DHT v3.0.0 shield that logs temp and humidity up to an emoncms server.

You can create your own emoncms server, the code is open source or create an account at emoncms.org.

Once you have an account, subsitute the values in the #defines in the top of the sketch, recompile and upload. Your values should appear in emoncms/inputs almost immediately. 
