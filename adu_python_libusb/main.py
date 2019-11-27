import os
#os.environ['PYUSB_DEBUG'] = 'debug' # uncomment for verbose pyusb output
import sys
import platform
import usb.core
import usb.backend.libusb1

VENDOR_ONTRAK = 0x0a07 # OnTrak Control Systems Inc. vendor ID
#PRODUCT_ADU222 = 222 # ADU222 Device product name - change this to match your product
PRODUCT_ADU222 = 208 # ADU222 Device product name - change this to match your product

def send_message(dev, msg_str):
    print("Sending message: {}".format(msg_str))
    # message structure:
    #   message is an ASCII string containing the command
    #   8 bytes in length
    #   0th byte must always be 0x01
    #   bytes 1 to 7 are ASCII character values representing the command
    #   remainder of message is padded to 8 bytes with character code 0
    byte_str = chr(0x01) + msg_str + chr(0) * max(7 - len(msg_str), 0)

    num_bytes_written = 0

    try:
        num_bytes_written = dev.write(0x01, byte_str)
    except usb.core.USBError as e:
        print (e.args)

    return num_bytes_written

def read_message(dev):
    try:
        data = dev.read(0x81, 64, 500) # timeout of 500 milliseconds
    except usb.core.USBError as e:
        print ("Error reading response: {}".format(e.args))
        return None

    byte_str = "".join(chr(n) for n in data[1:])
    print ("Read response: {}".format(byte_str))
    return byte_str

def main():
    was_kernel_driver_active = False

    if platform.system() == 'Windows':
        # required for Windows only
        # libusb DLLs from: https://sourceforge.net/projects/libusb/
        arch = platform.architecture()
        if arch[0] == '32bit':
            backend = usb.backend.libusb1.get_backend(find_library=lambda x: "libusb/x86/libusb-1.0.dll") # 32-bit DLL, select the appropriate one based on your Python installation
        elif arch[0] == '64bit':
            backend = usb.backend.libusb1.get_backend(find_library=lambda x: "libusb/x64/libusb-1.0.dll") # 64-bit DLL

        device = usb.core.find(backend=backend, idVendor=VENDOR_ONTRAK, idProduct=PRODUCT_ADU222)
    elif platform.system() == 'Linux':
        # Linux only
        # if the OS kernel already claimed the device, which is most likely true
        if device.is_kernel_driver_active() is True:
            # tell the kernel to detach
            device.detach_kernel_driver(0)
            was_kernel_driver_active = True

        device = usb.core.find(idVendor=VENDOR_ONTRAK, idProduct=PRODUCT_ADU222)
    else:
        device = usb.core.find(idVendor=VENDOR_ONTRAK, idProduct=PRODUCT_ADU222)

    if device is None:
        raise ValueError('ADU Device not found. Please ensure it is connected to the tablet.')
        sys.exit(1)

    device.reset()

    # set the active configuration. With no arguments, the first
    # configuration will be the active one
    device.set_configuration()

    usb.util.claim_interface(device, 0) # Windows only

    #bytes_written = send_message('MK3') # set PORT K to decimal value 3, note: device does not send a response for this command (see below)
    bytes_written = send_message(device, 'SK0') # set PORT K to decimal value 3, note: device does not send a response for this command (see below)
    bytes_written = send_message(device, 'RK0') # set PORT K to decimal value 3, note: device does not send a response for this command (see below)

    # uncommenting the two lines below will result in a timeout error on the device.read() as the previously sent MKd command
    # does not cause the ADU device to send a response. In order to avoid this behaviour, ensure that reads are only performed 
    # when a response should be available, such as with 'PK' messages. Review the documentation for your specific ADU device
    # for a list of commands that have return values.
    #data = read_message() # timeout error
    #print("Received: {}".format(data))

    #bytes_written = send_message('PK') # request the value of PORTK in decimal format
    bytes_written = send_message(device, 'RPK0') # request the value of PORTK in decimal format
    data = read_message(device)
    print("Received: {}".format(data))

    #bytes_written = send_message('WD') # request the watchdog setting 
    #data = read_message()
    #print("Received: {}".format(data))

    usb.util.release_interface(device, 0)

    if was_kernel_driver_active == True:
        device.attach_kernel_driver(0)

if __name__ == "__main__":
	main()


