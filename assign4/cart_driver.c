////////////////////////////////////////////////////////////////////////////////
//
//  File           : cart_driver.c
//  Description    : This is the implementation of the standardized IO functions
//                   for used to access the CART storage system.
//
//  Author         : Vibhu Patel
//  Last Modified  : on Nov 28 10:00:53 EDT 2016 by Vibhu Patel
//

// Project Includes

#include <stdlib.h>
#include "cart_driver.h"
#include "cart_controller.h"
#include "cmpsc311_log.h"
#include "unistd.h"
#include "string.h"
#include "cart_cache.h"
#include "cart_network.h"

static int fileHandelMain[CART_MAX_CARTRIDGES][CART_CARTRIDGE_SIZE];
static CartXferRegister regstate, oregstate, ky1, ky2, rt1, ct1, fm1;
static CartFrameIndex IX;
static CartridgeIndex CX;
static int numFiles;

typedef int bool;
#define true 1
#define false 0

struct {
    bool isOpen;
    char filename[CART_MAX_PATH_LENGTH];
    int position;
    int length;
    int START_INDEX;

} fileHandel [CART_MAX_TOTAL_FILES];

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_poweron
// Description  : Startup up the CART interface, initialize filesystem
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure
////////////////////////////////////////////////////////////////////////////////

int32_t cart_poweron(void) {
    int a, b, i;
    //init_cart_cache();
    //INITMS - Initializes the memory system
    if ((regstate = create_cart_opcode(CART_OP_INITMS, 0, 0, 0, 0)) == -1) {
        logMessage(LOG_ERROR_LEVEL, "CART driver failed: fail on init.");
        return (-1);
    }
    oregstate = client_cart_bus_request(regstate, NULL);
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
        oregstate = client_cart_bus_request(regstate, NULL);
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
        oregstate = client_cart_bus_request(regstate, NULL);
        if (extract_cart_opcode(oregstate, &ky1, &ky2, &rt1, &ct1, &fm1)) {
            logMessage(LOG_ERROR_LEVEL, "CART driver failed: failed: fail on BZERO (decon).");
            return (-1);
        }
        if (rt1) {
            logMessage(LOG_ERROR_LEVEL, "CART driver failed: failed: fail on BZERO (return).");
            return (-1);
        }
    }
    for (a = 0; a < CART_MAX_CARTRIDGES; a++) {
        for (b = 0; b < CART_CARTRIDGE_SIZE; b++) {
            fileHandelMain[a][b] = -777;
        }
    }
    CX = 0;
    IX = 0;
    numFiles = 0;
    for (i = 0; i < CART_MAX_TOTAL_FILES; i++) {
        fileHandel[i].position = 0;
        fileHandel[i].length = 0;
        strncpy(fileHandel[i].filename, "\0", CART_MAX_PATH_LENGTH);
        fileHandel[i].isOpen = false;
    }
    init_cart_cache();
    return 0;
}
////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_poweroff
// Description  : Shut down the CART interface, close all files
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure
////////////////////////////////////////////////////////////////////////////////

int32_t cart_poweroff(void) {
    for (int i = 0; i < numFiles; i++) {
        fileHandel[i].isOpen = false;
    }
    if ((regstate = create_cart_opcode(CART_OP_POWOFF, 0, 0, 0, 0)) == -1) {
        logMessage(LOG_ERROR_LEVEL, "CART driver failed: fail on POWOFF.");
        return (-1);
    }
    oregstate = client_cart_bus_request(regstate, NULL);
    if (extract_cart_opcode(oregstate, &ky1, &ky2, &rt1, &ct1, &fm1)) {
        logMessage(LOG_ERROR_LEVEL, "CART driver failed: failed: fail on POWOFF (decon).");
        return (-1);
    }
    if (rt1) {
        logMessage(LOG_ERROR_LEVEL, "CART driver failed: failed: fail on POWOFF (return).");
        return (-1);
    }
    close_cart_cache();
    return (0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_open
// Description  : This function opens the file and returns a file handle
//
// Inputs       : path - filename of the file to open
// Outputs      : file handle if successful, -1 if failure
////////////////////////////////////////////////////////////////////////////////

int16_t cart_open(char *path) {
    int i;
    for (i = 0; i < numFiles; i++) {
        //check if it exists
        if (strncmp(fileHandel[i].filename, path, CART_MAX_PATH_LENGTH) == 0) {
            //check if it's open
            if (fileHandel[i].isOpen) {
                fileHandel[i].START_INDEX = 0;
                fileHandel[i].position = 0;
                fileHandel[i].isOpen = true;
                return i;
            } else {
                logMessage(LOG_ERROR_LEVEL, "file isn't open");
                return -1;
            }
        }
    }
    //create the file
    if (numFiles < CART_MAX_TOTAL_FILES) {
        strncpy(fileHandel[numFiles].filename, path, CART_MAX_PATH_LENGTH);
        fileHandel[numFiles].isOpen = true;
        fileHandel[numFiles].position = 0;
        fileHandel[numFiles].START_INDEX = 0;
        fileHandel[numFiles].length = 0;
        numFiles++;
        return numFiles - 1;
    } else {
        logMessage(LOG_ERROR_LEVEL, "numFiles not working - FAILED ON OPEN");
        return -1;
    }
    logMessage(LOG_ERROR_LEVEL, "file(s) not added to struct, FAILED ON OPEN - file isn't being created");
    return (-1);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cart_close
// Description  : This function closes the file
//
// Inputs       : fd - the file descriptor
// Outputs      : 0 if successful, -1 if failure

int16_t cart_close(int16_t fd) {
    if (0 <= fd || fd <= CART_MAX_TOTAL_FILES) {
        if (fileHandel[fd].isOpen) {
            fileHandel[fd].isOpen = false;
            return 0;
        } else {
            logMessage(LOG_ERROR_LEVEL, "file not open - FAILED ON CLOSE");
            return -1;
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

int32_t cart_read(int16_t fd, void *buf, int32_t count) {
    int numFrames = 0, soFar = 0, controller = 0; //Number of frames for file, Read so far, loop controller
    char tooty[count], fruty[CART_FRAME_SIZE]; //temporary buffer and cart variable
    bool checkMe; //flag variable


    if ((fileHandel[fd].isOpen) && (0 <= fd) && (fd < CART_MAX_TOTAL_FILES)) {
        if ((count + fileHandel[fd].position) >= fileHandel[fd].length) {
            count = (fileHandel[fd].length - fileHandel[fd].position);
        }
        //reset
        IX = 0; //Frame index
        CX = 0; //Cart index
        numFrames = (fileHandel[fd].position / CART_FRAME_SIZE) + 1; //number of frames
        fileHandel[fd].START_INDEX = (fileHandel[fd].position % CART_FRAME_SIZE); //frame position



        while (count != 0) {

            while (controller < numFrames) {
                if (IX == CART_CARTRIDGE_SIZE) {
                    CX++;
                    IX = 0;
                }
                if (CX == CART_MAX_CARTRIDGES) {
                    logMessage(LOG_ERROR_LEVEL, "Cart index out of cart range fd: \n%d", fd);
                    return -1;
                }

                checkMe = (fileHandelMain[CX][IX] == fd);
                if (checkMe) {
                    controller++;
                }

                if (controller != numFrames) IX++;
            }
            if (get_cart_cache(CX, IX) == NULL) {
                //LDCART
                if ((regstate = create_cart_opcode(CART_OP_LDCART, 0, 0, CX, 0)) == -1) {
                    logMessage(LOG_ERROR_LEVEL, "CART driver failed: fail on LDCART. In read");
                    return (-1);
                }
                oregstate = client_cart_bus_request(regstate, NULL);
                if (extract_cart_opcode(oregstate, &ky1, &ky2, &rt1, &ct1, &fm1)) {
                    logMessage(LOG_ERROR_LEVEL, "CART driver failed: failed: fail on LDCART (decon). In read outside the loop");
                    return (-1);
                }
                if (rt1) {
                    logMessage(LOG_ERROR_LEVEL, "CART driver failed: failed: fail on LDCART (return).In read outside the loop");
                    return (-1);
                }

                //put_cart_cache(CX, IX, (void *) tooty);
                //RDFRAME
                if ((regstate = create_cart_opcode(CART_OP_RDFRME, 0, 0, 0, IX)) == -1) {
                    logMessage(LOG_ERROR_LEVEL, "CART driver failed: fail on LDCART.");
                    return (-1);
                }
                oregstate = client_cart_bus_request(regstate, fruty);
                if (extract_cart_opcode(oregstate, &ky1, &ky2, &rt1, &ct1, &fm1)) {
                    logMessage(LOG_ERROR_LEVEL, "CART driver failed: failed: fail on RDFRME (decon).");
                    return (-1);
                }
                if (rt1) {
                    logMessage(LOG_ERROR_LEVEL, "CART driver failed: failed: fail on RDFRME (return). In Read");
                    return (-1);
                }

                put_cart_cache(CX, IX, (void*) fruty);


            } else {
                memcpy(fruty, (char*) get_cart_cache(CX, IX), CART_FRAME_SIZE);
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

            //fileHandel[fd].START_INDEX = 0;
            numFrames++;
            IX++;
            fileHandel[fd].START_INDEX = 0;
            //fileHandel[fd].length += fileHandel[fd].position;
        }
        fileHandel[fd].position += soFar;
        memcpy(buf, (void*) tooty, soFar);
        return soFar;

    }
    if (!fileHandel[fd].isOpen) {
        logMessage(LOG_ERROR_LEVEL, "File is not open, FAILED ON READ");
        return -1;
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

int32_t cart_write(int16_t fd, void *buf, int32_t count) {
    int numFrames = 0, soFar = 0, controller = 0; //Number of frames for file, Read so far, loop controller
    char tooty[count], fruty[CART_FRAME_SIZE]; //temporary buffer and cart variable
    bool checkMe; //flag variable
    if (0 <= fd && (fd < CART_MAX_TOTAL_FILES) && fileHandel[fd].isOpen) {
        //reset
        IX = 0; //Frame index
        CX = 0; //Cart index
        numFrames = (fileHandel[fd].position / CART_FRAME_SIZE) + 1; //number of frames
        fileHandel[fd].START_INDEX = (fileHandel[fd].position % CART_FRAME_SIZE); //frame position
        //Grab(store it) the buf into a variable/array of char so you can use it below right after
        memcpy(tooty, (char*) buf, count);
        while (count != 0) {
            while (controller < numFrames) {
                if (IX == CART_CARTRIDGE_SIZE) {
                    CX++;
                    IX = 0;
                }
                if (CX == CART_MAX_CARTRIDGES) {
                    logMessage(LOG_ERROR_LEVEL, "Cart index out of cart range CX: \n%d", CX);
                    return -1;
                }

                checkMe = (fileHandelMain[CX][IX] == -777 || fileHandelMain[CX][IX] == fd);
                if (checkMe) {
                    controller++;
                }
                if (controller != numFrames) IX++;
            }

            if ((regstate = create_cart_opcode(CART_OP_LDCART, 0, 0, CX, 0)) == -1) {
                logMessage(LOG_ERROR_LEVEL, "CART driver failed: fail on LDCART.");
                return (-1);
            }
            oregstate = client_cart_bus_request(regstate, NULL);
            if (extract_cart_opcode(oregstate, &ky1, &ky2, &rt1, &ct1, &fm1)) {
                logMessage(LOG_ERROR_LEVEL, "CART driver failed: failed: fail on LDCART (decon). In write outside the loop");
                return (-1);
            }
            if (rt1) {
                logMessage(LOG_ERROR_LEVEL, "CART driver failed: failed: fail on LDCART (return). In write outside the loop");
                return (-1);
            }

            if (count < CART_FRAME_SIZE || fileHandel[fd].START_INDEX != 0) {
                if (get_cart_cache(CX, IX) == NULL) {
                    if ((regstate = create_cart_opcode(CART_OP_RDFRME, 0, 0, 0, IX)) == -1) {
                        logMessage(LOG_ERROR_LEVEL, "CART driver failed: fail on LDCART.");
                        return (-1);
                    }
                    oregstate = client_cart_bus_request(regstate, fruty);
                    if (extract_cart_opcode(oregstate, &ky1, &ky2, &rt1, &ct1, &fm1)) {
                        logMessage(LOG_ERROR_LEVEL, "CART driver failed: failed: fail on RDFRME (decon).");
                        return (-1);
                    }
                    if (rt1) {
                        logMessage(LOG_ERROR_LEVEL, "CART driver failed: failed: fail on RDFRME (return). In Read");
                        return (-1);
                    }
                } else {
                    memcpy(fruty, (char*) get_cart_cache(CX, IX), CART_FRAME_SIZE);
                }
            }

            if ((CART_FRAME_SIZE - fileHandel[fd].START_INDEX) >= count) {
                //copy the whole thing
                memcpy(&fruty[fileHandel[fd].START_INDEX], &tooty[soFar], count);
                //add how much we have read to so far
                soFar += count;
                fileHandel[fd].position += count;

                //We read everything so count has to be reset
                count = 0;

            } else {
                //copy the new array portion only
                memcpy(&fruty[fileHandel[fd].START_INDEX], &tooty[soFar], (CART_FRAME_SIZE - fileHandel[fd].START_INDEX));
                //add how much we have read to so far to soFar
                soFar += (CART_FRAME_SIZE - fileHandel[fd].START_INDEX);
                //Decrement count by how much we read
                fileHandel[fd].position += (CART_FRAME_SIZE - fileHandel[fd].START_INDEX);
                count -= (CART_FRAME_SIZE - fileHandel[fd].START_INDEX);

            }

            if ((fileHandel[fd].length <= fileHandel[fd].position)) {
                fileHandel[fd].length += fileHandel[fd].position - fileHandel[fd].length;
            }
            //WRFRAME
            if ((regstate = create_cart_opcode(CART_OP_WRFRME, 0, 0, 0, IX)) == -1) {
                logMessage(LOG_ERROR_LEVEL, "CART driver failed: fail on WRFRAME, inside the write loop.");
                return (-1);
            }
            oregstate = client_cart_bus_request(regstate, fruty);
            if (extract_cart_opcode(oregstate, &ky1, &ky2, &rt1, &ct1, &fm1)) {
                logMessage(LOG_ERROR_LEVEL, "CART driver failed: failed: fail on WRFRME (decon). In the loop inside write");
                return (-1);
            }
            if (rt1) {
                logMessage(LOG_ERROR_LEVEL, "CART driver failed: failed: fail on WRFRME (return). In the loop inside write");
                return (-1);
            }
            put_cart_cache(CX, IX, (void*) fruty);
            fileHandel[fd].START_INDEX = 0;
            numFrames++;
            fileHandelMain[CX][IX] = fd;
            IX++;

        }
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

int32_t cart_seek(int16_t fd, uint32_t loc) {
    if (0 <= fd || fd < CART_MAX_TOTAL_FILES) {
        if (fileHandel[fd].length >= loc && (fileHandel[fd].isOpen)) {
            fileHandel[fd].position = loc;
            return 0;
            logMessage(LOG_ERROR_LEVEL, "Invalid fd failed on SEEK (1)");
        }
        logMessage(LOG_ERROR_LEVEL, "bingo\n%d\t%d", fileHandel[fd].length, loc);
        return -1;
    }
    logMessage(LOG_ERROR_LEVEL, "Invalid fd failed on SEEK\n%d", fd);
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

CartXferRegister create_cart_opcode(CartXferRegister ky1, CartXferRegister ky2, CartXferRegister rt1, CartXferRegister ct1, CartXferRegister fm1) {
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

CartXferRegister extract_cart_opcode(CartXferRegister oregstate, CartXferRegister *ky1, CartXferRegister *ky2, CartXferRegister *rt1, CartXferRegister *ct1, CartXferRegister * fm1) {
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


