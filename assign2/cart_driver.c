////////////////////////////////////////////////////////////////////////////////
//
//  File           : cart_driver.c
//  Description    : This is the implementation of the standardized IO functions
//                   for used to access the CART storage system.
//
//  Author         : Vibhu Patel
//  Last Modified  : on Oct 24 10:00:53 EDT 2016 by Vibhu Patel
//

// Project Includes

#include <stdlib.h>
#include "cart_driver.h"
#include "cart_controller.h"
#include "cmpsc311_log.h"
#include "unistd.h"
#include "string.h"

static CartXferRegister regstate, oregstate, ky1, ky2, rt1, ct1, fm1;
static CartFrameIndex IX;
static CartridgeIndex CX;

typedef int bool;
#define true 0
#define false 1

struct {
        bool isOpen;
        char filename[CART_MAX_PATH_LENGTH];
        int position;
        int length;
        int label;
        int START_INDEX;

} fileHandel [CART_MAX_TOTAL_FILES];

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_poweron
// Description  : Startup up the CART interface, initialize filesystem
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int32_t cart_poweron(void)
{
        int i;

        //INITMS - Initializes the memory system
        if ((regstate = create_cart_opcode(CART_OP_INITMS, 0, 0, 0, 0)) == -1) {
                logMessage(LOG_ERROR_LEVEL, "CART driver failed: fail on init.");
                return (-1);
        }
        oregstate = cart_io_bus(regstate, NULL);
        if (extract_cart_opcode(oregstate, &ky1, &ky2, &rt1, &ct1, &fm1)) {
                logMessage(LOG_ERROR_LEVEL, "CART driver failed: failed: fail on init (decon).");
                return (-1);
        }
        if (rt1) {
                logMessage(LOG_ERROR_LEVEL, "CART driver failed: failed: fail on init (return).");
                return (-1);
        }

        //LDCART - Loads all the cartridges
        for (i = 0; i < CART_MAX_CARTRIDGES; i++) {
                if ((regstate = create_cart_opcode(CART_OP_LDCART, 0, 0, i, 0)) == -1) {
                        logMessage(LOG_ERROR_LEVEL, "CART driver failed: fail on LDCART.");
                        return (-1);
                }
                oregstate = cart_io_bus(regstate, NULL);
                if (extract_cart_opcode(oregstate, &ky1, &ky2, &rt1, &ct1, &fm1)) {
                        logMessage(LOG_ERROR_LEVEL, "CART driver failed: failed: fail on LDCART (decon).");
                        return (-1);
                }
                if (rt1) {
                        logMessage(LOG_ERROR_LEVEL, "CART driver failed: failed: fail on LDCART (return).");
                        return (-1);
                }

                //BZero - Zeros current cartridge
                if ((regstate = create_cart_opcode(CART_OP_BZERO, 0, 0, 0, 0)) == -1) {
                        logMessage(LOG_ERROR_LEVEL, "CART driver failed: fail on BZERO.");
                        return (-1);
                }
                oregstate = cart_io_bus(regstate, NULL);
                if (extract_cart_opcode(oregstate, &ky1, &ky2, &rt1, &ct1, &fm1)) {
                        logMessage(LOG_ERROR_LEVEL, "CART driver failed: failed: fail on BZERO (decon).");
                        return (-1);
                }
                if (rt1) {
                        logMessage(LOG_ERROR_LEVEL, "CART driver failed: failed: fail on BZERO (return).");
                        return (-1);
                }
        }
        for (i = 0; i < CART_MAX_TOTAL_FILES; i++) {
                fileHandel[i].position = 0;
                fileHandel[i].length = 0;
                strncpy(fileHandel[i].filename, "0", 1);
                fileHandel[i].START_INDEX = 0;
        }
        return 0;
}
////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_poweroff
// Description  : Shut down the CART interface, close all files
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int32_t cart_poweroff(void)
{
        for (int i = 0; i < CART_FRAME_SIZE; i++) {
                fileHandel[i].isOpen = false;
        }
        if ((regstate = create_cart_opcode(CART_OP_POWOFF, 0, 0, 0, 0)) == -1) {
                logMessage(LOG_ERROR_LEVEL, "CART driver failed: fail on POWOFF.");
                return (-1);
        }
        oregstate = cart_io_bus(regstate, NULL);
        if (extract_cart_opcode(oregstate, &ky1, &ky2, &rt1, &ct1, &fm1)) {
                logMessage(LOG_ERROR_LEVEL, "CART driver failed: failed: fail on POWOFF (decon).");
                return (-1);
        }
        if (rt1) {
                logMessage(LOG_ERROR_LEVEL, "CART driver failed: failed: fail on POWOFF (return).");
                return (-1);
        }
        return (0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_open
// Description  : This function opens the file and returns a file handle
//
// Inputs       : path - filename of the file to open
// Outputs      : file handle if successful, -1 if failure

int16_t cart_open(char *path)
{

        int i, index, j;
        for (i = 0; i < CART_MAX_TOTAL_FILES; i++) {
                if (strncmp(fileHandel[i].filename, path, CART_MAX_PATH_LENGTH)) {
                        index = i;
                        fileHandel[i].label = i;
                        fileHandel[i].START_INDEX = 0;
                        fileHandel[index].position = 0;
                        fileHandel[index].isOpen = true;
                        return fileHandel[index].label;
                } else {
                        for (i = 0; i < CART_MAX_TOTAL_FILES; i++) {
                                for (j = 0; i < CART_MAX_PATH_LENGTH; j++) {
                                        if (fileHandel[i].filename[j] == 0) {
                                                strncpy(fileHandel[i].filename, path, 1);
                                                fileHandel[i].length = 0;
                                                fileHandel[i].position = 0;
                                                return fileHandel[i].label;
                                        }
                                }
                        }
                }
        }
        logMessage(LOG_ERROR_LEVEL, "file(s) not added to struct, FAILED ON OPEN");
        return (-1);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_close
// Description  : This function closes the file
//
// Inputs       : fd - the file descriptor
// Outputs      : 0 if successful, -1 if failure

int16_t cart_close(int16_t fd)
{
        int i;
        if (0 <= fd && fd < CART_MAX_TOTAL_FILES) {
                for (i = 0; i < CART_MAX_TOTAL_FILES; i++) {
                        if (fileHandel[i].label == fd) {
                                if (fileHandel[i].isOpen) {
                                        fileHandel[i].isOpen = false;
                                        return 0;
                                }
                        }
                }
        }
        logMessage(LOG_ERROR_LEVEL, "invalid file handel FAILED ON CLOSE");
        return -1;

}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_read
// Description  : Reads "count" bytes from the file handle "fh" into the
//                buffer "buf"
//
// Inputs       : fd - filename of the file to read from
//                buf - pointer to buffer to read into
//                count - number of bytes to read
// Outputs      : bytes read if successful, -1 if failure

int32_t cart_read(int16_t fd, void *buf, int32_t count)
{

        int soFar = 0;
        char fruty[CART_FRAME_SIZE];
        if (fileHandel[fd].isOpen == 0 && 0 <= fd && (fd < CART_MAX_TOTAL_FILES)) {
                if ((count + fileHandel[fd].position) >= fileHandel[fd].length) {
                        count = (fileHandel[fd].length - fileHandel[fd].position);
                }
                //Frame index
                IX = (fileHandel[fd].position / CART_FRAME_SIZE);
                //Cart index
                CX = (fileHandel[fd].position / (CART_CARTRIDGE_SIZE * CART_FRAME_SIZE));
                fileHandel[fd].START_INDEX = fileHandel[fd].position % CART_FRAME_SIZE;
                char tooty[CART_CARTRIDGE_SIZE * CART_FRAME_SIZE];
                //LDCART
                if ((regstate = create_cart_opcode(CART_OP_LDCART, 0, 0, CX, 0)) == -1) {
                        logMessage(LOG_ERROR_LEVEL, "CART driver failed: fail on LDCART. In read");
                        return (-1);
                }
                oregstate = cart_io_bus(regstate, NULL);
                if (extract_cart_opcode(oregstate, &ky1, &ky2, &rt1, &ct1, &fm1)) {
                        logMessage(LOG_ERROR_LEVEL, "CART driver failed: failed: fail on LDCART (decon). In read outside the loop");
                        return (-1);
                }
                if (rt1) {
                        logMessage(LOG_ERROR_LEVEL, "CART driver failed: failed: fail on LDCART (return).In read outside the loop");
                        return (-1);
                }

                while (count != 0) {
                        //RDFRAME
                        if ((regstate = create_cart_opcode(CART_OP_RDFRME, 0, 0, 0, IX)) == -1) {
                                logMessage(LOG_ERROR_LEVEL, "CART driver failed: fail on LDCART.");
                                return (-1);
                        }
                        oregstate = cart_io_bus(regstate, fruty);
                        if (extract_cart_opcode(oregstate, &ky1, &ky2, &rt1, &ct1, &fm1)) {
                                logMessage(LOG_ERROR_LEVEL, "CART driver failed: failed: fail on RDFRME (decon).");
                                return (-1);
                        }
                        if (rt1) {
                                logMessage(LOG_ERROR_LEVEL, "CART driver failed: failed: fail on RDFRME (return). In Read");
                                return (-1);
                        }
                        if ((CART_FRAME_SIZE - fileHandel[fd].START_INDEX) >= count) {
                                //copy the whole thing
                                memcpy(&tooty[soFar], &fruty[fileHandel[fd].START_INDEX], count);
                                soFar += count;
                                //We read everything so count has to be reset
                                count = 0;
                        } else {
                                //copy the temp portion only
                                memcpy(&tooty[soFar], &fruty[fileHandel[fd].START_INDEX], (CART_FRAME_SIZE - fileHandel[fd].START_INDEX));
                                soFar += (CART_FRAME_SIZE - fileHandel[fd].START_INDEX);
                                //Decrement count
                                count = count - (CART_FRAME_SIZE - fileHandel[fd].START_INDEX);
                        }
                        //So if the frame number is at the last frame go to the next frame
                        if (IX != CART_CARTRIDGE_SIZE - 1) {
                                fileHandel[fd].START_INDEX = 0;
                                IX++;
                        } else {
                                if (CX == (CART_MAX_CARTRIDGES - 1)) {
                                        logMessage(LOG_ERROR_LEVEL, "CART write failed cart is at the end");
                                        return (-1);
                                } else {
                                        CX++;
                                        fileHandel[fd].START_INDEX = 0;
                                        IX = 0;
                                }
                        }
                        fileHandel[fd].length += fileHandel[fd].position;
                }
                memcpy(buf, (void*) tooty, soFar);
                return soFar;

        }
        if (fileHandel[fd].isOpen != 0) {
                logMessage(LOG_ERROR_LEVEL, "File is not open, FAILED ON READ");
        }
        if (0 <= fd && (fd < CART_MAX_TOTAL_FILES)) {
                logMessage(LOG_ERROR_LEVEL, "CART read failed: fail on parameters.");
        }
        return -1;

}
////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_write
// Description  : Writes "count" bytes to the file handle "fh" from the
//                buffer  "buf"
//
// Inputs       : fd - filename of the file to write to
//                buf - pointer to buffer to write from
//                count - number of bytes to write
// Outputs      : bytes written if successful, -1 if failure

int32_t cart_write(int16_t fd, void *buf, int32_t count)
{

        int soFar = 0; //Read so far
        char fruty[CART_FRAME_SIZE];
        if (0 <= fd && (fd < CART_MAX_TOTAL_FILES) && fileHandel[fd].isOpen == 0) {
                //Frame index
                IX = (fileHandel[fd].position / CART_FRAME_SIZE);
                //Cart index
                CX = (fileHandel[fd].position / (CART_CARTRIDGE_SIZE * CART_FRAME_SIZE));
                fileHandel[fd].START_INDEX = fileHandel[fd].position % CART_FRAME_SIZE;
                char tooty[CART_FRAME_SIZE * CART_MAX_CARTRIDGES];
                //LDCART
                if ((regstate = create_cart_opcode(CART_OP_LDCART, 0, 0, CX, 0)) == -1) {
                        logMessage(LOG_ERROR_LEVEL, "CART driver failed: fail on LDCART.");
                        return (-1);
                }
                oregstate = cart_io_bus(regstate, NULL);
                if (extract_cart_opcode(oregstate, &ky1, &ky2, &rt1, &ct1, &fm1)) {
                        logMessage(LOG_ERROR_LEVEL, "CART driver failed: failed: fail on LDCART (decon). In write outside the loop");
                        return (-1);
                }
                if (rt1) {
                        logMessage(LOG_ERROR_LEVEL, "CART driver failed: failed: fail on LDCART (return). In write outside the loop");
                        return (-1);
                }
                //Grab(store it) the buf into a variable/array of char so you can use it below right after
                memcpy(tooty, (char*) buf, count);
                while (count != 0) {
                        if (fileHandel[fd].position != 0 || CART_FRAME_SIZE > count) {
                                //RDFRAME
                                if ((regstate = create_cart_opcode(CART_OP_RDFRME, 0, 0, 0, IX)) == -1) {
                                        logMessage(LOG_ERROR_LEVEL, "CART driver failed: fail on LDCART.");
                                        return (-1);
                                }
                                oregstate = cart_io_bus(regstate, fruty);
                                if (extract_cart_opcode(oregstate, &ky1, &ky2, &rt1, &ct1, &fm1)) {
                                        logMessage(LOG_ERROR_LEVEL, "CART driver failed: failed: fail on RDFRME (decon).");
                                        return (-1);
                                }
                                if (rt1) {
                                        logMessage(LOG_ERROR_LEVEL, "CART driver failed: failed: fail on RDFRME (return). In Read");
                                        return (-1);
                                }

                        }
                        if ((CART_FRAME_SIZE - fileHandel[fd].START_INDEX) >= count) {
                                //copy the whole thing
                                memcpy(&fruty[fileHandel[fd].START_INDEX], &tooty[soFar], count);
                                //add how much we have read to so far
                                soFar += count;
                                //We read everything so count has to be reset
                                count = 0;


                        } else {
                                //copy the new array portion only
                                memcpy(&fruty[fileHandel[fd].START_INDEX], &tooty[soFar], (CART_FRAME_SIZE - fileHandel[fd].START_INDEX));
                                //add how much we have read to so far to soFar
                                soFar += (CART_FRAME_SIZE - fileHandel[fd].START_INDEX);
                                //Decrement count by how much we read
                                count = count - (CART_FRAME_SIZE - fileHandel[fd].START_INDEX);

                        }
                        //WRFRAME
                        if ((regstate = create_cart_opcode(CART_OP_WRFRME, 0, 0, 0, IX)) == -1) {
                                logMessage(LOG_ERROR_LEVEL, "CART driver failed: fail on WRFRAME, inside the write loop.");
                                return (-1);
                        }
                        oregstate = cart_io_bus(regstate, fruty);
                        if (extract_cart_opcode(oregstate, &ky1, &ky2, &rt1, &ct1, &fm1)) {
                                logMessage(LOG_ERROR_LEVEL, "CART driver failed: failed: fail on WRFRME (decon). In the loop inside write");
                                return (-1);
                        }
                        if (rt1) {
                                logMessage(LOG_ERROR_LEVEL, "CART driver failed: failed: fail on WRFRME (return). In the loop inside write");
                                return (-1);
                        }
                        if (IX != (CART_CARTRIDGE_SIZE - 1)) {
                                fileHandel[fd].START_INDEX = 0;
                                IX++;
                        } else {
                                if (CX == (CART_MAX_CARTRIDGES - 1)) {
                                        logMessage(LOG_ERROR_LEVEL, "CART write failed cart is at the end");
                                        return (-1);
                                } else {
                                        CX++;
                                        fileHandel[fd].START_INDEX = 0;
                                        IX = 0;
                                }
                        }
                        fileHandel[fd].length  += fileHandel[fd].position;

                }
                //Update the length and position in the struct
                fileHandel[fd].length += soFar;
                fileHandel[fd].position += soFar;
                return soFar;
        }
        if (fileHandel[fd].isOpen != 0) {
                logMessage(LOG_ERROR_LEVEL, "File is not open, FAILED ON WRITE");
        }
        if (0 <= fd && (fd < CART_MAX_TOTAL_FILES)) {
                logMessage(LOG_ERROR_LEVEL, "CART write failed: fail on parameters.");
        }
        //logMessage(LOG_ERROR_LEVEL, "BAD PARAMATERS WRITE TERMINATES");
        return -1;
}



////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_seek
// Description  : Seek to specific point in the file
//
// Inputs       : fd - filename of the file to write to
//                loc - offfset of file in relation to beginning of file
// Outputs      : 0 if successful, -1 if failure

int32_t cart_seek(int16_t fd, uint32_t loc)
{
        if (0 <= fd && fd < CART_MAX_TOTAL_FILES) {
                if (fileHandel[fd].length >= loc && (fileHandel[fd].isOpen == 0)) {
                        fileHandel[fd].position = loc;
                        return 0;
                }
        }
        logMessage(LOG_ERROR_LEVEL, "Invalid fd failed on SEEK");
        return (-1);

}


//
// Implementation

////////////////////////////////////////////////////////////////////////////////
//
// Function     :create_cart_opcode
// Description  :packs/encodes the registers into a 64-bit opcode
//
// Inputs       :
//              ky1 -Key Register 1 of type CartXferRegister
//              ky2 -Key Register 2 of type CartXferRegister
//              rt1 -Return code register 1 of type CartXferRegister
//              ct1 -Cartridge register 1 of type CartXferRegister
//              fm1 -Frame register 1 of type CartXferRegister
// Outputs      :opcode containing packed registers

CartXferRegister create_cart_opcode(CartXferRegister ky1, CartXferRegister ky2, CartXferRegister rt1, CartXferRegister ct1, CartXferRegister fm1)
{

        ky1 = ky1 << 56;
        ky2 = ky2 << 48;
        ct1 = ct1 << 47;
        ct1 = ct1 << 31;
        fm1 = fm1 << 15;

        return (ky1 | ky2 | rt1 | ct1 | rt1 | fm1);



}

//
// Implementation

////////////////////////////////////////////////////////////////////////////////
//
// Function     :extract_cart_opcode
// Description  :unpacks/decodes oregstate and gives a value to all the registers. Will return zero for success
//
// Inputs       :
//              oregstate
//              pointer to ky1 -Key Register 1 of type CartXferRegister
//              pointer to ky2 -Key Register 2 of type CartXferRegister
//              pointer rt1 -Return code register 1 of type CartXferRegister
//              pointer to ct1 -Cartridge register 1 of type CartXferRegister
//              pointer to fm1 -Frame register 1 of type CartXferRegister
// Outputs      : returns 0 for success

CartXferRegister extract_cart_opcode(CartXferRegister oregstate, CartXferRegister *ky1, CartXferRegister *ky2, CartXferRegister *rt1, CartXferRegister *ct1, CartXferRegister * fm1)
{
        *ky1 = oregstate << 0;
        *ky1 = oregstate >> 56;

        *ky2 = oregstate << 8;
        *ky2 = *ky2 >> 56;

        *rt1 = oregstate << 16;
        *rt1 = *rt1 >> 63;

        *ct1 = oregstate << 17;
        *ct1 = *ct1 >> 48;

        *fm1 = oregstate << 33;
        *fm1 = *fm1 >> 48;

        return (0);
}


