#include "ft260hidapi.h"

#include <QThread>
#include <QDateTime>
#include <QDebug>

#include <hidapi.h>

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

Ft260HidApi::Ft260HidApi(const Ft260DeviceInfo &device, QObject *parent) : Ft260(device, parent)
{
    if (hid_init() < 0) {
        qWarning() << "hid_init error";
    }

#if defined(Q_OS_MACOS) && HID_API_VERSION >= HID_API_MAKE_VERSION(0, 12, 0)
    // To work properly needs to be called before hid_open/hid_open_path after hid_init.
    // Best/recommended option - call it right after hid_init.
    hid_darwin_set_open_exclusive(0);
#endif
}

Ft260HidApi::~Ft260HidApi()
{
    Ft260HidApi::close();
    hid_exit();
}

bool Ft260HidApi::open()
{
    struct hid_device_info *devs;
    QString secondaryPath;
    bool initialized = false;

    // Find secondary endpoint
    devs = hid_enumerate(deviceInfo_.vendorId(), deviceInfo_.productId());
    struct hid_device_info *cur_dev = devs;
    for (; cur_dev; cur_dev = cur_dev->next) {
        if (deviceInfo_.serialNumber() == QString::fromWCharArray(cur_dev->serial_number)
            && cur_dev->interface_number == 1) {
            secondaryPath = QString::fromLatin1(cur_dev->path);
            break;
        }
    }
    hid_free_enumeration(devs);

    if (secondaryPath.isEmpty()) {
        qWarning() << "Unable to find secondary endpoint";
        return false;
    }

    do {
        handle_[0] = hid_open_path(deviceInfo_.devicePath().toLatin1());
        if (!handle_[0]) {
            qWarning() << "Unable to open primary endpoint";
            break;
        }

        handle_[1] = hid_open_path(secondaryPath.toLatin1());
        if (!handle_[1]) {
            qWarning() << "Unable to open secondary endpoint";
            break;
        }

        uint8_t bus_status;
        uint16_t speed;
        if (!chipVersion(nullptr)) {
            qWarning() << "Unable to query chip version";
            break;
        }

        if (!systemStatus()) {
            qWarning() << "Unable to query system status";
            break;
        }

        if (!setI2cClockSpeed(400)) {
            qWarning() << "Unable to set I2C clock speed";
            break;
        }

        if (!i2cStatus(&bus_status, &speed)) {
            qWarning() << "Unable to get I2C status";
            break;
        }

        qDebug().nospace() << "I2C bus status: 0x" << Qt::hex << bus_status;
        qDebug().nospace() << "I2C speed: " << speed << "kHz";

        if (!setUartEnableRiWakeup(true)) {
            qWarning() << "Unable to enable RI wakeup";
            break;
        }

        if(!setUartRiWakeupConfig(false /* rising edge */)) {
            qWarning() << "Unable to set RI wakeup config";
            break;
        }

        if (!setUartEnableDcdRi(true)) {
            qWarning() << "Unable to enable DCD RI";
            break;
        }
        initialized = true;
    } while (0);

    if (!initialized) {
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
            nbytes = hid_read_timeout(handle_[1], buf, sizeof(buf), 100);
            if (nbytes == 0) { continue; }
            else if (nbytes < 0) {
                qWarning() << "hid_read_timeout error:" << QString::fromWCharArray(hid_error(handle_[1]));
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

            if (nbytes >= 3 && buf[0] == 0xB1) {
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
                qDebug() << nbytes << QByteArray::fromRawData((const char *)buf, nbytes).toHex();
            }
        } while (!closing_.load());
        qWarning() << "Fell out of loop";
    });
    connect(thread_, &QThread::finished, this, &Ft260HidApi::onIntThreadFinished);
    thread_->start();

    connected_ = true;
    emit connectionOpened();

    return true;
}

void Ft260HidApi::close()
{
    if (thread_ && thread_->isRunning()) {
        disconnect(thread_, &QThread::finished, this, &Ft260HidApi::onIntThreadFinished);
    }
    closing_ = true;
    if (thread_) {
        thread_->wait();
        thread_->deleteLater();
        thread_ = nullptr;
    }
    for (int i = 0; i < 2; i++) {
        if (handle_[i]) {
            hid_close(handle_[i]);
            handle_[i] = nullptr;
        }
    }
    if (connected_) {
        connected_ = false;
        emit connectionClosed();
    }
}

void Ft260HidApi::onIntThreadFinished()
{
    if (connected_) {
        close();
    }
}

bool Ft260HidApi::chipVersion(Ft260ChipVersion *chipVersion)
{
    if (chipVersion && (chipVersion_.chip[0] != 0 || chipVersion_.chip[1] != 0)) {
        memcpy(chipVersion, &chipVersion_, sizeof(Ft260ChipVersion));
        return true;
    } else {
        if (!handle_[0]) { return false; }

        int ret;
        uint8_t buf[REQUEST_BUF_SIZE];

        memset(buf, 0, sizeof(buf));

        buf[0] = HID_REPORT_FT260_CHIP_CODE;
        ret = hid_get_feature_report(handle_[0], buf, sizeof(buf));

        if (ret < 0) {
            qWarning() << "chipVersion hid_get_feature_report error:" << QString::fromWCharArray(hid_error(handle_[0]));
            return false;
        }

        qDebug().nospace() << "Chip version; Report ID: 0x" << Qt::hex << buf[0];
        qDebug().nospace() << "Chip code: " << Qt::hex << buf[1] << buf[2] << " " << buf[3] << " " << buf[4];

        chipVersion_.chip[0] = buf[1];
        chipVersion_.chip[1] = buf[2];
        chipVersion_.minor = buf[3];
        chipVersion_.major = buf[4];

        if (chipVersion) {
            memcpy(chipVersion, &chipVersion_, sizeof(Ft260ChipVersion));
        }

        return true;
    }
}

bool Ft260HidApi::systemStatus()
{
    if (!handle_[0]) { return false; }

    int ret;
    uint8_t buf[REQUEST_BUF_SIZE];

    memset(buf, 0, sizeof(buf));

    buf[0] = HID_REPORT_FT260_SYSTEM_SETTING;
    ret = hid_get_feature_report(handle_[0], buf, sizeof(buf));

    if (ret < 0) {
        qWarning() << "systemStatus hid_get_feature_report error:" << QString::fromWCharArray(hid_error(handle_[0]));
        return false;
    }

    qDebug().nospace() << "System status; Report ID: 0x" << Qt::hex << buf[0];
    qDebug().nospace() << "Chip mode: DCNF0=" << ((buf[1] & 0x01) != 0) << ", DCNF1=" << ((buf[1] & 0x02) != 0);

    switch (buf[2]) {
    case 0:
        qDebug() << "Clock: 12MHz";
        systemClock_ = FT260_CLOCK_12MHZ;
        break;
    case 1:
        qDebug() << "Clock: 24MHz";
        systemClock_ = FT260_CLOCK_24MHZ;
        break;
    case 2:
        qDebug() << "Clock: 48MHz";
        systemClock_ = FT260_CLOCK_48MHZ;
        break;
    default:
        qDebug().nospace() << "Clock: [" << buf[2] << "]";
        systemClock_ = FT260_CLOCK_MAX;
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

Ft260SystemClock Ft260HidApi::systemClock() const
{
    return systemClock_;
}

bool Ft260HidApi::i2cStatus(uint8_t *busStatus, uint16_t *speed)
{
    if (!handle_[0]) { return false; }

    int ret;
    uint8_t buf[REQUEST_BUF_SIZE];

    memset(buf, 0, sizeof(buf));

    buf[0] = HID_REPORT_FT260_I2C_STATUS;
    ret = hid_get_feature_report(handle_[0], buf, sizeof(buf));

    if (ret < 0) {
        qWarning() << "i2cStatus hid_get_feature_report error:" << QString::fromWCharArray(hid_error(handle_[0]));
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

bool Ft260HidApi::setI2cClockSpeed(uint16_t speed)
{
    if (!handle_[0]) { return false; }

    int ret;
    uint8_t buf[4];

    buf[0] = HID_REPORT_FT260_SYSTEM_SETTING;
    buf[1] = 0x22; // Set I2C clock speed
    buf[2] = (uint8_t)(speed & 0x00FF);
    buf[3] = (uint8_t)((speed & 0xFF00) >> 8);

    ret = hid_send_feature_report(handle_[0], buf, sizeof(buf));

    if (ret < 0) {
        qWarning() << "setI2cClockSpeed hid_send_feature_report error:" << QString::fromWCharArray(hid_error(handle_[0]));
        return false;
    }

    return true;
}

bool Ft260HidApi::setUartMode(quint8 mode)
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

    ret = hid_send_feature_report(handle_[0], buf, sizeof(buf));

    if (ret < 0) {
        qWarning() << "setUartMode hid_send_feature_report error:" << QString::fromWCharArray(hid_error(handle_[0]));
        return false;
    }

    return true;
}

bool Ft260HidApi::setUartEnableDcdRi(bool enable)
{
    if (!handle_[1]) { return false; }

    int ret;
    uint8_t buf[3];

    buf[0] = HID_REPORT_FT260_SYSTEM_SETTING;
    buf[1] = 0x07; /* Enable UART DCD RI */
    buf[2] = enable ? 1 : 0;

    ret = hid_send_feature_report(handle_[0], buf, sizeof(buf));

    if (ret < 0) {
        qWarning() << "setUartEnableDcdRi hid_send_feature_report error:" << QString::fromWCharArray(hid_error(handle_[0]));
        return false;
    }

    return true;
}

bool Ft260HidApi::setUartEnableRiWakeup(bool enable)
{
    if (!handle_[1]) { return false; }

    int ret;
    uint8_t buf[3];

    buf[0] = HID_REPORT_FT260_SYSTEM_SETTING;
    buf[1] = 0x0C; /* Enable UART RI Wakeup */
    buf[2] = enable ? 1 : 0;

    ret = hid_send_feature_report(handle_[0], buf, sizeof(buf));

    if (ret < 0) {
        qWarning() << "setUartEnableRiWakeup hid_send_feature_report error:" << QString::fromWCharArray(hid_error(handle_[0]));
        return false;
    }

    return true;
}

bool Ft260HidApi::setUartRiWakeupConfig(bool edge)
{
    if (!handle_[1]) { return false; }

    int ret;
    uint8_t buf[3];

    buf[0] = HID_REPORT_FT260_SYSTEM_SETTING;
    buf[1] = 0x0D; /* Set UART RI Wakeup Config */
    buf[2] = edge ? 1 : 0; /* true = falling, false = rising */

    ret = hid_send_feature_report(handle_[0], buf, sizeof(buf));

    if (ret < 0) {
        qWarning() << "setUartRiWakeupConfig hid_send_feature_report error:" << QString::fromWCharArray(hid_error(handle_[0]));
        return false;
    }

    return true;
}

bool Ft260HidApi::gpioRead(Ft260GpioReport *report)
{
    if (!handle_[0]) { return false; }

    int ret;
    uint8_t buf[REQUEST_BUF_SIZE];

    memset(buf, 0, sizeof(buf));

    buf[0] = HID_REPORT_FT260_GPIO;
    ret = hid_get_feature_report(handle_[0], buf, sizeof(buf));

    if (ret < 0) {
        qWarning() << "gpioRead hid_get_feature_report error:" << QString::fromWCharArray(hid_error(handle_[0]));
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

bool Ft260HidApi::gpioWrite(const Ft260GpioReport *report)
{
    if (!handle_[0] || !report) { return false; }

    int ret;
    uint8_t buf[5];

    buf[0] = HID_REPORT_FT260_GPIO;
    buf[1] = report->gpio_value;
    buf[2] = report->gpio_dir;
    buf[3] = report->gpio_ex_value;
    buf[4] = report->gpio_ex_dir;

    ret = hid_send_feature_report(handle_[0], buf, sizeof(buf));
    if (ret < 0) {
        qWarning() << "gpioWrite hid_send_feature_report error:" << QString::fromWCharArray(hid_error(handle_[0]));
        return false;
    }

    return true;
}

bool Ft260HidApi::i2cWriteRequest(quint8 addr, uint8_t flags, const uint8_t *payload, quint8 payloadSize)
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

    ret = hid_write(handle_[0], (unsigned char*)buf, transferLen);

    if (ret < 0) {
        qWarning() << "i2cWriteRequest hid_write error:" << QString::fromWCharArray(hid_error(handle_[0]));
        return false;
    }

    return true;
}

bool Ft260HidApi::i2cReadRequest(quint8 addr, uint8_t flags, quint16 payloadSize)
{
    int ret;
    uint8_t buf[REQUEST_BUF_SIZE];

    memset(buf, 0, REQUEST_BUF_SIZE);
    buf[0] = HID_REPORT_FT260_I2C_READ_REQUEST;
    buf[1] = addr;
    buf[2] = flags;
    buf[3] = payloadSize & 0x00FF;
    buf[4] = (payloadSize & 0xFF00) >> 8;

    ret = hid_write(handle_[0], (unsigned char*)buf, 5);

    if (ret < 0) {
        qWarning() << "i2cReadRequest hid_write error:" << QString::fromWCharArray(hid_error(handle_[0]));
        return false;
    }

    return true;
}

QByteArray Ft260HidApi::i2cRead(quint8 addr, quint8 reg, quint8 len)
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

    ret = hid_read(handle_[0], buf, sizeof(buf));

    if (ret < 0) {
        qWarning() << "i2cRead hid_read error:" << QString::fromWCharArray(hid_error(handle_[0]));
    }

    if (len != buf[1]) {
        qWarning() << "Unexpected size returned:" << buf[1] << "!=" << len;
        return QByteArray();
    }

    QByteArray result(reinterpret_cast<const char *>(buf + 2), len);

    return result;
}

bool Ft260HidApi::i2cReadByte(quint8 addr, quint8 reg, quint8 *data)
{
    const QByteArray response = i2cRead(addr, reg, 1);
    if (response.isEmpty()) { return false; }

    if (data) { *data = static_cast<quint8>(response[0]); }
    return true;
}

bool Ft260HidApi::i2cReadRawByte(quint8 addr, quint8 *data)
{
    if (!handle_[0] || addr > 0x7F) { return false; }

    int ret;
    uint8_t buf[REQUEST_BUF_SIZE];

    if (!i2cReadRequest(addr, FT260_I2C_START_AND_STOP, 1)) {
        return false;
    }

    ret = hid_read(handle_[0], buf, sizeof(buf));

    if (ret < 0) {
        qWarning() << "i2cRead hid_read error:" << QString::fromWCharArray(hid_error(handle_[0]));
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

bool Ft260HidApi::i2cWrite(quint8 addr, quint8 reg, const QByteArray &data)
{
    if (!handle_[0] || addr > 0x7F || data.isEmpty() || data.size() > HID_MAX_TRAN_SIZE) { return false; }

    if (!i2cWriteRequest(addr, FT260_I2C_START, &reg, 1)) {
        return false;
    }

    if (!i2cWriteRequest(addr, FT260_I2C_STOP, reinterpret_cast<const uint8_t *>(data.constData()), data.size())) {
        return false;
    }

    return true;
}

bool Ft260HidApi::i2cWriteByte(quint8 addr, quint8 reg, quint8 data)
{
    QByteArray buf(1, Qt::Uninitialized);
    buf[0] = data;
    return i2cWrite(addr, reg, buf);
}

bool Ft260HidApi::i2cWriteRawByte(quint8 addr, quint8 data)
{
    if (!handle_[0] || addr > 0x7F) { return false; }

    uint8_t buf;

    buf = data;

    if (!i2cWriteRequest(addr, FT260_I2C_START_AND_STOP, &buf, 1)) {
        return false;
    }

    return true;
}

QList<Ft260DeviceInfo> listDevicesByHidApi()
{
    QList<Ft260DeviceInfo> list;

    struct hid_device_info *devs;

    if (hid_init() < 0) {
        return list;
    }

#if defined(Q_OS_MACOS) && HID_API_VERSION >= HID_API_MAKE_VERSION(0, 12, 0)
    // To work properly needs to be called before hid_open/hid_open_path after hid_init.
    // Best/recommended option - call it right after hid_init.
    hid_darwin_set_open_exclusive(0);
#endif

    devs = hid_enumerate(0x0, 0x0);

    struct hid_device_info *cur_dev = devs;
    for (; cur_dev; cur_dev = cur_dev->next) {
        if (!Ft260::isMatchingDevice(cur_dev->vendor_id, cur_dev->product_id)) {
            continue;
        }
        // Only put the primary interface into the list.
        // We'll search for the secondary interface at connection time.
        if (cur_dev->interface_number != 0) { continue; }

        Ft260DeviceInfoPrivate privDevice;
        privDevice.devicePath = QString::fromLatin1(cur_dev->path);
        privDevice.deviceDisplayPath = privDevice.devicePath;
        privDevice.deviceDriver = Ft260DeviceInfo::DriverHidApi;
        privDevice.vendorId = cur_dev->vendor_id;
        privDevice.productId = cur_dev->product_id;
        privDevice.manufacturer = QString::fromWCharArray(cur_dev->manufacturer_string);
        privDevice.product = QString::fromWCharArray(cur_dev->product_string);
        privDevice.serialNumber = QString::fromWCharArray(cur_dev->serial_number);
        privDevice.description = QString("%1 (%2)").arg(privDevice.product, privDevice.serialNumber);
        list.append(privDevice);
    }

    hid_free_enumeration(devs);

    hid_exit();

    return list;
}

QList<Ft260DeviceInfo> Ft260HidApi::listDevices()
{
    return listDevicesByHidApi();
}
