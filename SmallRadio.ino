#include <Arduino.h>
#include <Wire.h>
#include <HardwareSerial.h>

const uint8_t radioAddress = 0x10;

void setup(){
    Wire.begin();
    Serial.begin(9600);
}

bool simpleFlag = false;

void loop(){
    if (!simpleFlag){
        Serial.println("Go");
        Wire.beginTransmission(radioAddress);         
        Wire.write(0x1B);                  
        Wire.endTransmission();

        Wire.requestFrom(radioAddress, 2);
        while (Wire.available() > 0){
            Serial.print(Wire.read(), BIN);
            Serial.println(Wire.read(), BIN);
        }     
        Serial.println("End");      
        simpleFlag = true;
    }
}
