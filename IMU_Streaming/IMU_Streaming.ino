/*
AUTHOR: Daniel Greenheck
PROGRAM: IMU_Streaming
DESCRIPTION: Reads calibrated data from the IMU data registers over the I2C bus.
*/

#include <Wire.h>
#include <SoftwareSerial.h>

// Set true to output calibrated data, false to output only
// angular velocity and specific force
bool calibratedOutput = false;

// Sampling rate (Hz)
unsigned int sampleRate = 250;
// Output divider rate (possible values: 2 to 15)
unsigned int outputDivider = 8;
// Time between outputs (OUTPUT_RATE_DIV/250 Hz)
unsigned int sampleTime = 1000*((float)outputDivider)/((float)sampleRate);  

// IMU address
const byte IMU_SLAVE_ADDRESS = 0x30;

// IMU DAQ configuration register address
const byte IMU_DAQ_REG = 0x04;
// IMU status register
const byte IMU_STAT_REG = 0x06;
// IMU tick count register
const byte IMU_TICK_REG = 0x3F;

const byte IMU_DAQ_EN = 0b00000001;
const byte IMU_SD_OVERWRITE = 0b00001000;
const byte IMU_I2C_MODE = 0b00000010;
const byte IMU_SD_WRITE_MODE = 0b00000100;
const byte IMU_SD_READ_MODE = 0b00000110;

char outstr[64];
byte data[61] = {0};
unsigned long count = 0;
unsigned long prevCount = 0;
unsigned long prevTimeStamp = 0;

// Serial connection for displaying data
SoftwareSerial mySerial(10, 11); // RX, TX

void setup()
{
  mySerial.begin(230400);
  Wire.begin();
  prevCount = millis();
  
  // Put the IMU into I2C output mode
  Wire.beginTransmission(IMU_SLAVE_ADDRESS);
  Wire.write(IMU_DAQ_REG);
  Wire.write((outputDivider << 4) | IMU_I2C_MODE | IMU_DAQ_EN);
  Wire.endTransmission();
}

byte x = 0;

void loop() {
  count = millis();
  float elapsedTime = count - prevCount;
  
  // Sample the IMU at the sampling time. If the sampling time is too
  // short, the Arduino won't be able to sample fast enough and data
  // will be lost.
  if(elapsedTime >= sampleTime) {
    prevCount = count;
    
    if(calibratedOutput) {
      // Write the starting address for the burst read
      Wire.beginTransmission(IMU_SLAVE_ADDRESS);
      Wire.write(IMU_STAT_REG);
      Wire.endTransmission(false);
      
      // Sequentially read all of the IMU data registers
      Wire.requestFrom(IMU_SLAVE_ADDRESS, 61);
    
      int i = 0;
      while(Wire.available()) { 
        data[i++] = Wire.read(); 
      }
    
      // Convert data to float
      float* ptrData = (float*)(&data[1]);
      unsigned long* ptrTimeStamp = (unsigned long*)(&data[57]);
      unsigned long timeStamp = *ptrTimeStamp;
      
      // Check to see if the timestamp for this data is the
      // same as the previous set of data. If it is, don't print it
      mySerial.print(timeStamp);
      mySerial.print(';');
      
      // Delta theta
      mySerial.print(ptrData[0],3);
      mySerial.print(',');
      mySerial.print(ptrData[1],3);
      mySerial.print(',');
      mySerial.print(ptrData[2],3);
      mySerial.print(';');
      
      // Delta velocity
      mySerial.print(ptrData[3],3);
      mySerial.print(',');
      mySerial.print(ptrData[4],3);
      mySerial.print(',');
      mySerial.print(ptrData[5],3);
      mySerial.print(';');
      
      // Integrated quaternion
      mySerial.print(ptrData[6],3);
      mySerial.print(',');
      mySerial.print(ptrData[7],3);
      mySerial.print(',');
      mySerial.print(ptrData[8],3);
      mySerial.print(',');
      mySerial.print(ptrData[9],3);
      mySerial.print(';');    
              
      // Temperature
      mySerial.print(ptrData[10],3);
      mySerial.print(';');
      
      // Accumulated velocity
      mySerial.print(ptrData[11],3);
      mySerial.print(',');
      mySerial.print(ptrData[12],3);
      mySerial.print(',');
      mySerial.print(ptrData[13],3);
      mySerial.println(';');
    }
    else {
      // Write the starting address for the burst read
      Wire.beginTransmission(IMU_SLAVE_ADDRESS);
      Wire.write(IMU_TICK_REG);
      Wire.endTransmission(false);
      
      // Sequentially read all of the IMU data registers
      Wire.requestFrom(IMU_SLAVE_ADDRESS, 28);
    
      int i = 0;
      while(Wire.available()) { 
        data[i++] = Wire.read(); 
      }
    
      // Convert data to float
      float* ptrData = (float*)(&data[4]);
      unsigned long* ptrTimeStamp = (unsigned long*)(&data[0]);
      unsigned long timeStamp = *ptrTimeStamp;
      
      // Check to see if the timestamp for this data is the
      // same as the previous set of data. If it is, don't print it
      mySerial.print(timeStamp);
      mySerial.print(';');
      
      // Angular velocity
      mySerial.print(ptrData[0],4);
      mySerial.print(',');
      mySerial.print(ptrData[1],4);
      mySerial.print(',');
      mySerial.print(ptrData[2],4);
      mySerial.print(';');
      
      // Specific force
      mySerial.print(ptrData[3],4);
      mySerial.print(',');
      mySerial.print(ptrData[4],4);
      mySerial.print(',');
      mySerial.print(ptrData[5],4);
      mySerial.println(';');
    }
  }
}
