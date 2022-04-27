#include <ESP8266WiFi.h>
#define SENSOR  D2
#include "config.h"

#include "DHT.h"

#define DHTPIN 2


// Digital pin connected to the DHT sensor
#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

// Connect pin 1 (on the left) of the sensor to +5V
// NOTE: If using a board with 3.3V logic like an Arduino Due connect pin 1
// to 3.3V instead of 5V!
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 3 (on the right) of the sensor to GROUND (if your sensor has 3 pins)
// Connect pin 4 (on the right) of the sensor to GROUND and leave the pin 3 EMPTY
//(if your sensor has 4 pins)
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

// Initialize DHT sensor.
// Note that older versions of this library took an optional third parameter to
// tweak the timings for faster processors.  This parameter is no longer needed
// as the current DHT reading algorithm adjusts itself to work on faster procs.
DHT dht(DHTPIN, DHTTYPE);
// this int will hold the current count for our sketch
int count = 0;

// set up the 'counter' feed
AdafruitIO_Feed *counter = io.feed("temperature");

AdafruitIO_Feed *counter2 = io.feed("humidity");

AdafruitIO_Feed *counter3 = io.feed("water_current");

AdafruitIO_Feed *counter4 = io.feed("TDS");


long currentMillis = 0;
long previousMillis = 0;
int interval = 1000;
//boolean ledState = LOW;
float calibrationFactor = 4.5;
volatile byte pulseCount;
byte pulse1Sec = 0;
float water_current;
float liquid_level;
int liquid_percentage;
int top_level = 500;//Maximum water level
int bottom_level = 0;//Minimum water level

namespace pin 
{
const byte tds_sensor = A0;   // TDS Sensor
}
 
namespace device 
{
float aref = 3.3;
}
 
namespace sensor 
{
float ec = 0;
unsigned int tds = 0;
float ecCalibration = 1;
} 
void IRAM_ATTR pulseCounter()
{
  pulseCount++;
}


void setup() {

  // start the serial connection
  Serial.begin(115200);

  // wait for serial monitor to open
  while(!Serial);

  Serial.print("Connecting to Adafruit IO");

  // connect to io.adafruit.com
  io.connect();

  // wait for a connection
  while(io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
 
  // we are connected
  Serial.println();
  Serial.println(io.statusText());
   pinMode(SENSOR, INPUT_PULLUP);
   pulseCount = 0;
   water_current = 0.0;
   previousMillis = 0;
   attachInterrupt(digitalPinToInterrupt(SENSOR), pulseCounter, FALLING);
   dht.begin();
}

void loop() {

  // io.run(); is required for all sketches.
  // it should always be present at the top of your loop
  // function. it keeps the client connected to
  // io.adafruit.com, and processes any incoming data.
  io.run();
  double waterTemp;
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  currentMillis = millis();
  
  if (currentMillis - previousMillis > interval) {
    pulse1Sec = pulseCount;
    pulseCount = 0;
    water_current = ((1000.0 / (millis() - previousMillis)) * pulse1Sec) / calibrationFactor;
    previousMillis = millis();
    counter3->save(water_current);
  };
  waterTemp  = temperature*0.0625; // conversion accuracy is 0.0625 / LSB

  float rawEc = analogRead(pin::tds_sensor) * device::aref / 1024.0; // read the analog value more stable by the median filtering algorithm, and convert to voltage value
  float temperatureCoefficient = 1.0 + 0.02 * (waterTemp - 25.0); // temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
            
  sensor::ec = (rawEc / temperatureCoefficient) * sensor::ecCalibration; // temperature and calibration compensation
  sensor::tds = (133.42 * pow(sensor::ec, 3) - 255.86 * sensor::ec * sensor::ec + 857.39 * sensor::ec) * (0.5); //convert voltage value to tds value
    counter4->save(sensor::tds);
  delay(5000);
  
  // save count to the 'counter' feed on Adafruit IO
  Serial.print("sending -> ");
  Serial.println(count);
  counter->save(temperature);
  delay(5000);
   counter2->save(humidity);
  delay(5000);
  
  
  
  // increment the count by 1
  count++;

  // Adafruit IO is rate limited for publishing, so a delay is required in
  // between feed->save events. In this example, we will wait three seconds
  // (1000 milliseconds == 1 second) during each loop.
  delay(5000);
  
}
