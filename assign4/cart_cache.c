////////////////////////////////////////////////////////////////////////////////
//
//  File           : cart_cache.c
//  Description    : This is the implementation of the cache for the CART
//                   driver.
//
//  Author         : Vibhu Patel
//  Last Modified  : on Nov 28 10:00:53 EDT 2016 by Vibhu Patel
////////////////////////////////////////////////////////////////////////////////


#include "cart_controller.h"
#include "cmpsc311_util.h"
#include "cmpsc311_log.h"
#include "cart_cache.h"

typedef int bool;
#define true 1
#define false 0


static CartXferRegister clockTime;
static uint32_t totalCacheAllocate = DEFAULT_CART_FRAME_CACHE_SIZE;

struct cache {
    int numElementAccessTime;
    CartridgeIndex CX;
    CartFrameIndex IX;
    CartFrame fruty; //a frame 
} ch[CART_CARTRIDGE_SIZE * CART_MAX_CARTRIDGES];

////////////////////////////////////////////////////////////////////////////////
//
// Function     : set_cart_cache_size
// Description  : Set the size of the cache (must be called before init)
//
// Inputs       : max_frames - the maximum number of items your cache can hold
// Outputs      : 0 if successful, -1 if failure
////////////////////////////////////////////////////////////////////////////////

int set_cart_cache_size(uint32_t max_frames) {
    if (max_frames >= 0) {
        totalCacheAllocate = max_frames;
        return 0;
    } else {
        logMessage(LOG_OUTPUT_LEVEL, "max_frames is negative.");
        return -1;

    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : init_cart_cache
// Description  : Initialize the cache and note maximum frames
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure
////////////////////////////////////////////////////////////////////////////////

int init_cart_cache(void) {
    for (int i = 0; i < totalCacheAllocate; i++) {
        ch[i].CX = CART_MAX_CARTRIDGES + 777;
        ch[i].IX = CART_CARTRIDGE_SIZE + 777;
        ch[i].numElementAccessTime = 1;
        memset(ch[i].fruty, 1, CART_FRAME_SIZE); 
        //logMessage(LOG_OUTPUT_LEVEL, "init cache success.");
    }
    clockTime = 0;
    
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : close_cart_cache
// Description  : Clear all of the contents of the cache, cleanup
//
// Inputs       : none
// Outputs      : o if successful, -1 if failure
////////////////////////////////////////////////////////////////////////////////

int close_cart_cache(void) {
    for (int i = 0; i < totalCacheAllocate; i++) {
        ch[i].CX = 0;
        ch[i].IX = 0;
        ch[i].numElementAccessTime = 0;
        memset(ch[i].fruty, 0, CART_FRAME_SIZE);
        //logMessage(LOG_OUTPUT_LEVEL, "close cache success.");
    }
    clockTime = 0;
    return 0;

}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : put_cart_cache
// Description  : Put an object into the frame cache
//
// Inputs       : cart - the cartridge number of the frame to cache
//                frame - the frame number of the frame to cache
//                buf - the buffer to insert into the cache
// Outputs      : 0 if successful, -1 if failure
////////////////////////////////////////////////////////////////////////////////

int put_cart_cache(CartridgeIndex cart, CartFrameIndex frame, void *buf) {
    clockTime++;
    int LRU_index = 0;


    for (int i = 0; i < totalCacheAllocate; i++) {
        bool checkMe = (ch[i].CX == cart && ch[i].IX == frame);
        if (checkMe) {
            ch[i].CX = cart;
            ch[i].IX = frame;
            memcpy(ch[i].fruty, (char*) buf, CART_FRAME_SIZE);
            ch[i].numElementAccessTime = clockTime;
            return 0;
        }
    }


    for (int i = 0; i < totalCacheAllocate; i++) {
        if (ch[i].numElementAccessTime < ch[LRU_index].numElementAccessTime) {
            LRU_index = i;
        }
    }
    memcpy(ch[LRU_index].fruty, (char*) buf, CART_FRAME_SIZE);
    ch[LRU_index].CX = cart;
    ch[LRU_index].IX = frame;
    ch[LRU_index].numElementAccessTime = clockTime;
    //logMessage(LOG_OUTPUT_LEVEL, "put cart cache success.");
    return 0;

}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : get_cart_cache
// Description  : Get an frame from the cache (and return it)
//
// Inputs       : cart - the cartridge number of the cartridge to find
//                frm - the  number of the frame to find
// Outputs      : pointer to cached frame or NULL if not found

void * get_cart_cache(CartridgeIndex cart, CartFrameIndex frame) {
    clockTime++;
    for (int i = 0; i < totalCacheAllocate; i++) {
        bool checkMe = (ch[i].CX == cart && ch[i].IX == frame);
        if (checkMe) {
            ch[i].numElementAccessTime = clockTime;
            return (void*) &(ch[i].fruty);
        }
    }
    
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : delete_cart_cache
// Description  : Remove a frame from the cache (and return it)
//
// Inputs       : cart - the cart number of the frame to remove from cache
//                blk - the frame number of the frame to remove from cache
// Outputs      : pointe buffer inserted into the object

void * delete_cart_cache(CartridgeIndex cart, CartFrameIndex blk) {
    return 0;
}

//
// Unit test

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cartCacheUnitTest
// Description  : Run a UNIT test checking the cache implementation
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int cartCacheUnitTest(void) {
    // Return successfully
    logMessage(LOG_OUTPUT_LEVEL, "Cache unit test completed successfully.");
    return (0);
}