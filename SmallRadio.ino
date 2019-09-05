#include <Arduino.h>
#include <Wire.h>
#include <HardwareSerial.h>

const uint8_t ar1010Address = 0x10;


void setup(){
    Wire.begin();
    Serial.begin(9600);
}

bool simpleFlag = false;

void loop(){
    if (!simpleFlag){
        Serial.println("Go");
        Serial.println(IsAR1010());
        // uint16_t silverRain = (1 << 9) + 100110111;
        writeBitToRegister(ar1010Address, 2, 0, 9);
        writeToRegister(ar1010Address, 0x02, 100110111);
        writeBitToRegister(ar1010Address, 2, 1, 9);
        Serial.println("End");      
        simpleFlag = true;
    }
}

bool IsAR1010(){
    writeToRegister(ar1010Address, 0x1C);
    Wire.requestFrom(ar1010Address, 2);
    byte highByte, leastByte;
    while(Wire.available()){
        highByte = Wire.read();        
        leastByte = Wire.read();        
    }
    uint16_t response = (highByte << 8) + leastByte;    
    return response == 0x1010;
}

void writeToRegister(uint8_t address, int registerAddress){
    Wire.beginTransmission(address);
    Wire.write(registerAddress);
    Wire.endTransmission();
}

void writeToRegister(uint8_t address, int registerAddress, uint16_t registerData){
    Wire.beginTransmission(address);
    Wire.write(registerAddress);
    Wire.write(registerData);
    Wire.endTransmission();
}

void writeBitToRegister(uint8_t address, int registerAddress, bool bitValue, byte shiftBits){
    uint16_t loadData = bitValue << shiftBits;
    writeToRegister(address, registerAddress, loadData);
}
