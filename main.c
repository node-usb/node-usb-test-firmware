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
    .bcdUSB = 0x0200,
    .bDeviceClass = 0x00,
    .bDeviceSubClass = 0,
    .bDeviceProtocol = 0,
    .bMaxPacketSize0 = BULK_EP_MAXPACKET,
    .idVendor = 0x59e3,
    .idProduct = 0x0a23,
    .bcdDevice = 0x0200,
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 3,
    .bNumConfigurations = 1,
};

static const struct usb_endpoint_descriptor endp_bulk[] = {{
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = USB_REQ_TYPE_IN | 1,
    .bmAttributes = USB_ENDPOINT_ATTR_BULK,
    .wMaxPacketSize = BULK_EP_MAXPACKET,
    .bInterval = 1,
}, {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = USB_REQ_TYPE_OUT | 2,
    .bmAttributes = USB_ENDPOINT_ATTR_BULK,
    .wMaxPacketSize = BULK_EP_MAXPACKET,
    .bInterval = 1,
}, {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = USB_REQ_TYPE_IN | 3,
    .bmAttributes = USB_ENDPOINT_ATTR_BULK,
    .wMaxPacketSize = BULK_EP_MAXPACKET,
    .bInterval = 1,
}, {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = USB_REQ_TYPE_IN | 4,
    .bmAttributes = USB_ENDPOINT_ATTR_BULK,
    .wMaxPacketSize = BULK_EP_MAXPACKET,
    .bInterval = 1,
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
    .bmAttributes = 0x80,
    .bMaxPower = 0x32,
    .interface = ifaces,
};

static const char *usb_strings[] = {
    "Nonolith Labs",
    "STM32F103 Example Device",
    "DEMO",
};

/* Buffer to be used for control requests. */
uint8_t usbd_control_buffer[128];
uint16_t pktcount;
uint8_t testbuf[16];

static void ep_1_data_in_cb(usbd_device *usbd_dev, uint8_t ep) {
    uint32_t buf[BULK_EP_MAXPACKET];
    buf[0] = pktcount++;
    usbd_ep_write_packet(usbd_dev, ep, buf, BULK_EP_MAXPACKET);
}

static void ep_2_data_out_cb(usbd_device *usbd_dev, uint8_t ep) {
    char buf[BULK_EP_MAXPACKET];
    usbd_ep_read_packet(usbd_dev, ep, buf, BULK_EP_MAXPACKET);
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

    usbd_ep_setup(usbd_dev, USB_REQ_TYPE_IN | 1, USB_ENDPOINT_ATTR_BULK, BULK_EP_MAXPACKET, ep_1_data_in_cb);
    usbd_ep_setup(usbd_dev, USB_REQ_TYPE_OUT | 2, USB_ENDPOINT_ATTR_BULK, BULK_EP_MAXPACKET, ep_2_data_out_cb);
    usbd_ep_setup(usbd_dev, USB_REQ_TYPE_IN | 3, USB_ENDPOINT_ATTR_BULK, BULK_EP_MAXPACKET, NULL);
    usbd_ep_setup(usbd_dev, USB_REQ_TYPE_OUT | 4, USB_ENDPOINT_ATTR_BULK, BULK_EP_MAXPACKET, NULL);

    usbd_register_control_callback(usbd_dev, USB_REQ_TYPE_VENDOR, USB_REQ_TYPE_TYPE, control_callback);
    ep_1_data_in_cb(usbd_dev, USB_REQ_TYPE_IN | 1);
}

int main(void) {
    usbd_device *usbd_dev;

    rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_HSI_48MHZ]);
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_AFIO);

    AFIO_MAPR |= AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON;

    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, 0, GPIO15);

    usbd_dev = usbd_init(&st_usbfs_v1_usb_driver, &dev, &config, usb_strings, 3, usbd_control_buffer, sizeof(usbd_control_buffer));
    usbd_register_set_config_callback(usbd_dev, config_callback);
                
    gpio_set(GPIOA, GPIO15);
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ,
              GPIO_CNF_OUTPUT_PUSHPULL, GPIO15);

    while (1)
        usbd_poll(usbd_dev);
}
