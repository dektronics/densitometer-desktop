#include "stickrunner.h"

#include <QDebug>

#include "sticksettings.h"

namespace
{
static const tsl2585_gain_t STARTING_GAIN = TSL2585_GAIN_256X;
static const quint16 SAMPLE_TIME = 719;
static const quint16 SAMPLE_COUNT = 199;
static const quint16 AGC_SAMPLE_COUNT = 49;
static const int SKIP_COUNT = 2;
static const int READING_COUNT = 2;
}

StickRunner::StickRunner(StickInterface *stickInterface, QObject *parent)
    : QObject{parent}, stickInterface_(stickInterface)
{
    if (stickInterface_ && !stickInterface_->parent()) {
        stickInterface_->setParent(this);
    }

    connect(stickInterface_, &StickInterface::buttonEvent, this, &StickRunner::onButtonEvent);
    connect(stickInterface_, &StickInterface::sensorReading, this, &StickRunner::onSensorReading);
}

StickInterface *StickRunner::stickInterface()
{
    return stickInterface_;
}

void StickRunner::setEnabled(bool enabled)
{
    enabled_ = enabled;
}

bool StickRunner::enabled() const
{
    return enabled_;
}

void StickRunner::reloadCalibration()
{
    if (!stickInterface_ || !stickInterface_->hasSettings()) { return; }
    calData_ = stickInterface_->settings()->readCalTsl2585();
}

void StickRunner::onButtonEvent(bool pressed)
{
    if (pressed && enabled_ && !measuring_ && !calData_.isEmpty()) {
        startMeasurement();
    }
}

void StickRunner::onSensorReading(const StickReading& reading)
{
    if (!measuring_) { return; }

    if (skipCount_ > 0) {
        skipCount_--;
        return;
    }

    if (reading.status() != StickReading::ResultValid) {
        readingList_.clear();
        return;
    }

    if (readingList_.size() > 0 && readingList_.last().gain() != reading.gain()) {
        readingList_.clear();
    }

    readingList_.append(reading);

    if (readingList_.size() >= READING_COUNT) {
        finishMeasurement();
    }
}

void StickRunner::startMeasurement()
{
    qDebug() << "Measuring target";
    stickInterface_->setLightBrightness(0);
    stickInterface_->setSensorConfig(STARTING_GAIN, SAMPLE_TIME, SAMPLE_COUNT);
    stickInterface_->setSensorAgcEnable(AGC_SAMPLE_COUNT);

    stickInterface_->setLightEnable(true);
    stickInterface_->sensorStart();
    skipCount_ = SKIP_COUNT;
    readingList_.clear();
    measuring_ = true;
}

void StickRunner::finishMeasurement()
{
    measuring_ = false;
    stickInterface_->setLightEnable(false);
    stickInterface_->sensorStop();

    float sum = 0;
    size_t count = 0;
    for (const StickReading& reading : readingList_) {
        if (reading.status() == StickReading::ResultValid) {
            sum += (float)reading.reading();
            count++;
        }
    }
    float rawReading = sum / (float)count;

    const float timeMs = TSL2585::integrationTimeMs(SAMPLE_TIME, SAMPLE_COUNT);

    float gainValue = calData_.gainCalibration(readingList_.last().gain());
    if (qIsNaN(gainValue) || gainValue <= 0.0F || gainValue > 512.0F) {
        qWarning() << "Bad gain calibration value:" << gainValue;
        gainValue = TSL2585::gainValue(readingList_.last().gain());
    }

    float alsReading = (float)rawReading / 16.0F;
    float basicReading = alsReading / (timeMs * gainValue);

    qDebug() << "Reading:" << rawReading << basicReading;
    emit targetMeasurement(basicReading);

    const Tsl2585CalTarget calTarget = calData_.targetCalibration();

    /* Convert all values into log units */
    float meas_ll = std::log10(basicReading);
    float cal_hi_ll = std::log10(calTarget.hiReading());
    float cal_lo_ll = std::log10(calTarget.loReading());

    /* Calculate the slope of the line */
    float m = (calTarget.hiDensity() - calTarget.loDensity()) / (cal_hi_ll - cal_lo_ll);

    /* Calculate the measured density */
    float meas_d = (m * (meas_ll - cal_lo_ll)) + calTarget.loDensity();
    qDebug() << "Target density:" << meas_d;
    emit targetDensity(meas_d);
}
