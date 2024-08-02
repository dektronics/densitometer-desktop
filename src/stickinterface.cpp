#include "stickinterface.h"

#include <QDebug>
#include "ft260.h"
#include "tsl2585.h"
#include "m24c08.h"
#include "sticksettings.h"

namespace
{
static const uint8_t EEPROM_ADDRESS = 0x50;
}

StickInterface::StickInterface(Ft260 *ft260, QObject *parent)
    : QObject(parent), ft260_(ft260)
{
    if (ft260_ && !ft260_->parent()) {
        ft260_->setParent(this);
    }
}

StickInterface::~StickInterface()
{
    close();
}

bool StickInterface::open()
{
    quint8 data;

    if (!ft260_) { return false; }

    if (valid_) {
        qWarning() << "Device already open";
        return false;
    }

    if (!ft260_->open()) {
        qWarning() << "Unable to open device";
        close();
        return false;
    }

    // Create the configuration EEPROM device handler
    eeprom_ = new M24C08(ft260_, EEPROM_ADDRESS);

    // Create the settings data wrapper
    settings_ = new StickSettings(eeprom_);
    if (!settings_->init()) {
        qWarning() << "Unable to read settings";
        close();
        return false;
    }

    hasSettings_ = settings_->headerValid();

    // Create the light sensor device handler
    sensor_ = new TSL2585(ft260_);
    if (!sensor_->init()) {
        qWarning() << "Unable to initialize sensor";
        close();
        return false;
    }

    // Read initial GPIO state
    if (!ft260_->gpioRead(&gpioReport_)) {
        qWarning() << "Unable to read initial GPIO state";
        close();
        return false;
    }

    // Update initial GPIO state
    gpioReport_.gpio_value = 0; // GPIO 0-5 set to off
    gpioReport_.gpio_dir = 0; // GPIO 0-5 set as input
    gpioReport_.gpio_ex_value = 0x20; // GPIOF set to high, all others set to low
    gpioReport_.gpio_ex_dir = 0x80; // GPIOH set as output, all others set as input

    // Write initial GPIO state
    if (!ft260_->gpioWrite(&gpioReport_)) {
        qWarning() << "Unable to write initial GPIO state";
        close();
        return false;
    }

    connect(ft260_, &Ft260::buttonInterrupt, this, &StickInterface::onButtonInterrupt);
    connect(ft260_, &Ft260::sensorInterrupt, this, &StickInterface::onSensorInterrupt);

    valid_ = true;

    return true;
}

void StickInterface::close()
{
    hasSettings_ = false;
    valid_ = false;

    disconnect(ft260_, &Ft260::buttonInterrupt, this, &StickInterface::onButtonInterrupt);
    disconnect(ft260_, &Ft260::sensorInterrupt, this, &StickInterface::onSensorInterrupt);

    if (settings_) {
        delete settings_;
        settings_ = nullptr;
    }

    if (eeprom_) {
        delete eeprom_;
        eeprom_ = nullptr;
    }

    if (sensor_) {
        delete sensor_;
        sensor_ = nullptr;
    }

    if (ft260_) {
        ft260_->close();
    }
}

void StickInterface::onButtonInterrupt(bool pressed)
{
}

void StickInterface::onSensorInterrupt()
{
}
