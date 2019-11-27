// NOTE: when running the example, it must be run with root privileges in order to access the USB device.

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

// libusb library must be available. It can be installed on Debian/Ubuntu using apt-get install libusb-1.0-0-dev
#include <libusb-1.0/libusb.h>

#define VENDOR_ID      0x0a07  // Ontrak vendor ID. This should never change
#define PRODUCT_ID     208     // ADU208 product ID. Set this product ID to match your device.
                               // Product IDs can be found at https://www.ontrak.net/Nodll.htm.

//#define TRANSFER_SIZE    64  // Data transfer size is either 8 bytes (low-speed USB devices) or 64 bytes (full-speed USB devices).
                               // Set this to 8 or 64 depending on the ADU device you are using (see https://www.ontrak.net/Nodll.htm).
                               // You can also type `dmesg` in your terminal immediately after connecting an ADU to your computer.
                               // This should list whether it is low-speed or full-speed.

#define TRANSFER_SIZE    8     // ADU208 is a low-speed device, so we must use 8 byte transfers

// Write a command to an ADU device with a specified timeout
int write_to_adu( libusb_device_handle * _device_handle, const char * _cmd, int _timeout )
{
    const int command_len = strlen( _cmd ); // Get the length of the command string we are sending

    int bytes_sent = 0;

    // Buffer to hold the command we will send to the ADU device.
    // Its size is set to the transfer size for low or full speed USB devices (ADU model specific - see defines at top of file)
    unsigned char buffer[ TRANSFER_SIZE ]; 

    if ( command_len > TRANSFER_SIZE )
    {
        printf( "Error: command is larger than our limit of %i\n", TRANSFER_SIZE );
        return -1;
    }

    memset( buffer, 0, TRANSFER_SIZE ); // Zero out buffer to pad with null values (command buffer needs to be padded with 0s)

    buffer[0] = 0x01; // First byte of the command buffer needs to be set to a decimal value of 1

    // Copy the command ASCII bytes into our buffer, starting at the second byte (we need to leave the first byte as decimal value 1)
    memcpy( &buffer[1], _cmd, command_len ); 

    // Attempt to send the command to the OUT endpoint (0x01) with the use specified millisecond timeout
    int result = libusb_interrupt_transfer( _device_handle, 0x01, buffer, TRANSFER_SIZE, &bytes_sent, _timeout );
    printf( "Write '%s' result: %i, Bytes sent: %u\n", _cmd, result, bytes_sent );

    if ( result < 0 ) // Was the interrupt transfer successful?
    {
        printf( "Error sending interrupt transfer: %s\n", libusb_error_name( result ) );
    }

    return result; // Returns 0 on success, a negative number specifying the libusb error otherwise
}

// Read a command from an ADU device with a specified timeout
int read_from_adu( libusb_device_handle * _device_handle, int * _read_value, int _timeout )
{
    int bytes_read = 0;

    // Buffer to hold the command we will receive from the ADU device
    // Its size is set to the transfer size for low or full speed USB devices (ADU model specific - see defines at top of file)
    unsigned char buffer[ TRANSFER_SIZE ];

    memset( buffer, 0, TRANSFER_SIZE ); // Zero out buffer to pad with null values (command buffer needs to be padded with 0s)

    // Attempt to read the result from the IN endpoint (0x81) with user specified timeout
    int result = libusb_interrupt_transfer( _device_handle, 0x81, buffer, TRANSFER_SIZE, &bytes_read, _timeout );
    printf( "Read result: %i, Bytes read: %u\n", result, bytes_read );

    if ( result < 0 ) // Was the interrupt transfer successful?
    {
        printf( "Error reading interrupt transfer: %s\n", libusb_error_name( result ) );
        return result;
    }

    // Our buffer should now hold the data read from the ADU device. The first byte will be a value of decimal 1.
    // The data we are interested in follows in ASCII form. We will need to convert the ASCII data to an integer
    // in order for it to be usable to us in a numeric format. The buffer is null terminated, and as such atoi()
    // can be used for this.

    // Convert the ASCII string result stored in the buffer's 2nd index and onward from a string to an integer
    // and store it in the read_value pointer user argument
    *_read_value = atoi( (const char *)&buffer[1] );

    return result; // returns 0 on success, a negative number specifying the libusb error otherwise
}


int main( int argc, char **argv )
{
    struct libusb_device_handle * device_handle = NULL; // Our ADU's USB device handle
    int result;
    int value; // to hold our value read from the ADU

    // Initialize libusb
    result = libusb_init( NULL );
    if ( result < 0 )
    {
        printf( "Error initializing libusb: %s\n", libusb_error_name( result ) );
        exit( -1 );
    }

    // Set debugging output to max level
    libusb_set_option( NULL, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_WARNING );

    // Open our ADU device that matches our vendor id and product id
    device_handle = libusb_open_device_with_vid_pid( NULL, VENDOR_ID, PRODUCT_ID );
    if ( !device_handle )
    {
        printf( "Error finding USB device\n" );
        libusb_exit( NULL );
        exit( -2 );
    }

    // Enable auto-detaching of the kernel driver.
    // If a kernel driver currently has an interface claimed, it will be automatically be detached
    // when we claim that interface. When the interface is restored, the kernel driver is allowed
    // to be re-attached. This can alternatively be manually done via libusb_detach_kernel_driver().
    libusb_set_auto_detach_kernel_driver( device_handle, 1 );

    // Claim interface 0 on the device
    result = libusb_claim_interface( device_handle, 0 );
    if ( result < 0 )
    {
        printf( "Error claiming interface: %s\n", libusb_error_name( result ) );
        if ( device_handle )
        {
            libusb_close( device_handle );
        }

        libusb_exit( NULL );
        exit( -3 );
    }

    // Read until there is nothing left to read from the device. This is to clear
    // any pending reads that was not initiated by us. This will print out an LIBUSB ERROR TIMEOUT
    // when there is no longer any data waiting for us
    while ( 0 == read_from_adu( device_handle, &value, 200 ) )
    {
    }
 
    // Now that we have our device and have claimed interface 0, we can write to and read from it:
    result = write_to_adu( device_handle, "RK0", 200 ); // reset relay 0

    // We aren't checking the result of write_to_adu, but it will return 0 on success and a negative number on failure.
    result = write_to_adu( device_handle, "SK0", 200 ); // close relay 0

    // Important note: if a responsive command is issued, such as RPK0 below, a read must immediately follow it to
    // obtain the result. If we do not do perform a read and continue issuing writes, the device may be disconnected
    // by the OS and need to be reset.

    // Send a command to request value of relay 0 (should be 1 as we closed the relay with SK0)
    result = write_to_adu( device_handle, "RPK0", 200 );
    if ( 0 == read_from_adu( device_handle, &value, 200 ) ) // Read the result
    {
        printf( "Read value: %i\n", value );
    }

    result = write_to_adu( device_handle, "MK255", 200 ); // Set PORT K to 255
    result = write_to_adu( device_handle, "PK", 200 ); // Read PORT K value
    if ( 0 == read_from_adu( device_handle, &value, 200 ) ) // Read the result (should be 255)
    {
        printf( "Read value: %i\n", value );
    }

    // We are done with our device and will now release the interface we previously claimed as well as the device
    libusb_release_interface( device_handle, 0 );
    libusb_close( device_handle );

    // Shutdown libusb
    libusb_exit( NULL );

    return 0;
}
