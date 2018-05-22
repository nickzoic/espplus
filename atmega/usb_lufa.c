// usb_lufa.c : LUFA-based implementation of the usb interface
//
// This is a horrible piece of hackery that owes its life to the LUFA
// project, especially the VirtualSerial and GenericHID examples.

#include "usb.h"

#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/pgmspace.h>

#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Platform/Platform.h>

// Descriptors.h

/* Macros: */
/** Endpoint address of the CDC interface's device-to-host data IN endpoint. */
#define CDC_TX_EPADDR                 (ENDPOINT_DIR_IN  | 1)

/** Endpoint address of the CDC interface's host-to-device data OUT endpoint. */
#define CDC_RX_EPADDR                 (ENDPOINT_DIR_OUT | 2)

/** Endpoint address of the CDC interface's device-to-host notification IN endpoint. */
#define CDC_NOTIFICATION_EPADDR       (ENDPOINT_DIR_IN  | 3)

/** Endpoint address of the Generic HID reporting IN endpoint. */
#define GENERIC_IN_EPADDR         (ENDPOINT_DIR_IN  | 4)

/** Endpoint address of the Generic HID reporting OUT endpoint. */
#define GENERIC_OUT_EPADDR        (ENDPOINT_DIR_OUT | 5)

/** Size in bytes of the CDC device-to-host notification IN endpoint. */
#define CDC_NOTIFICATION_EPSIZE        8
#define GENERIC_EPSIZE                 8

/** Size in bytes of the CDC data IN and OUT endpoint. */
#define CDC_TXRX_EPSIZE              64
#define GENERIC_REPORT_SIZE            64

/* Type Defines: */
/** Type define for the device configuration descriptor structure. This must be defined in the
 *  application code, as the configuration descriptor contains several sub-descriptors which
 *  vary between devices, and which describe the device's usage to the host.
 */
typedef struct
{
    USB_Descriptor_Configuration_Header_t    Config;

    // CDC Control Interface
    /*USB_Descriptor_Interface_Association_t   CDC_IAD;*/
    USB_Descriptor_Interface_t               CDC_CCI_Interface;
    USB_CDC_Descriptor_FunctionalHeader_t    CDC_Functional_Header;
    USB_CDC_Descriptor_FunctionalACM_t       CDC_Functional_ACM;
    USB_CDC_Descriptor_FunctionalUnion_t     CDC_Functional_Union;
    USB_Descriptor_Endpoint_t                CDC_ManagementEndpoint;

    // CDC Data Interface
    USB_Descriptor_Interface_t               CDC_DCI_Interface;
    USB_Descriptor_Endpoint_t                CDC_DataOutEndpoint;
    USB_Descriptor_Endpoint_t                CDC_DataInEndpoint;

    // Generic HID Interface
    USB_Descriptor_Interface_t            HID_Interface;
    USB_HID_Descriptor_HID_t              HID_GenericHID;
    USB_Descriptor_Endpoint_t             HID_ReportINEndpoint;
    USB_Descriptor_Endpoint_t             HID_ReportOUTEndpoint;

} USB_Descriptor_Configuration_t;

/** Enum for the device interface descriptor IDs within the device. Each interface descriptor
 *  should have a unique ID index associated with it, which can be used to refer to the
 *  interface from other descriptors.
 */
enum InterfaceDescriptors_t
{
    INTERFACE_ID_CDC_CCI = 0, /**< CDC CCI interface descriptor ID */
    INTERFACE_ID_CDC_DCI = 1, /**< CDC DCI interface descriptor ID */
    INTERFACE_ID_GenericHID = 2, /**< GenericHID interface descriptor ID */
};

/** Enum for the device string descriptor IDs within the device. Each string descriptor should
 *  have a unique ID index associated with it, which can be used to refer to the string from
 *  other descriptors.
 */
enum StringDescriptors_t
{
    STRING_ID_Language     = 0, /**< Supported Languages string descriptor ID (must be zero) */
    STRING_ID_Manufacturer = 1, /**< Manufacturer string ID */
    STRING_ID_Product      = 2, /**< Product string ID */
};

/* Function Prototypes: */
//uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
//                                   const uint16_t wIndex,
//                                    const void** const DescriptorAddress)
//ATTR_WARN_UNUSED_RESULT ATTR_NON_NULL_PTR_ARG(3);

#define USB_BOS_DESCRIPTOR_TYPE (15)
#define USB_BOS_DESCRIPTOR_LENGTH (57)
#define WINUSB_REQUEST_DESCRIPTOR (0x07)
#define MS_OS_20_DESCRIPTOR_LENGTH (0x1e)
#define WL_REQUEST_WINUSB (0xfc)
#define WL_REQUEST_WEBUSB (0xfe)

const uint8_t USB_BOS_DESCRIPTOR[USB_BOS_DESCRIPTOR_LENGTH] = {
  // BOS descriptor header
  0x05, // bLength of this header
  USB_BOS_DESCRIPTOR_TYPE,
  USB_BOS_DESCRIPTOR_LENGTH, 0x00, // 16 bit length
  0x02, // bNumDeviceCaps

  // WebUSB Platform Capability descriptor
  0x18,  // Descriptor size (24 bytes)
  0x10,  // Descriptor type (Device Capability)
  0x05,  // Capability type (Platform)
  0x00,  // Reserved

  // WebUSB Platform Capability ID (3408b638-09a9-47a0-8bfd-a0768815b665)
  0x38, 0xB6, 0x08, 0x34,
  0xA9, 0x09,
  0xA0, 0x47,
  0x8B, 0xFD,
  0xA0, 0x76, 0x88, 0x15, 0xB6, 0x65,

  0x00, 0x01,         // WebUSB version 1.0
  WL_REQUEST_WEBUSB,  // Vendor-assigned WebUSB request code
  0x01,               // Landing page: https://sowbug.github.io/webusb

  // Microsoft OS 2.0 Platform Capability Descriptor
  // Thanks http://janaxelson.com/files/ms_os_20_descriptors.c
  0x1C,  // Descriptor size (28 bytes)
  0x10,  // Descriptor type (Device Capability)
  0x05,  // Capability type (Platform)
  0x00,  // Reserved

  // MS OS 2.0 Platform Capability ID (D8DD60DF-4589-4CC7-9CD2-659D9E648A9F)
  0xDF, 0x60, 0xDD, 0xD8,
  0x89, 0x45,
  0xC7, 0x4C,
  0x9C, 0xD2,
  0x65, 0x9D, 0x9E, 0x64, 0x8A, 0x9F,

  0x00, 0x00, 0x03, 0x06,    // Windows version (8.1) (0x06030000)
  MS_OS_20_DESCRIPTOR_LENGTH, 0x00,
  WL_REQUEST_WINUSB,         // Vendor-assigned bMS_VendorCode
  0x00                       // Doesn’t support alternate enumeration
};

uint8_t MS_OS_20_DESCRIPTOR_SET[MS_OS_20_DESCRIPTOR_LENGTH] = {
  // Microsoft OS 2.0 descriptor set header (table 10)
  0x0A, 0x00,  // Descriptor size (10 bytes)
  0x00, 0x00,  // MS OS 2.0 descriptor set header
  0x00, 0x00, 0x03, 0x06,  // Windows version (8.1) (0x06030000)
  MS_OS_20_DESCRIPTOR_LENGTH, 0x00,  // Size, MS OS 2.0 descriptor set

  // Microsoft OS 2.0 compatible ID descriptor (table 13)
  0x14, 0x00,  // wLength
  0x03, 0x00,  // MS_OS_20_FEATURE_COMPATIBLE_ID
  'W',  'I',  'N',  'U',  'S',  'B',  0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

#define WEBUSB_REQUEST_GET_ALLOWED_ORIGINS (0x01)
#define WEBUSB_REQUEST_GET_URL (0x02)

uint8_t webusb_url[] = { 30, 3, 1, 'n', 'i', 'c', 'k', 'z', 'o', 'i', 'c', '.', 'g', 'i', 't', 'h', 'u', 'b', '.', 'i', 'o', '/', 'e', 's', 'p', 'p', 'l', 'u', 's', '/' };

// Descriptors.c


/** Device descriptor structure. This descriptor, located in FLASH memory, describes the overall
 *  device characteristics, including the supported USB version, control endpoint size and the
 *  number of device configurations. The descriptor is read out by the USB host when the enumeration
 *  process begins.
 */
const USB_Descriptor_Device_t PROGMEM DeviceDescriptor =
{
    .Header                 = {.Size = sizeof(USB_Descriptor_Device_t), .Type = DTYPE_Device},

    .USBSpecification       = VERSION_BCD(1,1,0),
    .Class                  = USB_CSCP_IADDeviceClass,
    .SubClass               = USB_CSCP_IADDeviceSubclass,
    .Protocol               = USB_CSCP_IADDeviceProtocol,

    .Endpoint0Size          = FIXED_CONTROL_ENDPOINT_SIZE,

    .VendorID               = 0x1209,
    .ProductID              = 0xadda,
    .ReleaseNumber          = VERSION_BCD(0,0,1),

    .ManufacturerStrIndex   = STRING_ID_Manufacturer,
    .ProductStrIndex        = STRING_ID_Product,
    .SerialNumStrIndex      = USE_INTERNAL_SERIAL,

    .NumberOfConfigurations = FIXED_NUM_CONFIGURATIONS
};

/** Configuration descriptor structure. This descriptor, located in FLASH memory, describes the usage
 *  of the device in one of its supported configurations, including information about any device interfaces
 *  and endpoints. The descriptor is read out by the USB host during the enumeration process when selecting
 *  a configuration so that the host may correctly communicate with the USB device.
 */
const USB_Descriptor_Configuration_t PROGMEM ConfigurationDescriptor =
{
    .Config =
    {
        .Header                 = {.Size = sizeof(USB_Descriptor_Configuration_Header_t), .Type = DTYPE_Configuration},

        .TotalConfigurationSize = sizeof(USB_Descriptor_Configuration_t),
        .TotalInterfaces        = 2,

        .ConfigurationNumber    = 1,
        .ConfigurationStrIndex  = NO_DESCRIPTOR,

        .ConfigAttributes       = (USB_CONFIG_ATTR_RESERVED | USB_CONFIG_ATTR_SELFPOWERED),

        .MaxPowerConsumption    = USB_CONFIG_POWER_MA(500)
    },

    /*.CDC_IAD =
    {
        .Header                 = {.Size = sizeof(USB_Descriptor_Interface_Association_t), .Type = DTYPE_InterfaceAssociation},

        .FirstInterfaceIndex    = INTERFACE_ID_CDC_CCI,
        .TotalInterfaces        = 2,

        .Class                  = CDC_CSCP_CDCClass,
        .SubClass               = CDC_CSCP_ACMSubclass,
        .Protocol               = CDC_CSCP_ATCommandProtocol,

        .IADStrIndex            = NO_DESCRIPTOR
    },*/

    .CDC_CCI_Interface =
    {
        .Header                 = {.Size = sizeof(USB_Descriptor_Interface_t), .Type = DTYPE_Interface},

        .InterfaceNumber        = INTERFACE_ID_CDC_CCI,
        .AlternateSetting       = 0,

        .TotalEndpoints         = 1,

        .Class                  = CDC_CSCP_CDCClass,
        .SubClass               = CDC_CSCP_ACMSubclass,
        .Protocol               = CDC_CSCP_ATCommandProtocol,

        .InterfaceStrIndex      = NO_DESCRIPTOR
    },

    .CDC_Functional_Header =
    {
        .Header                 = {.Size = sizeof(USB_CDC_Descriptor_FunctionalHeader_t), .Type = DTYPE_CSInterface},
        .Subtype                = CDC_DSUBTYPE_CSInterface_Header,

        .CDCSpecification       = VERSION_BCD(1,1,0),
    },

    .CDC_Functional_ACM =
    {
        .Header                 = {.Size = sizeof(USB_CDC_Descriptor_FunctionalACM_t), .Type = DTYPE_CSInterface},
        .Subtype                = CDC_DSUBTYPE_CSInterface_ACM,

        .Capabilities           = 0x06,
    },

    .CDC_Functional_Union =
    {
        .Header                 = {.Size = sizeof(USB_CDC_Descriptor_FunctionalUnion_t), .Type = DTYPE_CSInterface},
        .Subtype                = CDC_DSUBTYPE_CSInterface_Union,

        .MasterInterfaceNumber  = INTERFACE_ID_CDC_CCI,
        .SlaveInterfaceNumber   = INTERFACE_ID_CDC_DCI,
    },

    .CDC_ManagementEndpoint =
    {
        .Header                 = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},

        .EndpointAddress        = CDC_NOTIFICATION_EPADDR,
        .Attributes             = (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        .EndpointSize           = CDC_NOTIFICATION_EPSIZE,
        .PollingIntervalMS      = 0xFF
    },

    .CDC_DCI_Interface =
    {
        .Header                 = {.Size = sizeof(USB_Descriptor_Interface_t), .Type = DTYPE_Interface},

        .InterfaceNumber        = INTERFACE_ID_CDC_DCI,
        .AlternateSetting       = 0,

        .TotalEndpoints         = 2,

        .Class                  = CDC_CSCP_CDCDataClass,
        .SubClass               = CDC_CSCP_NoDataSubclass,
        .Protocol               = CDC_CSCP_NoDataProtocol,

        .InterfaceStrIndex      = NO_DESCRIPTOR
    },

    .CDC_DataOutEndpoint =
    {
        .Header                 = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},

        .EndpointAddress        = CDC_RX_EPADDR,
        .Attributes             = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        .EndpointSize           = CDC_TXRX_EPSIZE,
        .PollingIntervalMS      = 0x05
    },

    .CDC_DataInEndpoint =
    {
        .Header                 = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},

        .EndpointAddress        = CDC_TX_EPADDR,
        .Attributes             = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        .EndpointSize           = CDC_TXRX_EPSIZE,
        .PollingIntervalMS      = 0x05
    }

};

/** Language descriptor structure. This descriptor, located in FLASH memory, is returned when the host requests
 *  the string descriptor with index 0 (the first index). It is actually an array of 16-bit integers, which indicate
 *  via the language ID table available at USB.org what languages the device supports for its string descriptors.
 */
const USB_Descriptor_String_t PROGMEM LanguageString = USB_STRING_DESCRIPTOR_ARRAY(LANGUAGE_ID_ENG);

/** Manufacturer descriptor string. This is a Unicode string containing the manufacturer's details in human readable
 *  form, and is read out upon request by the host when the appropriate string ID is requested, listed in the Device
 *  Descriptor.
 */
const USB_Descriptor_String_t PROGMEM ManufacturerString = USB_STRING_DESCRIPTOR(L"MicroPython");

/** Product descriptor string. This is a Unicode string containing the product's details in human readable form,
 *  and is read out upon request by the host when the appropriate string ID is requested, listed in the Device
 *  Descriptor.
 */
const USB_Descriptor_String_t PROGMEM ProductString = USB_STRING_DESCRIPTOR(L"EspPlus Board");

/** This function is called by the library when in device mode, and must be overridden (see library "USB Descriptors"
 *  documentation) by the application code so that the address and size of a requested descriptor can be given
 *  to the USB library. When the device receives a Get Descriptor request on the control endpoint, this function
 *  is called so that the descriptor details can be passed back and the appropriate descriptor sent back to the
 *  USB host.
 */
uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint16_t wIndex,
                                    const void** const DescriptorAddress)
{
    const uint8_t  DescriptorType   = (wValue >> 8);
    const uint8_t  DescriptorNumber = (wValue & 0xFF);

    const void* Address = NULL;
    uint16_t    Size    = NO_DESCRIPTOR;

    switch (DescriptorType)
    {
        case DTYPE_Device:
            Address = &DeviceDescriptor;
            Size    = sizeof(USB_Descriptor_Device_t);
            break;
        case DTYPE_Configuration:
            Address = &ConfigurationDescriptor;
            Size    = sizeof(USB_Descriptor_Configuration_t);
            break;
        case DTYPE_String:
            switch (DescriptorNumber)
            {
                case STRING_ID_Language:
                    Address = &LanguageString;
                    Size    = pgm_read_byte(&LanguageString.Header.Size);
                    break;
                case STRING_ID_Manufacturer:
                    Address = &ManufacturerString;
                    Size    = pgm_read_byte(&ManufacturerString.Header.Size);
                    break;
                case STRING_ID_Product:
                    Address = &ProductString;
                    Size    = pgm_read_byte(&ProductString.Header.Size);
                    break;
            }
            break;
	case USB_BOS_DESCRIPTOR_TYPE:
	    Address = &USB_BOS_DESCRIPTOR;
	    Size = USB_BOS_DESCRIPTOR_LENGTH;
	    break;
	case WL_REQUEST_WEBUSB: 
            switch (wIndex) {
                case WEBUSB_REQUEST_GET_ALLOWED_ORIGINS:
                case WEBUSB_REQUEST_GET_URL:
                    Address = webusb_url;
                    Size = sizeof(webusb_url);
            }
            break;
        case WL_REQUEST_WINUSB:
            switch (wIndex) {
                case WINUSB_REQUEST_DESCRIPTOR:
                    Address = MS_OS_20_DESCRIPTOR_SET;
                    Size = MS_OS_20_DESCRIPTOR_LENGTH;
            }
	    break;

    }

    *DescriptorAddress = Address;
    return Size;
}


// Application.c

/** \file
 *
 *  Main source file for the DualVirtualSerial demo. This file contains the main tasks of the demo and
 *  is responsible for the initial application hardware configuration.
 */

/** Contains the current baud rate and other settings of the first virtual serial port. While this demo does not use
 *  the physical USART and thus does not use these settings, they must still be retained and returned to the host
 *  upon request or the host will assume the device is non-functional.
 *
 *  These values are set by the host via a class-specific request, however they are not required to be used accurately.
 *  It is possible to completely ignore these value or use other settings as the host is completely unaware of the physical
 *  serial link characteristics and instead sends and receives data in endpoint streams.
 */
static CDC_LineEncoding_t LineEncoding = { .BaudRateBPS = 0,
                                           .CharFormat  = CDC_LINEENCODING_OneStopBit,
                                           .ParityType  = CDC_PARITY_None,
                                           .DataBits    = 8
                                         };

usb_recv_t *_usb_recv = NULL;

void usb_init(usb_recv_t *usb_recv) {

    _usb_recv = usb_recv;

#if (ARCH == ARCH_AVR8)
    /* Disable watchdog if enabled by bootloader/fuses */
    MCUSR &= ~(1 << WDRF);
    wdt_disable();

    /* Disable clock division */
    clock_prescale_set(clock_div_1);
#elif (ARCH == ARCH_XMEGA)
    /* Start the PLL to multiply the 2MHz RC oscillator to 32MHz and switch the CPU core to run from it */
    XMEGACLK_StartPLL(CLOCK_SRC_INT_RC2MHZ, 2000000, F_CPU);
    XMEGACLK_SetCPUClockSource(CLOCK_SRC_PLL);

    /* Start the 32MHz internal RC oscillator and start the DFLL to increase it to 48MHz using the USB SOF as a reference */
    XMEGACLK_StartInternalOscillator(CLOCK_SRC_INT_RC32MHZ);
    XMEGACLK_StartDFLL(CLOCK_SRC_INT_RC32MHZ, DFLL_REF_INT_USBSOF, F_USB);

    PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;
#endif

    /* Hardware Initialization */
    USB_Init();

    GlobalInterruptEnable();
}

/** Event handler for the USB_Connect event. This indicates that the device is enumerating via the status LEDs and
 *  starts the library USB task to begin the enumeration and USB management process.
 */
void EVENT_USB_Device_Connect(void)
{
    /* Indicate USB enumerating */
    //LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
}

/** Event handler for the USB_Disconnect event. This indicates that the device is no longer connected to a host via
 *  the status LEDs and stops the USB management and CDC management tasks.
 */
void EVENT_USB_Device_Disconnect(void)
{
    /* Indicate USB not ready */
    //LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}

/** Event handler for the USB_ConfigurationChanged event. This is fired when the host set the current configuration
 *  of the USB device after enumeration - the device endpoints are configured and the CDC management tasks are started.
 */
void EVENT_USB_Device_ConfigurationChanged(void)
{
    bool ConfigSuccess = true;

    /* Setup first CDC Interface's Endpoints */
    ConfigSuccess &= Endpoint_ConfigureEndpoint(CDC_TX_EPADDR, EP_TYPE_BULK, CDC_TXRX_EPSIZE, 1);
    ConfigSuccess &= Endpoint_ConfigureEndpoint(CDC_RX_EPADDR, EP_TYPE_BULK, CDC_TXRX_EPSIZE, 1);
    ConfigSuccess &= Endpoint_ConfigureEndpoint(CDC_NOTIFICATION_EPADDR, EP_TYPE_INTERRUPT, CDC_NOTIFICATION_EPSIZE, 1);

    /* Setup HID Report Endpoints */
    ConfigSuccess &= Endpoint_ConfigureEndpoint(GENERIC_IN_EPADDR, EP_TYPE_INTERRUPT, GENERIC_EPSIZE, 1);
    ConfigSuccess &= Endpoint_ConfigureEndpoint(GENERIC_OUT_EPADDR, EP_TYPE_INTERRUPT, GENERIC_EPSIZE, 1);

    /* Reset line encoding baud rates so that the host knows to send new values */
    LineEncoding.BaudRateBPS = 0;

    /* Indicate endpoint configuration success or failure */
    //LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the USB_ControlRequest event. This is used to catch and process control requests sent to
 *  the device from the USB host before passing along unhandled control requests to the library for processing
 *  internally.
 */
void EVENT_USB_Device_ControlRequest(void)
{
        switch (USB_ControlRequest.bRequest)
        {
        case CDC_REQ_GetLineEncoding:
            if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE))
            {
                Endpoint_ClearSETUP();

                /* Write the line coding data to the control endpoint */
                Endpoint_Write_Control_Stream_LE(&LineEncoding, sizeof(CDC_LineEncoding_t));
                Endpoint_ClearOUT();
            }

            break;
        case CDC_REQ_SetLineEncoding:
            if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE))
            {
                Endpoint_ClearSETUP();

                /* Read the line coding data in from the host into the global struct */
                Endpoint_Read_Control_Stream_LE(&LineEncoding, sizeof(CDC_LineEncoding_t));
                Endpoint_ClearIN();
            }

            break;
        case CDC_REQ_SetControlLineState:
            if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE))
            {
                Endpoint_ClearSETUP();
                Endpoint_ClearStatusStage();
            }

            break;
        case HID_REQ_GetReport:
            if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE))
            {
                uint8_t GenericData[GENERIC_REPORT_SIZE];

                // XXX somehow force a report to get created
                //CreateGenericHIDReport(GenericData);

                Endpoint_ClearSETUP();

                /* Write the report data to the control endpoint */
                Endpoint_Write_Control_Stream_LE(&GenericData, sizeof(GenericData));
                Endpoint_ClearOUT();
            }

            break;
        case HID_REQ_SetReport:
            if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE))
            {
                uint8_t GenericData[GENERIC_REPORT_SIZE];

                Endpoint_ClearSETUP();

                /* Read the report data from the control endpoint */
                Endpoint_Read_Control_Stream_LE(&GenericData, sizeof(GenericData));
                Endpoint_ClearIN();

                _usb_recv(USB_CHANNEL_HID, GenericData, GENERIC_REPORT_SIZE);
            }

            break;
        }
}

/** Function to manage CDC data transmission and reception to and from the host for the second CDC interface, which echoes back
 *  all data sent to it from the host.
 */
void usb_task(void)
{

    /* Device must be connected and configured for the task to run */
    if (USB_DeviceState != DEVICE_STATE_Configured)
        return;

    /* Select the Serial Rx Endpoint */
    Endpoint_SelectEndpoint(CDC_RX_EPADDR);

    /* Check to see if any data has been received */
    if (Endpoint_IsOUTReceived())
    {
        /* Create a temp buffer big enough to hold the incoming endpoint packet */
        uint8_t  Buffer[Endpoint_BytesInEndpoint()];

        /* Remember how large the incoming packet is */
        uint16_t DataLength = Endpoint_BytesInEndpoint();

        /* Read in the incoming packet into the buffer */
        Endpoint_Read_Stream_LE(&Buffer, DataLength, NULL);

        /* Finalize the stream transfer to send the last packet */
        Endpoint_ClearOUT();

        _usb_recv(USB_CHANNEL_CDC, (uint8_t *)Buffer, DataLength);

    }

        Endpoint_SelectEndpoint(GENERIC_OUT_EPADDR);

        /* Check to see if a packet has been sent from the host */
        if (Endpoint_IsOUTReceived())
        {
                /* Check to see if the packet contains data */
                if (Endpoint_IsReadWriteAllowed())
                {
                        /* Create a temporary buffer to hold the read in report from the host */
                        uint8_t GenericData[GENERIC_REPORT_SIZE];

                        /* Read Generic Report Data */
                        Endpoint_Read_Stream_LE(&GenericData, sizeof(GenericData), NULL);

                        /* Process Generic Report Data */
                        _usb_recv(USB_CHANNEL_HID, GenericData, sizeof(GenericData));
                }

                /* Finalize the stream transfer to send the last packet */
                Endpoint_ClearOUT();
        }

}

void usb_send(uint8_t channel, uint8_t *message, int16_t size) {
    switch (channel) {
    case USB_CHANNEL_CDC:
        /* Select the Serial Tx Endpoint */
        Endpoint_SelectEndpoint(CDC_TX_EPADDR);

        /* Write the received data to the endpoint */
        Endpoint_Write_Stream_LE(message, size, NULL);

        /* Finalize the stream transfer to send the last packet */
        Endpoint_ClearIN();

        /* Wait until the endpoint is ready for the next packet */
        Endpoint_WaitUntilReady();

        /* Send an empty packet to prevent host buffering */
        Endpoint_ClearIN();

        break;
    case USB_CHANNEL_HID:

        Endpoint_SelectEndpoint(GENERIC_IN_EPADDR);
        Endpoint_WaitUntilReady();
        Endpoint_Write_Stream_LE(message, size, NULL);
        Endpoint_ClearIN();
        break;
    }

}

