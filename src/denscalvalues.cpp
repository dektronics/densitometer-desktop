#include "denscalvalues.h"

#include <QJsonValue>
#include <QJsonObject>
#include <QJsonArray>

#include <array>
#include "util.h"

class DensCalLightData : public QSharedData
{
public:
    int reflectionValue = 0;
    int transmissionValue = 0;
};

class DensCalGainData : public QSharedData
{
public:
    float low0 = qSNaN();
    float low1 = qSNaN();
    float med0 = qSNaN();
    float med1 = qSNaN();
    float high0 = qSNaN();
    float high1 = qSNaN();
    float max0 = qSNaN();
    float max1 = qSNaN();
};

class DensUvVisCalGainData : public QSharedData
{
public:
    DensUvVisCalGainData() : empty(true)
    {
        for (size_t i = 0; i <= static_cast<size_t>(DensUvVisCalGain::Gain256X); i++) {
            values.append(qSNaN());
        }
    }
    bool empty;
    QVector<float> values;
};

class DensCalCoefficientSetData : public QSharedData
{
public:
    float b0 = qSNaN();
    float b1 = qSNaN();
    float b2 = qSNaN();
};

class DensCalTargetData : public QSharedData
{
public:
    float loDensity = qSNaN();
    float loReading = qSNaN();
    float hiDensity = qSNaN();
    float hiReading = qSNaN();
};

DensCalLight::DensCalLight() : data(new DensCalLightData)
{
}

DensCalLight::DensCalLight(const DensCalLight &rhs)
    : data{rhs.data}
{
}

DensCalLight &DensCalLight::operator=(const DensCalLight &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

DensCalLight::~DensCalLight()
{
}

void DensCalLight::setReflectionValue(int reflectionValue) { data->reflectionValue = reflectionValue; }
int DensCalLight::reflectionValue() const { return data->reflectionValue; }

void DensCalLight::setTransmissionValue(int transmissionValue) { data->transmissionValue = transmissionValue; }
int DensCalLight::transmissionValue() const { return data->transmissionValue; }

bool DensCalLight::isValid() const
{
    if (data->reflectionValue <= 0 || data->transmissionValue <= 0) {
        return false;
    }

    if (data->reflectionValue > 128 || data->transmissionValue > 128) {
        return false;
    }

    return true;
}

DensCalGain::DensCalGain() : data(new DensCalGainData)
{
}

DensCalGain::DensCalGain(const DensCalGain &rhs)
    : data{rhs.data}
{
}

DensCalGain &DensCalGain::operator=(const DensCalGain &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

DensCalGain::~DensCalGain()
{
}

void DensCalGain::setLow0(float low0) { data->low0 = low0; }
float DensCalGain::low0() const { return data->low0; }

void DensCalGain::setLow1(float low1) { data->low1 = low1; }
float DensCalGain::low1() const { return data->low1; }

void DensCalGain::setMed0(float med0) { data->med0 = med0; }
float DensCalGain::med0() const { return data->med0; }

void DensCalGain::setMed1(float med1) { data->med1 = med1; }
float DensCalGain::med1() const { return data->med1; }

void DensCalGain::setHigh0(float high0) { data->high0 = high0; }
float DensCalGain::high0() const { return data->high0; }

void DensCalGain::setHigh1(float high1) { data->high1 = high1; }
float DensCalGain::high1() const { return data->high1; }

void DensCalGain::setMax0(float max0) { data->max0 = max0; }
float DensCalGain::max0() const { return data->max0; }

void DensCalGain::setMax1(float max1) { data->max1 = max1; }
float DensCalGain::max1() const { return data->max1; }

bool DensCalGain::isValid() const
{
    // Invalid if any values are NaN
    if (qIsNaN(data->low0) || qIsNaN(data->low1)
            || qIsNaN(data->med0) || qIsNaN(data->med1)
            || qIsNaN(data->high0) || qIsNaN(data->high1)
            || qIsNaN(data->max0) || qIsNaN(data->max1)) {
        return false;
    }

    // Invalid if low values are not effectively 1x
    if (qAbs(1.0F - data->low0) > 0.001F || qAbs(1.0F - data->low1) > 0.001F) {
        return false;
    }

    // Invalid if low is greater than medium
    if (data->low0 >= data->med0 || data->low1 >= data->med1) {
        return false;
    }

    // Invalid if medium is greater than high
    if (data->med0 >= data->high0 || data->med1 >= data->high1) {
        return false;
    }

    // Invalid if high is greater than max
    if (data->high0 >= data->max0 || data->high1 >= data->max1) {
        return false;
    }

    return true;
}

QJsonValue DensCalGain::toJson() const
{
    QJsonObject jsonCalGain;
    jsonCalGain["L0"] = QString::number(data->low0, 'f', 6);
    jsonCalGain["L1"] = QString::number(data->low1, 'f', 6);
    jsonCalGain["M0"] = QString::number(data->med0, 'f', 6);
    jsonCalGain["M1"] = QString::number(data->med1, 'f', 6);
    jsonCalGain["H0"] = QString::number(data->high0, 'f', 6);
    jsonCalGain["H1"] = QString::number(data->high1, 'f', 6);
    jsonCalGain["X0"] = QString::number(data->max0, 'f', 6);
    jsonCalGain["X1"] = QString::number(data->max1, 'f', 6);
    return jsonCalGain;
}

DensCalGain DensCalGain::fromJson(const QJsonValue &jsonValue)
{
    DensCalGain calGain;

    if (!jsonValue.isObject()) { return calGain; }

    const QJsonObject jsonCalGain = jsonValue.toObject();

    if (jsonCalGain.contains("L0")) {
        calGain.setLow0(util::parseJsonFloat(jsonCalGain["L0"]));
    }
    if (jsonCalGain.contains("L1")) {
        calGain.setLow1(util::parseJsonFloat(jsonCalGain["L1"]));
    }
    if (jsonCalGain.contains("M0")) {
        calGain.setMed0(util::parseJsonFloat(jsonCalGain["M0"]));
    }
    if (jsonCalGain.contains("M1")) {
        calGain.setMed1(util::parseJsonFloat(jsonCalGain["M1"]));
    }
    if (jsonCalGain.contains("H0")) {
        calGain.setHigh0(util::parseJsonFloat(jsonCalGain["H0"]));
    }
    if (jsonCalGain.contains("H1")) {
        calGain.setHigh1(util::parseJsonFloat(jsonCalGain["H1"]));
    }
    if (jsonCalGain.contains("X0")) {
        calGain.setMax0(util::parseJsonFloat(jsonCalGain["X0"]));
    }
    if (jsonCalGain.contains("X1")) {
        calGain.setMax1(util::parseJsonFloat(jsonCalGain["X1"]));
    }

    return calGain;
}

DensUvVisCalGain::DensUvVisCalGain() : data(new DensUvVisCalGainData)
{
}

DensUvVisCalGain::DensUvVisCalGain(const DensUvVisCalGain &rhs)
    : data{rhs.data}
{
}

DensUvVisCalGain::~DensUvVisCalGain()
{
}

DensUvVisCalGain &DensUvVisCalGain::operator=(const DensUvVisCalGain &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

bool DensUvVisCalGain::isEmpty() const
{
    return data->empty;
}

float DensUvVisCalGain::gainSpecValue(GainLevel gainLevel)
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

float DensUvVisCalGain::gainValue(DensUvVisCalGain::GainLevel gainLevel) const
{
    if (gainLevel > Gain256X) { return qSNaN(); }
    return data->values[static_cast<size_t>(gainLevel)];
}

void DensUvVisCalGain::setGainValue(DensUvVisCalGain::GainLevel gainLevel, float value)
{
    if (gainLevel > Gain256X) { return; }
    data->empty = false;
    data->values[static_cast<size_t>(gainLevel)] = value;
}

bool DensUvVisCalGain::isValid() const
{
    if (data->empty) { return false; }

    for (size_t i = 0; i < static_cast<size_t>(DensUvVisCalGain::Gain256X); i++) {
        const float curValue = data->values[i];
        const float nextValue = data->values[i + 1];
        if (qIsNaN(curValue)) { return false; }
        if (curValue >= nextValue) { return false; }
    }

    return true;
}

QJsonValue DensUvVisCalGain::toJson() const
{
    QJsonArray gainArray;
    for (size_t i = 0; i <= static_cast<size_t>(DensUvVisCalGain::Gain256X); i++) {
        gainArray.append(QString::number(data->values[i], 'f', 6));
    }
    return gainArray;
}

DensUvVisCalGain DensUvVisCalGain::fromJson(const QJsonValue &jsonValue)
{
    DensUvVisCalGain calGain;

    if (!jsonValue.isArray()) { return calGain; }

    const QJsonArray jsonCalGain = jsonValue.toArray();

    if (jsonCalGain.size() != static_cast<size_t>(DensUvVisCalGain::Gain256X) + 1) {
        return calGain;
    }

    for (size_t i = 0; i <= static_cast<size_t>(DensUvVisCalGain::Gain256X); i++) {
        const float value = util::parseJsonFloat(jsonCalGain.at(i));
        calGain.setGainValue(static_cast<DensUvVisCalGain::GainLevel>(i), value);
    }

    return calGain;
}

DensCalCoefficientSet::DensCalCoefficientSet() : data(new DensCalCoefficientSetData)
{
}

DensCalCoefficientSet::DensCalCoefficientSet(float b0, float b1, float b2)
    : data(new DensCalCoefficientSetData)
{
    data->b0 = b0;
    data->b1 = b1;
    data->b2 = b2;
}

DensCalCoefficientSet::DensCalCoefficientSet(std::tuple<float, float, float> tuple)
    : data(new DensCalCoefficientSetData)
{
    std::tie(data->b0, data->b1, data->b2) = tuple;
}

DensCalCoefficientSet::DensCalCoefficientSet(const DensCalCoefficientSet &rhs)
    : data{rhs.data}
{
}

DensCalCoefficientSet &DensCalCoefficientSet::operator=(const DensCalCoefficientSet &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

DensCalCoefficientSet::~DensCalCoefficientSet()
{
}

void DensCalCoefficientSet::setB0(float b0) { data->b0 = b0; }
float DensCalCoefficientSet::b0() const { return data->b0; }

void DensCalCoefficientSet::setB1(float b1) { data->b1 = b1; }
float DensCalCoefficientSet::b1() const { return data->b1; }

void DensCalCoefficientSet::setB2(float b2) { data->b2 = b2; }
float DensCalCoefficientSet::b2() const { return data->b2; }

bool DensCalCoefficientSet::isValid() const
{
    // Invalid if any values are NaN
    if (qIsNaN(data->b0) || qIsNaN(data->b1) || qIsNaN(data->b2)) {
        return false;
    }

    return true;
}

QJsonValue DensCalCoefficientSet::toJson() const
{
    QJsonObject jsonCoefficientSet;
    jsonCoefficientSet["B0"] = data->b0;
    jsonCoefficientSet["B1"] = data->b1;
    jsonCoefficientSet["B2"] = data->b2;
    return jsonCoefficientSet;
}

DensCalCoefficientSet DensCalCoefficientSet::fromJson(const QJsonValue &jsonValue)
{
    DensCalCoefficientSet coefficientSet;

    if (!jsonValue.isObject()) { return coefficientSet; }

    const QJsonObject jsonCoefficientSet = jsonValue.toObject();

    if (jsonCoefficientSet.contains("B0")) {
        coefficientSet.setB0(util::parseJsonFloat(jsonCoefficientSet["B0"]));
    }
    if (jsonCoefficientSet.contains("B1")) {
        coefficientSet.setB1(util::parseJsonFloat(jsonCoefficientSet["B1"]));
    }
    if (jsonCoefficientSet.contains("B2")) {
        coefficientSet.setB2(util::parseJsonFloat(jsonCoefficientSet["B2"]));
    }

    return coefficientSet;
}

DensCalTarget::DensCalTarget() : data(new DensCalTargetData)
{
}

DensCalTarget::DensCalTarget(const DensCalTarget &rhs)
    : data{rhs.data}
{
}

DensCalTarget &DensCalTarget::operator=(const DensCalTarget &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

DensCalTarget::~DensCalTarget()
{
}

void DensCalTarget::setLoDensity(float loDensity) { data->loDensity = loDensity; }
float DensCalTarget::loDensity() const { return data->loDensity; }

void DensCalTarget::setLoReading(float loValue) { data->loReading = loValue; }
float DensCalTarget::loReading() const { return data->loReading; }

void DensCalTarget::setHiDensity(float hiDensity) { data->hiDensity = hiDensity; }
float DensCalTarget::hiDensity() const { return data->hiDensity; }

void DensCalTarget::setHiReading(float hiValue) { data->hiReading = hiValue; }
float DensCalTarget::hiReading() const { return data->hiReading; }

bool DensCalTarget::isValid() const
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

bool DensCalTarget::isValidLoOnly() const
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

QJsonValue DensCalTarget::toJson() const
{
    QJsonObject jsonCalLo;
    jsonCalLo["density"] = QString::number(data->loDensity, 'f', 2);
    jsonCalLo["reading"] = QString::number(data->loReading, 'f', 6);

    QJsonObject jsonCalHi;
    jsonCalHi["density"] = QString::number(data->hiDensity, 'f', 2);
    jsonCalHi["reading"] = QString::number(data->hiReading, 'f', 6);

    QJsonObject jsonCal;
    jsonCal["cal-lo"] = jsonCalLo;
    jsonCal["cal-hi"] = jsonCalHi;

    return jsonCal;
}

DensCalTarget DensCalTarget::fromJson(const QJsonValue &jsonValue)
{
    DensCalTarget calTarget;

    if (!jsonValue.isObject()) { return calTarget; }

    const QJsonObject jsonCalTarget = jsonValue.toObject();

    if (jsonCalTarget.contains("cal-lo") && jsonCalTarget["cal-lo"].isObject()) {
        const QJsonObject jsonCalLo = jsonCalTarget["cal-lo"].toObject();

        if (jsonCalLo.contains("density")) {
            calTarget.setLoDensity(util::parseJsonFloat(jsonCalLo["density"]));
        }
        if (jsonCalLo.contains("reading")) {
            calTarget.setLoReading(util::parseJsonFloat(jsonCalLo["reading"]));
        }
    }
    if (jsonCalTarget.contains("cal-hi") && jsonCalTarget["cal-hi"].isObject()) {
        const QJsonObject jsonCalHi = jsonCalTarget["cal-hi"].toObject();

        if (jsonCalHi.contains("density")) {
            calTarget.setHiDensity(util::parseJsonFloat(jsonCalHi["density"]));
        }
        if (jsonCalHi.contains("reading")) {
            calTarget.setHiReading(util::parseJsonFloat(jsonCalHi["reading"]));
        }
    }

    return calTarget;
}

bool DensCalTarget::isValidReflection() const
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

bool DensCalTarget::isValidTransmission() const
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
