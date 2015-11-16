/*
 * sd.c
 *
 *  Description: Higher level functions for reading/writing files to an SD card.
 *
 *  Created on: Aug 31, 2015
 *      Author: Daniel Greenheck
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "ff.h"
#include "diskio.h"
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"

#include "registers.h"
#include "sd.h"
#include "util.h"

// *******************************************************************************
// SD CARD VARIABLES
// *******************************************************************************

// The FATFS file system object
FATFS fatFS;
// Reference to open file
FIL fil;
// If true, file is currently open
bool fileOpen = false;

// A structure that holds a mapping between an FRESULT numerical code, and a
// string representation.  FRESULT codes are returned from the FatFs FAT file
// system driver.
typedef struct
{
    FRESULT iFResult;
    char *pcResultStr;
}
tFResultString;

// A table that holds a mapping between the numerical FRESULT code and it's
// name as a string.  This is used for looking up error codes for printing to
// the console.

tFResultString fatFsResultStrings[] = {
    FRESULT_ENTRY(FR_OK),
    FRESULT_ENTRY(FR_DISK_ERR),
    FRESULT_ENTRY(FR_INT_ERR),
    FRESULT_ENTRY(FR_NOT_READY),
    FRESULT_ENTRY(FR_NO_FILE),
    FRESULT_ENTRY(FR_NO_PATH),
    FRESULT_ENTRY(FR_INVALID_NAME),
    FRESULT_ENTRY(FR_DENIED),
    FRESULT_ENTRY(FR_EXIST),
    FRESULT_ENTRY(FR_INVALID_OBJECT),
    FRESULT_ENTRY(FR_WRITE_PROTECTED),
    FRESULT_ENTRY(FR_INVALID_DRIVE),
    FRESULT_ENTRY(FR_NOT_ENABLED),
    FRESULT_ENTRY(FR_NO_FILESYSTEM),
    FRESULT_ENTRY(FR_MKFS_ABORTED),
    FRESULT_ENTRY(FR_TIMEOUT),
    FRESULT_ENTRY(FR_LOCKED),
    FRESULT_ENTRY(FR_NOT_ENOUGH_CORE),
    FRESULT_ENTRY(FR_TOO_MANY_OPEN_FILES),
    FRESULT_ENTRY(FR_INVALID_PARAMETER),
};


// This function returns a string representation of an error code
// that was returned from a function call to FatFs.  It can be used
// for printing human readable error messages.
const char * StringFromFResult(FRESULT iFResult) {
    uint32_t ui32Idx;

    // Enter a loop to search the error code table for a matching error code.
    for(ui32Idx = 0; ui32Idx < NUM_FRESULT_CODES; ui32Idx++) {
        // If a match is found, then return the string name of the error code.
        if(fatFsResultStrings[ui32Idx].iFResult == iFResult) {
            return(fatFsResultStrings[ui32Idx].pcResultStr);
        }
    }

    // At this point no matching code was found, so return a string indicating
    // an unknown error.
    return("UNKNOWN ERROR CODE");
}

// Returns true if a file is currently open
bool IsFileOpen(void) {
	return fileOpen;
}

// Mount the SD card
int SDMount(void) {
    FRESULT iFResult = FR_OK;

    // Mount the file system, using logical disk 0.
    iFResult = f_mount(0, &fatFS);

    // Check return status to see if mount was successful
    if(iFResult != FR_OK) {
        return 1;
    }
    else {
        return 0;
    }
}


// Unount the SD card
int SDUnmount(void) {
    FRESULT iFResult = FR_OK;

	// Unmount the SD card
    iFResult = f_mount(0, 0);

    // Check return status to see if unmount was successful
    if(iFResult != FR_OK) {
        return 1;
    }
    else {
        return 0;
    }
}


// Open/create file on SD card with write permissions
int SDFileOpenWrite() {
    FRESULT iFResult = FR_OK;

    // If a file is already open, close it.
    if(fileOpen) {
    	SDCloseFile();
    }

	// If file overwrite is true, overwrite the SD card file
	if(GetSDFileOverwrite()) {
		iFResult = f_open(&fil, "data.txt", FA_CREATE_ALWAYS | FA_WRITE);
	}
	// Otherwise open and append
	else {
		iFResult = f_open(&fil, "data.txt", FA_OPEN_ALWAYS | FA_WRITE);
	}

	// Check return status to see if opened successfully
	if(iFResult != FR_OK) {
#ifdef DEBUG_MODE
		UARTprintf("\tDATA.TXT failed to open for writing\n");
#endif
		return 1;
	}
	else {
#ifdef DEBUG_MODE
		UARTprintf("\tDATA.TXT opened for writing\n");
#endif
		fileOpen = true;
		return 0;
	}
}

// Open up file on SD card with read permissions
int SDFileOpenRead() {
    FRESULT iFResult = FR_OK;

    // If a file is already open, close it.
    if(fileOpen) {
    	SDCloseFile();
    }

	iFResult = f_open(&fil, "data.txt", FA_OPEN_EXISTING | FA_READ);

	// Check return status to see if opened successfully
	if(iFResult != FR_OK) {
#ifdef DEBUG_MODE
		UARTprintf("\tDATA.TXT failed to open for reading\n");
#endif
		return 1;
	}
	else {
#ifdef DEBUG_MODE
		UARTprintf("\tDATA.TXT opened for reading\n");
#endif
		fileOpen = true;
		return 0;
	}
}

// Close the file object
int SDCloseFile() {
    FRESULT iFResult = FR_OK;

	// Close the SD card file (if one is open)
    if(fileOpen) {
    	iFResult = f_close(&fil);
    }

	// Check return status to see if closed successfully
	if(iFResult != FR_OK) {
#ifdef DEBUG_MODE
		UARTprintf("\tDATA.TXT failed to close\n");
#endif
		return 1;
	}
	else {
#ifdef DEBUG_MODE
		UARTprintf("\tDATA.TXT closed\n");
#endif
		fileOpen = false;
		return 0;
	}
}

// Write data to the SD card
int SDWrite(char *data, uint32_t bytesToWrite) {
    FRESULT iFResult = FR_OK;
    uint32_t count = 0;

    // Write the buffer to the SD card (if file is open)
    if(fileOpen) {
    	iFResult = f_write(&fil, data, bytesToWrite, &count);
    }

	// Check return status to see if data successfully written
	if(iFResult != FR_OK) {
		return 1;
	}
	else {
		return 0;
	}
}

// Write data to the SD card
int SDRead(char *buff, uint16_t bytesToRead, uint32_t *count) {
    FRESULT iFResult = FR_OK;

    // Write the buffer to the SD card (if file is open)
    if(fileOpen) {
    	iFResult = f_read(&fil, buff, bytesToRead, count);
    }

	// Check return status to see if data successfully written
	if(iFResult != FR_OK) {
		return 1;
	}
	else {
		return 0;
	}
}


