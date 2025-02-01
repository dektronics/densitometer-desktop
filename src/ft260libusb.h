#ifndef FT260LIBUSB_H
#define FT260LIBUSB_H

#include "ft260.h"

typedef struct libusb_context libusb_context;
typedef struct libusb_device_handle libusb_device_handle;

class Ft260LibUsb : public Ft260
{
public:
    explicit Ft260LibUsb(const Ft260DeviceInfo &device, QObject *parent = nullptr);
    virtual ~Ft260LibUsb();

    bool open();
    void close();

    bool hasInterrupts() const;

    bool chipVersion(Ft260ChipVersion *chipVersion);
    bool systemStatus();
    Ft260SystemClock systemClock() const;
    bool i2cStatus(uint8_t *busStatus, uint16_t *speed);
    bool setI2cClockSpeed(uint16_t speed);

    bool setUartMode(quint8 mode);
    bool setUartEnableDcdRi(bool enable);
    bool setUartEnableRiWakeup(bool enable);
    bool setUartRiWakeupConfig(bool edge);

    bool gpioRead(Ft260GpioReport *report);
    bool gpioWrite(const Ft260GpioReport *report);

    QByteArray i2cRead(quint8 addr, quint8 reg, quint8 len);
    bool i2cReadByte(quint8 addr, quint8 reg, quint8 *data);
    bool i2cReadRawByte(quint8 addr, quint8 *data);
    bool i2cWrite(quint8 addr, quint8 reg, const QByteArray &data);
    bool i2cWriteByte(quint8 addr, quint8 reg, quint8 data);
    bool i2cWriteRawByte(quint8 addr, quint8 data);

    static QList<Ft260DeviceInfo> listDevices();

private slots:
    void onIntThreadFinished();

private:
    bool i2cWriteRequest(quint8 addr, uint8_t flags, const uint8_t *payload, quint8 payloadSize);
    bool i2cReadRequest(quint8 addr, uint8_t flags, quint16 payloadSize);
    libusb_context *context_ = nullptr;
    libusb_device_handle *handle_[2] = {nullptr, nullptr};
    uint8_t inputEp_[2] = {0, 0};
    uint16_t inputEpMaxPacketSize_[2] = {0, 0};
    uint8_t outputEp_[2] = {0, 0};
    QThread *thread_ = nullptr;
    bool connected_ = false;
    Ft260ChipVersion chipVersion_ = {0};
    Ft260SystemClock systemClock_ = FT260_CLOCK_MAX;
};

#endif // FT260LIBUSB_H
