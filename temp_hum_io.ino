#include <ESP8266WiFi.h>
#include <ESP_Mail_Client.h>
#include <Arduino.h> 
#define SENSOR  D2
#include "config.h"

#include "DHT.h"

#define DHTPIN 2
#define SMTP_HOST "smtp.gmail.com"

#define SMTP_PORT esp_mail_smtp_port_587

#define AUTHOR_EMAIL "esp32mailsys@gmail.com"
#define AUTHOR_PASSWORD "crvsevkohtxlfqte"

SMTPSession smtp;

void smtpCallback(SMTP_Status status);
void alert();

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
   if (temperature>35 || humidity<50 || sensor::tds>800 )
  {
    alert();
  }
  delay(5000);
  
}
void alert()
{
  #if defined(ARDUINO_ARCH_SAMD)
  while (!Serial)
    ;
  Serial.println();
  Serial.println("**** Custom built WiFiNINA firmware need to be installed.****\nTo install firmware, read the instruction here, https://github.com/mobizt/ESP-Mail-Client#install-custom-built-wifinina-firmware");

#endif

  Serial.println();

  Serial.print("Connecting to AP");

  

 
  smtp.debug(1);

  /* Set the callback function to get the sending results */
  smtp.callback(smtpCallback);

  /* Declare the session config data */
  ESP_Mail_Session session;

  /* Set the session config */
  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = AUTHOR_EMAIL;
  session.login.password = AUTHOR_PASSWORD;
  session.login.user_domain = F("mydomain.net");

  /* Set the NTP config time */
  session.time.ntp_server = F("pool.ntp.org,time.nist.gov");
  session.time.gmt_offset = 3;
  session.time.day_light_offset = 0;

  /* Declare the message class */
  SMTP_Message message;

  /* Set the message headers */
  message.sender.name = F("ESP Alert mail");
  message.sender.email = AUTHOR_EMAIL;
  message.subject = F("Alert meassage");
  message.addRecipient(F("goodboy"), F("jimson.ags@gmail.com"));

  String textMsg = "There was a problem in ur device please check";
  message.text.content = textMsg;

  message.text.charSet = F("us-ascii");

  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

  // If this is a reply message
  // message.in_reply_to = "<parent message id>";
  // message.references = "<parent references> <parent message id>";

  
  
  message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;

  // message.response.reply_to = "someone@somemail.com";
  // message.response.return_path = "someone@somemail.com";


   
  // message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;

  /* Set the custom message header */
  message.addHeader(F("Message-ID: <abcde.fghij@gmail.com>"));

  // For Root CA certificate verification (ESP8266 and ESP32 only)
  // session.certificate.cert_data = rootCACert;
  // or
  // session.certificate.cert_file = "/path/to/der/file";
  // session.certificate.cert_file_storage_type = esp_mail_file_storage_type_flash; // esp_mail_file_storage_type_sd
  // session.certificate.verify = true;

  // The WiFiNINA firmware the Root CA certification can be added via the option in Firmware update tool in Arduino IDE

  /* Connect to server with the session config */
  if (!smtp.connect(&session))
    return;

  /* Start sending Email and close the session */
  if (!MailClient.sendMail(&smtp, &message))
    Serial.println("Error sending Email, " + smtp.errorReason());

  // to clear sending result log
  // smtp.sendingResult.clear(); 

  ESP_MAIL_PRINTF("Free Heap: %d\n", MailClient.getFreeHeap());
  delay(60000); 
}
void smtpCallback(SMTP_Status status)
{
  /* Print the current status */
  Serial.println(status.info());

  /* Print the sending result */
  if (status.success())
  {
    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failled: %d\n", status.failedCount());
     Serial.println("----------------\n");
    struct tm dt;

    for (size_t i = 0; i < smtp.sendingResult.size(); i++)
    {
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(i);
      time_t ts = (time_t)result.timestamp;
      localtime_r(&ts, &dt);

      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      ESP_MAIL_PRINTF("Date/Time: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
    }
    Serial.println("----------------\n");

    // You need to clear sending result as the memory usage will grow up.
    smtp.sendingResult.clear();
  }
}
