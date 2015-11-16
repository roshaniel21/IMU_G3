/*
AUTHOR: Daniel Greenheck
PROGRAM: IMU_SDRead
DESCRIPTION: Collects raw and calibrated data and stores it on the SD card.
  - To change the length of the data acquisition, change the 'acqTime' variable
  - Change sdSuffix to change the suffix attached to the SD card data file
*/

#include <Wire.h>
#include <SoftwareSerial.h>

// IMU address
const byte IMU_SLAVE_ADDRESS = 0x30;

// IMU DAQ configuration register address
const byte IMU_DAQ_REG = 0x04;
// SD status register
const byte SD_STAT_REG = 0x5B;

const byte IMU_DAQ_EN = 0b00000001;
const byte SD_OVERWRITE = 0b00001000;
const byte STREAMING_MODE = 0b00000010;
const byte SD_WRITE_MODE = 0b00000100;
const byte SD_READ_MODE = 0b00000110;

SoftwareSerial mySerial(10, 11); // RX, TX

// Time to acquire data for (seconds)
unsigned long acqTime = 30;

void setup()
{
  Serial.begin(9600);
  mySerial.begin(230400);
  Wire.begin();

  Serial.println("Beginning data acquisition in 5 seconds...");
  delay(5000);
  
  // Put IMU into SD_WRITE mode. Overwrite the data file if there is one.
  Wire.beginTransmission(IMU_SLAVE_ADDRESS);
  Wire.write(IMU_DAQ_REG);
  Wire.write(SD_OVERWRITE | SD_WRITE_MODE | IMU_DAQ_EN);
  Wire.endTransmission();
  
  // Collect data
  Serial.print("Acquiring data... ");
  delay(acqTime*1000);

  // Stop data acquisition and go back to I2C output mode
  Wire.beginTransmission(IMU_SLAVE_ADDRESS);
  Wire.write(IMU_DAQ_REG);
  Wire.write(STREAMING_MODE | IMU_DAQ_EN);
  Wire.endTransmission();
  
  Serial.println("Done");  
}

void loop() {
}
