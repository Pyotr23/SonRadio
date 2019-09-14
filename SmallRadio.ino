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

const uint8_t volume_map[19] = {
	0x0F, 0xCF, 0xDF, 0xFF, 0xCB,
	0xDB, 0xFB, 0xFA, 0xF9, 0xF8,
	0xF7, 0xD6, 0xE6, 0xF6, 0xE3,
	0xF3, 0xF2, 0xF1, 0xF0
};

bool simpleFlag = false;

void setup(){
    Wire.begin();
    Serial.begin(9600);     
    Initialize(ar1010Address, register_init);    
}

void loop(){    
    Serial.println(Serial.available());    
    while (Serial.available()){
        uint16_t command = GetCommand();  
        Serial.println(command, HEX);     
        switch (command)
        {
        case 0x3030:    // Означает, что через посл. порт передались "00".
            {
                // Совершенно непонятно, откуда берётся нехватка единицы, здесь инициализируемой для компенсации.
                uint16_t frequencyInKHz = GetNumberFromSerialPort() + 1;            
                SetFrequency(ar1010Address, frequencyInKHz);
                break;  
            }
            
        case 0x3031:
            {
                Serial.println("Установка громкости.");
                byte volume = GetNumberFromSerialPort();
                Serial.println(volume);
                SetVolume(ar1010Address, volume);
                break;
            }            
        default:
            break;
        }            
    }   
    delay(2000);
}

void EnableSignal(bool enable){
    writeBitToRegister()
}

void SetVolume(uint8_t radioAddress, byte volume){
    volume = constrain(volume, 0, 18);
    uint8_t registersVolumeValue = volume_map[volume];

    uint16_t thirdRegisterData = readFromRegister(radioAddress, 3);   
    uint16_t thirdRegisterDataWithMask = thirdRegisterData & 0xF87F;
    uint16_t newThirdRegisterData = thirdRegisterDataWithMask | ((registersVolumeValue & 0x0F) << 7);
    
    uint16_t fourteenRegisterData = readFromRegister(radioAddress, 14);
    
    uint16_t fourteenRegisterDataWithMask = fourteenRegisterData & 0x0FFF;
    uint16_t newFourteenRegisterData = fourteenRegisterDataWithMask | ((registersVolumeValue & 0xF0) << 8);    

    writeToRegister(radioAddress, 3, newThirdRegisterData);
    writeToRegister(radioAddress, 14, newFourteenRegisterData);
}

uint16_t GetNumberFromSerialPort(){
    byte bytesNumber = Serial.available();
    uint16_t currentDigit;
    
    uint16_t answear = 0;       
    for (byte i = bytesNumber - 1; i != 255; i--)
    {        
        currentDigit = Serial.read() - 48;        
        answear += currentDigit * pow(10, i);        
    }
    return answear;
}

void Initialize(uint8_t radioAddress, uint16_t writableRegisters[18]){
    for (uint8_t i = 1; i < 18; i++)
    {        
        writeToRegister(radioAddress, i, writableRegisters[i]);       
    }   
    writeToRegister(radioAddress, 0, writableRegisters[0]);     
    bool isReady;
    do {
        isReady = ReadBitFromRegister(ar1010Address, 0x13, 5);
        // Serial.print(isReady);
    }
    while (!isReady);  
    Serial.println("Окончена инициализация!");     
}

uint16_t GetCommand(){
    byte bytesNumber = Serial.available();
    byte highByte, leastByte;
    highByte = Serial.read();
    leastByte = Serial.read();
    uint16_t command = (highByte << 8) + leastByte;
    return command;
}

void SetFrequency(uint8_t radioAddress, uint16_t kHzFrequency){
    uint16_t convertedFrequency = kHzFrequency - 690;
    
    // Частота должна лежать в нужном диапазоне.
    uint16_t constrainConvertedFrequency = constrain(convertedFrequency, 185, 390);
    
    writeBitToRegister(radioAddress, 2, 0, 9);
    uint16_t registerData = readFromRegister(radioAddress, 2);
    
    // Затираем нулями 9 младших разрядов для ввода частоты.
    uint16_t registerDataWithMask = registerData & 0xFE00;  
    
    uint16_t registerDataWithFrequencyValue = registerDataWithMask | constrainConvertedFrequency;    
    writeToRegister(radioAddress, 2, registerDataWithFrequencyValue);

    writeBitToRegister(radioAddress, 2, 1, 9);    
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
    Wire.write(uint8_t((registerData & 0xFF00) >> 8)); 
    Wire.write(uint8_t(registerData & 0x00FF));      
    Wire.endTransmission();
}

void writeBitToRegister(uint8_t radioAddress, uint8_t registerAddress, bool bitValue, byte bitNumber){
    uint16_t temp;

    // В условии накладывается маска в зависимости от значения бита (0 или 1).
    if (bitValue)
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

bool ReadBitFromRegister(uint8_t radioAddress, uint8_t registerAddress, byte bitNumber){
    uint16_t registerData = readFromRegister(radioAddress, registerAddress);
    uint16_t shiftedRegisterData = registerData >> bitNumber;
    return shiftedRegisterData & 1;
}
