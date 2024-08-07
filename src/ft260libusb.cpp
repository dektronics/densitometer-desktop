#include "ft260libusb.h"

#include <QThread>
#include <QDateTime>
#include <QDebug>

#include <libusb-1.0/libusb.h>

#include "ft260deviceinfo.h"
#include "ft260deviceinfo_p.h"

#define HID_MAX_TRAN_SIZE 0x3C
#define HID_OFFSET_BYTES  0x04
#define REQUEST_BUF_SIZE  64

namespace
{
typedef enum {
    //-------------SYSTEM CMD-----------------//
    HID_REPORT_FT260_CHIP_CODE            = 0xA0,
    HID_REPORT_FT260_SYSTEM_SETTING       = 0xA1,
    //-------------GPIO CMD-------------------//
    HID_REPORT_FT260_GPIO                 = 0xB0,
    HID_REPORT_FT260_INTERRUPT_STATUS     = 0xB1,
    //-------------I2C CMD--------------------//
    HID_REPORT_FT260_I2C_STATUS           = 0xC0,
    HID_REPORT_FT260_I2C_READ_REQUEST     = 0xC2,
    HID_REPORT_FT260_I2C_IO_BASE          = 0xD0,
    //-------------UART CMD--------------------//
    HID_REPORT_FT260_UART_STATUS          = 0xE0,
    HID_REPORT_FT260_UART_RI_DCD          = 0xE2,
    HID_REPORT_FT260_UART_IO_BASE         = 0xF0
} HID_REPORT_ID_DEF;

typedef enum {
    FT260_I2C_NONE           = 0x00,
    FT260_I2C_START          = 0x02,
    FT260_I2C_REPEATED_START = 0x03,
    FT260_I2C_STOP           = 0x04,
    FT260_I2C_START_AND_STOP = 0x06
} FT260_I2C_COMMAND;
}

Ft260LibUsb::Ft260LibUsb(const Ft260DeviceInfo &device, QObject *parent) : Ft260(parent), deviceInfo_(device)
{
    int r;
    r = libusb_init(&context_);
    if (r < 0) {
        qWarning() << "libusb_init error:" << libusb_strerror(r);
        context_ = nullptr;
    }
}

Ft260LibUsb::~Ft260LibUsb()
{
    Ft260LibUsb::close();
    if (context_) {
        libusb_exit(context_);
    }
}

bool Ft260LibUsb::open()
{
    int r;
    int i = 0;
    int j = 0;
    size_t count;
    libusb_device **devs = nullptr;
    libusb_device *dev;
    struct libusb_device_descriptor desc;
    struct libusb_config_descriptor *conf_desc = nullptr;
    uint8_t path[8];

    if (!context_ || handle_[0]) { return false; }

    const QByteArray devicePath = QByteArray::fromHex(deviceInfo_.devicePath().toLatin1());

    count = libusb_get_device_list(context_, &devs);
    if (count < 0) {
        return false;
    }

    while ((dev = devs[i++]) != NULL) {
        r = libusb_get_device_descriptor(dev, &desc);
        if (r < 0) {
            continue;
        }

        if (deviceInfo_.vendorId() != desc.idVendor && deviceInfo_.productId() != desc.idProduct) {
            continue;
        }

        r = libusb_get_port_numbers(dev, path, sizeof(path));
        if (r < 0) {
            continue;
        }

        if (devicePath == QByteArray::fromRawData(reinterpret_cast<const char *>(path), r)) {
            r = libusb_get_active_config_descriptor(dev, &conf_desc);
            if (r < 0) {
                libusb_get_config_descriptor(dev, 0, &conf_desc);
            }

            r = libusb_open(dev, &handle_[0]);
            if (r < 0) {
                qWarning() << "libusb_open error:" << libusb_strerror(r);
                handle_[0] = nullptr;
            }

            if (conf_desc->bNumInterfaces > 1) {
                r = libusb_open(dev, &handle_[1]);
                if (r < 0) {
                    qWarning() << "libusb_open error:" << libusb_strerror(r);
                    handle_[1] = nullptr;
                }
            }
            break;
        }
    }

    libusb_free_device_list(devs, 1);

    if (!handle_[0] || !conf_desc || conf_desc->bNumInterfaces < 1) {
        if (conf_desc) {
            libusb_free_config_descriptor(conf_desc);
        }
        return false;
    }

    for (int i = 0; i < conf_desc->bNumInterfaces; i++) {
        if (i >= 2) { break; }
        if (!handle_[i]) { break; }

        r = libusb_set_auto_detach_kernel_driver(handle_[i], 1);
        if (r < 0) {
            qWarning() << "libusb_set_auto_detach_kernel_driver error:" << libusb_strerror(r);
        }

        const struct libusb_interface *intf = &conf_desc->interface[i];
        if (intf->num_altsetting == 0) { r = -1; break; }

        const struct libusb_interface_descriptor *intf_desc = &intf->altsetting[0];

        r = libusb_claim_interface(handle_[i], i);
        if (r < 0) {
            qWarning() << "libusb_claim_interface error:" << libusb_strerror(r);
            break;
        }

        for (j = 0; j < intf_desc->bNumEndpoints; j++) {
            const struct libusb_endpoint_descriptor *ep = &intf_desc->endpoint[j];

            /* Determine the type and direction of this endpoint. */
            int is_interrupt = (ep->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) == LIBUSB_TRANSFER_TYPE_INTERRUPT;
            int is_output = (ep->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_OUT;
            int is_input = (ep->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN;

            /* Decide whether to use it for input or output. */
            if (inputEp_[i] == 0 && is_interrupt && is_input) {
                /* Use this endpoint for INPUT */
                inputEp_[i] = ep->bEndpointAddress;
                inputEpMaxPacketSize_[i] = ep->wMaxPacketSize;
            }
            if (outputEp_[i] == 0 && is_interrupt && is_output) {
                /* Use this endpoint for OUTPUT */
                outputEp_[i] = ep->bEndpointAddress;
            }
        }
    };
    if (inputEp_[0] == 0 || outputEp_[0] == 0) {
        qWarning() << "Failed to find primary endpoints";
        r = -1;
    }
    if (inputEp_[1] == 0 || outputEp_[1] == 0) {
        qWarning() << "Failed to find secondary endpoints";
    }

    if (conf_desc) {
        libusb_free_config_descriptor(conf_desc);
    }

    if (r < 0) {
        close();
        return false;
    }

    do {
        uint8_t bus_status;
        uint16_t speed;
        if (!chipVersion()) {
            qWarning() << "Unable to query chip version";
            r = -1;
            break;
        }

        if (!systemStatus()) {
            qWarning() << "Unable to query system status";
            r = -1;
            break;
        }

        if (!setI2cClockSpeed(400)) {
            qWarning() << "Unable to set I2C clock speed";
            r = -1;
            break;
        }

        if (!i2cStatus(&bus_status, &speed)) {
            qWarning() << "Unable to get I2C status";
            r = -1;
            break;
        }

        qDebug().nospace() << "I2C bus status: 0x" << Qt::hex << bus_status;
        qDebug().nospace() << "I2C speed: " << speed << "kHz";

        if (!handle_[1]) { break; }

        if (!setUartEnableRiWakeup(true)) {
            qWarning() << "Unable to enable RI wakeup";
            r = -1;
            break;
        }

        if(!setUartRiWakeupConfig(false /* rising edge */)) {
            qWarning() << "Unable to set RI wakeup config";
            r = -1;
            break;
        }

        if (!setUartEnableDcdRi(true)) {
            qWarning() << "Unable to enable DCD RI";
            r = -1;
            break;
        }

        qDebug() << "Secondary endpoint initialized";
    } while (0);

    if (r < 0) {
        close();
        return false;
    }

    thread_ = QThread::create([this] {
        const qint64 startTime = QDateTime::currentMSecsSinceEpoch();
        bool started = false;
        int r = 0;
        uint8_t buf[128];
        int nbytes;
        bool button_pressed = false;
        do {
            r = libusb_interrupt_transfer(handle_[1], inputEp_[1], buf, qMin(sizeof(buf), inputEpMaxPacketSize_[1]), &nbytes, 100);
            if (r == LIBUSB_ERROR_TIMEOUT) { r = 0; continue; }
            else if (r < 0) {
                qWarning() << "libusb_interrupt_transfer error:" << libusb_strerror(r);
                break;
            }

            // Ignore any data received in the first second since starting
            if (!started) {
                if (QDateTime::currentMSecsSinceEpoch() > startTime + 1000) {
                    started = true;
                } else {
                    continue;
                }
            }

            if (nbytes == 3 && buf[0] == 0xB1) {
                const uint8_t status_int = buf[1];
                const uint8_t status_dcd_ri = buf[2];

#if 0
                qDebug().nospace() << Qt::hex
                                   << "status_int=" << status_dcd_ri << ", status_dcd_ri=" << status_dcd_ri;
#endif
                if (status_int & 0x01) {
                    emit sensorInterrupt();
                }
                if (status_dcd_ri & 0x04) {
                    const bool pressed = (status_dcd_ri & 0x08) == 0;
                    if (button_pressed != pressed) {
                        button_pressed = pressed;
                        emit buttonInterrupt(pressed);
                    }
                }
            } else {
                qDebug() << Qt::hex << QByteArray::fromRawData((const char *)buf, nbytes);
            }
        } while (r >= 0);
        qWarning() << "Fell out of loop";
    });
    connect(thread_, &QThread::finished, this, &Ft260LibUsb::onIntThreadFinished);
    thread_->start();

    connected_ = true;
    emit connectionOpened();

    return true;
}

void Ft260LibUsb::close()
{
    if (thread_ && thread_->isRunning()) {
        disconnect(thread_, &QThread::finished, this, &Ft260LibUsb::onIntThreadFinished);
    }

    for (int i = 0; i < 2; i++) {
        if (handle_[i]) {
            libusb_release_interface(handle_[i], 0);
            libusb_close(handle_[i]);
            handle_[i] = nullptr;
            inputEp_[i] = 0;
            inputEpMaxPacketSize_[i] = 0;
            outputEp_[i] = 0;
        }
    }
    if (thread_) {
        thread_->wait();
        thread_->deleteLater();
        thread_ = nullptr;
    }
    if (connected_) {
        connected_ = false;
        emit connectionClosed();
    }
}

void Ft260LibUsb::onIntThreadFinished()
{
    if (connected_) {
        close();
    }
}

bool Ft260LibUsb::hasInterrupts() const
{
    return true;
}

bool Ft260LibUsb::chipVersion()
{
    if (!handle_[0]) { return false; }

    int ret;
    uint8_t buf[13];

    memset(buf, 0, sizeof(buf));

    ret = libusb_control_transfer(handle_[0],
                                  LIBUSB_REQUEST_TYPE_CLASS|LIBUSB_RECIPIENT_INTERFACE|LIBUSB_ENDPOINT_IN,
                                  0x01 /*HID get_report*/,
                                  (0x03/*HID feature*/ << 8) | HID_REPORT_FT260_CHIP_CODE,
                                  0,
                                  (unsigned char *)buf, 13,
                                  1000/*timeout millis*/);
    if (ret < 0) {
        qWarning() << "chipVersion libusb_control_transfer error:" << libusb_strerror(ret);
        return false;
    }

    qDebug().nospace() << "Chip version; Report ID: 0x" << Qt::hex << buf[0];
    qDebug().nospace() << "Chip code: " << Qt::hex << buf[1] << buf[2] << " " << buf[3] << " " << buf[4];

    //TODO collect this in a data structure that can be returned

    return true;
}

bool Ft260LibUsb::systemStatus()
{
    if (!handle_[0]) { return false; }

    int ret;
    uint8_t buf[26];

    memset(buf, 0, sizeof(buf));

    ret = libusb_control_transfer(handle_[0],
                                  LIBUSB_REQUEST_TYPE_CLASS|LIBUSB_RECIPIENT_INTERFACE|LIBUSB_ENDPOINT_IN,
                                  0x01 /*HID get_report*/,
                                  (0x03/*HID feature*/ << 8) | HID_REPORT_FT260_SYSTEM_SETTING,
                                  0,
                                  (unsigned char *)buf, 26,
                                  1000/*timeout millis*/);
    if (ret < 0) {
        qWarning() << "systemStatus libusb_control_transfer error:" << libusb_strerror(ret);
        return false;
    }

    qDebug().nospace() << "System status; Report ID: 0x" << Qt::hex << buf[0];
    qDebug().nospace() << "Chip mode: DCNF0=" << ((buf[1] & 0x01) != 0) << ", DCNF1=" << ((buf[1] & 0x02) != 0);

    switch (buf[2]) {
    case 0:
        qDebug() << "Clock: 12MHz";
        break;
    case 1:
        qDebug() << "Clock: 24MHz";
        break;
    case 2:
        qDebug() << "Clock: 48MHz";
        break;
    default:
        qDebug().nospace() << "Clock: [" << buf[2] << "]";
        break;
    }

    qDebug() << "SUSPEND:" << ((buf[3] == 0) ? "Not suspended": "Suspended");
    qDebug() << "PWREN:" << ((buf[4] == 1) ? "Ready" : "Not ready");
    qDebug() << "I2C:" << ((buf[5] == 1) ? "Enabled" : "Disabled");

    switch(buf[6]) {
    case 0:
        qDebug() << "UART: OFF";
        break;
    case 1:
        qDebug() << "UART: RTS_CTS";
        break;
    case 2:
        qDebug() << "UART: DTR_DSR";
        break;
    case 3:
        qDebug() << "UART: XON_XOFF";
        break;
    case 4:
        qDebug() << "UART: No flow control";
        break;
    default:
        qDebug().nospace() << "UART: [" << buf[6] << "]";
    }

    qDebug() << "HID-over-I2C:" << ((buf[7] == 0) ? "Not configured" : "Configured");
    qDebug().nospace() << "GPIO2=" << buf[8] << ", GPIOA=" << buf[9] << ", GPIOG=" << buf[10];
    qDebug() << "Suspend output:" << ((buf[11] == 0) ? "Active high" : "Active low");
    qDebug().nospace() << "Wakeup/Int: "
                       << ((buf[12] == 0) ? "Disabled" : "Enabled")
                       << " [" << (buf[13] & 0x03) << "," << ((buf[13] & 0x0C) >> 2) << "]";
    qDebug() << "Power saving:" << ((buf[14] == 0) ? "Disabled" : "Enabled");

    //TODO collect this in a data structure that can be returned

    return true;
}

bool Ft260LibUsb::i2cStatus(uint8_t *busStatus, uint16_t *speed)
{
    if (!handle_[0]) { return false; }

    int ret;
    uint8_t buf[5];

    memset(buf, 0, sizeof(buf));

    ret = libusb_control_transfer(handle_[0],
                                  LIBUSB_REQUEST_TYPE_CLASS|LIBUSB_RECIPIENT_INTERFACE|LIBUSB_ENDPOINT_IN,
                                  0x01 /*HID get_report*/,
                                  (0x03/*HID feature*/ << 8) | HID_REPORT_FT260_I2C_STATUS,
                                  0,
                                  (unsigned char *)buf, 5,
                                  1000/*timeout millis*/);
    if (ret < 0) {
        qWarning() << "i2cStatus libusb_control_transfer error:" << libusb_strerror(ret);
        return false;
    }

    if (busStatus) {
        *busStatus = buf[1];
    }

    if (speed) {
        *speed = (uint16_t)(buf[3]) << 8 | buf[2];
    }

    return true;
}

bool Ft260LibUsb::setI2cClockSpeed(uint16_t speed)
{
    if (!handle_[0]) { return false; }

    int ret;
    uint8_t buf[4];

    buf[0] = HID_REPORT_FT260_SYSTEM_SETTING;
    buf[1] = 0x22; // Set I2C clock speed
    buf[2] = (uint8_t)(speed & 0x00FF);
    buf[3] = (uint8_t)((speed & 0xFF00) >> 8);

    ret = libusb_control_transfer(handle_[0],
                                  LIBUSB_REQUEST_TYPE_CLASS|LIBUSB_RECIPIENT_INTERFACE|LIBUSB_ENDPOINT_OUT,
                                  0x09/*HID set_report*/,
                                  (3/*HID feature*/ << 8) | HID_REPORT_FT260_SYSTEM_SETTING,
                                  0,
                                  (unsigned char *)buf, 4,
                                  1000/*timeout millis*/);
    if (ret < 0) {
        qWarning() << "setI2cClockSpeed libusb_control_transfer error:" << libusb_strerror(ret);
        return false;
    }

    return true;
}

bool Ft260LibUsb::setUartMode(quint8 mode)
{
    /*
     * 0 = Off
     * 1 = hardware flow control RTS/CTS mode (GPIOB =>RTSN, GPIOE =>CTSN)
     * 2 = hardware flow control DTR/DSR mode (GPIOF =>DTRN, GPIOH => DSRN)
     * 3 = Software flow control, XON/XOFF mode
     * 4 = No flow control
     */

    if (!handle_[0]) { return false; }

    int ret;
    uint8_t buf[3];

    buf[0] = HID_REPORT_FT260_SYSTEM_SETTING;
    buf[1] = 0x03; /* Set UART Mode */
    buf[2] = mode;

    ret = libusb_control_transfer(handle_[0],
                                  LIBUSB_REQUEST_TYPE_CLASS|LIBUSB_RECIPIENT_INTERFACE|LIBUSB_ENDPOINT_OUT,
                                  0x09/*HID set_report*/,
                                  (3/*HID feature*/ << 8) | HID_REPORT_FT260_SYSTEM_SETTING,
                                  0,
                                  (unsigned char *)buf, 3,
                                  1000/*timeout millis*/);
    if (ret < 0) {
        qWarning() << "setUartMode libusb_control_transfer error:" << libusb_strerror(ret);
        return false;
    }

    return true;
}

bool Ft260LibUsb::setUartEnableDcdRi(bool enable)
{
    if (!handle_[1]) { return false; }

    int ret;
    uint8_t buf[3];

    buf[0] = HID_REPORT_FT260_SYSTEM_SETTING;
    buf[1] = 0x07; /* Enable UART DCD RI */
    buf[2] = enable ? 1 : 0;

    ret = libusb_control_transfer(handle_[0],
                                  LIBUSB_REQUEST_TYPE_CLASS|LIBUSB_RECIPIENT_INTERFACE|LIBUSB_ENDPOINT_OUT,
                                  0x09/*HID set_report*/,
                                  (3/*HID feature*/ << 8) | HID_REPORT_FT260_SYSTEM_SETTING,
                                  0,
                                  (unsigned char *)buf, 3,
                                  1000/*timeout millis*/);
    if (ret < 0) {
        qWarning() << "setUartEnableDcdRi libusb_control_transfer error:" << libusb_strerror(ret);
        return false;
    }

    return true;
}

bool Ft260LibUsb::setUartEnableRiWakeup(bool enable)
{
    if (!handle_[1]) { return false; }

    int ret;
    uint8_t buf[3];

    buf[0] = HID_REPORT_FT260_SYSTEM_SETTING;
    buf[1] = 0x0C; /* Enable UART RI Wakeup */
    buf[2] = enable ? 1 : 0;

    ret = libusb_control_transfer(handle_[0],
                                  LIBUSB_REQUEST_TYPE_CLASS|LIBUSB_RECIPIENT_INTERFACE|LIBUSB_ENDPOINT_OUT,
                                  0x09/*HID set_report*/,
                                  (3/*HID feature*/ << 8) | HID_REPORT_FT260_SYSTEM_SETTING,
                                  0,
                                  (unsigned char *)buf, 3,
                                  1000/*timeout millis*/);
    if (ret < 0) {
        qWarning() << "setUartEnableRiWakeup libusb_control_transfer error:" << libusb_strerror(ret);
        return false;
    }

    return true;
}

bool Ft260LibUsb::setUartRiWakeupConfig(bool edge)
{
    if (!handle_[1]) { return false; }

    int ret;
    uint8_t buf[3];

    buf[0] = HID_REPORT_FT260_SYSTEM_SETTING;
    buf[1] = 0x0D; /* Set UART RI Wakeup Config */
    buf[2] = edge ? 1 : 0; /* true = falling, false = rising */

    ret = libusb_control_transfer(handle_[0],
                                  LIBUSB_REQUEST_TYPE_CLASS|LIBUSB_RECIPIENT_INTERFACE|LIBUSB_ENDPOINT_OUT,
                                  0x09/*HID set_report*/,
                                  (3/*HID feature*/ << 8) | HID_REPORT_FT260_SYSTEM_SETTING,
                                  0,
                                  (unsigned char *)buf, 3,
                                  1000/*timeout millis*/);
    if (ret < 0) {
        qWarning() << "setUartRiWakeupConfig libusb_control_transfer error:" << libusb_strerror(ret);
        return false;
    }

    return true;
}

bool Ft260LibUsb::gpioRead(Ft260GpioReport *report)
{
    if (!handle_[0]) { return false; }

    int ret;
    uint8_t buf[5];

    memset(buf, 0, sizeof(buf));

    ret = libusb_control_transfer(handle_[0],
                                  LIBUSB_REQUEST_TYPE_CLASS|LIBUSB_RECIPIENT_INTERFACE|LIBUSB_ENDPOINT_IN,
                                  0x01 /*HID get_report*/,
                                  (0x03/*HID feature*/ << 8) | HID_REPORT_FT260_GPIO,
                                  0,
                                  (unsigned char *)buf, 5,
                                  1000/*timeout millis*/);
    if (ret < 0) {
        qWarning() << "gpioRead libusb_control_transfer error:" << libusb_strerror(ret);
        return false;
    }

    if (report) {
        report->gpio_value = buf[1];
        report->gpio_dir = buf[2];
        report->gpio_ex_value = buf[3];
        report->gpio_ex_dir = buf[4];
    }

    return true;
}

bool Ft260LibUsb::gpioWrite(const Ft260GpioReport *report)
{
    if (!handle_[0] || !report) { return false; }

    int ret;
    uint8_t buf[5];

    buf[0] = HID_REPORT_FT260_GPIO;
    buf[1] = report->gpio_value;
    buf[2] = report->gpio_dir;
    buf[3] = report->gpio_ex_value;
    buf[4] = report->gpio_ex_dir;

    ret = libusb_control_transfer(handle_[0],
                                  LIBUSB_REQUEST_TYPE_CLASS|LIBUSB_RECIPIENT_INTERFACE|LIBUSB_ENDPOINT_OUT,
                                  0x09/*HID set_report*/,
                                  (3/*HID feature*/ << 8) | HID_REPORT_FT260_GPIO,
                                  0,
                                  (unsigned char *)buf, 5,
                                  1000/*timeout millis*/);
    if (ret < 0) {
        qWarning() << "gpioWrite libusb_control_transfer error:" << libusb_strerror(ret);
        return false;
    }

    return true;
}

bool Ft260LibUsb::i2cWriteRequest(quint8 addr, uint8_t flags, const uint8_t *payload, quint8 payloadSize)
{
    int ret;
    uint8_t buf[REQUEST_BUF_SIZE];

    if (payloadSize == 0 || payloadSize > 60) {
        return 0;
    }
    const uint8_t reportId = 0xD0 + ((payloadSize - 1) >> 2);

    const uint8_t transferLen = ((reportId - 0xD0 + 1) * 4) + 4;

    memset(buf, 0, REQUEST_BUF_SIZE);
    buf[0] = reportId;
    buf[1] = addr;
    buf[2] = flags;
    buf[3] = payloadSize;
    memcpy(buf + 4, payload, payloadSize);

    int actual_length;
    ret = libusb_interrupt_transfer(handle_[0],
                                    outputEp_[0],
                                    (unsigned char*)buf,
                                    transferLen,
                                    &actual_length, 1000);
    Q_UNUSED(actual_length)
    if (ret < 0) {
        qWarning() << "i2cWriteRequest libusb_interrupt_transfer error:" << libusb_strerror(ret);
        return false;
    }

    return true;
}

bool Ft260LibUsb::i2cReadRequest(quint8 addr, uint8_t flags, quint16 payloadSize)
{
    int ret;
    uint8_t buf[REQUEST_BUF_SIZE];

    memset(buf, 0, REQUEST_BUF_SIZE);
    buf[0] = HID_REPORT_FT260_I2C_READ_REQUEST;
    buf[1] = addr;
    buf[2] = flags;
    buf[3] = payloadSize & 0x00FF;
    buf[4] = (payloadSize & 0xFF00) >> 8;

    int actual_length;
    ret = libusb_interrupt_transfer(handle_[0],
                                    outputEp_[0],
                                    (unsigned char*)buf,
                                    5,
                                    &actual_length, 1000);
    Q_UNUSED(actual_length)
    if (ret < 0) {
        qWarning() << "i2cReadRequest libusb_interrupt_transfer error:" << libusb_strerror(ret);
        return false;
    }

    return true;
}

QByteArray Ft260LibUsb::i2cRead(quint8 addr, quint8 reg, quint8 len)
{
    if (!handle_[0] || addr > 0x7F || len == 0) { return QByteArray(); }

    int ret;
    uint8_t buf[REQUEST_BUF_SIZE];

    if (!i2cWriteRequest(addr, FT260_I2C_START, &reg, 1)) {
        return QByteArray();
    }

    if (!i2cReadRequest(addr, FT260_I2C_REPEATED_START | FT260_I2C_STOP, len)) {
        return QByteArray();
    }

    int transferred;
    ret = libusb_interrupt_transfer(handle_[0], inputEp_[0], buf, sizeof(buf), &transferred, 5000);
    Q_UNUSED(transferred);
    if (ret < 0) {
        qWarning() << "i2cRead libusb_interrupt_transfer error:" << libusb_strerror(ret);
    }

    if (len != buf[1]) {
        qWarning() << "Unexpected size returned:" << buf[1] << "!=" << len;
        return QByteArray();
    }

    QByteArray result(reinterpret_cast<const char *>(buf + 2), len);

    return result;
}

bool Ft260LibUsb::i2cReadByte(quint8 addr, quint8 reg, quint8 *data)
{
    const QByteArray response = i2cRead(addr, reg, 1);
    if (response.isEmpty()) { return false; }

    if (data) { *data = static_cast<quint8>(response[0]); }
    return true;
}

bool Ft260LibUsb::i2cReadRawByte(quint8 addr, quint8 *data)
{
    if (!handle_[0] || addr > 0x7F) { return false; }

    int ret;
    uint8_t buf[REQUEST_BUF_SIZE];

    if (!i2cReadRequest(addr, FT260_I2C_START_AND_STOP, 1)) {
        return false;
    }

    int transferred;
    ret = libusb_interrupt_transfer(handle_[0], inputEp_[0], buf, sizeof(buf), &transferred, 5000);
    Q_UNUSED(transferred);
    if (ret < 0) {
        qWarning() << "i2cRead libusb_interrupt_transfer error:" << libusb_strerror(ret);
    }

    if (buf[1] != 1) {
        qWarning() << "Unexpected size returned:" << buf[1] << "!= 1";
        return false;
    }

    if (data) {
        *data = buf[2];
    }

    return true;
}

bool Ft260LibUsb::i2cWrite(quint8 addr, quint8 reg, const QByteArray &data)
{
    if (!handle_[0] || addr > 0x7F || data.isEmpty() || data.size() > HID_MAX_TRAN_SIZE) { return false; }

    int ret;
    uint8_t buf[64];

    if (!i2cWriteRequest(addr, FT260_I2C_START, &reg, 1)) {
        return false;
    }

    if (!i2cWriteRequest(addr, FT260_I2C_STOP, reinterpret_cast<const uint8_t *>(data.constData()), data.size())) {
        return false;
    }

    return true;
}

bool Ft260LibUsb::i2cWriteByte(quint8 addr, quint8 reg, quint8 data)
{
    QByteArray buf(1, Qt::Uninitialized);
    buf[0] = data;
    return i2cWrite(addr, reg, buf);
}

bool Ft260LibUsb::i2cWriteRawByte(quint8 addr, quint8 data)
{
    if (!handle_[0] || addr > 0x7F) { return false; }

    int ret;
    uint8_t buf;

    buf = data;

    if (!i2cWriteRequest(addr, FT260_I2C_START_AND_STOP, &buf, 1)) {
        return false;
    }

    return true;
}

QList<Ft260DeviceInfo> listDevicesByLibUsb()
{
    QList<Ft260DeviceInfo> list;
    libusb_context *ctx = nullptr;
    libusb_device **devs = nullptr;
    libusb_device *dev;
    int r;
    int i = 0;
    int j = 0;
    ssize_t count;
    uint8_t path[8];
    uint8_t buf[64];
    QString pathStr;

    do {
        r = libusb_init(&ctx);
        if (r < 0) {
            break;
        }

        count = libusb_get_device_list(ctx, &devs);
        if (count < 0) {
            break;
        }

        while ((dev = devs[i++]) != NULL) {
            struct libusb_device_descriptor desc;
            libusb_device_handle *handle;
            r = libusb_get_device_descriptor(dev, &desc);
            if (r < 0) {
                qWarning() << "Failed to get device descriptor";
                continue;
            }

            if (!Ft260::isMatchingDevice(desc.idVendor, desc.idProduct)) {
                continue;
            }

            r = libusb_get_port_numbers(dev, path, sizeof(path));
            if (r < 0) {
                qWarning() << "Failed to get port numbers";
                continue;
            }

            pathStr = QString("Bus %1, Device %2, Path: ")
                          .arg(libusb_get_bus_number(dev))
                          .arg(libusb_get_device_address(dev));
            pathStr.append(QString::number(path[0]));
            for (j = 1; j < r; j++) {
                pathStr.append('.');
                pathStr.append(QString::number(path[j]));
            }

            Ft260DeviceInfoPrivate privDevice;
            privDevice.devicePath = QByteArray(reinterpret_cast<const char *>(path), r).toHex();
            privDevice.deviceDisplayPath = pathStr;
            privDevice.deviceDriver = Ft260DeviceInfo::DriverLibUsb;
            privDevice.vendorId = desc.idVendor;
            privDevice.productId = desc.idProduct;

            r = libusb_open(dev, &handle);
            if (r >= 0) {
                if (libusb_get_string_descriptor_ascii(handle, desc.iManufacturer, buf, sizeof(buf)) > 0) {
                    privDevice.manufacturer = QString::fromLatin1(reinterpret_cast<char *>(buf));
                }
                if (libusb_get_string_descriptor_ascii(handle, desc.iProduct, buf, sizeof(buf)) > 0) {
                    privDevice.product = QString::fromLatin1(reinterpret_cast<char *>(buf));
                }
                if (libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber, buf, sizeof(buf)) > 0) {
                    privDevice.serialNumber = QString::fromLatin1(reinterpret_cast<char *>(buf));
                }
                libusb_close(handle);
            } else {
                qWarning() << "libusb_open error:" << libusb_strerror(r);
            }

            privDevice.description = QString("%1 (%2)").arg(privDevice.product, privDevice.serialNumber);
            list.append(privDevice);
        }
        if (r < 0) {
            break;
        }
    } while (0);

    if (devs) {
        libusb_free_device_list(devs, 1);
    }

    if (ctx) {
        libusb_exit(ctx);
    }

    return list;
}

QList<Ft260DeviceInfo> Ft260LibUsb::listDevices()
{
    return listDevicesByLibUsb();
}
