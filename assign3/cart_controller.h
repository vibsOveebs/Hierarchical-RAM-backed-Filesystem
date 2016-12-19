#ifndef CART_CONTROLLER_INCLUDED
#define CART_CONTROLLER_INCLUDED

////////////////////////////////////////////////////////////////////////////////
//
//  File           : cart_controller.h
//  Description    : This is the interface of the controller for the CART
//                   memory system interface.
//
//  Author         : Patrick McDaniel
//  Last Modified  : Sat Sep  6 08:24:25 EDT 2014
//

// Include
#include <stdint.h>

// These are the constants defining the size of the controllers
#define CART_MAX_CARTRIDGES 64
#define CART_CARTRIDGE_SIZE 1024
#define CART_FRAME_SIZE 1024
#define CART_NO_CARTRIDGE (CART_MAX_CARTRIDGES+0xff)

// Type definitions
typedef uint64_t CartXferRegister; // This is the value passed through the 
                        	       // controller register
typedef uint16_t CartridgeIndex;   // Index number of cartridge
typedef uint16_t CartFrameIndex;   // Frame index in current cartridge
typedef char CartFrame[CART_FRAME_SIZE];              // A Frame
typedef CartFrame CartCartridge[CART_CARTRIDGE_SIZE]; // A Cartridge

/*

 CartXferRegister Specification (See slides)

  Bits    Register (note that top is bit 0)
 ------   -------------------------------------------------------------
   0-7 - KY1 (Key Register 1)
  7-15 - KY2 (Key Register 2)
    16 - RT1 (Return code register 1)
 17-32 - CT1 (Cartridge register 1)
 33-48 - FM1 (Frame register 1)
 48-63 - UNUSED

*/

// These are the registers for the controller
typedef enum {

	CART_REG_KY1 = 0,   // Key 1 register (8 bits)
	CART_REG_KY2 = 1,   // Key 2 register (8 bits)
	CART_REG_RT1 = 2,   // Return code 1 (1 bit)
	CART_REG_CT1 = 3,   // Cartridge register 1
	CART_REG_FM1 = 4,   // Frame register 1
	CART_REG_MAXVAL = 5 // Maximum opcode value

} CartRegisters;

// These are the opcode (instructions) for the controller
typedef enum {

	CART_OP_INITMS = 0,  // Initialize the memory interfaces
	CART_OP_BZERO  = 1,  // Zero the current cartridge
	CART_OP_LDCART = 2,  // Load the current cartridge
	CART_OP_RDFRME = 3,  // Read the cartidge frame
	CART_OP_WRFRME = 4,  // Write to the cartridge frame
	CART_OP_POWOFF = 5,  // Power off the memory system
	CART_OP_MAXVAL = 6   // Maximum opcode value

} CartOpCodes;

//
// Global Data 

extern unsigned long CartControllerLLevel;  // Controller log level
extern unsigned long CartDriverLLevel;      // Driver log level
extern unsigned long CartSimulatorLLevel;   // Driver log level


//
// Functional Prototypes

CartXferRegister cart_io_bus(CartXferRegister regstate, void *buf);
	// This is the bus interface for communicating with controller

int cart_unit_test(void);
	// This function runs the unit tests for the cart controller.

#endif