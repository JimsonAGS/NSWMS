#include <Arduino.h> 
#include "config.h"
#include <ESP_Mail_Client.h>
#define watersensor A3

#define SMTP_HOST "smtp.gmail.com"

#define SMTP_PORT esp_mail_smtp_port_587

#define AUTHOR_EMAIL "esp32mailsys@gmail.com"
#define AUTHOR_PASSWORD "crvsevkohtxlfqte"

SMTPSession smtp;

void smtpCallback(SMTP_Status status);
void alert();
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


  io.run();

  
  Serial.print("sending -> ");
  Serial.println(count);
  water_level = (analogRead(watersensor));
  counter->save(water_level);
  delay(5000);
  Value= analogRead(potPin);
  float voltage=Value*(3.3/4095.0);
  ph=(3.3*voltage);
  counter1->save(ph);
 
  count++;
  if (water_level > 1600 || ph == 0){
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
