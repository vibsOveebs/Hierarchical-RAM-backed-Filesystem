////////////////////////////////////////////////////////////////////////////////
//
//  File           : cartsim.c
//  Description    : This is the main program for the CMPSC311 programming
//                   assignment #2 (beginning of CART interface).
//
//   Author        : Patrick McDaniel
//   Last Modified : Thu Sep 15 14:49:37 EDT 2016
//

// Include Files
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Project Includes
#include <cart_driver.h>
#include <cart_cache.h>
#include <cart_network.h>
#include <cmpsc311_log.h>
#include <cmpsc311_util.h>

// Defines
#define CART_WORKLOAD_DIR "workload"
#define CART_SIM_MAX_OPEN_FILES 128
#define CART_ARGUMENTS "huvl:c:i:p:"
#define USAGE \
	"USAGE: cart_sim [-h] [-v] [-l <logfile>] [-c <sz>] <workload-file>\n" \
	"\n" \
	"where:\n" \
	"    -h - help mode (display this message)\n" \
	"    -v - verbose output\n" \
	"    -l - write log messages to the filename <logfile>\n" \
	"    -c - set the cart block cache to size <sz> (disabled for assign #2)\n" \
	"    -i - IP address of server to connect to.\n" \
	"    -p - port number of server to connect to.\n" \
	"\n" \
	"    <workload-file> - file contain the workload to simulate\n" \
	"\n" \

// This is the file table
typedef struct {
	char     *filename;  // This is the filename for the test file
	int16_t   fhandle;   // This is a file handle for the opened file
} CartSimulationTable;

//
// Global Data
int verbose;

//
// Functional Prototypes

int simulate_CART( char *wload );             // control loop of the CART simulation
int validate_file(char *fname, int16_t mfh);  // Validate a file in the filesystem

//
// Functions

////////////////////////////////////////////////////////////////////////////////
//
// Function     : main
// Description  : The main function for the CART simulator
//
// Inputs       : argc - the number of command line parameters
//                argv - the parameters
// Outputs      : 0 if successful test, -1 if failure

int main( int argc, char *argv[] ) {

	// Local variables
	int ch, verbose = 0, log_initialized = 0, unit_tests = 0;
	uint32_t cache_size = 0;

	// Process the command line parameters
	while ((ch = getopt(argc, argv, CART_ARGUMENTS)) != -1) {

		switch (ch) {
		case 'h': // Help, print usage
			fprintf( stderr, USAGE );
			return( -1 );

		case 'v': // Verbose Flag
			verbose = 1;
			break;

		case 'u': // Unit test Flag
			unit_tests = 1;
			break;

		case 'l': // Set the log filename
			initializeLogWithFilename( optarg );
			log_initialized = 1;
			break;

		case 'c': // Set cache line size
			if ( sscanf( optarg, "%u", &cache_size ) != 1 ) {
			    logMessage( LOG_ERROR_LEVEL, "Bad cache size [%s]", argv[optind] );
			}
			break;

        case 'i': // Get the IP address
            if (inet_addr(optarg) == INADDR_NONE) {
			    logMessage( LOG_ERROR_LEVEL, "Bad IP address [%s]", argv[optind] );
                return(-1);
            }
            cart_network_address = (unsigned char *)strdup(optarg);
			break;

        case 'p': // Set the network port number
			if ( sscanf(optarg, "%hu", &cart_network_port) != 1 ) {
			    logMessage( LOG_ERROR_LEVEL, "Bad  port number [%s]", argv[optind] );
                return(-1);
			}
            break;			

		default:  // Default (unknown)
			fprintf( stderr, "Unknown command line option (%c), aborting.\n", ch );
			return( -1 );
		}
	}

	// Setup the log as needed
	if ( ! log_initialized ) {
		initializeLogWithFilehandle( CMPSC311_LOG_STDERR );
	}
	if ( verbose ) {
		enableLogLevels(LOG_INFO_LEVEL);
	}

	// Setup the cache size as needed
	if (cache_size != 0) {
		set_cart_cache_size(cache_size);
	}

	// If exgtracting file from data
	if (unit_tests) {

		// Run the unit tests
		enableLogLevels( LOG_INFO_LEVEL );
		logMessage(LOG_INFO_LEVEL, "Running unit tests ....\n\n");
		if ( (cartCacheUnitTest() == 0) && (cartCacheUnitTest() == 0) ) {
			logMessage(LOG_INFO_LEVEL, "Unit tests completed successfully.\n\n");
		} else {
			logMessage(LOG_ERROR_LEVEL, "Unit tests failed, aborting.\n\n");
		}

	} else {

		// The filename should be the next option
		if ( optind >= argc ) {

			// No filename
			fprintf( stderr, "Missing command line parameters, use -h to see usage, aborting.\n" );
			return( -1 );

		}

		// Run the simulation
		if ( simulate_CART(argv[optind]) == 0 ) {
			logMessage( LOG_INFO_LEVEL, "CART simulation completed successfully.\n\n" );
		} else {
			logMessage( LOG_INFO_LEVEL, "CART simulation failed.\n\n" );
		}
	}

	// Return successfully
	return( 0 );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : simulate_CART
// Description  : The main control loop for the processing of the CART
//                simulation.
//
// Inputs       : wload - the name of the workload file
// Outputs      : 0 if successful test, -1 if failure

int simulate_CART( char *wload ) {

	// Local variables
	char line[1024], fname[128], command[128], text[1025], *sep, *rbuf;
	FILE *fhandle = NULL;
	int32_t err=0, len, off, fields, linecount;
	CartSimulationTable ftable[CART_SIM_MAX_OPEN_FILES];
	int idx, i;

	// Setup the file table
	memset(ftable, 0x0, sizeof(CartSimulationTable)*CART_SIM_MAX_OPEN_FILES);

	// Open the workload file
	linecount = 0;
	if ( (fhandle=fopen(wload, "r")) == NULL ) {
		logMessage( LOG_ERROR_LEVEL, "Failure opening the workload file [%s], error: %s.\n",
			wload, strerror(errno) );
		return( -1 );
	}

	// Startup the interface
	if (cart_poweron() == -1) {
		logMessage( LOG_ERROR_LEVEL, "CART simulator failed initialization.");
		fclose( fhandle );
		return( -1 );
	}
	logMessage(CartSimulatorLLevel, "CART simulator initialization complete.");

	// While file not done
	while (!feof(fhandle)) {

		// Get the line and bail out on fail
		if (fgets(line, 1024, fhandle) != NULL) {

			// Parse out the string
			linecount ++;
			fields = sscanf(line, "%s %s %d %d", fname, command, &len, &off);
			sep = strchr(line, ':');
			if ( (fields != 4) || (sep == NULL) ) {
				logMessage( LOG_ERROR_LEVEL, "CART un-parsable workload string, aborting [%s], line %d",
						line, linecount );
				fclose( fhandle );
				return( -1 );
			}

			// Just log the contents
			logMessage(CartSimulatorLLevel, "File [%s], command [%s], len=%d, offset=%d",
					fname, command, len, off);

			// Now walk the the table looking for the file
			idx = -1;
			i = 0;
			while ( (i < CART_SIM_MAX_OPEN_FILES) && (idx == -1) ) {
				if ( (ftable[i].filename != NULL) && (strcmp(ftable[i].filename,fname) == 0) ) {
					idx = i;
				}
				i++;
			}

			// File is not found, open the file
			if (idx == -1) {

				// Log message, find unused index and save filename for later use
				logMessage(CartSimulatorLLevel, "CART_SIM : Opening file [%s]", fname);
				idx = 0;
				while ((ftable[idx].filename != NULL) && (idx < CART_SIM_MAX_OPEN_FILES)) {
					idx++;
				}
				CMPSC_ASSERT1(idx<CART_SIM_MAX_OPEN_FILES, "Too many open files on CART sim [%d]", idx);
				ftable[idx].filename = strdup(fname);

				// Now perform the open
				ftable[idx].fhandle = cart_open(ftable[idx].filename);
				if (ftable[idx].fhandle == -1) {
					// Failed, error out
					logMessage(LOG_ERROR_LEVEL, "Open of new file [%s] failed, aborting simulation.", fname);
					return(-1);
				}

			}

			// Now execute the specific command
			if (strncmp(command, "WRITEAT", 7) == 0) {

				// Log the command executed
				logMessage(CartSimulatorLLevel, "CART_SIM : Writing %d bytes at position %d from file [%s]", len, off, fname);

				// First perform the seek
				if (cart_seek(ftable[idx].fhandle, off)) {
					// Failed, error out
					logMessage(LOG_ERROR_LEVEL, "Seek/WriteAt file [%s] to position %d failed, aborting simulation.", fname, off);
					return(-1);
				}

				// Now see if we need more data to fill, terminate the lines
				CMPSC_ASSERT1(len<1024, "Simulated workload command text too large [%d]", len);
				CMPSC_ASSERT2((strlen(sep+1)>=len), "Workload str [%d<%d]", strlen(sep+1), len);
				strncpy(text, sep+1, len);
				text[len] = 0x0;
				for (i=0; i<strlen(text); i++) {
					if (text[i] == '^') {
						text[i] = '\n';
					}
				}

				// Now perform the write
				if (cart_write(ftable[idx].fhandle, text, len) != len) {
					// Failed, error out
					logMessage(LOG_ERROR_LEVEL, "WriteAt of file [%s], length %d failed, aborting simulation.", fname, len);
					return(-1);
				}


			} else if (strncmp(command, "WRITE", 5) == 0) {

				// Now see if we need more data to fill, terminate the lines
				CMPSC_ASSERT1(len<1024, "Simulated workload command text too large [%d]", len);
				CMPSC_ASSERT2((strlen(sep+1)>=len), "Workload str [%d<%d]", strlen(sep+1), len);
				strncpy(text, sep+1, len);
				text[len] = 0x0;
				for (i=0; i<strlen(text); i++) {
					if (text[i] == '^') {
						text[i] = '\n';
					}
				}

				// Log the command executed
				logMessage(CartSimulatorLLevel, "CART_SIM : Writing %d bytes to file [%s]", len, fname);

				// Now perform the write
				if (cart_write(ftable[idx].fhandle, text, len) != len) {
					// Failed, error out
					logMessage(LOG_ERROR_LEVEL, "Write of file [%s], length %d failed, aborting simulation.", fname, len);
					return(-1);
				}


			} else if (strncmp(command, "SEEK", 4) == 0) {

				// Log the command executed
				logMessage(CartSimulatorLLevel, "CART_SIM : Seeking to position %d in file [%s]", off, fname);

				// Now perform the seek
				if (cart_seek(ftable[idx].fhandle, off) != len) {
					// Failed, error out
					logMessage(LOG_ERROR_LEVEL, "Seek in file [%s] to position %d failed, aborting simulation.", fname, off);
					return(-1);
				}

			} else if (strncmp(command, "READ", 4) == 0) {

				// Log the command executed
				logMessage(CartSimulatorLLevel, "CART_SIM : Reading %d bytes from file [%s]", len, fname);

				// Now perform the read
				rbuf = malloc(len);
				if (cart_read(ftable[idx].fhandle, rbuf, len) != len) {
					// Failed, error out
					logMessage(LOG_ERROR_LEVEL, "Read file [%s] of length %d failed, aborting simulation.", fname, off);
					return(-1);
				}
				free(rbuf);
				rbuf = NULL;

			} else {

				// Bomb out, don't understand the command
				CMPSC_ASSERT1(0, "CART_SIM : Failed, unknown command [%s]", command);

			}
		}

		// Check for the virtual level failing
		if ( err ) {
			logMessage( LOG_ERROR_LEVEL, "CRUS system failed, aborting [%d]", err );
			fclose( fhandle );
			return( -1 );
		}
	}

	// Now walk the the table of files to validate
	for (i=0; i<CART_SIM_MAX_OPEN_FILES; i++) {
		if (ftable[i].filename != NULL) {
			if (validate_file(ftable[i].filename, ftable[i].fhandle) != 0) {
				logMessage(LOG_ERROR_LEVEL, "CART Validation failed on file [%s].", ftable[i].filename,fname);
				fclose( fhandle );
				return(-1);
			}
		}		
	}

	// Shut down the interface
	if (cart_poweroff() == -1) {
		logMessage( LOG_ERROR_LEVEL, "CART simulator failed shutdown.");
		fclose( fhandle );
		return( -1 );
	}
	logMessage(CartSimulatorLLevel, "CART simulator shutdown complete.");
	logMessage(LOG_OUTPUT_LEVEL, "CART simulation: all tests successful!!!.");

	// Close the workload file, successfully
	fclose( fhandle );
	return( 0 );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : validate_file
// Description  : Vadliate a file in the filesystem
//
// Inputs       : fname - the name of the file to validate
//                mfh - the memory file handle
// Outputs      : 0 if successful test, -1 if failure

int validate_file(char *fname, int16_t mfh) {

	// Local variables
	char filename[256], bkfile[256], *filbuf, *membuf;
	struct stat stats;
	int idx, fh;

	// First figure out how big the file is, setup buffer
	snprintf(filename, 256, "%s/%s", CART_WORKLOAD_DIR, fname);
	logMessage(LOG_OUTPUT_LEVEL, "Validating [%s] file ....", fname);
	if ((stat(filename, &stats) != 0) || (stats.st_size == 0)) {
		logMessage(LOG_ERROR_LEVEL, "Failure validating file [%s], missing or "
			"unknown source.", filename);
		return(-1);		
	}
	if ( ((filbuf = malloc(stats.st_size)) == NULL) || ((membuf = malloc(stats.st_size)) == NULL) ) {
		logMessage(LOG_ERROR_LEVEL, "Failure validating file [%s], failed "
			"buffer allocation.", filename);
		return(-1);		
	}

	// Now open the file and read the contents
	if ((fh=open(filename, O_RDONLY)) == -1) {
		logMessage(LOG_ERROR_LEVEL, "Failure validating file [%s], open failed ", filename);
		return(-1);		
	}
	if ((read(fh, filbuf, stats.st_size)) == -1) {
		logMessage(LOG_ERROR_LEVEL, "Failure validating file [%s], read failed ", filename);
		return(-1);
	}
	close(fh);

	// Seek to the beginning of the memory file, read the contents
	if (cart_seek(mfh, 0) == -1) {
		// Failed, error out
		logMessage(LOG_ERROR_LEVEL, "Read cart file [%s] see to zero failed.", fname);
		return(-1);
	}
	if (cart_read(mfh, membuf, stats.st_size) != stats.st_size) {
		// Failed, error out
		logMessage(LOG_ERROR_LEVEL, "Read cart file [%s] of length %d failed.", fname, stats.st_size);
		return(-1);
	}

	// Now create a backup of the memory file so people can debug
	snprintf(bkfile, 256, "%s/%s.cmm", CART_WORKLOAD_DIR, fname);
	if ((fh=open(bkfile, O_RDWR|O_CREAT|O_TRUNC, S_IRWXU)) == -1) {
		logMessage(LOG_ERROR_LEVEL, "Failure creating backup file [%s], open failed (%s) ", 
			bkfile, strerror(errno));
		return(-1);		
	}
	if ((write(fh, membuf, stats.st_size)) == -1) {
		logMessage(LOG_ERROR_LEVEL, "Failure writing backup file [%s].", bkfile);
		return(-1);
	}
	close(fh);


	// Now walk the buffers and compare byte for byte
	for (idx=0; idx<stats.st_size; idx++) {
		if (membuf[idx] != filbuf[idx]) {
			logMessage(LOG_ERROR_LEVEL, "Validation of [%s] failed at offset %d (mem %x/'%c' "
				"!= fil %x/'%c'", fname, idx, membuf[idx], membuf[idx], filbuf[idx], filbuf[idx]);
			return(-1);
		}
	}

	// Free the buffers, log success, and return successfully
	free(filbuf);
	free(membuf);
	logMessage(LOG_OUTPUT_LEVEL, "Validation of [%s], length %d sucessful.", fname, stats.st_size);
	return( 0 );
}
