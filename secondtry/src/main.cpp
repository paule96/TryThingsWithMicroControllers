#include <Arduino.h>

int LED_2 = 2;
int LED_3 = 3;
int LED_4 = 4;
int LED_5 = 5;
int LED_6 = 6;
int LED_7 = 7;
int LED_8 = 8;
int LED_9 = 9;
int LED_10 = 10;

void setup(){
    pinMode(LED_2, OUTPUT);
    pinMode(LED_3, OUTPUT);
    pinMode(LED_4, OUTPUT);
    pinMode(LED_5, OUTPUT);
    pinMode(LED_6, OUTPUT);
    pinMode(LED_7, OUTPUT);
    pinMode(LED_8, OUTPUT);
    pinMode(LED_9, OUTPUT);
    pinMode(LED_10, OUTPUT);
    printf("Ich bin an");
}

void loop(){
    digitalWrite(LED_2, HIGH);
    delay(200);
    digitalWrite(LED_3, HIGH);
    delay(200);
    digitalWrite(LED_4, HIGH);
    delay(200);
    digitalWrite(LED_5, HIGH);
    delay(200);
    digitalWrite(LED_6, HIGH);
    delay(200);
    digitalWrite(LED_7, HIGH);
    delay(200);
    digitalWrite(LED_8, HIGH);
    delay(200);
    digitalWrite(LED_9, HIGH);
    delay(200);
    digitalWrite(LED_10, HIGH);
    delay(200);
    digitalWrite(LED_2, LOW);
    delay(200);
    digitalWrite(LED_3, LOW);
    delay(200);
    digitalWrite(LED_4, LOW);
    delay(200);
    digitalWrite(LED_5, LOW);
    delay(200);
    digitalWrite(LED_6, LOW);
    delay(200);
    digitalWrite(LED_7, LOW);
    delay(200);
    digitalWrite(LED_8, LOW);
    delay(200);
    digitalWrite(LED_9, LOW);
    delay(200);
    digitalWrite(LED_10, LOW);
    delay(200);

}