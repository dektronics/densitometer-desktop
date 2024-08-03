#include "tsl2585calibration.h"

#include <QVector>

#include "tsl2585.h"

class Tsl2585CalSlopeData : public QSharedData
{
public:
    float b0 = qSNaN();
    float b1 = qSNaN();
    float b2 = qSNaN();
};

Tsl2585CalSlope::Tsl2585CalSlope() : data(new Tsl2585CalSlopeData)
{
}

Tsl2585CalSlope::Tsl2585CalSlope(const Tsl2585CalSlope &rhs)
    : data{rhs.data}
{
}

Tsl2585CalSlope &Tsl2585CalSlope::operator=(const Tsl2585CalSlope &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

Tsl2585CalSlope::~Tsl2585CalSlope()
{
}

void Tsl2585CalSlope::setB0(float b0) { data->b0 = b0; }
float Tsl2585CalSlope::b0() const { return data->b0; }

void Tsl2585CalSlope::setB1(float b1) { data->b1 = b1; }
float Tsl2585CalSlope::b1() const { return data->b1; }

void Tsl2585CalSlope::setB2(float b2) { data->b2 = b2; }
float Tsl2585CalSlope::b2() const { return data->b2; }

bool Tsl2585CalSlope::isValid() const
{
    // Invalid if any values are NaN
    if (qIsNaN(data->b0) || qIsNaN(data->b1) || qIsNaN(data->b2)) {
        return false;
    }

    return true;
}

class Tsl2585CalTargetData : public QSharedData
{
public:
    float loDensity = qSNaN();
    float loReading = qSNaN();
    float hiDensity = qSNaN();
    float hiReading = qSNaN();
};

Tsl2585CalTarget::Tsl2585CalTarget() : data(new Tsl2585CalTargetData)
{
}

Tsl2585CalTarget::Tsl2585CalTarget(const Tsl2585CalTarget &rhs)
    : data{rhs.data}
{
}

Tsl2585CalTarget &Tsl2585CalTarget::operator=(const Tsl2585CalTarget &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

Tsl2585CalTarget::~Tsl2585CalTarget()
{
}

void Tsl2585CalTarget::setLoDensity(float loDensity) { data->loDensity = loDensity; }
float Tsl2585CalTarget::loDensity() const { return data->loDensity; }

void Tsl2585CalTarget::setLoReading(float loValue) { data->loReading = loValue; }
float Tsl2585CalTarget::loReading() const { return data->loReading; }

void Tsl2585CalTarget::setHiDensity(float hiDensity) { data->hiDensity = hiDensity; }
float Tsl2585CalTarget::hiDensity() const { return data->hiDensity; }

void Tsl2585CalTarget::setHiReading(float hiValue) { data->hiReading = hiValue; }
float Tsl2585CalTarget::hiReading() const { return data->hiReading; }

bool Tsl2585CalTarget::isValid() const
{
    // Invalid if any values are NaN
    if (qIsNaN(data->loDensity) || qIsNaN(data->loReading)
        || qIsNaN(data->hiDensity) || qIsNaN(data->hiReading)) {
        return false;
    }

    // Invalid if any values are less than zero
    if (data->loDensity < 0 || data->loReading < 0
        || data->hiDensity < 0 || data->hiReading < 0) {
        return false;
    }

    // Invalid if low density is greater than high density
    if (data->loDensity >= data->hiDensity) {
        return false;
    }

    // Invalid if low reading is less than high reading
    if (data->loReading <= data->hiReading) {
        return false;
    }

    // Low density must be greater than zero
    if (data->loDensity < 0.01F) {
        return false;
    }

    return true;
}

class Tsl2585CalibrationData : public QSharedData
{
public:
    Tsl2585CalibrationData() : empty(true)
    {
        for (size_t i = 0; i <= static_cast<size_t>(TSL2585_GAIN_256X); i++) {
            gainCalibration.append(qSNaN());
        }
    }
    bool empty;
    QVector<float> gainCalibration;
    Tsl2585CalSlope slopeCalibration;
    Tsl2585CalTarget targetCalibration;
};

Tsl2585Calibration::Tsl2585Calibration() : data(new Tsl2585CalibrationData)
{
}

Tsl2585Calibration::Tsl2585Calibration(const Tsl2585Calibration &rhs)
    : data{rhs.data}
{
}

Tsl2585Calibration &Tsl2585Calibration::operator=(const Tsl2585Calibration &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

Tsl2585Calibration::~Tsl2585Calibration()
{
}

bool Tsl2585Calibration::isEmpty() const
{
    return data->empty;
}

float Tsl2585Calibration::gainCalibration(tsl2585_gain_t gain) const
{
    if (gain > TSL2585_GAIN_256X) { return qSNaN(); }
    return data->gainCalibration[gain];
}

void Tsl2585Calibration::setGainCalibration(tsl2585_gain_t gain, float value)
{
    if (gain > TSL2585_GAIN_256X) { return; }
    data->empty = false;
    data->gainCalibration[gain] = value;
}

Tsl2585CalSlope Tsl2585Calibration::slopeCalibration() const
{
    return data->slopeCalibration;
}

void Tsl2585Calibration::setSlopeCalibration(const Tsl2585CalSlope &calSlope)
{
    data->empty = false;
    data->slopeCalibration = calSlope;
}

Tsl2585CalTarget Tsl2585Calibration::targetCalibration() const
{
    return data->targetCalibration;
}

void Tsl2585Calibration::setTargetCalibration(const Tsl2585CalTarget &calTarget)
{
    data->empty = false;
    data->targetCalibration = calTarget;
}
