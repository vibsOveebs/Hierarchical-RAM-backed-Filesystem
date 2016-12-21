# All credit for the assigning the project goes to Patrick Drew McDaniel http://www.patrickmcdaniel.org/

Note: Like all assignments, for future/current CMPSC 311 students, you are prohibited from copying my code and any act of plagiarism may result in failure from the course enforced by Patrick Drew McDaniel and or Penn State University. This is simply for viewing purposes for anyone interested in my work. 

http://www.cse.psu.edu/~pdm12/cmpsc311-f16/cmpsc311-assign2.html




Assignment #2 - Hierarchical RAM Backed Filesystem
CMPSC311 - Introduction to Systems Programming
Fall 2016 - Prof. McDaniel
Due date: October 14, 2016 (11:59pm)

All remaining assignments for this class are based on the creation and extension of a user-space device driver for a in memory filesystem that is built on top of a hierarchical random access memory system (HRAM). At the highest level, you will translate file system commands into memory frame level memory operations (see memory interface specification below). The file system commands include open, read, write, seek and close for files that are written to your file system driver. These operations perform the same as the normal UNIX I/O operations, with the caveat that they direct file contents to the HRAM storage device instead of the host filesystem. The arrangement of software is as follows:

System and Project Overview


You are to write a basic device driver that will sit between a virtual application and virtualized hardware. The application makes use of the abstraction you will provide called HRAM "cartridge" memory system (CART). The design of the system is shown in the figure to the right:

Described in detail below, we can see three layers. The CART application is provided to you and will call your device driver functions with the basic UNIX file operations (open, ...). You are to write the device driver code to implement the file operations. Your code will communicate with a virtual controller by sending opcodes and data over an I/O bus.

All of the code for application (called the simulator) and CART memory system is given to you. Numerous utility functions are also provided to help debug and test your program, as well as create readable output. Sample workloads have been generated and will be used to extensively test the program. Students that make use of all of these tools (which take a bit of time up front to figure out) will find that the later assignments will become much easier to complete.

The Cartridge Memory System Driver (CART)

You are to write the driver that will implement the the basic UNIX file interface using the memory system. You will write code to implement the filesystem, and make several design decisions that will determine the structure and performance of the driver. In essence you will write code for the read, write, open, close and other high level filesystem functions. Each function will translate the call into low-level operations on the device (see below).

The functions you will implement are:
int32_t cart_poweron(void); - This function will initialize the CART interface basic filesystem. To do this you will execute the init CART opcode and zero all of the memory locations. You will also need to setup your internal data structures that track the state of the filesystem.
int32_t cart_poweroff(void); - This function will show down the CART interface basic filesystem. To do this you will execute the shutdown CART opcode and close all of the files. You will also need to cleanup your internal data structures that track the state of the filesystem
int16_t cart_open(char *path); - This function will open a file (named path) in the filesystem. If the file does not exist, it should be created and set to zero length. If it does exist, it should be opened and its read/write postion should be set to the first byte. Note that there are no subdirectories in the filesystem, just files (so you can treat the path as a filename). The function should return a unique file handle used for subsequent operations or -1 if a failure occurs.
int16_t cart_close(int16_t fd); - This function closes the file referenced by the file handle that was previously open. The function should fail (and return -1) if the file handle is bad or the file was not previously open.
int32_t cart_read(int16_t fd, void *buf, int32_t count); - This function should read count bytes from the file referenced by the file handle at the current position. Note that if there are not enough bytes left in the file, the function should read to the end of the file and return the number of bytes read. If there are enough bytes to fulfill the read, the function should return count. The function should fail (and return -1) if the file handle is bad or the file was not previously open.
int32_t cart_write(int16_t fd, void *buf, int32_t count); - The function should write count bytes into the file referenced by the file handle. If the write goes beyond the end of the file the size should be increased. The function should always return the number of bytes written, e.g., count. The function should fail (and return -1) if the file handle is bad or the file was not previously open.
int32_t cart_seek(int16_t fd, uint32_t loc); - The function should set the current position into the file to loc, where 0 is the first byte in the file. The function should fail (and return -1) if the loc is beyond the end of the file, the file handle is bad or the file was not previously open.
The key to this assignment if figuring out what you need to do to implement these functions. You are specifically not given guidance on how to do it. You need to (a) maintain information about current files in the file system, (b) allocate parts if the memory system to place data, (c) copy data into and out of the memory system as needed to serve reads and writes. How you do this is up to you, but think carefull about it before beginning to code. What is important is that the code you write will be built upon the whole semester.

The Cartridge Memory System (CART)

You will implement your driver on top of the CART memory system (which is referred to throughout simply as the CART). The CART is a hierarchical memory system, which means that there are multiple levels of memory organization. In the case of the CART, there are three levels: the system as a whole consists of multiple cartridges, each of which contain multiple frames. Each frame is a fixed byte sized memory block. Some key facts of the system include (see cart_controller.h for definitions):


There are CART_MAX_CARTRIDGES cartridges in the system, each of which is numbered from 0 to CART_MAX_CARTRIDGES-1.
Each cartridge contains are CART_CARTRIDGE_SIZE frames, each of which is numbered from 0 to CART_MAX_CARTRIDGES-1.
A frame is memory block of CART_FRAME_SIZE bytes.
You communicate with the memory system by sending code through a set of packed registers. These registers are set within a 64-bit value to encode the opcode and arguments to the hardware device. The opcode is laid out as follows:


  Bits    Register (note that top is bit 0)
 ------   -----------------------------------
   0-7 - KY1 (Key Register 1)
  7-15 - KY2 (Key Register 2)
    16 - RT1 (Return code register 1)
 17-32 - CT1 (Cartridge register 1)
 33-48 - FM1 (Frame register 1)
 48-63 - UNUSED
The following opcodes define how you interact with the controller. Note that the UNUSED parts of the opcode should always be zero, as should any register not explicitly listed in the specifications below. If the frame argument is not specified, then it should be passed as NULL.

Register	Request Value	Response Value
CART_OP_INITMS - Initialize the memory system
Register: KY1	CART_OP_INITMS	CART_OP_INITMS
Register: RT	N/A	0 if successful, -1 if failure
CART_OP_BZERO - zero the currently loaded cartridge
Register: KY1	CART_OP_BZERO	CART_OP_BZERO
Register: RT	N/A	0 if successful, -1 if failure
CART_OP_LDCART - load a cartridge
Register: KY1	CART_OP_LDCART	CART_OP_LDCART
Register: RT	N/A	0 if successful, -1 if failure
Register: CT1	cartridge number to load	N/A
CART_OP_RDFRME - read a frame from the current cartridge
Register: KY1	CART_OP_RDFRME	CART_OP_RDFRME
Register: RT	N/A	0 if successful, -1 if failure
Register: FM1	frame number to read from	N/A
CART_OP_WRFRME - write a frame to the current cartridge
Register: KY1	CART_OP_WRFRME	CART_OP_WRFRME
Register: RT	N/A	0 if successful, -1 if failure
Register: FM1	frame number to write to	N/A
CART_OP_POWOFF - power off the memory system
Register: KY1	CART_OP_POWOFF	CART_OP_POWOFF
Register: RT	N/A	0 if successful, -1 if failure
To execute an opcode, create a 64 bit value (uint64_t) and pass it any needed buffers to the bus function defined in cart_controller.h:

CartXferRegister cart_io_bus(CartXferRegister regstate, void *buf)
The function returns packed register values with as listed in the "Response Value" above.

Honors Option

The device driver should implement three different frame allocation strategies. They should be defined by adding an enumerated type and associated variable that is used to tell the driver which allocation to use for which run. The allocation strategies include:

CARTALLOC_LINEAR - this allocation strategy should allocate frame on the first cartridge until full, then second, then third, and so on. The frame on each cartridge should be allocated from highest to lowest, in reverse order.
CARTALLOC_BALANCED - this allocation strategy should allocate frames evenly across the cartridges (round-robin). The frames on each cartridge should be allocated from highest to lowest.
CARTALLOC_RANDOM - this allocation strategy should allocate frames randomly across the cartridges. The frames on each cartridge should be allocated randmonly as well.
You need to modify the main program to accept run-time parameters "--linear", "--balanced, and "--random". The program should fail if more than one of these is provided, and the driver should be configured with the appropriate value if the parameter is provided. The driver should default to random.

Instructions

Login to your virtual machine. From your virtual machine, download the starter source code provided for this assignment. To do this, use the wget utility to download the file off the main course website:

http://www.cse.psu.edu/~mcdaniel/cmpsc311-f16/docs/assign2-starter.tgz
Create a directory for your assignments and copy the file into it. Change into that directory.

% mkdir cmpsc311
% cp assign2-starter.tgz cmpsc311
% cd cmpsc311
% tar xvzf assign2-starter.tgz
Once unpacked, you will have the starter files in the assign2, including the source code, libraries, and a Makefile. There is also a subdirectory containing workload files.

You are to complete the cart_drver.c functions defined above. Note that you may need to create additional supporting functions within the same file. Include functional prototypes for these functions at the top of the file.

Add comments to all of your files stating what the code is doing. Fill out the comment function header for each function you are defining in the code. A sample header you can copy for this purpose is provided for the main function in the code.

To test your program with these provided workload files, run the code specifying a workload file as follows:

./cart_sim –v workload/cmpsc311-f16-assign2-workload.txt
Note that you don't necessarily have to use the -v option, but it provides a lot of debugging information that is helpful.

If the program completes successfully, the following should be displayed as the last log entry:
CART simulation: all tests successful!!!.
To turn in:

Create a tarball file containing the assign2 directory, source code and build files as completed above. Email the program to Professor McDaniel and the section TA (listed on course website) by the assignment deadline (11:59pm of the day of the assignment). The tarball should be named LASTNAME-PSUEMAILID-assign2.tgz, where LASTNAME is your last name in all capital letters and PSUEMAILID is your PSU email address without the "@psu.edu". For example, the professor was submitting a homework, he would call the file MCDANIEL-pdm12-assign2.tgz. Any file that is incorrectly named, has the incorrect directory structure, or has misnamed files, will be assessed a one day late penalty.

Before sending the tarball, test it using the following commands (in a temporary directory -- NOT the directory you used to develop the code):

% tar xvzf LASTNAME-PSUEMAILID-assign2.tgz
% cd assign2
% make
... (TEST THE PROGRAM)

Note: Like all assignments in this class you are prohibited from copying any content from the Internet or discussing, sharing ideas, code, configuration, text or anything else or getting help from anyone in or outside of the class. Consulting online sources is acceptable, but under no circumstances should anything be copied. Failure to abide by this requirement will result dismissal from the class as described in our course syllabus.

 

-----------------------------------------------------------------------------------------------------------------------------------------
http://www.cse.psu.edu/~pdm12/cmpsc311-f16/cmpsc311-assign3.html





Assignment #3 - Cart Device Driver (version 1.1)
CMPSC311 - Introduction to Systems Programming
Fall 2016 - Prof. McDaniel
Due date: November 18, 2016 (11:59pm)

In this assignment you will write extend the device driver written in the previous assignment. Everything about the cart device remains as before, except as described in the specification here. At the highest level, you will extend the code to support many and larger files and frame caching. All of the extensions will be made to the functions modified in the previous section as well as a new file cart_cache.c.

Cartridge Driver Extension

The code addition to the driver will be the ability to support multiple open files at a given time. The driver must be able to support CART_MAX_TOTAL_FILES files, all of which may be open at the same time. The files have no maximum size.

You will also add a frame cache to your driver implementation. It will store dynamically allocated frames in a write through LRU cache you will implement. The function declarations are provided to you in cart_cache.h defined below. You will need to check the cache every time you read a frame to see if is already there and used the cached entry rather than going to the bus. On writes, you will need to insert into the cache if it exists or update if it does exist. The cache should allow for a maximum number of entries. When the number of entries is exceeded, you need to delete the frame (and deallocate the frame) per the cache replacement policy.

int set_cart_cache_size(uint32_t max_frames);
        // Set the size of the cache (must be called before init)

int init_cart_cache(void);
        // Initialize the cache 

int close_cart_cache(void);
        // Clear all of the contents of the cache, cleanup

int put_cart_cache(CartridgeIndex cart, CartFrameIndex frm, void *frame);
        // Put an object into the object cache, evicting other items as necessary

void * get_cart_cache(CartridgeIndex dsk, CartFrameIndex blk);
        // Get an object from the cache (and return it)
You should refer to the lecture on caching for hints on how to implement the cache. Note that you will have to insert calls to the above functions in the existing code, including during initialization of the driver, shut down, and during reads and writes. The set_cart_cache_size function is called by the main program to set the maximum number of frames (and thus you don't have to call this in your code).

Honors Option

The cache implementation should implement three replacement policies; LRU (as above), LFU, and random replacement. For the LFU, you should hold newly inserted in the cache for at least 100 cache operations for warming the entry (reads or writes). You will need to add command line arguments to allow the algorithm selection: --lru --lfu --random.

Extra credit

Complete the cache unit test function with code that initializes the cache, create 10,000 cache operations of random frame gets and puts, the closes the cache and completely frees all of the memory associated with the created entries. Note that the [-u] option on the command line with run this and the other unit tests. The test should check that the data retrieved from the cache is correct (the bytes sent in are the same ones you get out).

Assignment Details

Below are the step by step details of how to implement, test, and submit your device drivers as part of the class assignment. As always, the instructor and TAs are available to answer questions and help you make progress. Note that this assignment is deceptively complicated and will likely take even the best students a substantial amount of time. Please plan to allocate your time accordingly.

From your virtual machine, download the starter source code provided for this assignment. To do this, use the wget utility to download the file off the main course website:

http://www.cse.psu.edu/~mcdaniel/cmpsc311-f16/docs/assign3-starter.tgz
Copy the downloaded file into your development directory and unpack it.

% cd cmpsc311
% tar xvfz assign3-starter.tgz
Once unpacked, you will have the following starter files in the assign3 directory. All of your work should be done in this directory.

Copy over your cart_driver.c file from the previous assignment. You are to complete the cart_sim program but extending the previous assignment. All of this code will be implemented in the source code file cart_driver.c. You are free to create any additional functions that you see a need for, so long as they are all in the same code file.

Complete the cart_cache.c file. Provide code to implement the cache functionality as described above.

Add comments to all of your files stating what the code is doing. Fill out the comment function header for each function you are defining in the code. A sample header you can copy for this purpose is provided for the main function in the code.

To test your program, you will run it with sample workload files provide. To do this, run the program from the command line with the sample input:

./cart_sim –v workload/cmpsc311-f16-assign3-workload.txt
You must use the "-v" option before the workload filename when running the simulation to get meaningful output. Once you have your program running correctly, you see the following message at the end of the output:

[INFO] Cartridge simulation completed successfully.
To turn in:

Create a tarball file containing the assign3 directory, source code and build files as completed above. Email the program to Professor McDaniel and the section TA (listed on course website) by the assignment deadline (11:59pm of the day of the assignment), and CC yourself. The tarball should be named LASTNAME-PSUEMAILID-assign3.tgz, where LASTNAME is your last name in all capital letters and PSUEMAILID is your PSU email address without the "@psu.edu". For example, the professor was submitting a homework, he would call the file MCDANIEL-pdm12-assign3.tgz. Any file that is incorrectly named, has the incorrect directory structure, or has misnamed files, will be assessed a one day late penalty.

Before sending the tarball, test it using the following commands (in a temporary directory -- NOT the directory you used to develop the code):

% tar xvzf LASTNAME-PSUEMAILID-assign3.tgz
% cd assign3
% make
... (TEST THE PROGRAM)

Note: Like all assignments in this class you are prohibited from copying any content from the Internet or discussing, sharing ideas, code, configuration, text or anything else or getting help from anyone in or outside of the class. Consulting online sources is acceptable, but under no circumstances should anything be copied. Failure to abide by this requirement will result dismissal from the class as described in our course syllabus.


-----------------------------------------------------------------------------------------------------------------------------------------

http://www.cse.psu.edu/~pdm12/cmpsc311-f16/cmpsc311-assign4.html

Assignment #4 - Cart Device Driver (version 1.2)
CMPSC311 - Introduction to Systems Programming
Fall 2016 - Prof. McDaniel
Due date: December 9, 2016 (11:59pm)

In this assignment you will write extend the device driver written in the previous assignment. Everything about the cart device remains as before, except as described in the specification here. At the highest level, you will extend the code to support networking communication to a server program that implements the memory device. All of the extensions will be made to the functions modified in the previous section as well as a new file cart_client.c.

Network Attached Cartridge System

For this assignment, the operation of the memory device is separated into two programs; the cart client program which you will complete the code for, and the	cart server program which is provided to you. To run the program, you will run the server in one window and the client in another. You will use a local network connection (called the loopback interface) to connect the two programs running on the same machine (yours).

The challenge of this assignment is that you will be sending your cart requests and receiving responses over a network. You must start with your code from assignment #3, as it requires it. The assignment requires you to perform the following steps (after getting the starter code and copying over needed files from your implementation of the previous assignments).

The first thing you are to do is to replace the all of the calls to cartridge bus request in your code to:
CartXferRegister client_cart_bus_request(CartXferRegister reg, void *buf);
This function is defined in the new file cart_network.h and performs the same function as the original bus request function it is replacing.

The remainder of this part of the assignment is your implementation of the client_cart_bus_request function in the cart_client.c code file. The idea is that you will coordinate with the a server via a network protocol to transfer the commands and data from the client to the server. The server will execute the commmands and modify the cart storage accordingly.

The first time you want to send a message to the server, you have to connect. Connect to address CART_DEFAULT_IP and port CART_DEFAULT_PORT.

To transfer the commands, you send the request values over the network and receive the response values. For each command the client will send the 64-bit register value (in network byte order) to the server. If the request has a non-NULL frame (e.g., ky1 is a CART_OP_WRFRME), then the frame will be sent over the network immediately following the register value.

The server will respond to each command with the 64-bit register return value (in network byte order). If the request has a non-NULL return frame (e.g., ky1 is a CART_OP_RDFRME), then the frame will be sent over the network immediately following the registers.

Note that you need to convert the 64 bit register values and Length values into network byte order before sending them and converting them to host byte order when receiving them. The functions htonll64 and ntohll64 are used to perform these functions respectively. The frames are not numeric data, and thus don't need to be put in network byte order.

You will use the network APIs described in the associated networking lecture to complete this assignment. Note that the last thing you should do in your client program after a CART_OP_POWOFF is to disconnect.

Honors Option

For the honors option, you must implement encrypt the data before sending to the memory device and decrypt it when is returned. You should use the gcrypt library using the AES 128-bit cipher and a random key generated at run time.

Assignment Details

Below are the step by step details of how to implement, test, and submit your device drivers as part of the class assignment. As always, the instructor and TAs are available to answer questions and help you make progress. Note that this assignment is complicated and will likely take even the best students a substantial amount of time. Please plan to allocate your time accordingly.

From your virtual machine, download the starter source code provided for this assignment. To do this, use the wget utility to download the file off the main course website:

http://www.cse.psu.edu/~mcdaniel/cmpsc311-f16/docs/assign4-starter.tgz
Copy the downloaded file into your development directory and unpack it.

% cd cmpsc311
% tar xvfz assign4-starter.tgz
Once unpacked, you will have the starter files in the assign4 directory. All of your work should be done in this directory.

Copy over your driver and cache implementation file from the previous assignment. You are to complete the cart_client program by extending the functions described above in the cart_driver.c, cart_cache.c, and cart_client.c source code files. You are free to create any additional functions that you see a need for, so long as they are all in the same code file as they are used in.

Add comments to all of your files stating what the code is doing. Fill out the comment function header for each function you are defining in the code. A sample header you can copy for this purpose is provided for the main function in the code.

To test your program, you will run it with sample workload files provided. The first step is to start the server program in a second terminal window. To do this, run the server program from the command line with sample input. E.g.,

./cart_server -v
To run your client, run the program in a separate window from the command line with workload input. E.g.,

./cart_client -v workload/cmpsc311-f16-assign4-workload.txt
You must use the "-v" option before the workload filename when running the simulation to get meaningful output (but the program should work find without the -v option). You may wish to use the "-l [logile]" option to redirect your output to a file of either the client or server. Once you have your program running correctly, you see the following message at the end of the output:

[INFO] Cartridge simulation completed successfully.
To turn in:

Create a tarball file containing the assign4 directory, source code and build files as completed above. Email the program to Professor McDaniel and the section TA (listed on course website) by the assignment deadline (11:59pm of the day of the assignment). The tarball should be named LASTNAME-PSUEMAILID-assign4.tgz, where LASTNAME is your last name in all capital letters and PSUEMAILID is your PSU email address without the "@psu.edu". For example, the professor was submitting a homework, he would call the file MCDANIEL-pdm12-assign4.tgz. Any file that is incorrectly named, has the incorrect directory structure, or has misnamed files, will be assessed a one day late penalty.

Before sending the tarball, test it using the following commands (in a temporary directory -- NOT the directory you used to develop the code):

% tar xvzf LASTNAME-PSUEMAILID-assign4.tgz
% cd assign4
% make
... (TEST THE PROGRAM)
Submission a received when it arrives in the professor's inbox. NO EXCEPTIONS! To ensure this happends safely, it is strongly suggested that (a) do a “make clean” before tarring up the file and (b) CC yourself on the email to make sure you got it.

Note: Like all assignments in this class you are prohibited from copying any content from the Internet or discussing, sharing ideas, code, configuration, text or anything else or getting help from anyone in or outside of the class. Consulting online sources is acceptable, but under no circumstances should anything be copied. Failure to abide by this requirement will result dismissal from the class as described in our course syllabus.

 
CMPSC311 • Introduction to Systems Programming • Fall 2016 • Professor McDaniel
-----------------------------------------------------------------------------------------------------------------------------------------
