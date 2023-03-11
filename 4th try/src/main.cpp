#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

//Dies ist das Offizielle Projekt 2
//Kurzer Abriss was geplant ist
//Bei einem einzelnen Signal auf Step wird ein Schritt eines Schrittmotors aus Gelöst
//Die Richtung der Drehung wir über Dir Bestimmt
//Der Motor soll So Lange schritte ausfüren wie eine Taste auf der Fernbedienung gedrückt wird oder der jeweilige Endschalter erreicht wird
//den code für die Fernbedienung hab ich mir aus der Doku geborgt

#define StopPos 45 // Upper Endstop
#define StopNeg 48 // Lower Endstop
#define Ena 10 // Enable Stepperdriver
#define Step 11 // Steppdriver Stepp
#define Dir 12 // Steppdriver Direction

const uint16_t recvPin = 21; // Infrared receiving pin
IRrecv irrecv(recvPin);      // Create a class object used to receive class
decode_results results;       // Create a decoding results class object

void setup() {
  Serial.begin(115200);       // Initialize the serial port and set the baud rate to 115200
  irrecv.enableIRIn();        // Start the receiver
  Serial.print("IRrecvDemo is now running and waiting for IR message on Pin ");
  Serial.println(recvPin);   //print the infrared receiving pin
  delay(2000);
  digitalWrite(Ena, HIGH);
}

void loop() {

}
