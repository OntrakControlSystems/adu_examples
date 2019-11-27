from ctypes import *
from ctypes.wintypes import *
import platform
import os
ontrak_vendor_id = 0x0A07
INVALID_HANDLE_VALUE = c_void_p(-1).value

class ADU_DEVICE_ID(Structure):
	_fields_ = [
		("vendor_id", c_ushort),
		("product_id", c_ushort),
		("serial_number", c_char * 7)
	]

os.environ['PATH'] = os.path.dirname(__file__) + ';' + os.environ['PATH']
arch = platform.architecture()
if arch[0] == '32bit':
	adu_lib = WinDLL('AduHid')
elif arch[0] == '64bit':
	adu_lib = WinDLL('AduHid64')

_adu_count = adu_lib.ADUCount
_adu_count.restype = c_int
_adu_count.argtypes = [c_ulong]

_show_adu_device_list = adu_lib.ShowAduDeviceList
_show_adu_device_list.restype = None
_show_adu_device_list.argtypes = [POINTER(ADU_DEVICE_ID), c_char_p]

_get_adu_device_list = adu_lib.GetAduDeviceList
_get_adu_device_list.restype = None
_get_adu_device_list.argtypes = [POINTER(ADU_DEVICE_ID), c_ushort, c_ulong, POINTER(c_ushort), POINTER(BOOL)]

_open_adu_device = adu_lib.OpenAduDevice
_open_adu_device.restype = POINTER(HANDLE)
_open_adu_device.argtypes = [c_ulong]

_open_adu_device_by_product_id = adu_lib.OpenAduDeviceByProductId
_open_adu_device_by_product_id.restype = POINTER(HANDLE)
_open_adu_device_by_product_id.argtypes = [c_int, c_ulong]

_open_adu_device_by_serial_number = adu_lib.OpenAduDeviceBySerialNumber
_open_adu_device_by_serial_number.restype = POINTER(HANDLE)
_open_adu_device_by_serial_number.argtypes = [c_char_p, c_ulong]

_close_adu_device = adu_lib.CloseAduDevice
_close_adu_device.restype = None
_close_adu_device.argtypes = [POINTER(HANDLE)]

_write_adu_device = adu_lib.WriteAduDevice
_write_adu_device.restype = c_int
_write_adu_device.argtypes = [POINTER(HANDLE), c_char_p, c_ulong, POINTER(c_ulong), c_ulong]

_read_adu_device = adu_lib.ReadAduDevice
_read_adu_device.restype = c_int
_read_adu_device.argtypes = [POINTER(HANDLE), c_char_p, c_ulong, POINTER(c_ulong), c_ulong]

adu_handle_type = POINTER(HANDLE)

def count(timeout):
	return _adu_count(timeout)

def device_list(timeout):
	num_adus = _adu_count(timeout)
	device_list = (ADU_DEVICE_ID * num_adus)()
	num_devices_found = c_ushort()
	result = BOOL()
	_get_adu_device_list(device_list, num_adus, timeout, byref(num_devices_found), byref(result))
	return device_list

def show_device_list(header_string):
	device_id = ADU_DEVICE_ID()
	_show_adu_device_list(device_id, c_char_p(header_string))
	return device_id

def open_device_by_product_id(product_id, timeout):
	device_handle = _open_adu_device_by_product_id(product_id, timeout)
	return device_handle

def open_device_by_serial_number(serial_number, timeout):
	device_handle = _open_adu_device_by_serial_number(serial_number, timeout)
	return device_handle
	
def close_device(device_handle):
	_close_adu_device(device_handle)

def write_device(device_handle, command, timeout):
	bytes_written = c_ulong()
	result = _write_adu_device(device_handle, c_char_p(command), len(command), byref(bytes_written), timeout)
	return result
	
def read_device(device_handle, timeout):
	read_string = '#' * 7
	read_buffer = c_char_p(read_string)
	bytes_read = c_ulong()
	result = _read_adu_device(device_handle, read_buffer, 7, byref(bytes_read), timeout)

	if result == 0: # unsuccessful read
		return (result, None)

	return (result, int(read_string.rstrip('\x00')))
	