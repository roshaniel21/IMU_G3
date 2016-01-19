/*
 * sd.h
 *
 *  Created on: Aug 31, 2015
 *      Author: Daniel Greenheck
 */

#ifndef SD_H_
#define SD_H_

#define SD_SPI_BASE             SSI1_BASE
#define SD_SPI_RX_PORT_BASE		GPIO_PORTE_BASE
#define SD_SPI_RX              	GPIO_PIN_5
#define SD_SPI_TX_PORT_BASE		GPIO_PORTE_BASE
#define SD_SPI_TX              	GPIO_PIN_4
#define SD_SPI_FSS_PORT_BASE	GPIO_PORTB_BASE
#define SD_SPI_FSS             	GPIO_PIN_4
#define SD_SPI_CLK_PORT_BASE	GPIO_PORTB_BASE
#define SD_SPI_CLK             	GPIO_PIN_5

// A macro to make it easy to add result codes to the table.
#define FRESULT_ENTRY(f)        { (f), (#f) }
// A macro that holds the number of result codes.
#define NUM_FRESULT_CODES       (sizeof(fatFsResultStrings) / sizeof(tFResultString))

// Returns true if the file is currently open
bool IsFileOpen(void);

// Mount the SD card
int SDMount(void);

// Unmount the SD card
int SDUnmount(void);

// Create file object with write permissions
int SDFileOpenWrite();

// Create file object with read permissions
int SDFileOpenRead();

// Close the file object
int SDCloseFile();

// Write data to the SD card
int SDWrite(char *data, uint32_t bytesToWrite);

// Read data from the SD card
int SDRead(char *buff, uint16_t bytesToRead, uint32_t *count);


#endif /* SPI_H_ */

