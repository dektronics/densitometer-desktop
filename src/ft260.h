#ifndef FT260_H
#define FT260_H

#include <QObject>

#include "ft260deviceinfo.h"

typedef struct {
    uint8_t gpio_value;
    uint8_t gpio_dir;
    uint8_t gpio_ex_value;
    uint8_t gpio_ex_dir;
} Ft260GpioReport;

class Ft260 : public QObject
{
    Q_OBJECT
protected:
    explicit Ft260(QObject *parent = nullptr);

public:
    virtual ~Ft260();

    virtual bool open() = 0;
    virtual void close() = 0;

    virtual bool i2cStatus(uint8_t *busStatus, uint16_t *speed) = 0;
    virtual bool setI2cClockSpeed(uint16_t speed) = 0;

    virtual bool setUartMode(quint8 mode) = 0;
    virtual bool setUartEnableDcdRi(bool enable) = 0;
    virtual bool setUartEnableRiWakeup(bool enable) = 0;
    virtual bool setUartRiWakeupConfig(bool edge) = 0;

    virtual bool gpioRead(Ft260GpioReport *report) = 0;
    virtual bool gpioWrite(const Ft260GpioReport *report) = 0;

    virtual QByteArray i2cRead(quint8 addr, quint8 reg, quint8 len) = 0;
    virtual bool i2cReadByte(quint8 addr, quint8 reg, quint8 *data) = 0;
    virtual bool i2cReadRawByte(quint8 addr, quint8 *data) = 0;
    virtual bool i2cWrite(quint8 addr, quint8 reg, const QByteArray &data) = 0;
    virtual bool i2cWriteByte(quint8 addr, quint8 reg, quint8 data) = 0;
    virtual bool i2cWriteRawByte(quint8 addr, quint8 data) = 0;

    static Ft260 *createDriver(const Ft260DeviceInfo &device);
    static QList<Ft260DeviceInfo> listDevices();
    static bool isMatchingDevice(quint16 vid, quint16 pid);

signals:
    void connectionOpened();
    void connectionClosed();
    void buttonInterrupt(bool pressed);
    void sensorInterrupt();
};

#endif // FT260_H
