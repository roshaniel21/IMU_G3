/*
 * imu_spi.h
 *
 *  Created on: Oct 4, 2015
 *      Author: Daniel Greenheck
 */
#include "driverlib/gpio.h"

#ifndef IMU_H_
#define IMU_H_

#define NUM_SENSORS         (32)

// *************************************************
// SPI PERIPHERAL PIN ASSIGNMENTS
// *************************************************
#define IMU_SPI_BASE        SSI2_BASE
#define IMU_SPI_PORT_BASE	GPIO_PORTD_BASE
#define IMU_SPI_RX          GPIO_PIN_0
#define IMU_SPI_TX          GPIO_PIN_1
#define IMU_SPI_FSS         GPIO_PIN_2
#define IMU_SPI_CLK         GPIO_PIN_3

// *************************************************
// ICM-20608 REGISTER DEFINITIONS
// *************************************************

// Self test registers
#define SELF_TEST_X_GYRO    (0x00)
#define SELF_TEST_Y_GYRO    (0x01)
#define SELF_TEST_Z_GYRO    (0x02)
#define SELF_TEST_X_ACCEL   (0x0D)
#define SELF_TEST_Y_ACCEL   (0x0E)
#define SELF_TEST_Z_ACCEL   (0x0F)

// Interrupt registers
#define FSYNC_INT           (0x36)
#define INT_PIN_CFG         (0x37)
#define INT_ENABLE			(0x38)
#define INT_STATUS          (0x3A)

// Data registers
#define ACCEL_XOUT_H        (0x3B)
#define ACCEL_XOUT_L        (0x3C)
#define ACCEL_YOUT_H        (0x3D)
#define ACCEL_YOUT_L        (0x3E)
#define ACCEL_ZOUT_H        (0x3F)
#define ACCEL_ZOUT_L        (0x40)
#define TEMP_OUT_H        	(0x41)
#define TEMP_OUT_L        	(0x42)
#define GYRO_XOUT_H        	(0x43)
#define GYRO_XOUT_L        	(0x44)
#define GYRO_YOUT_H        	(0x45)
#define GYRO_YOUT_L        	(0x46)
#define GYRO_ZOUT_H        	(0x47)
#define GYRO_ZOUT_L        	(0x48)

// Configuration Registers
#define SMPLRT_DIV          	(0x19)
#define CONFIG              	(0x1A)
#define GYRO_CONFIG         	(0x1B)
#define GYRO_CONFIG_FS_250  	(0b00000000)
#define GYRO_CONFIG_BYP_LPF 	(0b00000000)
#define ACCEL_CONFIG        	(0x1C)
#define ACCEL_CONFIG_FS_2G  	(0b00000000)
#define ACCEL_CONFIG_2      	(0x1D)
#define ACCEL_CONFIG_2_4X_AVG   (0b00000000)
#define ACCEL_CONFIG_2_BYP_LPF  (0b00000000)
#define ACCEL_CONFIG_2_DLPF_CFG (0b00000111)
#define LP_MODE_CFG         	(0x1E)
#define LP_MODE_CFG_GLP     	(0b10000000)
#define LP_MODE_CFG_2X_AVG  	(0b00100000)
#define ACCEL_WOM_THR       	(0x1F)
#define SIGNAL_PATH_RESET		(0x68)
#define ACCEL_INTEL_CTRL  		(0x69)
#define USER_CTRL 				(0x6A)
#define USER_CTRL_I2C_EN    	(0b00010000)
#define PWR_MGMT_1				(0x6B)
#define PWR_MGMT_1_RST      	(0b10000000)
#define PWR_MGMT_1_SLP      	(0b01000000)
#define PWR_MGMT_1_PLL      	(0b00000001)
#define PWR_MGMT_2				(0x6C)

// FIFO registers
#define FIFO_EN				(0x23)
#define FIFO_COUNTH	        (0x72)
#define FIFO_COUNTL			(0x73)
#define FIFO_R_W			(0x74)

#define WHO_AM_I			(0x75)

// Bias offset registers
#define XG_OFFS_USRH        (0x13)
#define XG_OFFS_USRL        (0x14)
#define YG_OFFS_USRH        (0x15)
#define YG_OFFS_USRL        (0x16)
#define ZG_OFFS_USRH        (0x17)
#define ZG_OFFS_USRL        (0x18)
#define XA_OFFSET_H			(0x77)
#define XA_OFFSET_L			(0x78)
#define YA_OFFSET_H			(0x7A)
#define YA_OFFSET_L			(0x7B)
#define ZA_OFFSET_H			(0x7D)
#define ZA_OFFSET_L			(0x7E)

#define COUNTER_MAX			(10)
#define IMU_DEVICE_ID       (0xAF)


// ********************************************
// CHIP SELECT PIN ASSIGNMENTS
// ********************************************

static const uint32_t IMU_PORT_BASE[32] = {
	GPIO_PORTC_BASE,	// IMU 1
	GPIO_PORTC_BASE,	// IMU 2
	GPIO_PORTC_BASE,	// IMU 3
	GPIO_PORTC_BASE,	// IMU 4
	GPIO_PORTE_BASE,	// IMU 5
	GPIO_PORTE_BASE,	// IMU 6
	GPIO_PORTE_BASE,	// IMU 7
	GPIO_PORTE_BASE,	// IMU 8
	GPIO_PORTD_BASE,	// IMU 9
	GPIO_PORTD_BASE,	// IMU 10
	GPIO_PORTD_BASE,	// IMU 11
	GPIO_PORTD_BASE,	// IMU 12
	GPIO_PORTP_BASE,	// IMU 13
	GPIO_PORTP_BASE,	// IMU 14
	GPIO_PORTP_BASE,	// IMU 15
	GPIO_PORTQ_BASE,	// IMU 16
	GPIO_PORTB_BASE,	// IMU 17
	GPIO_PORTB_BASE,	// IMU 18
	GPIO_PORTL_BASE,	// IMU 19
	GPIO_PORTL_BASE,	// IMU 20
	GPIO_PORTL_BASE,	// IMU 21
	GPIO_PORTL_BASE,	// IMU 22
	GPIO_PORTL_BASE,	// IMU 23
	GPIO_PORTL_BASE,	// IMU 24
	GPIO_PORTK_BASE,	// IMU 25
	GPIO_PORTK_BASE,	// IMU 26
	GPIO_PORTK_BASE,	// IMU 27
	GPIO_PORTA_BASE,	// IMU 28
	GPIO_PORTA_BASE,	// IMU 29
	GPIO_PORTA_BASE,	// IMU 30
	GPIO_PORTA_BASE,	// IMU 31
	GPIO_PORTA_BASE		// IMU 32
};

static const uint8_t IMU_PIN[32] = {
	GPIO_PIN_4,			// IMU 1
	GPIO_PIN_5,			// IMU 2
	GPIO_PIN_6,			// IMU 3
	GPIO_PIN_7,			// IMU 4
	GPIO_PIN_0,			// IMU 5
	GPIO_PIN_1,			// IMU 6
	GPIO_PIN_2,			// IMU 7
	GPIO_PIN_3,			// IMU 8
	GPIO_PIN_7,			// IMU 9
	GPIO_PIN_6,			// IMU 10
	GPIO_PIN_5,			// IMU 11
	GPIO_PIN_4,			// IMU 12
	GPIO_PIN_4,			// IMU 13
	GPIO_PIN_3,			// IMU 14
	GPIO_PIN_2,			// IMU 15
	GPIO_PIN_4,			// IMU 16
	GPIO_PIN_1,			// IMU 17
	GPIO_PIN_0,			// IMU 18
	GPIO_PIN_5,			// IMU 19
	GPIO_PIN_4,			// IMU 20
	GPIO_PIN_3,			// IMU 21
	GPIO_PIN_2,			// IMU 22
	GPIO_PIN_1,			// IMU 23
	GPIO_PIN_0,			// IMU 24
	GPIO_PIN_5,			// IMU 25
	GPIO_PIN_6,			// IMU 26
	GPIO_PIN_7,			// IMU 27
	GPIO_PIN_6,			// IMU 28
	GPIO_PIN_5,			// IMU 29
	GPIO_PIN_4,			// IMU 30
	GPIO_PIN_3,			// IMU 31
	GPIO_PIN_2			// IMU 32
};

// ***********************************************
// FUNCTION DEFINITIONS
// ***********************************************

// Returns the entire IMU enable vector
uint32_t GetIMUEnableVector(void);

// Returns true if the 'i'-th bit of 'imuEnable' is 1
bool IsIMUEnabled(uint8_t i);

// Set the 'i'-th bit in 'imuEnable' to 1
void EnableIMU(uint8_t i);

// Set the 'i'-th bit in 'imuEnable' to 0
void DisableIMU(uint8_t i);

// Put the 'i'-th IMU into sleep mode
void PowerDownIMU(uint8_t i);

// Bring the 'i'-th IMU out of sleep mode
void PowerUpIMU(uint8_t i);

// Reads the value of the register 'reg' from the 'i'-th IMU
// Returns the value at that register
uint32_t SPIReadByte(char reg, uint8_t i);

// Begin a burst read starting at register 'reg' from the 'i'-th IMU
void SPIBurstReadStart(char reg, uint8_t i);

// After calling SPIBurstReadStart, call this to read a 16-bit value
uint32_t SPIBurstReadShort(void);

// Write 'data' to register 'reg' of the 'i'-th IMU
void SPIWriteByte(char reg, char data, uint8_t i);

// Initialization procedure for the IMUs. Configures each IMU to the
// correct settings. If an IMU does not respond during the initialization
// phase, that bit in 'imuEnable' is set to 0
void ConfigureIMUs(uint32_t systemClock);

#endif /* IMU_H_ */
