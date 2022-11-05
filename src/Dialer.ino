/*
 * Project Dialer
 * Description: Detect rotary phone dialer
 * Author: adapted from Instructibles.com (among other things, removed the debouncing logic)
 *    guidomax - https://www.instructables.com/Interface-a-rotary-phone-dial-to-an-Arduino/
 * Date: October 25, 2022
 */

#include <MQTT.h>
#include "secrets.h"

bool digitComplete = FALSE;   // end of one digit dialed
bool dialing = FALSE; // end of sequence dialed
int count;                    // pulse count for one digital dialed on rotor
int in = D2;                  // rotor connected to pin D2
int out = D0;                 // LED connected to D0;
int switchHook = D4;          // switchhook on D4
bool onHook = TRUE;
int lastState = HIGH;
int trueState = HIGH;
double lastStateChangeTime = 0;
int cleared = 0;
int reading = 0;
int phoneNumber[10];
int phoneNumberDigits = 0;
char phoneString[11];
String dialedNumber;

// constants

double dialHasFinishedRotatingAfterMs = 100;
double userHasFinishedDialingAfterMs = 3000; // after 3 seconds assume the dailing is done

// MQTT
const char *TOPIC_DIGIT= "ha/rotary/digit";     // sending single digit dialed
const char *TOPIC_NUMBER = "ha/rotary/number";  // sending entire dialed sequence
#define MQTT_KEEPALIVE 30 * 60              //  sec
// MQTT functions
void timer_callback_send_mqqt_data();
 // MQTT callbacks implementation
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
     char p[length + 1];
     memcpy(p, payload, length);
     p[length] = 0; // was = NULL but that threw a warning
     Particle.publish("mqtt", p, 3600, PRIVATE);
 }
MQTT client(MY_SERVER, 1883, MQTT_KEEPALIVE, mqtt_callback);

void setup()  {
  // Serial.begin(9600);
  pinMode(in, INPUT_PULLUP);
  pinMode(out, OUTPUT);
  Particle.publish("dbug", "Start me up...", 3600, PRIVATE);
      // make sure mqtt is working
  client.connect(CLIENT_NAME, HA_USR, HA_PWD); //see secrets.h
  if (client.isConnected()) { Particle.publish("MQTT", "Connected to HA");
    } else {  Particle.publish("MQTT", "Failed connect HA - check secrets.h"); }
}

void loop() {
  delay(25);

 // if (onHook) {
 //   onHook = digitalRead(switchHook);  // if receiver is lifted, we can listen for dialing
 // } else {
    reading = digitalRead(in);

// finished dialing sequence
      if ((millis() - lastStateChangeTime) > userHasFinishedDialingAfterMs) {  // finished dialing 
        if (dialing) {
          dialedNumber = "";
          for (int d=0; d<phoneNumberDigits; d++) {
            dialedNumber += phoneNumber[d];
          }
          tellHASS(TOPIC_NUMBER, dialedNumber);
          // blink the LEDs
          /* for (int p=0; p<phoneNumberDigits; p++){
            blinkLed(phoneNumber[p], 250, 500);
            delay(700);
          }*/
          //Particle.publish("Number is", dialedNumber);
          //Particle.publish("Number digits is,", String(phoneNumberDigits));
          dialing=FALSE;
          phoneNumberDigits = 0;
          onHook = TRUE;
        }
      } 

    if ((millis() - lastStateChangeTime) > dialHasFinishedRotatingAfterMs) { // no action or end of digit rotation


    if (digitComplete) {
          // if it's only just finished being dialed, we need to send the number down the serial
          // line and reset the count. We mod the count by 10 because '0' will send 10 pulses.

      Particle.publish("Dialed ", String(count%10));
      phoneNumber[phoneNumberDigits] = count;
      phoneNumberDigits++;
      digitComplete = FALSE;
      count = 0;
      cleared = 0;
    }
  }

    if (reading != lastState) {
        lastStateChangeTime = millis();
        dialing=TRUE;
      if (reading != trueState) {     //  switch has either just gone from closed->open or vice versa.
        trueState = reading;
        if (trueState == HIGH) {        // increment the pulse count if it's gone high.
          count++;
          digitComplete = TRUE;              // we'll need to print this number (once the dial has finished rotating)
        } 
      }
      lastState = reading;
    }
//  }
}

// led blinky
void blinkLed(int ringyDingy, int duration, int period) {
  for (int k=0; k<ringyDingy; k++) {
    digitalWrite (out, HIGH);
    delay(duration);
    digitalWrite(out, LOW);
    delay (period-duration);
  }

}
// mqtt comms

void tellHASS(const char *ha_topic, String ha_payload) {
  int returnCode = 0;
  delay(100); // take it easy on the server
  if(client.isConnected()) {
    returnCode = client.publish(ha_topic, ha_payload);
    if (returnCode != 1) {
      delay(1000);
      Particle.publish("mqtt return code = ", String(returnCode));
      client.disconnect();
    } 
  } else {
    delay(1000);
    Particle.publish("mqtt", "Connection dropped");
    client.connect(CLIENT_NAME, HA_USR, HA_PWD);
    client.publish(ha_topic, ha_payload);
  }
}