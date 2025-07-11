#ifndef PERIPHERALCALVALUES_H
#define PERIPHERALCALVALUES_H

#include <QSharedDataPointer>
#include <QVector>

class PeripheralCalGainData;
class PeripheralCalLinearTargetData;
class PeripheralCalDensityTargetData;
class MeterProbeCalibrationData;
class DensiStickCalibrationData;

class PeripheralCalGain
{
public:
    enum GainLevel {
        Gain0_5X = 0,
        Gain1X   = 1,
        Gain2X   = 2,
        Gain4X   = 3,
        Gain8X   = 4,
        Gain16X  = 5,
        Gain32X  = 6,
        Gain64X  = 7,
        Gain128X = 8,
        Gain256X = 9,
    };

    PeripheralCalGain();
    PeripheralCalGain(const PeripheralCalGain &);
    PeripheralCalGain &operator=(const PeripheralCalGain &);
    ~PeripheralCalGain();

    bool isEmpty() const;

    static float gainSpecValue(GainLevel gainLevel);
    float gainValue(GainLevel gainLevel) const;
    void setGainValue(GainLevel gainLevel, float value);

    bool isValid() const;

private:
    QSharedDataPointer<PeripheralCalGainData> data;
};


class PeripheralCalLinearTarget
{
public:
    PeripheralCalLinearTarget();
    PeripheralCalLinearTarget(const PeripheralCalLinearTarget &);
    PeripheralCalLinearTarget &operator=(const PeripheralCalLinearTarget &);
    ~PeripheralCalLinearTarget();

    void setSlope(float slope);
    float slope() const;

    void setIntercept(float intercept);
    float intercept() const;

    bool isValid() const;

private:
    QSharedDataPointer<PeripheralCalLinearTargetData> data;
};


class PeripheralCalDensityTarget
{
public:
    PeripheralCalDensityTarget();
    PeripheralCalDensityTarget(const PeripheralCalDensityTarget &);
    PeripheralCalDensityTarget &operator=(const PeripheralCalDensityTarget &);
    ~PeripheralCalDensityTarget();

    void setLoDensity(float loDensity);
    float loDensity() const;

    void setLoReading(float loValue);
    float loReading() const;

    void setHiDensity(float hiDensity);
    float hiDensity() const;

    void setHiReading(float hiValue);
    float hiReading() const;

    bool isValid() const;
    bool isValidLoOnly() const;

    bool isValidReflection() const;
    bool isValidTransmission() const;

private:
    QSharedDataPointer<PeripheralCalDensityTargetData> data;
};


class MeterProbeCalibration
{
public:
    MeterProbeCalibration();
    MeterProbeCalibration(const MeterProbeCalibration &);
    MeterProbeCalibration &operator=(const MeterProbeCalibration &);
    ~MeterProbeCalibration();

    bool isEmpty() const;

    PeripheralCalGain gainCalibration() const;
    void setGainCalibration(const PeripheralCalGain &gainCal);

    PeripheralCalLinearTarget targetCalibration() const;
    void setTargetCalibration(const PeripheralCalLinearTarget &targetCal);

private:
    QSharedDataPointer<MeterProbeCalibrationData> data;
};


class DensiStickCalibration
{
public:
    DensiStickCalibration();
    DensiStickCalibration(const DensiStickCalibration &);
    DensiStickCalibration &operator=(const DensiStickCalibration &);
    ~DensiStickCalibration();

    bool isEmpty() const;

    PeripheralCalGain gainCalibration() const;
    void setGainCalibration(const PeripheralCalGain &gainCal);

    PeripheralCalDensityTarget targetCalibration() const;
    void setTargetCalibration(const PeripheralCalDensityTarget &targetCal);

private:
    QSharedDataPointer<DensiStickCalibrationData> data;
};

#endif // PERIPHERALCALVALUES_H
