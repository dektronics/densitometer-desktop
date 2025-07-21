#include "peripheralcalvalues.h"

#include <array>

class PeripheralCalGainData : public QSharedData
{
public:
    PeripheralCalGainData() : empty(true)
    {
        for (size_t i = 0; i <= static_cast<size_t>(PeripheralCalGain::Gain256X); i++) {
            values.append(qSNaN());
        }
    }
    bool empty;
    QVector<float> values;
};

class PeripheralCalLinearTargetData : public QSharedData
{
public:
    float slope = qSNaN();
    float intercept = qSNaN();
};

class PeripheralCalDensityTargetData : public QSharedData
{
public:
    float loDensity = qSNaN();
    float loReading = qSNaN();
    float hiDensity = qSNaN();
    float hiReading = qSNaN();
};

class MeterProbeCalibrationData : public QSharedData
{
public:
    MeterProbeCalibrationData() : empty(true)
    {
    }
    bool empty;
    PeripheralCalGain gainCalibration;
    PeripheralCalLinearTarget targetCalibration;
};

class DensiStickCalibrationData : public QSharedData
{
public:
    DensiStickCalibrationData() : empty(true)
    {
    }
    bool empty;
    PeripheralCalGain gainCalibration;
    PeripheralCalDensityTarget targetCalibration;
};


PeripheralCalGain::PeripheralCalGain() : data(new PeripheralCalGainData)
{
}

PeripheralCalGain::PeripheralCalGain(const PeripheralCalGain &rhs)
    : data{rhs.data}
{
}

PeripheralCalGain::~PeripheralCalGain()
{
}

PeripheralCalGain &PeripheralCalGain::operator=(const PeripheralCalGain &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

bool PeripheralCalGain::isEmpty() const
{
    return data->empty;
}

float PeripheralCalGain::gainSpecValue(GainLevel gainLevel)
{
    static std::array gainSpecValues{
        0.5F, 1.0F, 2.0F, 4.0F, 8.0F, 16.0F, 32.0F, 64.0F,
        128.0F, 256.0F, 512.0F, 1024.0F, 2048.0F, 4096.0F
    };

    const size_t gainIndex = static_cast<size_t>(gainLevel);

    if (gainIndex < gainSpecValues.size()) {
        return gainSpecValues[gainIndex];
    } else {
        return qSNaN();
    }
}

float PeripheralCalGain::gainValue(PeripheralCalGain::GainLevel gainLevel) const
{
    if (gainLevel > Gain256X) { return qSNaN(); }
    return data->values[static_cast<size_t>(gainLevel)];
}

void PeripheralCalGain::setGainValue(PeripheralCalGain::GainLevel gainLevel, float value)
{
    if (gainLevel > Gain256X) { return; }
    data->empty = false;
    data->values[static_cast<size_t>(gainLevel)] = value;
}

bool PeripheralCalGain::isValid() const
{
    if (data->empty) { return false; }

    for (size_t i = 0; i < static_cast<size_t>(PeripheralCalGain::Gain256X); i++) {
        const float curValue = data->values[i];
        const float nextValue = data->values[i + 1];
        if (qIsNaN(curValue)) { return false; }
        if (curValue >= nextValue) { return false; }
    }

    return true;
}


PeripheralCalLinearTarget::PeripheralCalLinearTarget() : data(new PeripheralCalLinearTargetData)
{
}

PeripheralCalLinearTarget::PeripheralCalLinearTarget(const PeripheralCalLinearTarget &rhs)
    : data{rhs.data}
{
}

PeripheralCalLinearTarget &PeripheralCalLinearTarget::operator=(const PeripheralCalLinearTarget &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

PeripheralCalLinearTarget::~PeripheralCalLinearTarget()
{
}

void PeripheralCalLinearTarget::setSlope(float slope) { data->slope = slope; }
float PeripheralCalLinearTarget::slope() const { return data->slope; }

void PeripheralCalLinearTarget::setIntercept(float intercept) { data->intercept = intercept; }
float PeripheralCalLinearTarget::intercept() const { return data->intercept; }

bool PeripheralCalLinearTarget::isValid() const
{
    // Invalid if any values are NaN
    if (qIsNaN(data->slope) || qIsNaN(data->intercept)) {
        return false;
    }

    return true;
}


PeripheralCalDensityTarget::PeripheralCalDensityTarget() : data(new PeripheralCalDensityTargetData)
{
}

PeripheralCalDensityTarget::PeripheralCalDensityTarget(const PeripheralCalDensityTarget &rhs)
    : data{rhs.data}
{
}

PeripheralCalDensityTarget &PeripheralCalDensityTarget::operator=(const PeripheralCalDensityTarget &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

PeripheralCalDensityTarget::~PeripheralCalDensityTarget()
{
}

void PeripheralCalDensityTarget::setLoDensity(float loDensity) { data->loDensity = loDensity; }
float PeripheralCalDensityTarget::loDensity() const { return data->loDensity; }

void PeripheralCalDensityTarget::setLoReading(float loValue) { data->loReading = loValue; }
float PeripheralCalDensityTarget::loReading() const { return data->loReading; }

void PeripheralCalDensityTarget::setHiDensity(float hiDensity) { data->hiDensity = hiDensity; }
float PeripheralCalDensityTarget::hiDensity() const { return data->hiDensity; }

void PeripheralCalDensityTarget::setHiReading(float hiValue) { data->hiReading = hiValue; }
float PeripheralCalDensityTarget::hiReading() const { return data->hiReading; }

bool PeripheralCalDensityTarget::isValid() const
{
    // Invalid if any values are NaN
    if (qIsNaN(data->loDensity) || qIsNaN(data->loReading)
        || qIsNaN(data->hiDensity) || qIsNaN(data->hiReading)) {
        return false;
    }

    // Invalid if CAL-LO is less than -0.5, or any other values are less than zero
    if (data->loDensity < -0.5F || data->loReading < 0
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

    return true;
}

bool PeripheralCalDensityTarget::isValidLoOnly() const
{
    // Invalid if low values are NaN
    if (qIsNaN(data->loDensity) || qIsNaN(data->loReading)) {
        return false;
    }

    // Invalid if CAL-LO is less than -0.5
    if (data->loDensity < -0.5F || data->loReading < 0) {
        return false;
    }

    // Invalid if either CAL-HI value is not NAN
    if (!qIsNaN(data->hiDensity) || !qIsNaN(data->hiReading)) {
        return false;
    }

    return true;
}

bool PeripheralCalDensityTarget::isValidReflection() const
{
    // Do general validity check
    if (!isValid()) {
        return false;
    }

    // Low density must be greater than zero
    if (data->loDensity < 0.01F) {
        return false;
    }

    return true;
}

bool PeripheralCalDensityTarget::isValidTransmission() const
{
    // Do general validity check
    if (!isValid()) {
        return false;
    }

    // Low density must be effectively zero
    if (qAbs(data->loDensity) > 0.001F) {
        return false;
    }

    return true;
}


MeterProbeCalibration::MeterProbeCalibration() : data(new MeterProbeCalibrationData)
{
}

MeterProbeCalibration::MeterProbeCalibration(const MeterProbeCalibration &rhs)
    : data{rhs.data}
{
}

MeterProbeCalibration &MeterProbeCalibration::operator=(const MeterProbeCalibration &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

MeterProbeCalibration::~MeterProbeCalibration()
{
}

bool MeterProbeCalibration::isEmpty() const
{
    return data->empty;
}

PeripheralCalGain MeterProbeCalibration::gainCalibration() const { return data->gainCalibration; }

void MeterProbeCalibration::setGainCalibration(const PeripheralCalGain &gainCal)
{
    data->empty = false;
    data->gainCalibration = gainCal;
}

PeripheralCalLinearTarget MeterProbeCalibration::targetCalibration() const { return data->targetCalibration; }

void MeterProbeCalibration::setTargetCalibration(const PeripheralCalLinearTarget &targetCal)
{
    data->empty = false;
    data->targetCalibration = targetCal;
}


DensiStickCalibration::DensiStickCalibration() : data(new DensiStickCalibrationData)
{
}

DensiStickCalibration::DensiStickCalibration(const DensiStickCalibration &rhs)
    : data{rhs.data}
{
}

DensiStickCalibration &DensiStickCalibration::operator=(const DensiStickCalibration &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

DensiStickCalibration::~DensiStickCalibration()
{
}

bool DensiStickCalibration::isEmpty() const
{
    return data->empty;
}

PeripheralCalGain DensiStickCalibration::gainCalibration() const { return data->gainCalibration; }

void DensiStickCalibration::setGainCalibration(const PeripheralCalGain &gainCal)
{
    data->empty = false;
    data->gainCalibration = gainCal;
}

PeripheralCalDensityTarget DensiStickCalibration::targetCalibration() const { return data->targetCalibration; }

void DensiStickCalibration::setTargetCalibration(const PeripheralCalDensityTarget &targetCal)
{
    data->empty = false;
    data->targetCalibration = targetCal;
}
