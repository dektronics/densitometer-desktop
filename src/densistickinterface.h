#ifndef DENSISTICKINTERFACE_H
#define DENSISTICKINTERFACE_H

#include <QObject>
#include "ft260.h"
#include "densistickreading.h"

class M24C08;
class DensiStickSettings;
class TSL2585;

class DensiStickInterface : public QObject
{
    Q_OBJECT
public:
    explicit DensiStickInterface(Ft260 *ft260, QObject *parent = nullptr);
    ~DensiStickInterface();

    bool open();
    void close();

    bool connected() const;
    bool hasSettings() const;
    bool running() const;

    DensiStickSettings *settings();

    bool setLightEnable(bool enable);
    bool lightEnabled() const;

    bool setLightBrightness(quint8 value);
    quint8 lightBrightness() const;
    float lightCurrent() const;

    bool setSensorGain(int gain);
    bool setSensorIntegration(int sampleTime, int sampleCount);
    bool setSensorAgcEnable(int sampleCount);
    bool setSensorAgcDisable();

    bool sensorStart();
    bool sensorStop();

signals:
    void connectionClosed();
    void buttonEvent(bool pressed);
    void sensorReading(const DensiStickReading& reading);

private slots:
    void onConnectionClosed();
    void onSensorInterrupt();

private:
    DensiStickReading readSensor();

    bool connected_ = false;
    Ft260 *ft260_ = nullptr;
    M24C08 *eeprom_ = nullptr;
    DensiStickSettings *settings_ = nullptr;
    TSL2585 *sensor_ = nullptr;
    bool hasSettings_ = false;
    Ft260GpioReport gpioReport_;
    quint8 lightBrightness_ = 0;
    quint8 sensorGain_ = 8;
    quint16 sensorSampleTime_ = 719;
    quint16 sensorSampleCount_ = 199;
    quint16 sensorAgcCount_ = 0;
    bool sensorAgcEnabled_ = false;
    bool discardNextReading_ = false;
    bool agcDisabledResetGain_ = false;
    bool sensorRunning_ = false;
    bool shutdown_ = false;
};

#endif // DENSISTICKINTERFACE_H
