#ifndef CART_DRIVER_INCLUDED
#define CART_DRIVER_INCLUDED

////////////////////////////////////////////////////////////////////////////////
//
//  File           : cart_driver.h
//  Description    : This is the header file for the standardized IO functions
//                   for used to access the CART storage system.
//
//  Author         : Patrick McDaniel
//  Last Modified  : Mon Oct 24 10:00:53 EDT 2016 by Vibhu Patel
//

// Include files
#include <stdint.h>

// Defines
#define CART_MAX_TOTAL_FILES 1024 // Maximum number of files ever
#define CART_MAX_PATH_LENGTH 128 // Maximum length of filename length

//
// Interface functions

int32_t cart_poweron(void);
// Startup up the CART interface, initialize filesystem

int32_t cart_poweroff(void);
// Shut down the CART interface, close all files

int16_t cart_open(char *path);
// This function opens the file and returns a file handle

int16_t cart_close(int16_t fd);
// This function closes the file

int32_t cart_read(int16_t fd, void *buf, int32_t count);
// Reads "count" bytes from the file handle "fh" into the buffer  "buf"

int32_t cart_write(int16_t fd, void *buf, int32_t count);
// Writes "count" bytes to the file handle "fh" from the buffer  "buf"

int32_t cart_seek(int16_t fd, uint32_t loc);
// Seek to specific point in the file

uint64_t create_cart_opcode(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
//Packs the register

uint64_t extract_cart_opcode(uint64_t oregstate, uint64_t *ky1, uint64_t *ky2, uint64_t *rt1, uint64_t *ct1, uint64_t *fm1);
//


#endif

