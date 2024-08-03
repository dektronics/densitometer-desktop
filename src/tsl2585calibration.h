#ifndef TSL2585CALIBRATION_H
#define TSL2585CALIBRATION_H

#include <QSharedDataPointer>

#include "tsl2585.h"

class Tsl2585CalSlopeData;
class Tsl2585CalTargetData;
class Tsl2585CalibrationData;

class Tsl2585CalSlope
{
public:
    Tsl2585CalSlope();
    Tsl2585CalSlope(const Tsl2585CalSlope &);
    Tsl2585CalSlope &operator=(const Tsl2585CalSlope &);
    ~Tsl2585CalSlope();

    void setB0(float b0);
    float b0() const;

    void setB1(float b1);
    float b1() const;

    void setB2(float b2);
    float b2() const;

    bool isValid() const;

private:
    QSharedDataPointer<Tsl2585CalSlopeData> data;
};

class Tsl2585CalTarget
{
public:
    Tsl2585CalTarget();
    Tsl2585CalTarget(const Tsl2585CalTarget &);
    Tsl2585CalTarget &operator=(const Tsl2585CalTarget &);
    ~Tsl2585CalTarget();

    void setLoDensity(float loDensity);
    float loDensity() const;

    void setLoReading(float loValue);
    float loReading() const;

    void setHiDensity(float hiDensity);
    float hiDensity() const;

    void setHiReading(float hiValue);
    float hiReading() const;

    bool isValid() const;

private:
    QSharedDataPointer<Tsl2585CalTargetData> data;
};

class Tsl2585Calibration
{
public:
    Tsl2585Calibration();
    Tsl2585Calibration(const Tsl2585Calibration &);
    Tsl2585Calibration &operator=(const Tsl2585Calibration &);
    ~Tsl2585Calibration();

    bool isEmpty() const;

    float gainCalibration(tsl2585_gain_t gain) const;
    void setGainCalibration(tsl2585_gain_t gain, float value);

    Tsl2585CalSlope slopeCalibration() const;
    void setSlopeCalibration(const Tsl2585CalSlope &calSlope);

    Tsl2585CalTarget targetCalibration() const;
    void setTargetCalibration(const Tsl2585CalTarget &calTarget);

private:
    QSharedDataPointer<Tsl2585CalibrationData> data;
};

#endif // TSL2585CALIBRATION_H
