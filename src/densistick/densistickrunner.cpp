#include "densistickrunner.h"

#include <QDateTime>
#include <QTimer>
#include <QDebug>

#include "densisticksettings.h"

namespace
{
static const tsl2585_gain_t STARTING_GAIN = TSL2585_GAIN_256X;
static const quint16 SAMPLE_TIME = 719;
static const quint16 SAMPLE_COUNT = 199;
static const quint16 PRE_SAMPLE_COUNT = 29;
static const quint16 AGC_SAMPLE_COUNT = 19;
static const int READING_COUNT = 2;
}

DensiStickRunner::DensiStickRunner(DensiStickInterface *stickInterface, QObject *parent)
    : QObject{parent}, stickInterface_(stickInterface), measStartTime_(0), agcStep_(0)
{
    if (stickInterface_ && !stickInterface_->parent()) {
        stickInterface_->setParent(this);
    }

    connect(stickInterface_, &DensiStickInterface::buttonEvent, this, &DensiStickRunner::onButtonEvent);
    connect(stickInterface_, &DensiStickInterface::sensorReading, this, &DensiStickRunner::onSensorReading);
}

DensiStickInterface *DensiStickRunner::stickInterface()
{
    return stickInterface_;
}

void DensiStickRunner::setEnabled(bool enabled)
{
    if (enabled_ != enabled) {
        if (enabled) {
            stickInterface_->setLightBrightness(127);
            stickInterface_->setLightEnable(true);
        } else {
            stickInterface_->setLightEnable(false);
        }
        enabled_ = enabled;
    }
}

bool DensiStickRunner::enabled() const
{
    return enabled_;
}

void DensiStickRunner::reloadCalibration()
{
    if (!stickInterface_ || !stickInterface_->hasSettings()) { return; }
    calData_ = stickInterface_->settings()->readCalTsl2585();
}

void DensiStickRunner::onButtonEvent(bool pressed)
{
    if (pressed && enabled_ && !measuring_ && !calData_.isEmpty()) {
        startMeasurement();
    }
}

void DensiStickRunner::onSensorReading(const DensiStickReading& reading)
{
    if (!measuring_) { return; }

    qDebug() << reading << (QDateTime::currentMSecsSinceEpoch() - measStartTime_);

    if (reading.status() != DensiStickReading::ResultValid) {
        readingList_.clear();
        return;
    }

    if (agcStep_ == 1) {
        // Disable AGC
        stickInterface_->setSensorAgcDisable();
        agcStep_++;
        return;
    } else if (agcStep_ == 2) {
        // Set measurement sample time
        stickInterface_->setSensorIntegration(SAMPLE_TIME, SAMPLE_COUNT);
        agcStep_ = 0;
        return;
    }

    readingList_.append(reading);

    if (readingList_.size() >= READING_COUNT) {
        finishMeasurement();
    }
}

void DensiStickRunner::startMeasurement()
{
    qDebug() << "Measuring target";
    measStartTime_ = QDateTime::currentMSecsSinceEpoch();
    stickInterface_->setLightBrightness(0);
    stickInterface_->setSensorGain(STARTING_GAIN);
    stickInterface_->setSensorIntegration(SAMPLE_TIME, PRE_SAMPLE_COUNT);
    stickInterface_->setSensorAgcEnable(AGC_SAMPLE_COUNT);

    stickInterface_->setLightEnable(true);
    stickInterface_->sensorStart();
    readingList_.clear();
    agcStep_ = 1;
    measuring_ = true;
}

void DensiStickRunner::finishMeasurement()
{
    measuring_ = false;
    stickInterface_->setLightEnable(false);
    stickInterface_->sensorStop();
    stickInterface_->setLightBrightness(127);
    stickInterface_->setLightEnable(true);

    qint64 measDuration = QDateTime::currentMSecsSinceEpoch() - measStartTime_;
    qDebug() << "Measurement completed in" << measDuration << "ms";

    float sum = 0;
    size_t count = 0;
    for (const DensiStickReading& reading : std::as_const(readingList_)) {
        if (reading.status() == DensiStickReading::ResultValid) {
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

    qDebug() << "Reading:" << Qt::fixed << rawReading << basicReading;
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
    qDebug() << "Target density:" << Qt::fixed << meas_d;
    emit targetDensity(meas_d);
}
