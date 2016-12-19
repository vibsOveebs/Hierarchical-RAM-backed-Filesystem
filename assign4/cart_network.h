#ifndef CART_NETWORK_INCLUDED
#define CART_NETWORK_INCLUDED

////////////////////////////////////////////////////////////////////////////////
//
//  File          : cart_network.h
//  Description   : This is the network definitions for the cartridge simulator.
//
//  Author        : Patrick McDaniel
//  Last Modified : Sun Oct 30 07:48:47 EDT 2016
//

// Include Files

// Project Include Files
#include <cart_controller.h>

// Defines
#define CART_MAX_BACKLOG 5
#define CART_NET_HEADER_SIZE sizeof(CartXferRegister)
#define CART_DEFAULT_IP "127.0.0.1"
#define CART_DEFAULT_PORT 21785

// Global data
extern int            cart_network_shutdown; // Flag indicating shutdown
extern unsigned char *cart_network_address;  // Address of CART server
extern unsigned short cart_network_port;     // Port of CART server

//
// Functional Prototypes

CartXferRegister client_cart_bus_request(CartXferRegister reg, void *buf);
	// This is the implementation of the client operation (cart_client.c)

int cart_server( void );
	// This is the implementation of the server application (cart_server.c)

#endif
