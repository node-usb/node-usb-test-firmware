/*
 * Copyright (c) Rob Moran 2026 <github@thegecko.org>
 */

#include <string.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/stm32/rcc.h>

#define BULK_EP_MAXPACKET 64
#define CONTROL_EP_PACKET 16

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
    .bcdDevice = 0x0110, // Version
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
}, {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = USB_REQ_TYPE_IN | 5,
    .bmAttributes = USB_ENDPOINT_ATTR_BULK | USB_ENDPOINT_ATTR_NOSYNC | USB_ENDPOINT_ATTR_DATA,
    .wMaxPacketSize = BULK_EP_MAXPACKET,
    .bInterval = 0,
}, {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = USB_REQ_TYPE_OUT | 6,
    .bmAttributes = USB_ENDPOINT_ATTR_BULK | USB_ENDPOINT_ATTR_NOSYNC | USB_ENDPOINT_ATTR_DATA,
    .wMaxPacketSize = BULK_EP_MAXPACKET,
    .bInterval = 0,
}};

static const struct usb_interface_descriptor data_iface[] = {{
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = 0,
    .bAlternateSetting = 0,
    .bNumEndpoints = 6,
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
    "STM32F103 Test Device",
    "TEST_DEVICE",
};

static usbd_device *test_dev;
static uint8_t control_buf[CONTROL_EP_PACKET];
static uint8_t poll_buf[BULK_EP_MAXPACKET];
static uint8_t bulk_buf[BULK_EP_MAXPACKET];
static volatile int ep_pending_len = 0;

static void ep_out_cb(usbd_device *usbd_dev, uint8_t ep) {
    (void)ep;
    int len = usbd_ep_read_packet(usbd_dev, USB_REQ_TYPE_OUT | 4, bulk_buf, sizeof(bulk_buf));
    if (len > 0) {
        ep_pending_len = len;
    }
}

static enum usbd_request_return_codes control_callback(usbd_device *usbd_dev, struct usb_setup_data *req, uint8_t **buf,
        uint16_t *len, void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req)) {
    (void)usbd_dev;
    (void)complete;

    switch(req->bRequest) {
        case 0x81: {
            if (req->bmRequestType & USB_REQ_TYPE_IN) {
                memcpy(*buf, control_buf, CONTROL_EP_PACKET);
                *len = CONTROL_EP_PACKET;
            } else {
                if (*len > CONTROL_EP_PACKET) *len = CONTROL_EP_PACKET;
                memcpy(control_buf, *buf, *len);
            }

            return USBD_REQ_HANDLED;
        }
    }

    return USBD_REQ_NOTSUPP;
}

static void config_callback(usbd_device *usbd_dev, uint16_t wValue) {
    (void)wValue;

    // For polling
    usbd_ep_setup(usbd_dev, USB_REQ_TYPE_IN | 1, USB_ENDPOINT_ATTR_BULK, BULK_EP_MAXPACKET, NULL);
    usbd_ep_setup(usbd_dev, USB_REQ_TYPE_OUT | 2, USB_ENDPOINT_ATTR_BULK, BULK_EP_MAXPACKET, NULL);

        // For timeouts
    usbd_ep_setup(usbd_dev, USB_REQ_TYPE_IN | 3, USB_ENDPOINT_ATTR_BULK, BULK_EP_MAXPACKET, NULL);
    usbd_ep_setup(usbd_dev, USB_REQ_TYPE_OUT | 4, USB_ENDPOINT_ATTR_BULK, BULK_EP_MAXPACKET, ep_out_cb);

    // For echoing back data received
    usbd_ep_setup(usbd_dev, USB_REQ_TYPE_IN | 5, USB_ENDPOINT_ATTR_BULK, BULK_EP_MAXPACKET, NULL);
    usbd_ep_setup(usbd_dev, USB_REQ_TYPE_OUT | 6, USB_ENDPOINT_ATTR_BULK, BULK_EP_MAXPACKET, NULL);

    // Control requests
    usbd_register_control_callback(usbd_dev, USB_REQ_TYPE_VENDOR, USB_REQ_TYPE_TYPE, control_callback);

    // NAK endpoint 6 so it times out
    usbd_ep_nak_set(usbd_dev, USB_REQ_TYPE_OUT | 6, 1);

    // SysTick interrupt every N clock pulses: set reload to N-1
	systick_set_reload(99999);
	systick_interrupt_enable();
	systick_counter_enable();
}

int main(void) {
    rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_HSI_48MHZ]);

    uint8_t usbd_control_buffer[128];
    test_dev = usbd_init(&st_usbfs_v1_usb_driver, &dev, &config, usb_strings, 3, usbd_control_buffer, sizeof(usbd_control_buffer));
    usbd_register_set_config_callback(test_dev, config_callback);

    while (1) {
        usbd_poll(test_dev);

        if (ep_pending_len > 0) {
            int wrote = usbd_ep_write_packet(test_dev, USB_REQ_TYPE_IN | 3, bulk_buf, ep_pending_len);
            if (wrote > 0) {
                ep_pending_len = 0;
            }
        }
    }
}

void sys_tick_handler(void) {
	usbd_ep_write_packet(test_dev, USB_REQ_TYPE_IN | 1, poll_buf, BULK_EP_MAXPACKET);
	usbd_ep_read_packet(test_dev, USB_REQ_TYPE_OUT | 2, poll_buf, BULK_EP_MAXPACKET);
}
