#include "stickinterface.h"

#include <QDebug>
#include "ft260.h"
#include "tsl2585.h"
#include "m24c08.h"
#include "sticksettings.h"
#include "stickreading.h"

namespace
{
static const uint8_t EEPROM_ADDRESS = 0x50;
static const uint8_t MCP4017_ADDRESS = 0x2F;

/* TSL2585: Only enable Photopic photodiodes on modulator 0 */
static const tsl2585_modulator_t sensor_tsl2585_phd_mod_vis[] = {
    TSL2585_MOD_NONE, TSL2585_MOD0, TSL2585_MOD_NONE, TSL2585_MOD_NONE, TSL2585_MOD_NONE, TSL2585_MOD0
};
}

StickInterface::StickInterface(Ft260 *ft260, QObject *parent)
    : QObject(parent), ft260_(ft260)
{
    if (ft260_ && !ft260_->parent()) {
        ft260_->setParent(this);
    }
    connect(ft260_, &Ft260::connectionClosed, this, &StickInterface::onConnectionClosed);
}

StickInterface::~StickInterface()
{
    disconnect(ft260_, &Ft260::connectionClosed, this, &StickInterface::onConnectionClosed);
    shutdown_ = true;
    close();
}

bool StickInterface::open()
{
    if (!ft260_) { return false; }

    if (connected_) {
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

    // Read the LED current potentiometer setting
    if (!ft260_->i2cReadRawByte(MCP4017_ADDRESS, &lightBrightness_)) {
        qWarning() << "Unable to read initial LED current state";
        close();
        return false;
    }

    connect(ft260_, &Ft260::buttonInterrupt, this, &StickInterface::buttonEvent);
    connect(ft260_, &Ft260::sensorInterrupt, this, &StickInterface::onSensorInterrupt);

    connected_ = true;

    return true;
}

void StickInterface::close()
{
    setLightEnable(false);

    hasSettings_ = false;
    connected_ = false;
    sensorRunning_ = false;

    disconnect(ft260_, &Ft260::buttonInterrupt, this, &StickInterface::buttonEvent);
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
    if (!shutdown_) {
        emit connectionClosed();
    }
}

void StickInterface::onConnectionClosed()
{
    if (connected_) {
        close();
    }
}

bool StickInterface::connected() const
{
    return connected_;
}

bool StickInterface::hasSettings() const
{
    return hasSettings_;
}

bool StickInterface::running() const
{
    return sensorRunning_;
}

StickSettings *StickInterface::settings()
{
    return settings_;
}

bool StickInterface::setLightEnable(bool enable)
{
    if (!ft260_) { return false; }

    Ft260GpioReport updateReport;
    memcpy(&updateReport, &gpioReport_, sizeof(Ft260GpioReport));

    if (enable) {
        updateReport.gpio_ex_value = 0x20 | 0x80;
    } else {
        updateReport.gpio_ex_value = 0x20;
    }

    if (ft260_->gpioWrite(&updateReport)) {
        memcpy(&gpioReport_, &updateReport, sizeof(Ft260GpioReport));
        return true;
    } else {
        return false;
    }
}

bool StickInterface::lightEnabled() const
{
    return (gpioReport_.gpio_ex_value & 0x80) != 0;
}

bool StickInterface::setLightBrightness(quint8 value)
{
    if (!ft260_) { return false; }

    if (value > 127) { value = 127; }

    if (ft260_->i2cWriteRawByte(MCP4017_ADDRESS, value)) {
        lightBrightness_ = value;
        return true;
    } else {
        return false;
    }
}

quint8 StickInterface::lightBrightness() const
{
    return lightBrightness_;
}

float StickInterface::lightCurrent() const
{
    static const float FIXED_RESISTANCE = 16.5F;
    static const float WIPER = 0.075F;

    // Two channels per LED, two LEDs
    static const float CURRENT_MULTIPLIER = 4.0F;

    const float potValue = WIPER + (((float)lightBrightness_ / 127.0F) * 100.0F);
    const float rSet = potValue + FIXED_RESISTANCE;

    // RSET[kΩ] = (820 / ILED[mA]) + 0.139
    // ILED[mA] = 820 / (RSET[k] - 0.139)

    const float currentMa = 820.0F / (rSet - 0.139);

    return (currentMa * CURRENT_MULTIPLIER) / 1000.0F;
}

bool StickInterface::setSensorGain(int gain)
{
    if (sensorRunning_) {
        if (!sensor_->setModGain(TSL2585_MOD0, TSL2585_STEP0, static_cast<tsl2585_gain_t>(gain))) {
            return false;
        }
        sensorGain_ = gain;
        discardNextReading_ = true;
    } else {
        sensorGain_ = gain;
    }
    return true;
}

bool StickInterface::setSensorIntegration(int sampleTime, int sampleCount)
{
    if (sensorRunning_) {
        if (!sensor_->setSampleTime(sampleTime)) {
            return false;
        }
        sensorSampleTime_ = sampleTime;

        if (!sensor_->setAlsNumSamples(sampleCount)) {
            return false;
        }
        sensorSampleCount_ = sampleCount;
        discardNextReading_ = true;
    } else {
        sensorSampleTime_ = sampleTime;
        sensorSampleCount_ = sampleCount;
    }
    return true;
}

bool StickInterface::setSensorAgcEnable(int sampleCount)
{
    if (sensorRunning_) {
        if (!sensor_->setAgcNumSamples(sampleCount)) {
            return false;
        }
        sensorAgcCount_ = sampleCount;

        if (!sensor_->setAgcCalibration(true)) {
            return false;
        }
        sensorAgcEnabled_ = true;
        discardNextReading_ = true;
    } else {
        sensorAgcCount_ = sampleCount;
        sensorAgcEnabled_ = true;
    }
    return true;
}

bool StickInterface::setSensorAgcDisable()
{
    if (sensorRunning_) {
        if (!sensor_->setAgcNumSamples(0)) {
            return false;
        }
        sensorAgcCount_ = 0;

        if (!sensor_->setAgcCalibration(false)) {
            return false;
        }
        sensorAgcEnabled_ = false;
        agcDisabledResetGain_ = true;
        discardNextReading_ = true;
    } else {
        sensorAgcCount_ = 0;
        sensorAgcEnabled_ = false;
    }
    return true;
}

bool StickInterface::sensorStart()
{
    if (sensorRunning_) { return false; }

    do {
        // Enable writing of ALS status to the FIFO
        if (!sensor_->setFifoAlsStatusWriteEnable(true)) { break; }

        // Enable writing of results to the FIFO
        if (!sensor_->setFifoDataWriteEnable(TSL2585_MOD0, true)) { break; }
        if (!sensor_->setFifoDataWriteEnable(TSL2585_MOD1, false)) { break; }
        if (!sensor_->setFifoDataWriteEnable(TSL2585_MOD2, false)) { break; }

        // Set FIFO data format to 32-bits
        if (!sensor_->setFifoAlsDataFormat(TSL2585_ALS_FIFO_32BIT)) { break; }

        // Set MSB position for full 26-bit result
        if (!sensor_->setAlsMsbPosition(6)) { break; }

        // Make sure residuals are enabled
        if (!sensor_->setModResidualEnable(TSL2585_MOD0, TSL2585_STEPS_ALL)) { break; }
        if (!sensor_->setModResidualEnable(TSL2585_MOD1, TSL2585_STEPS_ALL)) { break; }
        if (!sensor_->setModResidualEnable(TSL2585_MOD2, TSL2585_STEPS_ALL)) { break; }

        // Select alternate gain table, which caps gain at 256x but gives us more residual bits
        if (!sensor_->setModGainTableSelect(true)) { break; }

        // Set maximum gain to 256x per app note on residual measurement
        if (!sensor_->setMaxModGain(TSL2585_GAIN_256X)) { break; }

        // Enable modulator 0
        if (!sensor_->enableModulators(TSL2585_MOD0)) { break; }

        // Enable internal calibration on every sequencer round
        if (!sensor_->setCalibrationNthIteration(1)) { break; }

        // Configure photodiodes
        if (!sensor_->setModPhotodiodeSmux(TSL2585_STEP0, sensor_tsl2585_phd_mod_vis)) { break; }

        // Set initial gain
        if (!sensor_->setModGain(TSL2585_MOD0, TSL2585_STEP0, static_cast<tsl2585_gain_t>(sensorGain_))) { break; }

        // Set initial integration time
        if (!sensor_->setSampleTime(sensorSampleTime_)) { break; }
        if (!sensor_->setAlsNumSamples(sensorSampleCount_)) { break; }

        if (sensorAgcEnabled_) {
            // Enable AGC
            if (!sensor_->setAgcNumSamples(sensorAgcCount_)) { break; }
            if (!sensor_->setAgcCalibration(sensorAgcEnabled_)) { break; }

        } else {
            // Disable AGC
            if (!sensor_->setAgcCalibration(false)) { break; }
            if (!sensor_->setAgcNumSamples(0)) { break; }
        }

        // Enable sensor interrupts
        if (!sensor_->setAlsInterruptPersistence(0)) { break; }
        if (!sensor_->setFifoThreshold(255)) { break; }
        if (!sensor_->setInterruptEnable(TSL2585_INTENAB_AIEN)) { break; }

        // Enable the sensor
        if (!sensor_->enable()) { break; }

        discardNextReading_ = true;
        agcDisabledResetGain_ = false;
        sensorRunning_ = true;
    } while (0);

    if (!sensorRunning_) {
        sensor_->disable();
    }

    return sensorRunning_;
}

bool StickInterface::sensorStop()
{
    if (!sensorRunning_) { return false; }

    if (sensor_->disable()) {
        sensorRunning_ = false;
    }

    return !sensorRunning_;
}

void StickInterface::onSensorInterrupt()
{
    uint8_t status = 0;
    StickReading result;
    bool notifyReading = false;
    if (!sensor_->getStatus(&status)) {
        qWarning() << "Unable to get interrupt status";
        return;
    }

    if ((status & TSL2585_STATUS_AINT) != 0) {
        result = readSensor();
        if (discardNextReading_) {
            discardNextReading_ = false;
        } else if (result.status() != StickReading::ResultInvalid) {
            sensorGain_ = result.gain();
            notifyReading = true;
        }
    }

    if (agcDisabledResetGain_) {
        if (!sensor_->setModGain(TSL2585_MOD0, TSL2585_STEP0, static_cast<tsl2585_gain_t>(sensorGain_))) {
            qWarning() << "Unable to reset gain after disabling AGC";
        }
        agcDisabledResetGain_ = false;
        discardNextReading_ = true;
    }

    if (status != 0) {
        if (!sensor_->setStatus(status)) {
            qWarning() << "Unable to clear interrupt status";
        }
    }

    if (notifyReading) {
        emit sensorReading(result);
    }
}

StickReading StickInterface::readSensor()
{
    tsl2585_fifo_status_t fifo_status;
    const uint8_t data_size = 7;
    uint8_t counter = 0;
    QByteArray data;
    uint32_t als_data0 = 0;
    uint8_t als_status = 0;
    uint8_t als_status2 = 0;
    uint8_t als_status3 = 0;
    bool overflow = false;
    bool empty = false;

    do {
        if (!sensor_->getFifoStatus(&fifo_status)) { break; }

        if (fifo_status.overflow) {
            qWarning() << "FIFO overflow, clearing";
            if (!sensor_->clearFifo()) { break; }

            overflow = true;
            break;
        } else if (fifo_status.level < data_size) {
            // Short-cut out if there is no data in the FIFO
            empty = true;
            break;
        }

        while (fifo_status.level >= data_size) {
            data = sensor_->readFifo(data_size);
            if (data.isEmpty()) { break; }

            if (!sensor_->getFifoStatus(&fifo_status)) { data.clear(); break; }

            counter++;
        }
        if (data.isEmpty()) { break; }

        if (counter > 1) {
            qWarning() << "Missed" << (counter - 1) << "sensor read cycles";
        }

        QDataStream in(data);
        in.setByteOrder(QDataStream::LittleEndian);
        in >> als_data0;
        in >> als_status;
        in >> als_status2;
        in >> als_status3;
    } while (0);

    Q_UNUSED(als_status3);

    tsl2585_gain_t gain = static_cast<tsl2585_gain_t>(als_status2 & 0x0F);
    StickReading::Status status = StickReading::ResultInvalid;
    uint32_t reading = 0;

    if (overflow) {
        status = StickReading::ResultOverflow;
    } else if ((als_status & TSL2585_ALS_DATA0_ANALOG_SATURATION_STATUS) != 0) {
        status = StickReading::ResultSaturated;
    } else if (!empty) {
        status = StickReading::ResultValid;
        reading = als_data0;
    }

    return StickReading(status, gain, reading);
}
