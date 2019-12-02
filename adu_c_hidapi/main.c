// NOTE: when running the example, it must be run with root privileges in order to access the USB device.

// libhidapi library must be available. It can be installed on Debian/Ubuntu using apt-get install libhidapi-dev
#include "hidapi/hidapi.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define VENDOR_ID      0x0a07  // Ontrak vendor ID. This should never change
#define PRODUCT_ID     200     // ADU200 product ID. Set this product ID to match your device.
							   // Product IDs can be found at https://www.ontrak.net/Nodll.htm.

#define COMMAND_SIZE   8	   // ADU commands are 8 bytes in length

#define COUNT_OF(array) (sizeof(array) / sizeof(array[0]))

// Write a command to an ADU device by creating a command buffer based on the provided command string
// Returns -1 on failure, number of bytes written on success
int write_to_adu( hid_device * _handle, const char * _cmd )
{
	const int command_len = strlen( _cmd ); // Get the length of the command string we are sending

	// Buffer to hold the command we will send to the ADU device.
	unsigned char buffer[ COMMAND_SIZE ];
	int bytes_sent;

	if ( command_len > COMMAND_SIZE - 1 )
	{
		printf( "Error: command is larger than our limit of %i\n", COMMAND_SIZE - 1 );
		return -1;
	}

    // Message structure:
    //   message is an ASCII string containing the command
    //   8 bytes in length
    //   0th byte must always be 0x01 (decimal 1)
    //   bytes 1 to 7 are ASCII character values representing the command
    //   remainder of message is padded to 8 bytes with character code 0

	memset( buffer, 0, COMMAND_SIZE ); // Zero out buffer to pad with null values (command buffer needs to be padded with 0s)

	buffer[ 0 ] = 0x01; // First byte of the command buffer needs to be set to a decimal value of 1

	// Copy the command ASCII bytes into our buffer, starting at the second byte (we need to leave the first byte as decimal value 1)
	memcpy( &buffer[ 1 ], _cmd, command_len );

	// Attempt to write the command to the device
	bytes_sent = hid_write( _handle, buffer, COMMAND_SIZE );

	if ( bytes_sent == -1 ) // Was the write successful?
	{
		wprintf( L"Error writing to device: %s\n", hid_error( _handle ) );
	}

	return bytes_sent;
}

// Read a command from an ADU device with a specified timeout
// It is expected that _read_str is of size 8 bytes (or greater)
int read_from_adu( hid_device * _handle, char * _read_str, int _read_str_len, int _timeout )
{
	if ( _read_str == NULL || _read_str_len < 8 )
	{
		return -2;
	}

	int bytes_read;

	// Buffer to hold the command we will receive from the ADU device
	// Its size is set to the transfer size for low or full speed USB devices (ADU model specific - see defines at top of file)
	unsigned char buffer[ COMMAND_SIZE ];
	memset( buffer, 0, COMMAND_SIZE ); // Zero out buffer to pad with null values (command buffer needs to be padded with 0s)

	// Attempt to read the result from the device
	bytes_read = hid_read_timeout( _handle, buffer, COMMAND_SIZE, _timeout );

	if ( bytes_read == -1 ) // Was the interrupt transfer successful?
	{
		wprintf( L"Error reading interrupt transfer: %s\n", hid_error( _handle ) );
		return -1;
	}

	// The buffer should now hold the data read from the ADU device. The first byte will contain 0x01, the remaining bytes
	// are the returned value in string format. Let's copy the string from the read buffer, starting at index 1, to our _read_str buffer 
	memcpy( _read_str, &buffer[1], 7 );
	buffer[7] = '\0'; // null terminate the string

	return bytes_read; // returns 0 on success, a negative number specifying the libusb error otherwise
}


int main( int argc, char * argv[] )
{
	hid_device * handle;
	char value_str[8]; // 8-byte buffer to store string values read from device (7 byte string + NULL terminating character)
	int result; // HIDAPI function call result

	// Initialize the HIDAPI library
	result = hid_init();

	// Open the device using the VID, PID,
	// and optionally the Serial number.
	handle = hid_open( VENDOR_ID, PRODUCT_ID, NULL );

	if ( handle == NULL )
	{
		wprintf( L"Unable to open ADU device with product ID %i. Please check that it is connected. If it is connected, ensure you are running the example with root privileges such as with sudo.\n", PRODUCT_ID );
		result = hid_exit();
		exit( -1 );
	}

	// Let's write to and read from the device:

	// write_to_adu() return the number of bytes written, or -1 on failure
	result = write_to_adu( handle, "RK0" ); // reset relay 0
	result = write_to_adu( handle, "SK0" ); // close relay 0

	// Send a command to request the value of PORT A. We'll need to read the result using read_from_adu() 
	result = write_to_adu( handle, "RPA" );

	// Read the result
	// read_from_adu() takes four parameters, the device object, a destination buffer to store the string result,
	// the length of this buffer (must be 8 bytes or larger), and a timeout in milliseconds
	if ( -1 != read_from_adu( handle, value_str, COUNT_OF(value_str), 200 ) )
	{
		wprintf( L"Read value as string: %s\n", value_str );

		// The data we are interested in follows in ASCII form. We will need to convert the ASCII data to an integer
		// in order for it to be usable to us in a numeric format. The buffer is null terminated, and as such atoi()
		// can be used for this.

		// Convert the ASCII string result stored in the buffer's 2nd index and onward from a string to an integer
		// and store it in the read_value pointer user argument
		int value_int = atoi( value_str );
		wprintf( L"Read value as int: %i\n", value_int );
	}

	// Close the device
	hid_close( handle );

	// Finalize the HIDAPI library
	result = hid_exit();

	return 0;
}
