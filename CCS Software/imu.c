#include <stdbool.h>
#include <stdint.h>

#include "driverlib/rom.h"
#include "driverlib/ssi.h"
#include "driverlib/sysctl.h"
#include "inc/hw_memmap.h"
#include "inc/tm4c1294ncpdt.h"
#include "utils/uartstdio.h"

#include "imu.h"
#include "util.h"

// If the i-th bit in imuEnable is 1, the i-th IMU is included in the data acquisition loop
uint32_t imuEnable = 0xFFFFFFFF;

// ***********************************
// SPI FUNCTIONS
// ***********************************

uint32_t SPIReadByte(char reg, uint8_t i) {

    uint32_t return_data = 0;

    // Select IMU by pulling CS low.
    GPIOPinWrite(IMU_PORT_BASE[i], IMU_PIN[i], 0);

    // Write register to SPI bus
    ROM_SSIDataPut(IMU_SPI_BASE, 0x80 | reg);
    ROM_SSIDataGet(IMU_SPI_BASE, &return_data);
    // Write dummy byte and read off result
    ROM_SSIDataPut(IMU_SPI_BASE, 0x00);
    ROM_SSIDataGet(IMU_SPI_BASE, &return_data);

    // Deselect IMU by pulling CS high.
    GPIOPinWrite(IMU_PORT_BASE[i], IMU_PIN[i], IMU_PIN[i]);

	return return_data;
}

void SPIBurstReadStart(char reg, uint8_t i) {

    uint32_t return_data = 0;

    ROM_SSIDataPut(IMU_SPI_BASE, 0x80 | reg);          // Send register to start reading at
    ROM_SSIDataGet(IMU_SPI_BASE, &return_data);        // Throw out junk data
}

uint32_t SPIBurstReadShort() {

	// Get high byte
    uint32_t val_h = 0;
    ROM_SSIDataPut(IMU_SPI_BASE, 0x00);
    ROM_SSIDataGet(IMU_SPI_BASE, &val_h);
    // Get low byte
    uint32_t val_l = 0;
    ROM_SSIDataPut(IMU_SPI_BASE, 0x00);
    ROM_SSIDataGet(IMU_SPI_BASE, &val_l);

    // Combine low and high bytes
	return (val_h << 8) + val_l;
}

void SPIWriteByte(char reg, char data, uint8_t i) {

    uint32_t return_data = 0;

    // Select IMU by pulling CS low.
    GPIOPinWrite(IMU_PORT_BASE[i], IMU_PIN[i], 0);

    // Write register to SPI bus
    ROM_SSIDataPut(IMU_SPI_BASE, reg);
    ROM_SSIDataGet(IMU_SPI_BASE, &return_data);
    // Write data to SPI bus
    ROM_SSIDataPut(IMU_SPI_BASE, data);
    ROM_SSIDataGet(IMU_SPI_BASE, &return_data);

    // Deselect IMU by pulling CS high.
    GPIOPinWrite(IMU_PORT_BASE[i], IMU_PIN[i], IMU_PIN[i]);
}


// ***********************************
// IMU FUNCTIONS
// ***********************************

uint32_t GetIMUEnableVector(void) {
	return imuEnable;
}

bool IsIMUEnabled(uint8_t i) {
	return CHECK_BIT(imuEnable,i);
}

void EnableIMU(uint8_t i) {
	SET_BIT(imuEnable,i);
}

void DisableIMU(uint8_t i) {
	CLEAR_BIT(imuEnable,i);
}

void PowerDownIMU(uint8_t i) {
	SPIWriteByte(PWR_MGMT_1, PWR_MGMT_1_SLP | PWR_MGMT_1_PLL, i);
}

void PowerUpIMU(uint8_t i) {
	SPIWriteByte(PWR_MGMT_1, PWR_MGMT_1_PLL, i);
}

void ConfigureIMUs(uint32_t systemClock) {

#ifdef DEBUG_MODE
	UARTprintf("\tConfigure IMUs...");
#endif
	bool imuFound = false;
	uint8_t i = 0;
	for(i = 0; i < NUM_SENSORS; i++) {
		// Get device ID from IMU
		uint8_t counter = 0;
		while(counter < COUNTER_MAX && !imuFound) {
			// If IMU returns correct device ID, continue with configuration
			if(SPIReadByte(WHO_AM_I, i) == IMU_DEVICE_ID) {
				break;
			}
			// If we reached maximum number of reads, assume IMU has failed
			// Set enable at that bit to 0
			if(++counter == COUNTER_MAX) {
				DisableIMU(i);
			}

			// Wait 10 ms
			SysCtlDelay(systemClock/3/100);
		}
		// Set clock source to PLL and pull out of sleep mode
		SPIWriteByte(PWR_MGMT_1, PWR_MGMT_1_PLL, i);
		// Disable I2C slave
		SPIWriteByte(USER_CTRL, USER_CTRL_I2C_EN, i);
		// Set accelerometer full range (+/- 2g)
		SPIWriteByte(ACCEL_CONFIG, ACCEL_CONFIG_FS_2G, i);
		// Set gyro full range (+/- 250 dps) and bypass low pass filter (since we are using averaging)
		SPIWriteByte(GYRO_CONFIG, GYRO_CONFIG_FS_250 | GYRO_CONFIG_BYP_LPF, i);
		// Enable gyro low-power mode and use 4X averaging
		SPIWriteByte(LP_MODE_CFG, LP_MODE_CFG_GLP | LP_MODE_CFG_2X_AVG, i);
		// Enable accelerometer low power mode, disable DLPF, 4x averaging
		SPIWriteByte(ACCEL_CONFIG_2, ACCEL_CONFIG_2_4X_AVG | ACCEL_CONFIG_2_BYP_LPF |
				ACCEL_CONFIG_2_DLPF_CFG, i);
	}
#ifdef DEBUG_MODE
	UARTprintf(" done\n");
#endif
}
