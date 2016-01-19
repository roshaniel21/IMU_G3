#include "cobs.h"

void COBSStuffData(char *src, char *dst, char length) {
    // COBS encoding
    // See Cheshire, S. & Baker, M. "Consistent Overhead Byte Stuffing" for algorithm

    char *end = src + length - 2; // End of array (assumes extra two bytes accounted for in length)
    char *code_ptr = dst++;       // Memory location of next code
    char code = 0x01;             // Code indicating number of non-zero bytes following code

    // Loop through entire source array
    while(src < end) {
        // If zero is found, insert code at beginning of block
        if(*src == 0) {
            *code_ptr = code;
            code_ptr = dst++;
            code = 0x01;
        }
        // Otherwise, copy character to destination array
        else {
            *dst++ = *src;
            code++;
            // If code is 0xFF, no zeros found in packet
            if(code == 0xFF) {
                *code_ptr = code;
                code_ptr = dst++;
                code = 0x01;
            }
        }
        src++;
    }
    *code_ptr = code;
}
