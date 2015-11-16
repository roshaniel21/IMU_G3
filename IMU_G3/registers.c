#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_i2c.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/i2c.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "inc/tm4c1294ncpdt.h"

#include "imu.h"
#include "registers.h"
#include "util.h"

// ***************************************************************************
// I2C system variables
// ***************************************************************************

// Flag indicating a register was updated
volatile bool registerUpdated = false;
// Current state of the I2C handler (default to IDLE)
volatile uint8_t state = I2C_STATE_IDLE;
// Memory address that master is reading/writing to
volatile uint8_t addr = 0x00;
// Memory registers accessible via I2C
char reg[REG_COUNT] = {0x00};
// Define which registers are read-only and which ones are read-write
bool regRW[REG_COUNT];


// **********************************************************************
// ************* Initialization Methods *********************************
// **********************************************************************


void InitializeRegisters(void) {

	// Set the read/write properties for the registers
	regRW[REG_IMU_EN_1] = 1;
	regRW[REG_IMU_EN_2] = 1;
	regRW[REG_IMU_EN_3] = 1;
	regRW[REG_IMU_EN_4] = 1;
	regRW[REG_IMU_DAQ] = 1;

	// Set the IMU enable registers to their default values
	reg[REG_IMU_EN_1] = (IMU_ENABLE_DEFAULT & 0x000000FF);
	reg[REG_IMU_EN_2] = (IMU_ENABLE_DEFAULT & 0x0000FF00) >> 8;
	reg[REG_IMU_EN_3] = (IMU_ENABLE_DEFAULT & 0x00FF0000) >> 16;
	reg[REG_IMU_EN_4] = (IMU_ENABLE_DEFAULT & 0xFF000000) >> 24;

	// Set the data acquisition register to the default values
	reg[REG_IMU_DAQ] |= (OUTPUT_RATE_DIV_DEFAULT << 4) & OUTPUT_RATE_DIV_MASK;
	reg[REG_IMU_DAQ] |= (SD_OVERWRITE_DEFAULT << 3) & SD_OVERWRITE_MASK;
	reg[REG_IMU_DAQ] |= (MODE_STREAMING << 1) & IMU_MODE_MASK;
	reg[REG_IMU_DAQ] |= IMU_DAQ_EN_MASK;
}


// **********************************************************************
// ************* Accessor Methods ***************************************
// **********************************************************************


uint8_t GetI2CState(void) {
	return state;
}

uint8_t GetIMUMode(void) {
	return ((reg[REG_IMU_DAQ] & IMU_MODE_MASK) >> 1);
}

uint8_t GetRegVal(uint8_t addr) {
	// If address is out of range, just return 0
	if(addr >= REG_COUNT) {
		return 0;
	}
	else {
		return reg[addr];
	}
}

bool GetSDFileOverwrite(void) {
	return (bool)((reg[REG_IMU_DAQ] & SD_OVERWRITE_MASK) >> 3);
}

uint8_t GetOutputRateDivider(void) {
	uint8_t outputRateDiv = (reg[REG_IMU_DAQ] & OUTPUT_RATE_DIV_MASK) >> 4;

	// If output rate in register is less than twice the sampling rate, set to default value
	// to prevent any issues
	if(outputRateDiv < 2) {
		outputRateDiv = OUTPUT_RATE_DIV_DEFAULT;
	}

	return outputRateDiv;
}

bool IsDAQEnabled(void) {
	return (bool)(reg[REG_IMU_DAQ] & IMU_DAQ_EN_MASK);
}

bool IsRegisterUpdateFlagRaised(void) {
	return registerUpdated;
}

bool IsSDDataRequested(void) {
	return (bool)(reg[REG_SD_STAT] & SD_READY_MASK);
}


// **********************************************************************
// ************* Setter Methods *****************************************
// **********************************************************************


void ClearSDEOFFlag(void) {
	reg[REG_SD_STAT] &= ~SD_EOF_MASK;
}

void ClearSDReadyFlag(void) {
	reg[REG_SD_STAT] &= ~SD_READY_MASK;
}

void ClearRegisterUpdateFlag(void) {
	registerUpdated = false;
}

void RaiseSDEOFFlag(void) {
	reg[REG_SD_STAT] |= SD_EOF_MASK;
}

void RegWriteFloat32(uint8_t addr, float val) {
	// Make sure we don't write to memory outside of registers
	if(addr <= REG_COUNT - sizeof(val)) {
		// Copy value to memory location at address
		memcpy(&reg[addr], &val, sizeof(val));
	}
}

void RegWriteUInt32(uint8_t addr, uint32_t val) {
	// Make sure we don't write to memory outside of registers
	if(addr <= REG_COUNT - sizeof(val)) {
		// Copy value to memory location at address
		memcpy(&reg[addr], &val, sizeof(val));
	}
}

void RegWriteUInt8(uint8_t addr, uint8_t val) {
	reg[addr] = val;
}


// **********************************************************************
// ************* Interrupt Methods **************************************
// **********************************************************************


void I2C0SlaveIntHandler(void) {

	// Read the interrupt status register (what kind of interrupt is this?)
	uint32_t slaveIntReg = HWREG(CDH_I2C_BASE + I2C_O_SMIS);
	// Status indicates if data is read or write
	uint32_t status = I2CSlaveStatus(CDH_I2C_BASE);

	// Master sent start condition
	if(slaveIntReg & I2C_SMIS_STARTMIS) {
		// Clear start bit in interrupt status register
		HWREG(CDH_I2C_BASE + I2C_O_SICR) |= I2C_SICR_STARTIC;

		switch(state) {
			// If currently idle, start new transaction
			case I2C_STATE_IDLE:
				state = I2C_STATE_START;
				break;
			default:
				state = I2C_STATE_READ;
				break;
		}
	}
	// Master sent stop condition
	if(slaveIntReg & I2C_SMIS_STOPMIS) {
		// Clear stop bit in interrupt status register
		HWREG(CDH_I2C_BASE + I2C_O_SICR) |= I2C_SICR_STOPIC;
		// No matter what state we were in previously, go to idle state
		state = I2C_STATE_IDLE;
	}
	// Master sent a byte of data. Could be address or data
	if(slaveIntReg & I2C_SMIS_DATAMIS) {
		// Clear data bit in interrupt status register
		HWREG(CDH_I2C_BASE + I2C_O_SICR) |= I2C_SICR_DATAIC;

		// Master is requesting data
		if(status & I2C_SLAVE_ACT_TREQ) {
			switch(state) {
				// Address received
				case I2C_STATE_ADDR:
					// Send data at address and increment address
					I2CSlaveDataPut(CDH_I2C_BASE, reg[addr++]);
					state = I2C_STATE_READ;
					break;
				// Master is writing data to IMU
				case I2C_STATE_WRITE:
					I2CSlaveDataPut(CDH_I2C_BASE, reg[addr++]);
					break;
				// Master is reading data from IMU
				case I2C_STATE_READ:
					// Send data at address and increment address
					I2CSlaveDataPut(CDH_I2C_BASE, reg[addr]);

					// If master reads last SD data register, need to update the data registers.
					// SD_READY flag is raised, notifying main loop to read more data from the SD
					// card and copy it to the data registers
					if(addr == REG_SD_DATA_LAST) {
						reg[REG_SD_STAT] |= SD_READY_MASK;
					}
					addr++;

					break;
				// This is an invalid state but we must handle it anyway
				default:
					// Write garbage
					I2CSlaveDataPut(CDH_I2C_BASE, 0x00);
					break;
			}
		}
		// Master is transmitting data
		else {
			switch(state) {
				// Start condition sent
				case I2C_STATE_START:
					// Store address
					addr = I2CSlaveDataGet(CDH_I2C_BASE);
					state = I2C_STATE_ADDR;
					break;
				// Master is writing data to IMU data registers
				case I2C_STATE_ADDR:
				case I2C_STATE_WRITE:
					// Only update register if it is read/write
					if(regRW[addr]) {
						// If settings register is updated, raise flag
						if(addr == REG_IMU_DAQ) {
							registerUpdated = true;
						}
						reg[addr++] = I2CSlaveDataGet(CDH_I2C_BASE);

						state = I2C_STATE_WRITE;
					}
					// If master is writing to read-only register, just toss out data
					else {
						uint32_t trash = I2CSlaveDataGet(CDH_I2C_BASE);
					}
					break;
				// This is an invalid state but we must handle it anyway
				default:
					// Grab data but don't store it
					I2CSlaveDataGet(CDH_I2C_BASE);
			}
		}
	}
}
