////////////////////////////////////////////////////////////////////////////////
//
//  File          : cart_client.c
//  Description   : This is the client side of the CART communication protocol.
//
//   Author       : Vibhyu Patel
//  Last Modified : 12/08/2016
//


#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "cart_network.h"
#include "cmpsc311_util.h"
#include "cmpsc311_log.h"
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>


typedef int bool;
#define true 1
#define false 0

int CART_FRAME_VALUE = 1024;
int socket_handle = -1;
int cart_network_shutdown = 0;
struct sockaddr_in handel;
unsigned char *cart_network_address = NULL;
unsigned short cart_network_port = 0;
unsigned long CartControllerLLevel = 0;
unsigned long CartDriverLLevel = 0;
unsigned long CartSimulatorLLevel = 0;

CartXferRegister client_cart_bus_request(CartXferRegister reg, void *buf) {
    CartXferRegister controller;
    controller = reg >> 56;

    if (socket_handle == -1) {
        handel.sin_family = AF_INET;
        handel.sin_port = htons(CART_DEFAULT_PORT);
        if (inet_aton(CART_DEFAULT_IP, &handel.sin_addr) == 0) {
            return (-1);
        }

        socket_handle = socket(PF_INET, SOCK_STREAM, 0);
        if (socket_handle == -1) {
            return (-1);
        }

        if (connect(socket_handle, (const struct sockaddr *) &handel, sizeof (handel)) == -1) {
            return (-1);
        }
    }
    reg = htonll64(reg);
    if (controller == CART_OP_RDFRME) {


        write(socket_handle, &reg, sizeof (reg));
        read(socket_handle, buf, CART_FRAME_VALUE);
        read(socket_handle, &reg, sizeof (reg));



    } else if (controller == CART_OP_WRFRME) {



        write(socket_handle, &reg, sizeof (reg));
        write(socket_handle, buf, CART_FRAME_VALUE);
        read(socket_handle, &reg, sizeof (reg));




    } else if (controller == CART_OP_POWOFF) {



        write(socket_handle, &reg, sizeof (reg));
        read(socket_handle, &reg, sizeof (reg));
        close(socket_handle);
        socket_handle = -1;



    } else {


        write(socket_handle, &reg, sizeof (reg));
        read(socket_handle, &reg, sizeof (reg));


    }
    return 0;
}
