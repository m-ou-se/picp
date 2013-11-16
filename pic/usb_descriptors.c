#define __USB_DESCRIPTORS_C

#include <USB/usb.h>
#include <USB/usb_function_cdc.h>

ROM USB_DEVICE_DESCRIPTOR device_dsc = {
	0x12,
	USB_DESCRIPTOR_DEVICE,
	0x0200,     // USB 2.0
	CDC_DEVICE, // class
	0x00,       // subclass
	0x00,       // protocol
	USB_EP0_BUFF_SIZE,
	0x0000,     // vendor ID
	0x0000,     // product ID
	0x0100,     // device release number
	0x01,       // manufacturer string index
	0x02,       // product string index
	0x00,       // device serial number string index
	0x01        // number of configurations
};

ROM BYTE configDescriptor1[] = {
	// Configuration Descriptor
	0x09,
	USB_DESCRIPTOR_CONFIGURATION,
	67, 0,            // total size of this configuration
	2,                // number of interfaces
	1,                // index of this configuration
	0,                // configuration string index
	_DEFAULT | _SELF, // attributes
	50,               // max power consumption (times 2 mA)

	// Interface Descriptor
	9,
	USB_DESCRIPTOR_INTERFACE,
	0,                      // interface number
	0,                      // Alternate setting number
	1,                      // number of endpoints in this interface
	COMM_INTF,              // class code
	ABSTRACT_CONTROL_MODEL, // subclass code
	0,                      // protocol code
	0,                      // interface string index

	// CDC Class-Specific Descriptors
	sizeof(USB_CDC_HEADER_FN_DSC),
	CS_INTERFACE,
	DSC_FN_HEADER,
	0x10,0x01,

	sizeof(USB_CDC_ACM_FN_DSC),
	CS_INTERFACE,
	DSC_FN_ACM,
	USB_CDC_ACM_FN_DSC_VAL,

	sizeof(USB_CDC_UNION_FN_DSC),
	CS_INTERFACE,
	DSC_FN_UNION,
	CDC_COMM_INTF_ID,
	CDC_DATA_INTF_ID,

	sizeof(USB_CDC_CALL_MGT_FN_DSC),
	CS_INTERFACE,
	DSC_FN_CALL_MGT,
	0x00,
	CDC_DATA_INTF_ID,

	// Endpoint Descriptor
	0x07,
	USB_DESCRIPTOR_ENDPOINT,
	_EP01_IN,   // endpoint address
	_INTERRUPT, // attributes
	0x08,0x00,  // size
	0x02,       // interval

	// Interface Descriptor
	0x09,
	USB_DESCRIPTOR_INTERFACE,
	1,           // interface number
	0,           // Alternate setting number
	2,           // number of endpoints in this interface
	DATA_INTF,   // class code
	0,           // subclass code
	NO_PROTOCOL, // protocol code
	0,           // interface string index

	// Endpoint Descriptor
	0x07,
	USB_DESCRIPTOR_ENDPOINT,
	_EP02_OUT,  // endpoint address
	_BULK,      // attributes
	0x40, 0x00, // size
	0x00,       // interval

	// Endpoint Descriptor
	0x07,
	USB_DESCRIPTOR_ENDPOINT,
	_EP02_IN,   // endpoint address
	_BULK,      // attributes
	0x40, 0x00, // size
	0x00,       // interval
};

// Language code string descriptor
ROM struct{BYTE bLength;BYTE bDscType;WORD string[1];} sd000 = {
	sizeof(sd000), USB_DESCRIPTOR_STRING,
	{0x0409}
};

// Manufacturer string descriptor
ROM struct { BYTE size; BYTE type; WORD string[26]; } sd001 = {
	sizeof(sd001), USB_DESCRIPTOR_STRING,
	{'M','a','r','a',' ','B','o','s',' ','<','m','-','o','u','.','s','e','@','m','-','o','u','.','s','e','>'}
};

// Product string descriptor
ROM struct { BYTE size; BYTE type; WORD string[21]; } sd002 = {
	sizeof(sd002), USB_DESCRIPTOR_STRING,
	{'P','I','C','1','6','F','1','4','5','x',' ','P','r','o','g','r','a','m','m','e','r'}
};

// Array of configuration descriptors
ROM BYTE * ROM USB_CD_Ptr[] = {
	(ROM BYTE *ROM)&configDescriptor1
};

// Array of string descriptors
ROM BYTE * ROM USB_SD_Ptr[] = {
	(ROM BYTE * ROM)&sd000,
	(ROM BYTE * ROM)&sd001,
	(ROM BYTE * ROM)&sd002
};

