/*
 * Copyright (c) Rob Moran 2021 <github@thegecko.org>
 */

#include <string.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/usb/usbd.h>

#define BULK_EP_MAXPACKET 64

static const struct usb_device_descriptor dev = {
    .bLength = USB_DT_DEVICE_SIZE,
    .bDescriptorType = USB_DT_DEVICE,
    .bcdUSB = 0x0110, // USB 1.10
    .bDeviceClass = 0x00, // Use class information in the Interface Descriptors
    .bDeviceSubClass = 0,
    .bDeviceProtocol = 0,
    .bMaxPacketSize0 = BULK_EP_MAXPACKET,
    .idVendor = 0x59e3,
    .idProduct = 0x0a23,
    .bcdDevice = 0x0200, // Version
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 3,
    .bNumConfigurations = 1,
};

static const struct usb_endpoint_descriptor endp_bulk[] = {{
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = USB_REQ_TYPE_IN | 1,
    .bmAttributes = USB_ENDPOINT_ATTR_BULK | USB_ENDPOINT_ATTR_NOSYNC | USB_ENDPOINT_ATTR_DATA,
    .wMaxPacketSize = BULK_EP_MAXPACKET,
    .bInterval = 0,
}, {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = USB_REQ_TYPE_OUT | 2,
    .bmAttributes = USB_ENDPOINT_ATTR_BULK | USB_ENDPOINT_ATTR_NOSYNC | USB_ENDPOINT_ATTR_DATA,
    .wMaxPacketSize = BULK_EP_MAXPACKET,
    .bInterval = 0,
}, {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = USB_REQ_TYPE_IN | 3,
    .bmAttributes = USB_ENDPOINT_ATTR_BULK | USB_ENDPOINT_ATTR_NOSYNC | USB_ENDPOINT_ATTR_DATA,
    .wMaxPacketSize = BULK_EP_MAXPACKET,
    .bInterval = 0,
}, {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = USB_REQ_TYPE_OUT | 4,
    .bmAttributes = USB_ENDPOINT_ATTR_BULK | USB_ENDPOINT_ATTR_NOSYNC | USB_ENDPOINT_ATTR_DATA,
    .wMaxPacketSize = BULK_EP_MAXPACKET,
    .bInterval = 0,
}};

static const struct usb_interface_descriptor data_iface[] = {{
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = 0,
    .bAlternateSetting = 0,
    .bNumEndpoints = 4,
    .bInterfaceClass = USB_CLASS_VENDOR,
    .bInterfaceSubClass = 0,
    .bInterfaceProtocol = 0,
    .iInterface = 0,
    .endpoint = endp_bulk,
}};

static const struct usb_interface ifaces[] = {{
    .num_altsetting = 1,
    .altsetting = data_iface,
}};

static const struct usb_config_descriptor config = {
    .bLength = USB_DT_CONFIGURATION_SIZE,
    .bDescriptorType = USB_DT_CONFIGURATION,
    .wTotalLength = 0,
    .bNumInterfaces = 1,
    .bConfigurationValue = 1,
    .iConfiguration = 0,
    .bmAttributes = USB_CONFIG_ATTR_DEFAULT,
    .bMaxPower = 500 >> 1,
    .interface = ifaces,
};

static const char *usb_strings[] = {
    "Nonolith Labs",
    "STM32F103 Example Device",
    "DEMO",
};

/* Buffer to be used for control requests. */
uint8_t usbd_control_buffer[128];
uint8_t testbuf[16];

static void ep_1_cb(usbd_device *usbd_dev, uint8_t ep) {
    char buf[BULK_EP_MAXPACKET];
    usbd_ep_write_packet(usbd_dev, ep, buf, BULK_EP_MAXPACKET);
}

static void ep_2_cb(usbd_device *usbd_dev, uint8_t ep) {
    char buf[BULK_EP_MAXPACKET];
    usbd_ep_read_packet(usbd_dev, ep, buf, BULK_EP_MAXPACKET);
}

static void ep_4_cb(usbd_device *usbd_dev, uint8_t ep)
{
    (void)usbd_dev;
    (void)ep;
    // Do nothing
}

static enum usbd_request_return_codes control_callback(usbd_device *usbd_dev, struct usb_setup_data *req, uint8_t **buf,
        uint16_t *len, void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req)) {

    (void)usbd_dev;
    (void)complete;

    switch(req->bRequest) {
        case 0x81: {
            if (req->bmRequestType & USB_REQ_TYPE_IN) {
                memcpy(*buf, testbuf, 16);
                *len = 16;
            } else {
                if (*len > 16) *len = 16;
                memcpy(testbuf, *buf, *len);
            }

            return USBD_REQ_HANDLED;
        }
    }

    return USBD_REQ_NOTSUPP;
}

static void config_callback(usbd_device *usbd_dev, uint16_t wValue) {
    (void)wValue;

    usbd_ep_setup(usbd_dev, USB_REQ_TYPE_IN | 1, USB_ENDPOINT_ATTR_BULK, BULK_EP_MAXPACKET, ep_1_cb);
    usbd_ep_setup(usbd_dev, USB_REQ_TYPE_OUT | 2, USB_ENDPOINT_ATTR_BULK, BULK_EP_MAXPACKET, ep_2_cb);
    usbd_ep_setup(usbd_dev, USB_REQ_TYPE_IN | 3, USB_ENDPOINT_ATTR_BULK, BULK_EP_MAXPACKET, NULL);
    usbd_ep_setup(usbd_dev, USB_REQ_TYPE_OUT | 4, USB_ENDPOINT_ATTR_BULK, BULK_EP_MAXPACKET, ep_4_cb);

    usbd_register_control_callback(usbd_dev, USB_REQ_TYPE_VENDOR, USB_REQ_TYPE_TYPE, control_callback);

    // Preload data for polling
    ep_1_cb(usbd_dev, USB_REQ_TYPE_IN | 1);

    // Break ep 4 so it times out
    usbd_ep_nak_set(usbd_dev, USB_REQ_TYPE_OUT | 4, 1);
}

int main(void) {
    usbd_device *usbd_dev;
    rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_HSI_48MHZ]);

    usbd_dev = usbd_init(&st_usbfs_v1_usb_driver, &dev, &config, usb_strings, 3, usbd_control_buffer, sizeof(usbd_control_buffer));
    usbd_register_set_config_callback(usbd_dev, config_callback);
                
    while (1)
        usbd_poll(usbd_dev);
}
