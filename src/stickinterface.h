#ifndef STICKINTERFACE_H
#define STICKINTERFACE_H

#include <QObject>
#include "ft260.h"
#include "stickreading.h"

class M24C08;
class StickSettings;
class TSL2585;

class StickInterface : public QObject
{
    Q_OBJECT
public:
    explicit StickInterface(Ft260 *ft260, QObject *parent = nullptr);
    ~StickInterface();

    bool open();
    void close();

    bool connected() const;
    bool running() const;

    bool setLightEnable(bool enable);
    bool lightEnabled() const;

    bool setLightBrightness(quint8 value);
    quint8 lightBrightness() const;
    float lightCurrent() const;

    bool setSensorConfig(int gain, int sampleTime, int sampleCount);
    bool setSensorAgcEnable(int sampleCount);
    bool setSensorAgcDisable();

    bool sensorStart();
    bool sensorStop();

signals:
    void sensorReading(const StickReading& reading);

private slots:
    void onButtonInterrupt(bool pressed);
    void onSensorInterrupt();

private:
    StickReading readSensor();

    bool connected_ = false;
    Ft260 *ft260_ = nullptr;
    M24C08 *eeprom_ = nullptr;
    StickSettings *settings_ = nullptr;
    TSL2585 *sensor_ = nullptr;
    bool hasSettings_ = false;
    Ft260GpioReport gpioReport_;
    quint8 lightBrightness_ = 0;
    quint8 sensorGain_ = 8;
    quint16 sensorSampleTime_ = 719;
    quint16 sensorSampleCount_ = 199;
    quint16 sensorAgcCount_ = 0;
    bool sensorAgcEnabled_ = false;
    bool sensorRunning_ = false;
};

#endif // STICKINTERFACE_H
