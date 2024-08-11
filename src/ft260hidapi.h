#ifndef FT260HIDAPI_H
#define FT260HIDAPI_H

#include <atomic>

#include "ft260.h"

typedef struct hid_device_ hid_device;

class Ft260HidApi : public Ft260
{
public:
    explicit Ft260HidApi(const Ft260DeviceInfo &device, QObject *parent = nullptr);
    virtual ~Ft260HidApi();

    bool open();
    void close();

    bool hasInterrupts() const;

    bool chipVersion();
    bool systemStatus();
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
    hid_device *handle_[2] = {nullptr, nullptr};
    QThread *thread_ = nullptr;
	bool connected_ = false;
    std::atomic<bool> closing_ = false;
};

#endif // FT260HIDAPI_H
