#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "inc/hw_gpio.h"
#include "inc/hw_hibernate.h"
#include "inc/hw_i2c.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/i2c.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/ssi.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"
#include "driverlib/uart.h"
#include "inc/tm4c1294ncpdt.h"
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"

#include "cobs.h"
#include "main.h"

#include "imu.h"
#include "registers.h"
#include "sd.h"
#include "util.h"
#include "vector3.h"

// *******************************************************************************
// SYSTEM
// *******************************************************************************

// Sampling time
const float Ts = 1.0 / (float)(SAMPLE_RATE);

// System clock rate in Hz.
uint32_t systemClock;
// Tick count to keep track of data samples
uint32_t tickCount = 0;
// Character buffer for UART debug output
char pcBuffer[128];

// *******************************************************************************
// DATA AND CALIBRATION
// *******************************************************************************

// Raw data for a single IMU
struct IMURawData {
	int16_t data[7];
};
// Raw data for multiple IMUs plus a time stamp
struct RawDataQueue {
	uint32_t timeStamp;
	struct IMURawData sensor[NUM_SENSORS];
};
// Create queue (or first-in-first-out buffer) so we can store up multiple data records
// Writing to the SD card can lock up the main loop for some time so this queue stores
// incoming data during that write period.
volatile struct RawDataQueue queue[QUEUE_SIZE];

// Indices to keep track of current read/write position in queue
volatile uint16_t writeIdx = 0;
uint16_t readIdx = 0;
// Number of unhandled records in queue
volatile uint16_t queueRecords = 0;

// Calibrated and averaged data samples
struct ProcDataRecord {
	// Latest delta theta
	float dTheta[3];
	// Latest delta velocity
	float dV[3];
	// Accumulated velocity
	float accumV[3];
	// Integrated quaternion
	float Q[4];
	// Averaged temperature
	float avgTemp;
	// Tick count associated with records in queue
	uint32_t timeStamp;
	// Averaged angular velocity
	float angVel[3];
	// Averaged specific force
	float specificForce[3];
};
// Most up-to-date output of IMU
struct ProcDataRecord processedData;

// Calibrated data samples
float dataCal[NUM_SENSORS][NUM_IMU_VALUES] = {0};
// Calibrated temperature of sensors
float tempCal[NUM_SENSORS] = {0};
// Averaged data samples
float dataAvgd[3][NUM_IMU_VALUES] = {0};

// OUTPUTS
// Integrated attitude quaternion
float q[4] = {1,0,0,0};
// Non-destruct delta thetas
float nddTheta[3] = {0,0,0};
// Non-destruct delta velocity
float nddV[3] = {0,0,0};
// Accumulated velocity
float accVel[3] = {0,0,0};
// Average temperature of the sensors
float tempAvg = 0;


struct CalibrationCoefficients {
	// Bias coefficients
	float b[6];
	// Scale factor coefficients
	float S[6];
	// Misalignment coefficients
	float M[6];
	// Temperature coefficients
	float T[6];
	// Gyro g-sensitivity coefficients
	float G[9];
	// Accelerometer inverted scale factor/misalignment matrix
	float A_ISM[9];
	// Gyro inverted scale factor/misalignment matrix
	float G_ISM[9];
};
struct CalibrationCoefficients cc[NUM_SENSORS];

// Keeps track of number of samples acquired for integration
uint32_t sampleCount = 0;
uint32_t outputCount = 0;

// Arrays used for encoding data
char rawData1[RAW_PKT_SIZE_1 - 2] = {0};
char rawData2[RAW_PKT_SIZE_2 - 2] = {0};
char calData[CAL_PKT_SIZE - 2] = {0};
char rawEncodedData[RAW_PKT_SIZE_1 + RAW_PKT_SIZE_2] = {0};
char calEncodedData[CAL_PKT_SIZE] = {0};

// *******************************************************************************
// SD
// *******************************************************************************


// SD card write buffer
char sdWriteBuff[SD_BUFFER_SIZE] = {0};
// Index to keep track of where we are within the write buffer
uint16_t sdWriteBuffIdx = 0;


// *******************************************************************************
// INTERRUPT HANDLERS
// *******************************************************************************


// Interrupt handling for acquiring data from IMUs
void AcquireDataIntHandler(void) {

	// Clear the timer interrupt.
    TimerIntClear(DATA_ACQ_TIMER_BASE, TIMER_TIMA_TIMEOUT);

    // If currently processing an I2C command, don't get new data
    GetIMUData(++tickCount);
}

// This is the handler for this SysTick interrupt.  FatFs requires a timer tick
// every 10 ms for internal timing purposes.
void SysTickHandler(void) {

    // Call the FatFs tick timer.
    disk_timerproc();
}


// *******************************************************************************
// INITIALIZATION
// *******************************************************************************

void ConfigurePeripherals(void) {

    // Initialize system clock
    systemClock = SysCtlClockFreqSet((SYSCTL_OSC_INT | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), MCU_CLK_SPEED);

	// Enable floating point
	ROM_FPUEnable();
    // Enable lazy stacking for interrupt handlers. This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
	ROM_FPULazyStackingEnable();

	// ****************************************************
    // Enable peripherals
	// ****************************************************

#ifdef DEBUG_MODE
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);	// UART
#endif
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);     // I2C
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI1);     // SD card SPI bus
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI2);     // IMU SPI bus
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);   // TIMER0 (Data acquisition)

	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);    // GPIO Banks
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOP);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ);

	// Unlock Pin D7 for use as GPIO
	HWREG(GPIO_PORTD_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
	HWREG(GPIO_PORTD_BASE + GPIO_O_CR) |= GPIO_PIN_7;

	// ****************************************************
    // Configure GPIO's
	// ****************************************************

#ifdef DEBUG_MODE
    // Configure GPIO Pins for UART mode.
	ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
	ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
#endif

    // Configure I2C pins
	ROM_GPIOPinConfigure(GPIO_PB2_I2C0SCL);
	ROM_GPIOPinConfigure(GPIO_PB3_I2C0SDA);

    // Configure SD SPI pins
	ROM_GPIOPinConfigure(GPIO_PE5_SSI1XDAT1);
	ROM_GPIOPinConfigure(GPIO_PE4_SSI1XDAT0);
	ROM_GPIOPinConfigure(GPIO_PB4_SSI1FSS);
	ROM_GPIOPinConfigure(GPIO_PB5_SSI1CLK);

    // Configure IMU SPI pins
	ROM_GPIOPinConfigure(GPIO_PD3_SSI2CLK);
	ROM_GPIOPinConfigure(GPIO_PD1_SSI2XDAT0);
	ROM_GPIOPinConfigure(GPIO_PD0_SSI2XDAT1);

	// ****************************************************
    // Set pin types
	// ****************************************************

#ifdef DEBUG_MODE
	// UART
	ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
#endif

	// I2C
	ROM_GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2);
    ROM_GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3);

    // SSI1 - SD card
    ROM_GPIOPinTypeSSI(SD_SPI_CLK_PORT_BASE, SD_SPI_CLK);
    ROM_GPIOPinTypeSSI(SD_SPI_RX_PORT_BASE, SD_SPI_RX);
    ROM_GPIOPinTypeSSI(SD_SPI_TX_PORT_BASE, SD_SPI_TX);
    ROM_GPIOPinTypeGPIOOutput(SD_SPI_FSS_PORT_BASE,  SD_SPI_FSS);

    // SSI2 - IMU
    ROM_GPIOPinTypeSSI(IMU_SPI_PORT_BASE, IMU_SPI_RX | IMU_SPI_TX | IMU_SPI_CLK);
    ROM_GPIOPinTypeGPIOOutput(IMU_SPI_PORT_BASE, IMU_SPI_FSS);
    ROM_GPIOPinWrite(IMU_SPI_PORT_BASE, IMU_SPI_FSS, IMU_SPI_FSS);

    // Configure IMU chip select pins to be outputs
    uint8_t i;
    for(i = 0 ; i < NUM_SENSORS; i++) {
        // Configure GPIO as output
        ROM_GPIOPinTypeGPIOOutput(IMU_PORT_BASE[i], IMU_PIN[i]);
        // Write to high
        ROM_GPIOPinWrite(IMU_PORT_BASE[i], IMU_PIN[i], IMU_PIN[i]);
    }

#ifdef DEBUG_MODE
    // Initialize the UART for console I/O.
    UARTStdioConfig(0, 230400, systemClock);

    UARTprintf("\n\nIMU CLUSTER GENERATION 3\n");
    UARTprintf("========================================\n");
    UARTprintf("Initializing\n");
    UARTprintf("\tConfiguring UART... done\n");

	UARTprintf("\tConfiguring I2C... ");
#endif

	// ****************************************************
    // Configure I2C as slave device
	// ****************************************************

    // Set the slave address to SLAVE_ADDRESS.  In loopback mode, it's an
    // arbitrary 7-bit number (set in a macro above) that is sent to the
    // I2CMasterSlaveAddrSet function.
    I2CSlaveInit(CDH_I2C_BASE, SLAVE_ADDRESS);

    // Enable the I2C0 interrupt on the processor (NVIC).
    ROM_IntEnable(INT_I2C0);

    // Configure and turn on the I2C0 slave interrupt.  The I2CSlaveIntEnableEx()
    // gives you the ability to only enable specific interrupts.  For this case
    // we are only interrupting when the slave device receives data.
    I2CSlaveIntEnableEx(CDH_I2C_BASE, I2C_SLAVE_INT_DATA | I2C_SLAVE_INT_START | I2C_SLAVE_INT_STOP);

    // Enable the I2C0 slave module.
    I2CSlaveEnable(CDH_I2C_BASE);

#ifdef DEBUG_MODE
	UARTprintf("done\n");
	UARTprintf("\tConfiguring SPI... ");
#endif

	// ****************************************************
    // Configure SD card SPI
	// ****************************************************

 	// Set the SSI output pins to 4MA drive strength and engage the
	// pull-up on the receive line.
	MAP_GPIOPadConfigSet(SD_SPI_RX_PORT_BASE, SD_SPI_RX, // RX
	                     GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD_WPU);
	MAP_GPIOPadConfigSet(SD_SPI_TX_PORT_BASE, SD_SPI_TX, // TX
	                     GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD);
	MAP_GPIOPadConfigSet(SD_SPI_FSS_PORT_BASE, SD_SPI_FSS, // FSS
	                     GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD);
	MAP_GPIOPadConfigSet(SD_SPI_CLK_PORT_BASE, SD_SPI_CLK, // SCLK
	                     GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD);

	// Configure clock for SD card SPI
	ROM_SSIConfigSetExpClk(SD_SPI_BASE, systemClock, SSI_FRF_MOTO_MODE_0,
			SSI_MODE_MASTER, 400000, 8);

	// Enable the SSI modules
	ROM_SSIEnable(SD_SPI_BASE);


	// ****************************************************
    // Configure IMU SPI
	// ****************************************************

 	// Set the SSI output pins to 8MA drive strength. Need
	// extra powa' due to the fanout of the sensors
	MAP_GPIOPadConfigSet(IMU_SPI_PORT_BASE, IMU_SPI_RX, // RX
	                     GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD);
	MAP_GPIOPadConfigSet(IMU_SPI_PORT_BASE, IMU_SPI_TX, // TX
	                     GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD);
	MAP_GPIOPadConfigSet(IMU_SPI_PORT_BASE, IMU_SPI_FSS, // FSS
	                     GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD);
	MAP_GPIOPadConfigSet(IMU_SPI_PORT_BASE, IMU_SPI_CLK, // SCLK
	                     GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD);

	// Configure clock for IMU SPI
	ROM_SSIConfigSetExpClk(IMU_SPI_BASE, systemClock, SSI_FRF_MOTO_MODE_0,
			SSI_MODE_MASTER, IMU_SPI_CLK_SPEED, 8);

	// Enable the SSI modules
	ROM_SSIEnable(IMU_SPI_BASE);

#ifdef DEBUG_MODE
	UARTprintf("done\n");
#endif
}

void ConfigureTimers(void) {

#ifdef DEBUG_MODE
	UARTprintf("\tConfiguring data acquisition timer... ");
#endif

	// Configure the 32-bit periodic timer
	ROM_TimerConfigure(DATA_ACQ_TIMER_BASE, TIMER_CFG_PERIODIC);
	ROM_TimerLoadSet(DATA_ACQ_TIMER_BASE, TIMER_A, systemClock / SAMPLE_RATE);
	// Setup the interrupts for the timer timeouts
	ROM_IntEnable(INT_TIMER0A);
	ROM_TimerIntEnable(DATA_ACQ_TIMER_BASE, TIMER_TIMA_TIMEOUT);

#ifdef DEBUG_MODE
	UARTprintf("done\n");
    // Configure SysTick for a 100Hz interrupt (required by FAT filesystem)
	UARTprintf("\tConfiguring FATFS SysTick... ");
#endif

    ROM_SysTickPeriodSet(systemClock / 100);
    ROM_SysTickEnable();
    ROM_SysTickIntEnable();

#ifdef DEBUG_MODE
	UARTprintf("done\n");
#endif

}

void InitializeCalibrationCoefficients() {

	// TODO: Load calibration coefficients from SD card

#ifdef DEBUG_MODE
	UARTprintf("\tComputing inverse of misalignment & scale-factor matrices... ");
#endif

	// ***************************************************************************************
	// Pre-compute the inverse of the misalignment-scale factor matrix used in the calibration
	// ***************************************************************************************

    int i;
    float a_den = 0;

    // Accelerometer
    for(i = 0; i < NUM_SENSORS; i++) {
        // Calculate the denominator
        a_den = (cc[i].S[0] + cc[i].S[1] + cc[i].S[2] + cc[i].S[0]*cc[i].S[1] + cc[i].S[0]*cc[i].S[2] + cc[i].S[1]*cc[i].S[2] +
            cc[i].M[1]*cc[i].M[1]*cc[i].S[1] + cc[i].M[2]*cc[i].M[2]*cc[i].S[0] + cc[i].M[0]*cc[i].M[0]*cc[i].S[2] + cc[i].M[0]*cc[i].M[0] +
            cc[i].M[1]*cc[i].M[1] + cc[i].M[2]*cc[i].M[2] + cc[i].S[0]*cc[i].S[1]*cc[i].S[2] + 1);

        cc[i].A_ISM[0] =  (cc[i].M[2]*cc[i].M[2] + cc[i].S[1] + cc[i].S[2] + cc[i].S[1]*cc[i].S[2] + 1) / a_den;
        cc[i].A_ISM[1] = -(cc[i].M[0] + cc[i].M[0]*cc[i].S[2] + cc[i].M[1]*cc[i].M[2]) / a_den;
        cc[i].A_ISM[2] = -(cc[i].M[1] + cc[i].M[1]*cc[i].S[1] - cc[i].M[0]*cc[i].M[2]) / a_den;
        cc[i].A_ISM[3] =  (cc[i].M[0] + cc[i].M[0]*cc[i].S[2] - cc[i].M[1]*cc[i].M[2]) / a_den;
        cc[i].A_ISM[4] =  (cc[i].M[1]*cc[i].M[1] + cc[i].S[0] + cc[i].S[2] + cc[i].S[0]*cc[i].S[2] + 1) / a_den;
        cc[i].A_ISM[5] = -(cc[i].M[2] + cc[i].M[2]*cc[i].S[0] + cc[i].M[0]*cc[i].M[1]) / a_den;
        cc[i].A_ISM[6] =  (cc[i].M[1] + cc[i].M[1]*cc[i].S[1] + cc[i].M[0]*cc[i].M[2]) / a_den;
        cc[i].A_ISM[7] =  (cc[i].M[2] + cc[i].M[2]*cc[i].S[0] - cc[i].M[0]*cc[i].M[1]) / a_den;
        cc[i].A_ISM[8] =  (cc[i].M[0]*cc[i].M[0] + cc[i].S[0] + cc[i].S[1] + cc[i].S[0]*cc[i].S[1] + 1) / a_den;
    }

    // GyroScope
    float g_den = 0;
    for(i = 0; i < NUM_SENSORS; i++) {
        // Calculate the denominator
        g_den = (cc[i].S[3] + cc[i].S[4] + cc[i].S[5] + cc[i].S[3]*cc[i].S[4] + cc[i].S[3]*cc[i].S[5] + cc[i].S[4]*cc[i].S[5] +
            cc[i].M[4]*cc[i].M[4]*cc[i].S[4] + cc[i].M[5]*cc[i].M[5]*cc[i].S[3] + cc[i].M[3]*cc[i].M[3]*cc[i].S[5] + cc[i].M[3]*cc[i].M[3] +
            cc[i].M[4]*cc[i].M[4] + cc[i].M[5]*cc[i].M[5] + cc[i].S[3]*cc[i].S[4]*cc[i].S[5] + 1);

        cc[i].G_ISM[0] =  (cc[i].M[5]*cc[i].M[5] + cc[i].S[4] + cc[i].S[5] + cc[i].S[4]*cc[i].S[5] + 1) / g_den;
        cc[i].G_ISM[1] = -(cc[i].M[3] + cc[i].M[3]*cc[i].S[5] + cc[i].M[4]*cc[i].M[5]) / g_den;
        cc[i].G_ISM[2] = -(cc[i].M[4] + cc[i].M[4]*cc[i].S[4] - cc[i].M[3]*cc[i].M[5]) / g_den;
        cc[i].G_ISM[3] =  (cc[i].M[3] + cc[i].M[3]*cc[i].S[5] - cc[i].M[4]*cc[i].M[5]) / g_den;
        cc[i].G_ISM[4] =  (cc[i].M[4]*cc[i].M[4] + cc[i].S[3] + cc[i].S[5] + cc[i].S[3]*cc[i].S[5] + 1) / g_den;
        cc[i].G_ISM[5] = -(cc[i].M[5] + cc[i].M[5]*cc[i].S[3] + cc[i].M[3]*cc[i].M[4]) / g_den;
        cc[i].G_ISM[6] =  (cc[i].M[4] + cc[i].M[4]*cc[i].S[4] + cc[i].M[3]*cc[i].M[5]) / g_den;
        cc[i].G_ISM[7] =  (cc[i].M[5] + cc[i].M[5]*cc[i].S[3] - cc[i].M[3]*cc[i].M[4]) / g_den;
        cc[i].G_ISM[8] =  (cc[i].M[3]*cc[i].M[3] + cc[i].S[3] + cc[i].S[4] + cc[i].S[3]*cc[i].S[4] + 1) / g_den;
    }

#ifdef DEBUG_MODE
	UARTprintf("done\n");
#endif
}


// *******************************************************************************
// DATA ACQUISITION
// *******************************************************************************


// Collect data from all of the sensors
void GetIMUData(uint32_t timeStamp) {

	uint8_t i,j = 0;
	uint32_t rx_data = 0;

	// Collect data from each of the sensors
	for(i = 0; i < NUM_SENSORS; i++) {
		// Check to see if this sensor is enabled
		if(IsIMUEnabled(i)) {
			// Select IMU by pulling CS low
		    GPIOPinWrite(IMU_PORT_BASE[i], IMU_PIN[i], 0);
			// Begin burst read at accelerometer register
			SPIBurstReadStart(ACCEL_XOUT_H, i);

			// Store tick count
			queue[writeIdx].timeStamp = timeStamp;

			for(j = 0; j < 7; j++) {
				// Read next register from IMU
				rx_data = SPIBurstReadShort();

				// Since sensors are not all in the same orientation, need to apply
				// appropriate transformation based on the sensor number
				if(i < 8) {			// Board X -> Sensor -X, Board Y -> Sensor -Y
					switch(j) {
						case 0: queue[writeIdx].sensor[i].data[AX] = -rx_data; break;
						case 1: queue[writeIdx].sensor[i].data[AY] = -rx_data; break;
						case 2: queue[writeIdx].sensor[i].data[AZ] = rx_data; break;
						case 3: queue[writeIdx].sensor[i].data[TEMP] = rx_data; break;
						case 4: queue[writeIdx].sensor[i].data[GX] = -rx_data; break;
						case 5: queue[writeIdx].sensor[i].data[GY] = -rx_data; break;
						default: queue[writeIdx].sensor[i].data[GZ] = rx_data; break;
					}
				}
				else if(i < 16) {	// Board X -> Sensor -Y, Board Y -> Sensor X
					switch(j) {
						case 0: queue[writeIdx].sensor[i].data[AY] = rx_data; break;
						case 1: queue[writeIdx].sensor[i].data[AX] = -rx_data; break;
						case 2: queue[writeIdx].sensor[i].data[AZ] = rx_data; break;
						case 3: queue[writeIdx].sensor[i].data[TEMP] = rx_data; break;
						case 4: queue[writeIdx].sensor[i].data[GY] = rx_data; break;
						case 5: queue[writeIdx].sensor[i].data[GX] = -rx_data; break;
						default: queue[writeIdx].sensor[i].data[GZ] = rx_data; break;
					}
				}
				else if(i < 24) {	// Board X -> Sensor X, Board Y -> Sensor Y
					switch(j) {
						case 0: queue[writeIdx].sensor[i].data[AX] = rx_data; break;
						case 1: queue[writeIdx].sensor[i].data[AY] = rx_data; break;
						case 2: queue[writeIdx].sensor[i].data[AZ] = rx_data; break;
						case 3: queue[writeIdx].sensor[i].data[TEMP] = rx_data; break;
						case 4: queue[writeIdx].sensor[i].data[GX] = rx_data; break;
						case 5: queue[writeIdx].sensor[i].data[GY] = rx_data; break;
						default: queue[writeIdx].sensor[i].data[GZ] = rx_data; break;
					}
				}
				else {				// Board X -> Sensor Y, Board Y -> Sensor -X
					switch(j) {
						case 0: queue[writeIdx].sensor[i].data[AY] = -rx_data; break;
						case 1: queue[writeIdx].sensor[i].data[AX] = rx_data; break;
						case 2: queue[writeIdx].sensor[i].data[AZ] = rx_data; break;
						case 3: queue[writeIdx].sensor[i].data[TEMP] = rx_data; break;
						case 4: queue[writeIdx].sensor[i].data[GY] = -rx_data; break;
						case 5: queue[writeIdx].sensor[i].data[GX] = rx_data; break;
						default: queue[writeIdx].sensor[i].data[GZ] = rx_data; break;
					}
				}
			}

			// Deselect IMU by pulling CS high.
		    GPIOPinWrite(IMU_PORT_BASE[i], IMU_PIN[i], IMU_PIN[i]);
		}
	}

	// Increment queue write index. If it exceeds size of queue, wrap around
	if(++writeIdx >= QUEUE_SIZE) {
		writeIdx = 0;
	}

	// Increment record count
	queueRecords++;
}

void CalibrateData(uint16_t k) {

	uint8_t i = 0;

	// Temporary array to store intermediate calculations
	float tmp[6] = {0};

	// Calibrate each sensor individually
	for(i = 0; i < NUM_SENSORS; i++) {
		// Check to see if this sensor is enabled
		if(IsIMUEnabled(i)) {
			// Calibrate temperature
			tempCal[i] = (queue[k].sensor[i].data[TEMP] / 326.8) + 25;
			// Calculate temperature deviation from 25 deg C
			float dT = tempCal[i] - 25;

			// Calculate true specific force
			// a_true = (K_a)*a-meas - bias - temp bias
			tmp[AX] = (K_A)*queue[k].sensor[i].data[AX] - cc[i].b[AX] - cc[i].T[AX]*dT;
			tmp[AY] = (K_A)*queue[k].sensor[i].data[AY] - cc[i].b[AY] - cc[i].T[AY]*dT;
			tmp[AZ] = (K_A)*queue[k].sensor[i].data[AZ] - cc[i].b[AZ] - cc[i].T[AZ]*dT;

			// Multiply by inverse of accelerometer misalignment/scale factor matrix
			dataCal[i][AX] = cc[i].A_ISM[0]*tmp[AX] + cc[i].A_ISM[1]*tmp[AY] + cc[i].A_ISM[2]*tmp[AZ];
			dataCal[i][AY] = cc[i].A_ISM[3]*tmp[AX] + cc[i].A_ISM[4]*tmp[AY] + cc[i].A_ISM[5]*tmp[AZ];
			dataCal[i][AZ] = cc[i].A_ISM[6]*tmp[AX] + cc[i].A_ISM[7]*tmp[AY] + cc[i].A_ISM[8]*tmp[AZ];

			// Calculate true angular rate
			// w_true = (K_g)*w_meas - bias - temp bias - g-sensitivity
			tmp[GX] = (K_G)*queue[k].sensor[i].data[GX] - cc[i].b[GX] - cc[i].T[GX]*dT - cc[i].G[0]*dataCal[i][AX] - cc[i].G[1]*dataCal[i][AY] - cc[i].G[2]*dataCal[i][AZ];
			tmp[GY] = (K_G)*queue[k].sensor[i].data[GY] - cc[i].b[GY] - cc[i].T[GY]*dT - cc[i].G[3]*dataCal[i][AX] - cc[i].G[4]*dataCal[i][AY] - cc[i].G[5]*dataCal[i][AZ];
			tmp[GZ] = (K_G)*queue[k].sensor[i].data[GZ] - cc[i].b[GZ] - cc[i].T[GZ]*dT - cc[i].G[6]*dataCal[i][AX] - cc[i].G[7]*dataCal[i][AY] - cc[i].G[8]*dataCal[i][AZ];

			// Multiply by inverse of gyroscope misalignment/scale factor matrix
			dataCal[i][GX] = cc[i].G_ISM[0]*tmp[GX] + cc[i].G_ISM[1]*tmp[GY] + cc[i].G_ISM[2]*tmp[GZ];
			dataCal[i][GY] = cc[i].G_ISM[3]*tmp[GX] + cc[i].G_ISM[4]*tmp[GY] + cc[i].G_ISM[5]*tmp[GZ];
			dataCal[i][GZ] = cc[i].G_ISM[6]*tmp[GX] + cc[i].G_ISM[7]*tmp[GY] + cc[i].G_ISM[8]*tmp[GZ];
		}
	}
}

void AverageData() {

	uint8_t i, j = 0;

	// Average over all GX, GY, GZ, AX, AY, AZ
	for(i = 0; i < 6; i++) {
		dataAvgd[sampleCount][i] = 0;
		tempAvg = 0;
		uint8_t count = 0;

		// Average data across all sensors
		for(j = 0; j < NUM_SENSORS; j++) {
			// Check to see if this sensor is enabled
			if(IsIMUEnabled(j)) {
				dataAvgd[sampleCount][i] += dataCal[j][i];
				tempAvg += tempCal[j];
				count++;
			}
		}

		dataAvgd[sampleCount][i] /= (float)count;
		tempAvg /= (float)count;
	}

	// Convert degrees to radians
	dataAvgd[sampleCount][GX] *= DEG_TO_RAD;
	dataAvgd[sampleCount][GY] *= DEG_TO_RAD;
	dataAvgd[sampleCount][GZ] *= DEG_TO_RAD;
	// Convert from g's to m/s^2
	dataAvgd[sampleCount][AX] *= GRAVITY;
	dataAvgd[sampleCount][AY] *= GRAVITY;
	dataAvgd[sampleCount][AZ] *= GRAVITY;

	sampleCount++;
	outputCount++;
}

void IntegrateGyroData() {
	float sigma[3] = {0};	// Integrated gyro
	float q1[4] = {0};		// Quaternion update
	float qProp[4] = {0};   // Propagated attitude quaternion

	// Integrate gyro output using Simpson's rule
	sigma[X] = (Ts/3)*(dataAvgd[2][GX] + 4*dataAvgd[1][GX] + dataAvgd[0][GX]);
	sigma[Y] = (Ts/3)*(dataAvgd[2][GY] + 4*dataAvgd[1][GY] + dataAvgd[0][GY]);
	sigma[Z] = (Ts/3)*(dataAvgd[2][GZ] + 4*dataAvgd[1][GZ] + dataAvgd[0][GZ]);
	// Add change in angle to non-destruct delta thetas
	nddTheta[X] += sigma[X];
	nddTheta[Y] += sigma[Y];
	nddTheta[Z] += sigma[Z];

	// Compute norm of sigma
	float phi_sq = sigma[X]*sigma[X] + sigma[Y]*sigma[Y] + sigma[Z]*sigma[Z];

	// Create quaternion which represents rotation of by angle phi
	if(phi_sq > 1E-12) {
	    // Approximate cosine by Taylor series up to 2nd order terms
		q1[0] = 1 - 0.125*phi_sq;

	    // Approximate sine by Taylor series up to 2nd order terms
		float a = 0.5 - phi_sq/48.0;
		q1[1] = sigma[X]*a;
		q1[2] = sigma[Y]*a;
		q1[3] = sigma[Z]*a;
	}

	// Propagate the attitude quaternion
	qProp[0] = q[0]*q1[0] - q[1]*q1[1] - q[2]*q1[2] - q[3]*q1[3];
	qProp[1] = q[0]*q1[1] + q[1]*q1[0] + q[2]*q1[3] - q[3]*q1[2];
	qProp[2] = q[0]*q1[2] - q[1]*q1[3] + q[2]*q1[0] + q[3]*q1[1];
	qProp[3] = q[0]*q1[3] + q[1]*q1[2] - q[2]*q1[1] + q[3]*q1[0];

	// Normalize the quaternion
	float qPropNorm = sqrt(qProp[0]*qProp[0] + qProp[1]*qProp[1] + qProp[2]*qProp[2] + qProp[3]*qProp[3]);
	q[0] = qProp[0]/qPropNorm;
	q[1] = qProp[1]/qPropNorm;
	q[2] = qProp[2]/qPropNorm;
	q[3] = qProp[3]/qPropNorm;

	// Store non-destruct delta thetas
	processedData.dTheta[X] = nddTheta[X];
	processedData.dTheta[Y] = nddTheta[Y];
	processedData.dTheta[Z] = nddTheta[Z];

	// Store attitude quaternion
	processedData.Q[X] = q[1];
	processedData.Q[Y] = q[2];
	processedData.Q[Z] = q[3];
	processedData.Q[W] = q[0];

	// Store latest angular velocity
	processedData.angVel[X] = dataAvgd[2][GX];
	processedData.angVel[Y] = dataAvgd[2][GY];
	processedData.angVel[Z] = dataAvgd[2][GZ];

	processedData.avgTemp = tempAvg;
}

void IntegrateAccelerometerData() {
	float dv[3] = {0};	// Integrated gyro

	// Integrate accelerometer output using Simpson's rule
	dv[X] = (Ts/3)*(dataAvgd[2][AX] + 4*dataAvgd[1][AX] + dataAvgd[0][AX]);
	dv[Y] = (Ts/3)*(dataAvgd[2][AY] + 4*dataAvgd[1][AY] + dataAvgd[0][AY]);
	dv[Z] = (Ts/3)*(dataAvgd[2][AZ] + 4*dataAvgd[1][AZ] + dataAvgd[0][AZ]);

	// Add change in velocity to non-destruct delta velocity
	nddV[X] += dv[X];
	nddV[Y] += dv[Y];
	nddV[Z] += dv[Z];

	// Store quaternion in processed data
	processedData.dV[X] = nddV[X];
	processedData.dV[Y] = nddV[Y];
	processedData.dV[Z] = nddV[Z];

	// Store accumulated velocity
	processedData.accumV[X] = dv[X];
	processedData.accumV[Y] = dv[Y];
	processedData.accumV[Z] = dv[Z];

	// Store latest specific force
	processedData.specificForce[X] = dataAvgd[2][AX];
	processedData.specificForce[Y] = dataAvgd[2][AY];
	processedData.specificForce[Z] = dataAvgd[2][AZ];
}

// *******************************************************************************
// DATA OUTPUT
// *******************************************************************************


void WriteDataToRegisters(uint32_t recordTimeStamp) {

	RegWriteFloat32(REG_DELTA_THETA_X_1, processedData.dTheta[X]);
	RegWriteFloat32(REG_DELTA_THETA_Y_1, processedData.dTheta[Y]);
	RegWriteFloat32(REG_DELTA_THETA_Z_1, processedData.dTheta[Z]);

	RegWriteFloat32(REG_DELTA_VEL_X_1, processedData.dV[X]);
	RegWriteFloat32(REG_DELTA_VEL_Y_1, processedData.dV[Y]);
	RegWriteFloat32(REG_DELTA_VEL_Z_1, processedData.dV[Z]);

	RegWriteFloat32(REG_IQUAT_X_1, processedData.Q[X]);
	RegWriteFloat32(REG_IQUAT_Y_1, processedData.Q[Y]);
	RegWriteFloat32(REG_IQUAT_Z_1, processedData.Q[Z]);
	RegWriteFloat32(REG_IQUAT_W_1, processedData.Q[W]);

	RegWriteFloat32(REG_TEMP_1, processedData.avgTemp);

	RegWriteFloat32(REG_ACC_VEL_X_1, processedData.accumV[X]);
	RegWriteFloat32(REG_ACC_VEL_Y_1, processedData.accumV[Y]);
	RegWriteFloat32(REG_ACC_VEL_Z_1, processedData.accumV[Z]);

	RegWriteUInt32(REG_TICK_1, recordTimeStamp);

	RegWriteFloat32(REG_ANG_VEL_X_1, processedData.angVel[X]);
	RegWriteFloat32(REG_ANG_VEL_Y_1, processedData.angVel[Y]);
	RegWriteFloat32(REG_ANG_VEL_Z_1, processedData.angVel[Z]);

	RegWriteFloat32(REG_SP_FORCE_X_1, processedData.specificForce[X]);
	RegWriteFloat32(REG_SP_FORCE_Y_1, processedData.specificForce[Y]);
	RegWriteFloat32(REG_SP_FORCE_Z_1, processedData.specificForce[Z]);
}

void WriteRawDataToSDCard(uint16_t k) {

	// COBS can only encode a maximum of 254 bytes. Since the data is 480 bytes total, need to run COBS
	// on different sets of data. Data is split up so tick count & first 17 sensors are in packet 1,
	// the other 15 sensors are in packet 2.
	COBSStuffData((char*)(&queue[k].timeStamp), &rawEncodedData[0], RAW_PKT_SIZE_1);
	COBSStuffData((char*)(&queue[k].sensor[17]), &rawEncodedData[RAW_PKT_SIZE_1], RAW_PKT_SIZE_2);

	uint16_t i = 0;
	for(i = 0; i < RAW_PKT_SIZE_1 + RAW_PKT_SIZE_2; i++) {
		// If SD buffer is full, write the buffer to the SD card
		if(sdWriteBuffIdx >= SD_BUFFER_SIZE) {
			// Write data buffer to SD card
			SDWrite(sdWriteBuff, SD_BUFFER_SIZE);
			// Reset write buffer index
			sdWriteBuffIdx = 0;
		}
		sdWriteBuff[sdWriteBuffIdx++] = rawEncodedData[i];
	}
}

void WriteCalibratedDataToSDCard() {

	// Encode the calibrated data
	COBSStuffData((char*)(&processedData.dTheta[X]), &calEncodedData[0], CAL_PKT_SIZE);

	uint16_t i = 0;
	for(i = 0; i < sizeof(calEncodedData); i++) {
		// If SD buffer is full, write the buffer to the SD card
		if(sdWriteBuffIdx >= SD_BUFFER_SIZE) {
			// Write data buffer to SD card
			SDWrite(sdWriteBuff, SD_BUFFER_SIZE);
			// Reset write buffer index
			sdWriteBuffIdx = 0;
		}
		sdWriteBuff[sdWriteBuffIdx++] = calEncodedData[i];
	}
}

void ProcessDataRecord(uint16_t k) {

	// Get the timestamp for this data record
	uint32_t recordTimeStamp = queue[k].timeStamp;

	// Calibrate the data record
	CalibrateData(k);
	// Averaged the calibrated data
 	AverageData();

	// ... otherwise we are in experimental modes, write the data to the SD card
	if(GetIMUMode() == MODE_SD_WRITE) {
		WriteRawDataToSDCard(k);
	}

	// Need three samples to do integration
	if(sampleCount == 3) {
		sampleCount = 1;

		IntegrateGyroData();
		IntegrateAccelerometerData();

		// Place last sample in front now
		uint8_t i = 0;
		for(i = 0; i < 6; i++) {
			dataAvgd[0][i] = dataAvgd[2][i];
		}
	}

	// Output if ready
	if(outputCount == GetOutputRateDivider()) {
		outputCount = 0;

		// Write data to I2C registers if in nominal mode
		if(GetIMUMode() == MODE_STREAMING) {
			WriteDataToRegisters(recordTimeStamp);
		}
		// Output calibrated data to SD card
		else if(GetIMUMode() == MODE_SD_WRITE) {
			WriteCalibratedDataToSDCard();
		}
	}
}


// *******************************************************************************
// IMU SETTINGS
// *******************************************************************************


void EnableDAQ(bool enable) {
	if(enable) {
		// Reset tick counter and queue
		tickCount = 0;
		readIdx = 0;
		writeIdx = 0;
		queueRecords = 0;

		// Power up the IMUs
		uint8_t i = 0;
		for(i = 0; i < NUM_SENSORS; i++) {
			if(IsIMUEnabled(i)) {
				PowerUpIMU(i);
			}
		}

		// Enable the DAQ timer interrupt
		ROM_TimerEnable(DATA_ACQ_TIMER_BASE, TIMER_A);
#ifdef DEBUG_MODE
		UARTprintf("Data acquisition ENABLED\n");
#endif
	}
	else {
		// Disable the DAQ timer interrupt
		ROM_TimerDisable(DATA_ACQ_TIMER_BASE, TIMER_A);

		// Power down the IMUs
		uint8_t i = 0;
		for(i = 0; i < NUM_SENSORS; i++) {
			if(IsIMUEnabled(i)) {
				PowerDownIMU(i);
			}
		}
#ifdef DEBUG_MODE
		UARTprintf("Data acquisition DISABLED\n");
#endif
	}
}

void UpdateIMUSettings(void) {

	// Disable interrupts while updating IMU settings
	ROM_IntMasterDisable();

#ifdef DEBUG_MODE
	UARTprintf("\n--- Updating IMU settings -------------------------\n");
#endif

	// Disable data acquisition before updating the settings
	EnableDAQ(false);

	// Registers updated with data
	if(GetIMUMode() == MODE_STREAMING) {
#ifdef DEBUG_MODE
		UARTprintf("Entering I2C mode\n");
#endif
		// Close
		SDCloseFile();
		EnableDAQ(IsDAQEnabled());
	}
	// Writing data to the SD card
	else if(GetIMUMode() == MODE_SD_WRITE) {
#ifdef DEBUG_MODE
		UARTprintf("Entering SD write mode\n");
#endif
		SDFileOpenWrite();
		EnableDAQ(IsDAQEnabled());
	}
	// Reading data from SD card and sending over I2C
	else if(GetIMUMode() == MODE_SD_READ) {
#ifdef DEBUG_MODE
		UARTprintf("Entering SD read mode\n");
#endif
		// Force data acquisition off
		CLEAR_BIT(reg[REG_IMU_DAQ], 0);
		// Clear SD status flags
		ClearSDReadyFlag();
		ClearSDEOFFlag();

		// Open SD card file for reading
		SDFileOpenRead();
	}

	// Re-enable interrupts, resume normal operations
	ROM_IntMasterEnable();
}


// *******************************************************************************
// MAIN LOOP
// *******************************************************************************


int main(void) {
	InitializeRegisters();
	ConfigurePeripherals();
    ConfigureIMUs(systemClock);
	ConfigureTimers();

	InitializeCalibrationCoefficients();

	// Mount the file system for the SD card
	SDMount();

	// Run this to finish initialization
	UpdateIMUSettings();

	char sdReadBuff[SD_DATA_REG_COUNT] = {0};
	uint32_t bytesReturned = 0;
	uint8_t i = 0;

	while(1) {
		// If I2C update flag is raised and I2C module is currently idle, update IMU settings
		if(IsRegisterUpdateFlagRaised() && (GetI2CState() == I2C_STATE_IDLE)) {
			UpdateIMUSettings();
			ClearRegisterUpdateFlag();
		}

		// 1) SD Read mode
		// 2) I2C interrupt routine has requested that main loop update the SD data register
		if((GetIMUMode() == MODE_SD_READ) && IsSDDataRequested()) {

			// Read a byte from the SD card
			SDRead(sdReadBuff, SD_DATA_REG_COUNT, &bytesReturned);

			// Copy data to registers
			for(i = 0; i < sizeof(sdReadBuff); i++) {
				RegWriteUInt8(REG_SD_DATA + i, sdReadBuff[i]);
			}

			// If less bytes were returned than requested, raise end of file flag
			if(bytesReturned < SD_DATA_REG_COUNT) {
#ifdef DEBUG_MODE
				UARTprintf("\nEOF detected, raising flag\n");
#endif
				RaiseSDEOFFlag();
			}
#ifdef DEBUG_MODE
			UARTprintf(".");
#endif
			// Data loaded, clear the flag to notify the master
			ClearSDReadyFlag();
		}

		// If there are unprocessed records on the queue, get to work!!!
		if(queueRecords > 0) {
			ProcessDataRecord(readIdx);
			queueRecords--;

			// Wrap read index if we have reached the end of the buffer
			if(++readIdx >= QUEUE_SIZE) {
				readIdx = 0;
			}
		}
	}
}
