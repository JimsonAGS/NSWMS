
#include "config.h"

#define watersensor A3
const int potPin=A0;

int count = 0;
float water_level;

float ph;
float Value=0;


// set up the 'counter' feed
AdafruitIO_Feed *counter = io.feed("water_level");

AdafruitIO_Feed *counter1 = io.feed("pH");

void setup() {

  // start the serial connection
  Serial.begin(115200);
  pinMode(potPin,INPUT);
  // wait for serial monitor to open
  while(! Serial);

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

}

void loop() {

  // io.run(); is required for all sketches.
  // it should always be present at the top of your loop
  // function. it keeps the client connected to
  // io.adafruit.com, and processes any incoming data.
  io.run();

  // save count to the 'counter' feed on Adafruit IO
  Serial.print("sending -> ");
  Serial.println(count);
  water_level = (analogRead(watersensor))/4;
  counter->save(water_level);
  delay(5000);
  Value= analogRead(potPin);
  float voltage=Value*(3.3/4095.0);
  ph=(3.3*voltage);
  counter1->save(ph);
  // increment the count by 1
  count++;

  // Adafruit IO is rate limited for publishing, so a delay is required in
  // between feed->save events. In this example, we will wait three seconds
  // (1000 milliseconds == 1 second) during each loop.
  delay(5000);

}
