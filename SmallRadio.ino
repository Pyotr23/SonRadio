#include <Arduino.h>
#include <Wire.h>
#include <HardwareSerial.h>
#include <EEPROM.h>
#include <LiquidCrystal.h>

const byte rsPin = 7;
const byte ePin = 8;
const byte d4Pin = 9;
const byte d5Pin = 10;
const byte d6Pin = 11;
const byte d7Pin = 12;

const byte _ar1010Address = 0x10;
const byte seekTreshold = 13;

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

const byte strengthRowNumber = 1;
const byte strengthDigitNumber = 15;
const byte oneLine[8] = {
    B00000,
    B00000,
    B00000,
    B00000,
    B00000,
    B00000,
    B11111,
};
const byte twoLines[8] = {
    B00000,
    B00000,
    B00000,
    B00000,
    B00000,
    B11111,
    B11111,
};
const byte threeLines[8] = {
    B00000,
    B00000,
    B00000,
    B00000,
    B11111,
    B11111,
    B11111,
};
const byte fourLines[8] = {
    B00000,
    B00000,
    B00000,
    B11111,
    B11111,
    B11111,
    B11111,
};
const byte fiveLines[8] = {
    B00000,
    B00000,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
};
const byte sixLines[8] = {
    B00000,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
};
const byte sevenLines[8] = {
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
};

byte strengthValue;
int _radioAddress;
LiquidCrystal lcd(rsPin, ePin, d4Pin, d5Pin, d6Pin, d7Pin);

void setup(){
    Wire.begin();
    Serial.begin(9600);
    lcd.begin(16, 2);
    if (IsAR1010()){
        _radioAddress = _ar1010Address;
    }
    Initialize(register_init);
    // Вот так нужно для того, чтобы установилась частота, записанная в EEPROM.
    uint16_t memoryFrequency = ReadTwoBytesFromEeprom(0) - 4;
    SetFrequency(memoryFrequency);
    
    byte volume = EEPROM.read(2);
    SetVolume(volume);
}

void loop(){
    while (Serial.available()){
        uint16_t command = GetCommand();
        Serial.println(command, HEX);
        switch (command)
        {
            case 0x3030:    // Означает, что через посл. порт передались "00".
            {
                // Совершенно непонятно, откуда берётся нехватка единицы, здесь инициализируемой для компенсации.
                uint16_t frequencyInKHz = GetNumberFromSerialPort() + 1;
                SetFrequency(frequencyInKHz);                
                break;
            }

            case 0x3031:    // "01"
            {
                Serial.println("Установка громкости.");
                byte volume = GetNumberFromSerialPort();
                volume = constrain(volume, 0, 18);
                Serial.println(volume);
                SetVolume(volume);
                EEPROM.write(2, volume);
                break;
            }
            case 0x3032:    // "02", Происходит скачок вверх на 4 кГц. Лучше выключать звук Hardmute.
            {                
                bool enable = GetNumberFromSerialPort();
                EnableSignal(enable);
                break;
            }
            case 0x3033:    // "03"
            {            
                bool enable = GetNumberFromSerialPort();
                HardMute(enable);
                break;
            }            
            case 0x3034:    // "04"
            {  
                bool isUpDirection = GetNumberFromSerialPort();
                StartSeek(isUpDirection);
                break;
            } 
            case 0x3035:    // "05"
            {
                byte signalStrength = GetStrength();
                Serial.println(signalStrength);
                break;
            }      
            default:
                break;
        }
    }
    lcd.setCursor(6, 1);
    strengthValue = GetStrength();
    lcd.print(strengthValue);
    ShowStrength(strengthValue);
    delay(2000);
}

void ShowStrength(byte strengthValue){
    byte constrainStrength = constrain(strengthValue, 0, 48);
    byte divisionedStrength = constrainStrength / 7;
    switch (divisionedStrength){
        case 0:
            lcd.createChar(0, oneLine);
            break;
        case 1:
            lcd.createChar(0, twoLines);
            break;
        case 2:
            lcd.createChar(0, threeLines);
            break;
        case 3:
            lcd.createChar(0, fourLines);
            break;
        case 4:
            lcd.createChar(0, fiveLines);
            break;
        case 5:
            lcd.createChar(0, sixLines);
            break;
        case 6:
            lcd.createChar(0, sevenLines);
            break;
    }
    lcd.setCursor(strengthDigitNumber, strengthRowNumber);
    lcd.write(byte(0));
}

byte GetStrength(){
    uint16_t registerData = ReadFromRegister(0x12);
    uint16_t maskedRegisterData = registerData & 0xFE00;
    byte shiftMaskedRegisterData = maskedRegisterData >> 9;
    return shiftMaskedRegisterData;
}

void WriteTwoBytesToEeprom(byte firstByteNumber, uint16_t data){
    byte highByte = data >> 8;
    EEPROM.write(firstByteNumber++, highByte);
    byte leastByte = data;
    EEPROM.write(firstByteNumber, leastByte);
}

uint16_t ReadTwoBytesFromEeprom(byte firstByteNumber){
    byte highByte = EEPROM.read(firstByteNumber++);
    byte leastByte = EEPROM.read(firstByteNumber);
    uint16_t data = (highByte << 8) + leastByte;
    // Serial.println(data, HEX);
    return data;
}

void StartSeek(bool isUpDirection){
    HardMute(true);   
    WriteBiteToRegister(3, 0, 14);              // Unset SEEK bit.
    WriteBiteToRegister(3, isUpDirection, 15);  // Set SEEKUP bit.
    SetSeekTreshold(seekTreshold);
    WriteBiteToRegister(3, 1, 14);              // Set SEEK bit.
    bool isContinuedSeek = true;
    do{      
        isContinuedSeek = !ReadBitFromRegister(0x13, 5);
    }
    while (isContinuedSeek);
    HardMute(false);
    uint16_t newFrequencyData = ReadFrequency();    
    uint16_t newFrequency = newFrequencyData + 690;    
    SetFrequency(newFrequency);
}

uint16_t ReadFrequency(){
    uint16_t frequencyRegisterData = ReadFromRegister(0x13);
    uint16_t frequencyData = frequencyRegisterData >> 7;
    return frequencyData;
}

void SetSeekTreshold(byte tresholdValue){
    uint16_t registerData = ReadFromRegister(3);
    uint16_t clearedData = registerData & 0xFF80;
    uint16_t writtenTresholdValueData = clearedData | tresholdValue;
    WriteToRegister(3, writtenTresholdValueData);
}

void HardMute(bool enable){
    WriteBiteToRegister(1, enable, 1);
}

void EnableSignal(bool enable){
    WriteBiteToRegister(0, enable, 0);
}

void SetVolume(byte volume){
    byte registersVolumeValue = volume_map[volume];

    uint16_t thirdRegisterData = ReadFromRegister(3);
    uint16_t thirdRegisterDataWithMask = thirdRegisterData & 0xF87F;
    uint16_t newThirdRegisterData = thirdRegisterDataWithMask | ((registersVolumeValue & 0x0F) << 7);

    uint16_t fourteenRegisterData = ReadFromRegister(14);

    uint16_t fourteenRegisterDataWithMask = fourteenRegisterData & 0x0FFF;
    uint16_t newFourteenRegisterData = fourteenRegisterDataWithMask | ((registersVolumeValue & 0xF0) << 8);

    WriteToRegister(3, newThirdRegisterData);
    WriteToRegister(14, newFourteenRegisterData);
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

void Initialize(uint16_t writableRegisters[18]){
    // Serial.println("Запуск инициализации.");
    for (byte i = 1; i < 18; i++)
    {
        WriteToRegister(i, writableRegisters[i]);
    }
    WriteToRegister(0, writableRegisters[0]);
    bool isReady;
    // Serial.println("Запуск цикла.");
    for (byte i = 0; i < 10; i++)
    {
        // Serial.print("Итерация №");
        // Serial.println(i);
        isReady = ReadBitFromRegister(0x13, 5);
        if (isReady)
            break;
    }
    // do {
    //     isReady = ReadBitFromRegister(0x13, 5);
    // }
    // while (!isReady);
    // Serial.println("Окончена инициализация!");
}

uint16_t GetCommand(){
    byte bytesNumber = Serial.available();
    byte highByte, leastByte;
    highByte = Serial.read();
    leastByte = Serial.read();
    uint16_t command = (highByte << 8) + leastByte;
    return command;
}

void SetFrequency(uint16_t kHzFrequency){
    uint16_t convertedFrequency = kHzFrequency - 690;

    // Частота должна лежать в нужном диапазоне.
    uint16_t constrainConvertedFrequency = constrain(convertedFrequency, 185, 390);

    WriteBiteToRegister(2, 0, 9);
    uint16_t registerData = ReadFromRegister(2);

    // Затираем нулями 9 младших разрядов для ввода частоты.
    uint16_t registerDataWithMask = registerData & 0xFE00;

    uint16_t registerDataWithFrequencyValue = registerDataWithMask | constrainConvertedFrequency;
    WriteToRegister(2, registerDataWithFrequencyValue);

    WriteBiteToRegister(2, 1, 9);
    WriteTwoBytesToEeprom(0, kHzFrequency);

    ShowFrequency(kHzFrequency);
}

void ShowFrequency(uint16_t frequencyInKHz){
    if (frequencyInKHz < 1000)
        lcd.setCursor(1, 1);
    else
        lcd.setCursor(0, 1);
    
    byte intDigit = frequencyInKHz / 10;
    lcd.print(intDigit);
    lcd.setCursor(3, 1);
    lcd.print(".");
    byte decimalDigit = frequencyInKHz % 10;
    lcd.print(decimalDigit);    
}



bool IsAR1010(){
    uint16_t registerData = ReadFromRegister(0x1C, _ar1010Address);
    // Serial.println(registerData, HEX);
    return registerData == 0x1010;
}

void WriteToRegister(byte registerAddress){
    Wire.beginTransmission(_radioAddress);
    Wire.write(registerAddress);
    Wire.endTransmission();
}

void WriteToRegister(byte registerAddress, uint16_t registerData){
    Wire.beginTransmission(_radioAddress);
    Wire.write(registerAddress);
    Wire.write(uint8_t((registerData & 0xFF00) >> 8));
    Wire.write(uint8_t(registerData & 0x00FF));
    Wire.endTransmission();
}

void WriteToRegisterWithAddress(byte radioAddress, byte registerAddress){
    Wire.beginTransmission(radioAddress);
    Wire.write(registerAddress);
    Wire.endTransmission();
}

void WriteBiteToRegister(byte registerAddress, bool bitValue, byte bitNumber){
    uint16_t temp;

    // В условии накладывается маска в зависимости от значения бита (0 или 1).
    if (bitValue)
        temp = ReadFromRegister(registerAddress) | (1 << bitNumber);
    else
        temp = ReadFromRegister(registerAddress) & ~(1 << bitNumber);     // ~ - побитовое НЕ

    WriteToRegister(registerAddress, temp);
}

uint16_t ReadFromRegister(byte registerAddress){
    WriteToRegister(registerAddress);
    Wire.requestFrom(_radioAddress, 2);
    byte highByte, leastByte;
    while(Wire.available()){
        highByte = Wire.read();
        leastByte = Wire.read();
    }
    uint16_t registerData = (highByte << 8) + leastByte;
    return registerData;
}

uint16_t ReadFromRegister(byte registerAddress, byte radioAddress){
    WriteToRegisterWithAddress(radioAddress, registerAddress);
    Wire.requestFrom(radioAddress, 2);
    byte highByte, leastByte;
    while(Wire.available()){
        highByte = Wire.read();
        leastByte = Wire.read();
    }
    uint16_t registerData = (highByte << 8) + leastByte;
    return registerData;
}

bool ReadBitFromRegister(byte registerAddress, byte bitNumber){
    uint16_t registerData = ReadFromRegister(registerAddress);
    uint16_t shiftedRegisterData = registerData >> bitNumber;
    return shiftedRegisterData & 1;
}
