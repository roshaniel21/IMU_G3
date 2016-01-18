// AUTHOR: Daniel Greenheck
// PROGRAM: STF1_CDH_EMULATOR
// DESCRIPTION: Emulates the CDH software responsible for communicating with the IMU
// VERSION: 1.0

// ----------------------------------------------------------------------------------------
// HOW TO USE:
//
// First, set the sampling rate of the IMU as defined in the CCS software (main.h -> SAMPLE_RATE)

unsigned int sampleRate = 200;

// There are three operating modes for the IMU
// ----------------------------------------------------------------------------------------
// 1) Streaming mode - The IMU continuously updates its data registers with updated
//    calibrated data. The CDH software reads the data from these registers at a fixed rate.
//
//    This mode has two sub-modes:
//       a) Output angular velocity and specific force
//       b) Output delta theta, delta velocity, integrated attitude quaternion, temperature,
//          and accumulated velocity.
//    For mode a), set streamingMode = false. For mode b), set streamingMode = true

bool streamingMode = true;

//    Data is output at a rate controlled by the sampling rate and the output rate divider.
//    The sample rate and output divider should be chosen such that their ratio is an integer.
//    [Example] sampleRate / outputDivider = 200 / 8 = 25 Hz

unsigned int outputDivider = 8;    // Possible values: 2 to 15

// ----------------------------------------------------------------------------------------
// 2) SD write mode - The IMU records both raw and calibrated data to the SD card. Later on,
//    the SD card can be removed and inserted into a workstation, or the data file can be
//    transmitted (SLOWLY) over the I2C bus.
//
//    Set the acquisition time below

unsigned long acqTime = 5;

// ----------------------------------------------------------------------------------------
// 3) SD read mode - As mentioned above, this mode allows the data contained in the SD card
//  data file to be transmitted over the I2C bus.
// ----------------------------------------------------------------------------------------

// Set operatingMode to the desired operating mode
const int STREAMING = 1;
const int SD_WRITE = 2;
const int SD_READ = 3;

int operatingMode = SD_READ;

// ----------------------------------------------------------------------------------------
// DO NOT MODIFY BELOW
// ----------------------------------------------------------------------------------------

#include <Wire.h>
#include <SoftwareSerial.h>

const byte IMU_SLAVE_ADDRESS = 0x30;    // IMU I2C slave address

const byte IMU_DAQ_REG = 0x04;          // Data acquisition register address
const byte IMU_STAT_REG = 0x06;         // IMU status register address
const byte SD_STATUS_REG = 0x5B;        // SD status register address
const byte SD_DATA_REG = 0x5C;          // SD data register address
const byte IMU_TICK_REG = 0x3F;         // IMU tick count register address

const byte IMU_DAQ_EN = 0b00000001;
const byte IMU_I2C_MODE = 0b00000010;
const byte SD_OVERWRITE = 0b00001000;
const byte STREAMING_MODE = 0b00000010;
const byte SD_WRITE_MODE = 0b00000100;
const byte SD_READ_MODE = 0b00000110;

const byte SD_DIRTY = 0x01;
const byte SD_EOF = 0x02;

bool collectData = true;
char outstr[64];
byte data[128] = {0};
unsigned long count = 0;
unsigned long prevCount = 0;
unsigned long prevTimeStamp = 0;
byte x = 0;
byte sdStatus;

// Sampling time used by Arduino for reading from IMU
unsigned int sampleTime = 1000 * ((float)outputDivider) / ((float)sampleRate);

// ----------------------------------------------------------------------------------------

SoftwareSerial mySerial(10, 11); // RX, TX

void setup()
{
  Serial.begin(9600);
  mySerial.begin(230400);
  Wire.begin();

  prevCount = millis();
  
  if (operatingMode == STREAMING) {
    // Put the IMU into streaming mode
    Wire.beginTransmission(IMU_SLAVE_ADDRESS);
    Wire.write(IMU_DAQ_REG);
    Wire.write((outputDivider << 4) | IMU_I2C_MODE | IMU_DAQ_EN);
    Wire.endTransmission();
  }
  else if (operatingMode == SD_WRITE) {
    // Put IMU into SD write mode. Overwrite the data file if there is one.
    Wire.beginTransmission(IMU_SLAVE_ADDRESS);
    Wire.write(IMU_DAQ_REG);
    Wire.write(SD_OVERWRITE | SD_WRITE_MODE | IMU_DAQ_EN);
    Wire.endTransmission();

    // Collect data for the desired amount of time
    Serial.print("Acquiring data for ");
    Serial.print(acqTime);
    Serial.println(" seconds...");
    delay(acqTime * 1000);
  }
  else if (operatingMode == SD_READ) {
    // Put IMU into SD read mode
    Wire.beginTransmission(IMU_SLAVE_ADDRESS);
    Wire.write(IMU_DAQ_REG);
    Wire.write(SD_READ_MODE);
    Wire.endTransmission();
    Serial.print("Reading data from IMU SD card...");
  }

  Serial.println("Done");
}

void loop() {
  if (operatingMode == STREAMING)
    ModeStreaming();
  else if (operatingMode == SD_READ)
    ModeSDRead();
}

void ModeSDRead() {
  if (collectData) {
    // Read a byte from the SD status register to see if data is ready
    Wire.beginTransmission(IMU_SLAVE_ADDRESS);
    Wire.write(SD_STATUS_REG);
    Wire.endTransmission(false);

    // First byte is SD status byte
    Wire.requestFrom(IMU_SLAVE_ADDRESS, 1);

    if (Wire.available()) {
      sdStatus = Wire.read();
    }

    // Check to see if data is fresh. If so, read rest of data and write to serial port
    if (!(sdStatus & SD_DIRTY)) {
      // Write the register address we want to read from
      Wire.beginTransmission(IMU_SLAVE_ADDRESS);
      Wire.write(SD_DATA_REG);
      Wire.endTransmission(false);

      // Read the SD data (128 bytes)
      Wire.requestFrom(IMU_SLAVE_ADDRESS, 128);

      int i = 0;
      while (Wire.available()) {
        data[i++] = Wire.read();
      }
      mySerial.write(data, 128);
    }
    // End of file detected
    if (sdStatus & SD_EOF) {
      collectData = false;
      Serial.println("... Done");
      Serial.println("SD EOF reached.");
    }
  }
}

void ModeStreaming() {
  count = millis();
  float elapsedTime = count - prevCount;

  // Sample the IMU at the sampling time. If the sampling time is too
  // short, the Arduino won't be able to sample fast enough and data
  // will be lost.
  if (elapsedTime >= sampleTime) {
    prevCount = count;

    // Output data in either calibrated or raw format
    if (streamingMode)
      CalibratedOutput();
    else
      RawOutput();
  }
}

void CalibratedOutput() {
  // Write the starting address for the burst read
  Wire.beginTransmission(IMU_SLAVE_ADDRESS);
  Wire.write(IMU_STAT_REG);
  Wire.endTransmission(false);

  // Sequentially read all of the IMU data registers
  Wire.requestFrom(IMU_SLAVE_ADDRESS, 61);

  int i = 0;
  while (Wire.available()) {
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
  mySerial.print(ptrData[0], 3);
  mySerial.print(',');
  mySerial.print(ptrData[1], 3);
  mySerial.print(',');
  mySerial.print(ptrData[2], 3);
  mySerial.print(';');

  // Delta velocity
  mySerial.print(ptrData[3], 3);
  mySerial.print(',');
  mySerial.print(ptrData[4], 3);
  mySerial.print(',');
  mySerial.print(ptrData[5], 3);
  mySerial.print(';');

  // Integrated quaternion
  mySerial.print(ptrData[6], 3);
  mySerial.print(',');
  mySerial.print(ptrData[7], 3);
  mySerial.print(',');
  mySerial.print(ptrData[8], 3);
  mySerial.print(',');
  mySerial.print(ptrData[9], 3);
  mySerial.print(';');

  // Temperature
  mySerial.print(ptrData[10], 3);
  mySerial.print(';');

  // Accumulated velocity
  mySerial.print(ptrData[11], 3);
  mySerial.print(',');
  mySerial.print(ptrData[12], 3);
  mySerial.print(',');
  mySerial.print(ptrData[13], 3);
  mySerial.println(';');
}

void RawOutput() {
  // Write the starting address for the burst read
  Wire.beginTransmission(IMU_SLAVE_ADDRESS);
  Wire.write(IMU_TICK_REG);
  Wire.endTransmission(false);

  // Sequentially read all of the IMU data registers
  Wire.requestFrom(IMU_SLAVE_ADDRESS, 28);

  int i = 0;
  while (Wire.available()) {
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
  mySerial.print(ptrData[0], 4);
  mySerial.print(',');
  mySerial.print(ptrData[1], 4);
  mySerial.print(',');
  mySerial.print(ptrData[2], 4);
  mySerial.print(';');

  // Specific force
  mySerial.print(ptrData[3], 4);
  mySerial.print(',');
  mySerial.print(ptrData[4], 4);
  mySerial.print(',');
  mySerial.print(ptrData[5], 4);
  mySerial.println(';');
}
