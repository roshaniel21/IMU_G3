/*
AUTHOR: Daniel Greenheck
PROGRAM: ReadDataSDtoI2C
DESCRIPTION: Commands the IMU to collect data and store it on the SD
  card for 'acqTime' seconds. After the allotted time has passed,
  command the IMU to enter 'SD read' mode. The IMU begins copying data
  to a block of 64 data registers. Once the Arduino reads the last register
  in that block, the IMU will copy the next 64 bytes from the SD card to those
  registers. The SD_DIRTY flag in the SD_STAT register provides an indication
  of if the IMU has finished writing the new 64 byte block of data. The process
  is repeated until the SD_EOF flag in the SD_STAT register is set to 1.
*/

#include <Wire.h>
#include <SoftwareSerial.h>

// Number of seconds to collect data for
unsigned long acqTime = 5;

// IMU DAQ configuration register address
const byte IMU_DAQ_REG = 0x04;
// SD status register
const byte SD_STAT_REG = 0x5B;

const byte IMU_OUTPUT_RATE_DIV = 0b10100000;
const byte IMU_DAQ_EN = 0b00000001;
const byte IMU_SD_OVERWRITE = 0b00001000;
const byte IMU_STREAMING_MODE = 0b00000010;
const byte IMU_SD_WRITE_MODE = 0b00000100;
const byte IMU_SD_READ_MODE = 0b00000110;

// Flag indicating if SD data registers on IMU have been updated since last read
const byte SD_DIRTY = 0x01;
// Flag indicating if the end of the file has been reached
const byte SD_EOF = 0x02;
// IMU address
const uint8_t IMU_SLAVE_ADDRESS = 0x30;

// Buffer to store I2C data
byte data[129];              
bool collectData = true; 
byte sdStatus;

SoftwareSerial mySerial(10, 11); // RX, TX

void setup()
{
  mySerial.begin(230400);
  Wire.begin();

  delay(5000);
  
  // Enter SD write mode
  Wire.beginTransmission(IMU_SLAVE_ADDRESS);
  // Update the value at the IMU_DAQ register
  Wire.write(IMU_DAQ_REG);
  Wire.write(IMU_SD_OVERWRITE | IMU_SD_WRITE_MODE | IMU_DAQ_EN);
  Wire.endTransmission();
  
  // Wait for the specified amount of time
  delay(acqTime*1000);

  // Enter SD read mode
  Wire.beginTransmission(IMU_SLAVE_ADDRESS);
  // Update the value at the IMU_DAQ register
  Wire.write(IMU_DAQ_REG);
  Wire.write(IMU_SD_READ_MODE);
  Wire.endTransmission();
  
  // Wait 1000 ms while IMU gets ready
  delay(1000);
}

void loop() {
  if(collectData) {
    // Write the register address we want to read from
    Wire.beginTransmission(IMU_SLAVE_ADDRESS);
    Wire.write(SD_STAT_REG);
    Wire.endTransmission(false);
      
    // Read the SD data (128 bytes + status byte)
    Wire.requestFrom(IMU_SLAVE_ADDRESS, 129);
    
    int i = 0;
    while(Wire.available()) { 
      data[i++] = Wire.read(); 
    }
    sdStatus = data[0];
    
    // If data is fresh, write it to the serial port
    if(!(sdStatus & SD_DIRTY)) {
      mySerial.write(&data[1], 128);
    }
    // End of file detected
    if(sdStatus & SD_EOF) {
      // Make sure to put the IMU back into I2C output mode
      Wire.beginTransmission(IMU_SLAVE_ADDRESS);
      Wire.write(IMU_DAQ_REG);
      Wire.write(IMU_OUTPUT_RATE_DIV | IMU_STREAMING_MODE | IMU_DAQ_EN);
      Wire.endTransmission();
  
      collectData = false;
    }
  }
}
