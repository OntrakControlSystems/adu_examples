from ontrak import aduhid

# Example of getting the number of connected ADU devices
num_adu = aduhid.count(100)
print('Number of ADU devices connected: %i' % (num_adu))

# Get a device list of connected ADUs. List will be empty if no devices are connected.
device_list = aduhid.device_list(100)
for device in device_list:
	print('Vendor ID: %i, Product ID: %i, Serial Number: %s' % (device.vendor_id, device.product_id, device.serial_number))

# Show the device list in a windows GUI popup. Return information on the selected device
selected_device_info = aduhid.show_device_list('Line 1\nLine 2')

# Open the device selected in the list
device_handle = aduhid.open_device_by_serial_number(selected_device_info.serial_number, 100)

# Or by product id
# device_handle = aduhid.open_device_by_product_id(selected_device_info.product_id, 100)

# Write a command to the device
result = aduhid.write_device(device_handle, 'RK0', 100)
print('Write result: %i' % result) # Should be non-zero if successful

result = aduhid.write_device(device_handle, 'SK0', 100)
print('Write result: %i' % result) # Should be non-zero if successful

# Comment the next two lines to see what happens when attempting a read without first issuing a resulting command
result = aduhid.write_device(device_handle, 'RPK0', 100)
print('Write result: %i' % result) # Should be non-zero if successful

# Read from device
(result, value) = aduhid.read_device(device_handle, 100)	

# Result will contain the returned value from the device in integer form 
# If read is not successful, result is 0 and value is None
if result != 0:
	print('Read result: %i, value: %i' % (result, value)) 
else:
	print('Read failed - was a resulting command issued prior to the read?') 

aduhid.close_device(device_handle)
