/*
 * util.h
 *
 * Utility functions
 *
 *  Created on: Nov 10, 2015
 *      Author: Daniel Greenheck
 */

#ifndef UTIL_H_
#define UTIL_H_

// Comment out to disable debug mode (i.e. no output to UART)
#define DEBUG_MODE

// Returns 1 if the bit at 'pos' is 1. Otherwise, returns 0
#define CHECK_BIT(var,pos) (var & (1 << pos))
// Sets the bit at 'pos' to 1
#define SET_BIT(var,pos) (var |= (1 << pos))
// Clears the bit at 'pos' to 0
#define CLEAR_BIT(var,pos) (var &= ~(1 << pos))

#endif /* UTIL_H_ */
