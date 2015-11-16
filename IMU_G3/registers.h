/*
 * imu_i2c.h
 *
 *  Created on: Oct 4, 2015
 *      Author: Daniel Greenheck
 *
 *  Handles all processing of I2C commands
 */


#ifndef REGISTERS_H_
#define REGISTERS_H_

// A zero in the R/S position of the first bit means that the master
// transmits (sends) data to the selected slave, and a one in this position
// means that the master receives data from the slave.
// Set the address for slave module. This is a 7-bit address sent in the
// following format:
//                      [A6:A5:A4:A3:A2:A1:A0:RS]
#define SLAVE_ADDRESS           	(0x30)

// Number of registers
#define REG_COUNT   	            (220)
// Number of SD data registers
#define SD_DATA_REG_COUNT           (128)
// Declare registers as external so they can be accessed by the main function
extern char reg[REG_COUNT];

// Operating modes
#define MODE_STREAMING             	(0x01)
#define MODE_SD_WRITE               (0x02)
#define MODE_SD_READ	            (0X03)

// Default initialization conditions
#define IMU_ENABLE_DEFAULT          (0xFFFFFFFF)	// Enable all IMUs
#define SD_OVERWRITE_DEFAULT        (0x00000000)	// No overwrite
#define OUTPUT_RATE_DIV_DEFAULT     (0x0000000A)	// 10X divider
#define DAQ_ENABLE_DEFAULT 	        (0x00000001)	// Enabled

// Peripheral pin assignments
#define CDH_I2C_BASE	        	I2C0_BASE
#define CDH_I2C_SDA	           		I2C0_SDA
#define CDH_I2C_SCL     			I2C0_SCL

// I2C interrupt state machine
#define I2C_STATE_IDLE              (0x00)      // I2C transcation is completed, nothing to do
#define I2C_STATE_START         	(0x01)		// Start condition detected
#define I2C_STATE_ADDR      		(0x02)		// Register address received
#define I2C_STATE_WRITE          	(0x03)      // Master is writing data to IMU
#define I2C_STATE_READ              (0x04)      // Master is reading data from IMU

// **********************************************************************************
// *** IMU Registers (see register map for description of registers and contents) ***
// **********************************************************************************

#define REG_IMU_EN_1            	(0x00)
#define REG_IMU_EN_2            	(0x01)
#define REG_IMU_EN_3            	(0x02)
#define REG_IMU_EN_4            	(0x03)
#define REG_IMU_DAQ             	(0x04)
#define REG_IMU_DIV             	(0x05)
#define REG_IMU_STAT            	(0x06)

#define REG_DELTA_THETA_X_1	        (0x07)
#define REG_DELTA_THETA_X_2	        (0x08)
#define REG_DELTA_THETA_X_3	        (0x09)
#define REG_DELTA_THETA_X_4	        (0x0A)
#define REG_DELTA_THETA_Y_1	        (0x0B)
#define REG_DELTA_THETA_Y_2	        (0x0C)
#define REG_DELTA_THETA_Y_3	        (0x0D)
#define REG_DELTA_THETA_Y_4	        (0x0E)
#define REG_DELTA_THETA_Z_1	        (0x0F)
#define REG_DELTA_THETA_Z_2	        (0x10)
#define REG_DELTA_THETA_Z_3	        (0x11)
#define REG_DELTA_THETA_Z_4	        (0x12)

#define REG_DELTA_VEL_X_1	        (0x13)
#define REG_DELTA_VEL_X_2	        (0x14)
#define REG_DELTA_VEL_X_3	        (0x15)
#define REG_DELTA_VEL_X_4	        (0x16)
#define REG_DELTA_VEL_Y_1	        (0x17)
#define REG_DELTA_VEL_Y_2	        (0x18)
#define REG_DELTA_VEL_Y_3	        (0x19)
#define REG_DELTA_VEL_Y_4	        (0x1A)
#define REG_DELTA_VEL_Z_1	        (0x1B)
#define REG_DELTA_VEL_Z_2	        (0x1C)
#define REG_DELTA_VEL_Z_3	        (0x1D)
#define REG_DELTA_VEL_Z_4	        (0x1E)

#define REG_IQUAT_X_1	            (0x1F)
#define REG_IQUAT_X_2	            (0x20)
#define REG_IQUAT_X_3	            (0x21)
#define REG_IQUAT_X_4	            (0x22)
#define REG_IQUAT_Y_1	            (0x23)
#define REG_IQUAT_Y_2	            (0x24)
#define REG_IQUAT_Y_3	            (0x25)
#define REG_IQUAT_Y_4	            (0x26)
#define REG_IQUAT_Z_1	            (0x27)
#define REG_IQUAT_Z_2	            (0x28)
#define REG_IQUAT_Z_3	            (0x29)
#define REG_IQUAT_Z_4	            (0x2A)
#define REG_IQUAT_W_1	            (0x2B)
#define REG_IQUAT_W_2	            (0x2C)
#define REG_IQUAT_W_3	            (0x2D)
#define REG_IQUAT_W_4	            (0x2E)

#define REG_TEMP_1	          		(0x2F)
#define REG_TEMP_2	            	(0x30)
#define REG_TEMP_3	            	(0x31)
#define REG_TEMP_4	            	(0x32)

#define REG_ACC_VEL_X_1	            (0x33)
#define REG_ACC_VEL_X_2	            (0x34)
#define REG_ACC_VEL_X_3	            (0x35)
#define REG_ACC_VEL_X_4	            (0x36)
#define REG_ACC_VEL_Y_1	            (0x37)
#define REG_ACC_VEL_Y_2	            (0x38)
#define REG_ACC_VEL_Y_3	            (0x39)
#define REG_ACC_VEL_Y_4	            (0x3A)
#define REG_ACC_VEL_Z_1	            (0x3B)
#define REG_ACC_VEL_Z_2	            (0x3C)
#define REG_ACC_VEL_Z_3	            (0x3D)
#define REG_ACC_VEL_Z_4	            (0x3E)

#define REG_TICK_1	          		(0x3F)
#define REG_TICK_2	            	(0x40)
#define REG_TICK_3	            	(0x41)
#define REG_TICK_4	            	(0x42)

#define REG_ANG_VEL_X_1	            (0x43)
#define REG_ANG_VEL_X_2	            (0x44)
#define REG_ANG_VEL_X_3	            (0x45)
#define REG_ANG_VEL_X_4	            (0x46)
#define REG_ANG_VEL_Y_1	            (0x47)
#define REG_ANG_VEL_Y_2	            (0x48)
#define REG_ANG_VEL_Y_3	            (0x49)
#define REG_ANG_VEL_Y_4	            (0x4A)
#define REG_ANG_VEL_Z_1	            (0x4B)
#define REG_ANG_VEL_Z_2	            (0x4C)
#define REG_ANG_VEL_Z_3	            (0x4D)
#define REG_ANG_VEL_Z_4	            (0x4E)

#define REG_SP_FORCE_X_1	        (0x4F)
#define REG_SP_FORCE_X_2	        (0x50)
#define REG_SP_FORCE_X_3	        (0x51)
#define REG_SP_FORCE_X_4	        (0x52)
#define REG_SP_FORCE_Y_1	        (0x53)
#define REG_SP_FORCE_Y_2	        (0x54)
#define REG_SP_FORCE_Y_3	        (0x55)
#define REG_SP_FORCE_Y_4	        (0x56)
#define REG_SP_FORCE_Z_1	        (0x57)
#define REG_SP_FORCE_Z_2	        (0x58)
#define REG_SP_FORCE_Z_3	        (0x59)
#define REG_SP_FORCE_Z_4	        (0x5A)

#define REG_SD_STAT                 (0x5B)
#define REG_SD_DATA                 (0x5C)
#define REG_SD_DATA_LAST 	        (0xDB)

// **********************************************************************
// ************* Bit Masks **********************************************
// **********************************************************************


#define OUTPUT_RATE_DIV_MASK		(0xF0)
#define SD_OVERWRITE_MASK			(0x08)
#define IMU_MODE_MASK				(0X06)
#define IMU_DAQ_EN_MASK				(0x01)
#define SD_READY_MASK      			(0x01)
#define SD_EOF_MASK        			(0x02)


// **********************************************************************
// ************* Initialization Methods *********************************
// **********************************************************************


// Configures the registers to their default values
void InitializeRegisters(void);


// **********************************************************************
// ************* Accessor Methods ***************************************
// **********************************************************************


// Gets the current state of the I2C state machine
uint8_t GetI2CState(void);

// Gets the operating mode
uint8_t GetIMUMode(void);

// Returns the value of the register at 'addr'
uint8_t GetRegVal(uint8_t addr);

// Returns the output rate divider
uint8_t GetOutputRateDivider(void);

// Returns true if the SD overwrite flag is set to true
bool GetSDFileOverwrite(void);

// Returns true if the data acquisition is enabled
bool IsDAQEnabled(void);

// Returns true if the register update flag is raised
bool IsRegisterUpdateFlagRaised(void);

// Returns true if the SD data request flag is raised
bool IsSDDataRequested(void);


// **********************************************************************
// ************* Setter Methods *****************************************
// **********************************************************************


// Clears the end of file flag
void ClearSDEOFFlag(void);

// Clears the SD data request flag
void ClearSDReadyFlag(void);

// Clears the flag indicating that the registers have been udpated
void ClearRegisterUpdateFlag(void);

// Raises the SD end of file flag
void RaiseSDEOFFlag(void);

// Writes the 32-bit floating point to the block of registers [addr : addr + 3]
// If write will modify memory outside register address range, no write occurs
void RegWriteFloat32(uint8_t addr, float val);

// Writes the unsigned 32-bit integer to the block of registers [addr : addr + 3]
// If write will modify memory outside register address range, no write occurs
void RegWriteUInt32(uint8_t addr, uint32_t val);

// Writes the unsigned 8-bit integer to the register with address 'addr'
// If write will modify memory outside register address range, no write occurs
void RegWriteUInt8(uint8_t addr, uint8_t val);


// **********************************************************************
// ************* Interrupt Methods **************************************
// **********************************************************************


// Interrupt handler for I2C0 slave
void I2C0SlaveIntHandler(void);


#endif /* REGISTERS_H_ */
