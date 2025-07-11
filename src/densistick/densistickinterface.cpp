#include "densistickinterface.h"

#include <QDebug>
#include "ft260.h"
#include "tsl2585.h"
#include "m24c08.h"
#include "densisticksettings.h"
#include "densistickreading.h"

namespace
{
static const uint8_t EEPROM_ADDRESS = 0x50;
static const uint8_t MCP4017_ADDRESS = 0x2F;

/* TSL2585: Only enable Photopic photodiodes on modulator 0 */
static const photodiode_modulator_array_t sensor_tsl2585_phd_mod_vis{
    TSL2585_MOD_NONE, TSL2585_MOD0, TSL2585_MOD_NONE, TSL2585_MOD_NONE, TSL2585_MOD_NONE, TSL2585_MOD0
};

/* TSL2522: Only enable Photopic photodiodes on modulator 0 */
static const photodiode_modulator_array_t sensor_tsl2522_phd_mod_vis{
    TSL2585_MOD_NONE, TSL2585_MOD0, TSL2585_MOD0, TSL2585_MOD0, TSL2585_MOD0, TSL2585_MOD_NONE
};

}

DensiStickInterface::DensiStickInterface(Ft260 *ft260, QObject *parent)
    : QObject(parent), ft260_(ft260)
{
    if (ft260_ && !ft260_->parent()) {
        ft260_->setParent(this);
    }
    connect(ft260_, &Ft260::connectionClosed, this, &DensiStickInterface::onConnectionClosed);
}

DensiStickInterface::~DensiStickInterface()
{
    disconnect(ft260_, &Ft260::connectionClosed, this, &DensiStickInterface::onConnectionClosed);
    shutdown_ = true;
    close();
}

bool DensiStickInterface::open()
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
    settings_ = new DensiStickSettings(eeprom_);
    if (!settings_->init()) {
        qWarning() << "Unable to read settings";
        close();
        return false;
    }

    hasSettings_ = settings_->headerValid();

    // Create the light sensor device handler
    sensor_ = new TSL2585(ft260_);
    tsl2585_ident_t ident;
    if (!sensor_->init(&ident)) {
        qWarning() << "Unable to initialize sensor";
        close();
        return false;
    }
    sensorType_ = TSL2585::sensorType(&ident);

    // Populate the version string
    Ft260ChipVersion chipVersion;
    if (ft260_->chipVersion(&chipVersion)) {
        ft260Version_ = QString("%1%2-%3.%4")
                            .arg(chipVersion.chip[0], 2, 16, QChar('0'))
                            .arg(chipVersion.chip[1], 2, 16, QChar('0'))
                            .arg(chipVersion.major)
                            .arg(chipVersion.minor);
    } else {
        ft260Version_.clear();
    }

    // Populate the system clock string
    switch (ft260_->systemClock()) {
    case FT260_CLOCK_12MHZ:
        ft260SystemClock_ = QLatin1String("12 MHz");
        break;
    case FT260_CLOCK_24MHZ:
        ft260SystemClock_ = QLatin1String("24 MHz");
        break;
    case FT260_CLOCK_48MHZ:
        ft260SystemClock_ = QLatin1String("48 MHz");
        break;
    case FT260_CLOCK_MAX:
    default:
        ft260SystemClock_.clear();
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

    connect(ft260_, &Ft260::buttonInterrupt, this, &DensiStickInterface::buttonEvent);
    connect(ft260_, &Ft260::sensorInterrupt, this, &DensiStickInterface::onSensorInterrupt);

    connected_ = true;

    return true;
}

void DensiStickInterface::close()
{
    setLightEnable(false);

    hasSettings_ = false;
    connected_ = false;
    sensorRunning_ = false;

    disconnect(ft260_, &Ft260::buttonInterrupt, this, &DensiStickInterface::buttonEvent);
    disconnect(ft260_, &Ft260::sensorInterrupt, this, &DensiStickInterface::onSensorInterrupt);

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

void DensiStickInterface::onConnectionClosed()
{
    if (connected_) {
        close();
    }
}

bool DensiStickInterface::connected() const
{
    return connected_;
}

bool DensiStickInterface::hasSettings() const
{
    return hasSettings_;
}

bool DensiStickInterface::running() const
{
    return sensorRunning_;
}

QString DensiStickInterface::version() const
{
    return ft260Version_;
}

QString DensiStickInterface::systemClock() const
{
    return ft260SystemClock_;
}

QString DensiStickInterface::serialNumber() const
{
    if (ft260_) {
        return ft260_->deviceInfo().serialNumber();
    } else {
        return QString();
    }
}

DensiStickSettings *DensiStickInterface::settings()
{
    return settings_;
}

bool DensiStickInterface::setLightEnable(bool enable)
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

bool DensiStickInterface::lightEnabled() const
{
    return (gpioReport_.gpio_ex_value & 0x80) != 0;
}

bool DensiStickInterface::setLightBrightness(quint8 value)
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

quint8 DensiStickInterface::lightBrightness() const
{
    return lightBrightness_;
}

float DensiStickInterface::lightCurrent() const
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

bool DensiStickInterface::setSensorGain(int gain)
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

bool DensiStickInterface::setSensorIntegration(int sampleTime, int sampleCount)
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

bool DensiStickInterface::setSensorAgcEnable(int sampleCount)
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

bool DensiStickInterface::setSensorAgcDisable()
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

bool DensiStickInterface::sensorStart()
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
        if (sensorType_ == SENSOR_TYPE_TSL2522) {
            if (!sensor_->setModPhotodiodeSmux(TSL2585_STEP0, sensor_tsl2522_phd_mod_vis)) { break; }
        } else if (sensorType_ == SENSOR_TYPE_TSL2585) {
            if (!sensor_->setModPhotodiodeSmux(TSL2585_STEP0, sensor_tsl2585_phd_mod_vis)) { break; }
        } else {
            qWarning() << "Unsupported sensor type, cannot configure SMUX";
            break;
        }

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

bool DensiStickInterface::sensorStop()
{
    if (!sensorRunning_) { return false; }

    if (sensor_->disable()) {
        sensorRunning_ = false;
    }

    return !sensorRunning_;
}

void DensiStickInterface::onSensorInterrupt()
{
    uint8_t status = 0;
    DensiStickReading result;
    bool notifyReading = false;
    if (!sensor_->getStatus(&status)) {
        qWarning() << "Unable to get interrupt status";
        return;
    }

    if ((status & TSL2585_STATUS_AINT) != 0) {
        result = readSensor();
        if (discardNextReading_) {
            discardNextReading_ = false;
        } else if (result.status() != DensiStickReading::ResultInvalid) {
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

DensiStickReading DensiStickInterface::readSensor()
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
    DensiStickReading::Status status = DensiStickReading::ResultInvalid;
    uint32_t reading = 0;

    if (overflow) {
        status = DensiStickReading::ResultOverflow;
    } else if ((als_status & TSL2585_ALS_DATA0_ANALOG_SATURATION_STATUS) != 0) {
        status = DensiStickReading::ResultSaturated;
    } else if (!empty) {
        status = DensiStickReading::ResultValid;
        reading = als_data0;
    }

    return DensiStickReading(status, gain, reading);
}
