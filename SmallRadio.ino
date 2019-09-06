#include <Arduino.h>
#include <Wire.h>
#include <HardwareSerial.h>

const uint8_t ar1010Address = 0x10;

const uint16_t register_init[18] = {
	0xFFFB,     // R0:  1111 1111 1111 1011 
	0x5B15,     // R1:  0101 1011 0001 0101 - Mono (D3), Softmute (D2), Hardmute (D1)  !! SOFT-MUTED BY DEFAULT !!
	0xD0B9,     // R2:  1101 0000 1011 1001 - Tune/Channel
	0xA010,     // R3:  1010 0000 0001 0000 - Seekup (D15), Seek bit (D14), Space 100kHz (D13), Seek threshold: 16 (D6-D0)
	0x0780,     // R4:  0000 0111 1000 0000
	0x28AB,     // R5:  0010 1000 1010 1011
	0x6400,     // R6:  0110 0100 0000 0000
	0x1EE7,		// R7:  0001 1110 1110 0111
	0x7141,		// R8:  0111 0001 0100 0001 
	0x007D,		// R9:  0000 0000 0111 1101
	0x82C6,		// R10: 1000 0010 1100 0110 - Seek wrap (D3)
	0x4E55,		// R11: 0100 1110 0101 0101
	0x970C,		// R12: 1001 0111 0000 1100
	0xB845,		// R13: 1011 1000 0100 0101
	0xFC2D,		// R14: 1111 1100 0010 1101 - Volume control 2 (D12-D15)
	0x8097,		// R15: 1000 0000 1001 0111
	0x04A1,		// R16: 0000 0100 1010 0001
	0xDF61		// R17: 1101 1111 0110 0001
};

void setup(){
    Wire.begin();
    Serial.begin(9600);
}

bool simpleFlag = false;

void loop(){
    if (!simpleFlag){
        Serial.println("Go");
        Serial.println(IsAR1010(ar1010Address));
        Initialize(ar1010Address, register_init);
        Serial.println(readFromRegister(ar1010Address, 0), BIN);
        Serial.println(readFromRegister(ar1010Address, 1), BIN);
        Serial.println(readFromRegister(ar1010Address, 2), BIN);
        // SetFrequency(ar1010Address, 100.1);
        Serial.println("End");      
        simpleFlag = true;
    }
}

void Initialize(uint8_t radioAddress, uint16_t writableRegisters[18]){
    for (uint8_t i = 0; i < 18; i++)
    {
        writeToRegister(radioAddress, i, writableRegisters[i]);
    }    
}

void SetFrequency(uint8_t radioAddress, float frequencyMHz){
    uint16_t convertedFrequency = 10 * (frequencyMHz - 69);

    // Частота должна лежать в нужном диапазоне.
    uint16_t constrainConvertedFrequency = constrain(convertedFrequency, 185, 390);

    writeBitToRegister(radioAddress, 2, 0, 9);
    uint16_t registerData = readFromRegister(radioAddress, 2);

    // Затираем нулями 9 младших разрядов для ввода частоты.
    uint16_t registerDataWithMask = registerData & 0xFE;  

    uint16_t registerDataWithFrequencyValue = registerDataWithMask | constrainConvertedFrequency;  
    writeBitToRegister(radioAddress, 2, 1, 9);
    Serial.println(readFromRegister(radioAddress, 2), BIN);
}

bool IsAR1010(uint8_t radioAddress){
    uint16_t registerData = readFromRegister(radioAddress, 0x1C);    
    return registerData == 0x1010;
}

void writeToRegister(uint8_t radioAddress, uint8_t registerAddress){
    Wire.beginTransmission(radioAddress);
    Wire.write(registerAddress);
    Wire.endTransmission();
}

void writeToRegister(uint8_t radioAddress, uint8_t registerAddress, uint16_t registerData){
    Wire.beginTransmission(radioAddress);
    Wire.write(registerAddress);
    Wire.write(registerData);
    Wire.endTransmission();
}

void writeBitToRegister(uint8_t radioAddress, uint8_t registerAddress, bool bitNumber, byte shiftBits){
    uint16_t temp;

    // В условии накладывается маска в зависимости от значения бита (0 или 1).
    if (bitNumber)
        temp = readFromRegister(radioAddress, registerAddress) | (1 << bitNumber);
    else
        temp = readFromRegister(radioAddress, registerAddress) & ~(1 << bitNumber);     // ~ - побитовое НЕ 
        
    writeToRegister(radioAddress, registerAddress, temp);
}

uint16_t readFromRegister(uint8_t radioAddress, uint8_t registerAddress){
    writeToRegister(radioAddress, registerAddress);
    Wire.requestFrom(radioAddress, 2);
    byte highByte, leastByte;
    while(Wire.available()){
        highByte = Wire.read();               
        leastByte = Wire.read();                
    }
    uint16_t registerData = (highByte << 8) + leastByte;    
    return registerData;
}
