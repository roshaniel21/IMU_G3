#ifndef MAIN_H_
#define MAIN_H_


// **********************************************************************************
// System defines
// **********************************************************************************

// Microcontroller clock speed (default: 120000000)
#define MCU_CLK_SPEED           	(120000000)
// IMU SPI bus clock speed
#define IMU_SPI_CLK_SPEED       	(8000000)
// Number of records to keep in data queue
#define QUEUE_SIZE     		        (100)

// Size of SD card write buffer
#define SD_BUFFER_SIZE          	(4096)
// Packet size for calibrated data
#define RAW_PKT_SIZE_1         		(244)
#define RAW_PKT_SIZE_2              (212)
#define CAL_PKT_SIZE                (58)

// IMU sampling rate (MUST BE INTEGER MULTIPLE OF UPDATE_RATE)
#define SAMPLE_RATE         (200)

// Digital conversion factors for accelerometer and gyro
const float K_A = 0.000061035;
const float K_G = 0.007633587;
const float DEG_TO_RAD = 0.0174533;
const float GRAVITY = 9.81;

// Definitions used to map array indices to what type of value is at that index.
// Used in the calibration code
#define AX                      	(0)
#define AY                      	(1)
#define AZ                      	(2)
#define GX                      	(3)
#define GY                      	(4)
#define GZ                      	(5)
#define TEMP                        (6)

#define X                      		(0)
#define Y                      		(1)
#define Z                      		(2)
#define W                           (3)

#define NUM_IMU_VALUES              (6)


// **********************************************************************************
// Data acquisition timer
// **********************************************************************************
#define DATA_ACQ_TIMER_BASE     TIMER0_BASE

void GetIMUData(uint32_t timeStamp);

// Write latest navigation data to the registers
void WriteDataToRegisters(uint32_t recordTimeStamp);

// Writes a raw data record to the SD card
void WriteRawDataToSDCard(uint16_t k);

// Writes a calibrated data record to the SD card
void WriteCalibratedDataToSDCard();

#endif /* MAIN_H_ */
