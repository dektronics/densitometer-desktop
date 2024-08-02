#ifndef STICKINTERFACE_H
#define STICKINTERFACE_H

#include <QObject>
#include "ft260.h"

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

private slots:
    void onButtonInterrupt(bool pressed);
    void onSensorInterrupt();

private:
    bool valid_ = false;
    Ft260 *ft260_ = nullptr;
    M24C08 *eeprom_ = nullptr;
    StickSettings *settings_ = nullptr;
    TSL2585 *sensor_ = nullptr;
    bool hasSettings_ = false;
    Ft260GpioReport gpioReport_;
};

#endif // STICKINTERFACE_H
