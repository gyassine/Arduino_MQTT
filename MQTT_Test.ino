/***************************************************
  Adafruit MQTT Library WINC1500 Modified by Yassine

  Demo written by Limor Fried/Ladyada for Adafruit Industries.
  Modified by Yassine Gangat for LE²P.

  MIT license, all text above must be included in any redistribution
 ****************************************************/
#include <SPI.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <WiFi101.h>

// ---------------------------- GROVE LCD Lib -------------------------
//#include <Wire.h>           // i2c LIB to use LCD
#include "rgb_lcd.h"        // lcd LIB

#include "arduino_secrets.h"
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
const char ssid[] = SECRET_SSID;        // your network SSID (name)
const char pass[] = SECRET_PASS;       // your network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS;     // the WiFi radio's status
// int keyIndex = 0;                // your network key Index number (needed only for WEP)

// Initialise the Arduino data pins for INPUT/OUTPUT
const int analogInPin = A0;
const int RELAY2 = 8;
const int mVperAmp = 185; // use 100 for 20A Module and 66 for 30A Module


/************************* Broker Setup **********************************/

#define SERVER      "192.168.2.2"
#define SERVERPORT  1883
#define USERNAME    "SolarPan"   // NetwEmul - CtrlLoad - SolarPan
#define PWD         "apqm"

/******************************* Global State  ****************************/

//Set up the wifi client & Adafruit MQTT Client
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, SERVER, SERVERPORT, USERNAME, PWD);

#define halt(s) { Serial.println(F( s )); while(1);  }

/****************************** Feeds ***************************************/

// Setup a feed called 'photocell' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish numbers = Adafruit_MQTT_Publish(&mqtt, USERNAME "/feeds/numbers");

// Setup a feed called 'onoff' for subscribing to changes.
Adafruit_MQTT_Subscribe onoffmsg = Adafruit_MQTT_Subscribe(&mqtt, USERNAME "/feeds/onoff");



/*************************** Sketch Code ************************************/

// --------------------------- Variables & others --------------------------------
rgb_lcd lcd;
byte heart[8] = {
  0b00000,
  0b01010,
  0b11111,
  0b11111,
  0b11111,
  0b01110,
  0b00100,
  0b00000
};

void setup() {
  // ------------------------- Initialization Serial ------------------------------
  while (!Serial);  // wait for serial port to connect. Needed for native USB port only
  Serial.begin(9600);

  //    Serial.println(F("Adafruit MQTT Library WINC1500 Modified by Yassine"));

  // ---------------------- Initialization Wifi WINC1500 --------------------------
  // Initialise the Client
  //    Serial.print(F("\nInit the WiFi module..."));
  // check for the presence of the breakout
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println(F("WINC1500 not present"));
    // don't continue:
    while (true);
  }
  //    Serial.println(F("ATWINC OK!"));

  // ------------------------------ Physical part ----------------------------------

  //    Serial.println(F("LCD Init"));
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  // Print a message to the LCD.
#if 1
  lcd.createChar(0, heart);
#endif
  lcd.setCursor(0, 0);
  lcd.print(F("Made with "));
  lcd.write((unsigned char)0);
  lcd.setCursor(0, 1);
  lcd.print(F("     by Yassine"));

  pinMode(RELAY2, OUTPUT);

  // --------------------------- Variables & others --------------------------------
  mqtt.subscribe(&onoffmsg);
}

// uint32_t x = 0;

void loop() {
  // ------------------------------ MQTT Connection --------------------------------
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).
  MQTT_connect();
  // lcd.setRGB(255,165,0);
  // ------------------------------ Subscription -----------------------------------
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000))) {
    if (subscription == &onoffmsg) {
      Serial.print(F("Got: "));
      Serial.println((char *)onoffmsg.lastread);

      if (0 == strcmp((char *)onoffmsg.lastread, "OFF")) {
        lcd.setRGB(255, 0, 0);
        lcd.setCursor(0, 1);
        lcd.print(F("Closing "));
        lcd.print(F(USERNAME));
        digitalWrite(RELAY2, LOW); // Turns OFF Relays 1
      }
      if (0 == strcmp((char *)onoffmsg.lastread, "ON")) {
        lcd.setRGB(0, 255, 0);
        lcd.setCursor(0, 1);
        lcd.print(F("Opening "));
        lcd.print(F(USERNAME));
        digitalWrite(RELAY2, HIGH); // Turns ON Relays 1
      }
    }
  }

  // ------------------------------ Publication ----------------------------------
  int32_t val = mesureSimple(analogInPin);
  if (val < 0) {
    lcd.setRGB(245, 130, 49);
  } else {
    lcd.setRGB(0, 0, 117);
  }


  // int32_t val = mesure(analogInPin) * 100;

  Serial.print(F("\nSending "));
  Serial.print(val);
  lcd.setCursor(0, 1);
  lcd.print(F("                "));
  lcd.setCursor(0, 1);
  lcd.print(F(USERNAME));
  lcd.setCursor(9, 1);
  lcd.print(val);

  // Si on met double au lieu de int32_t, plantage !!!
  if (! numbers.publish(val)) {
    Serial.println(F("...Failed"));
  } else {
    Serial.println(F("...OK!"));
  }

}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // attempt to connect to Wifi network:
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(F("Attempting to connect to SSID: "));
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    uint8_t timeout = 10;
    while (timeout && (WiFi.status() != WL_CONNECTED)) {
      timeout--;
      delay(1000);
    }
  }

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print(F("Connecting to MQTT... "));

  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println(ret);
    Serial.println(F("Retrying MQTT connection in 5 seconds..."));
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
  }
  Serial.println(F("MQTT Connected!"));
}
double mesure(int analogPin) {

  const int ACSoffset = 2500;
  double Amps = 0;
  /*double Voltage = 0;
    int RawValue= 0;
    RawValue = analogRead(analogPin);
    /*Voltage = (RawValue / 1024.0) * 5000; // Gets you mV
    Amps = ((Voltage - ACSoffset) / mVperAmp);*/

  Amps = (((analogRead(analogPin) / 1024.0) * 5000 - ACSoffset) / mVperAmp);

  /*Serial.print("Raw Value = " ); // shows pre-scaled value
    Serial.print(RawValue);
    Serial.print("\t mV = "); // shows the voltage measured
    Serial.print(Voltage,3);
    // the '3' after voltage allows you to display 3 digits after decimal point
    Serial.print("\t Amps = "); // shows the voltage measured
    Serial.println(Amps,3);
    // the '3' after voltage allows you to display 3 digits after decimal point
    delay(2500); */
  return (Amps);
}
int mesureSimple(int analogPin) {

  int Valeur = analogRead(analogPin);

  return (Valeur);
}
