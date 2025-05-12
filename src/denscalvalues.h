#ifndef DENSCALVALUES_H
#define DENSCALVALUES_H

#include <QSharedDataPointer>
#include <QVector>

class DensCalLightData;
class DensCalGainData;
class DensUvVisCalGainData;
class DensCalSlopeData;
class DensCalTemperatureData;
class DensCalTargetData;

typedef std::tuple<float, float, float> CoefficientSet;

class DensCalLight
{
public:
    DensCalLight();
    DensCalLight(const DensCalLight &);
    DensCalLight &operator=(const DensCalLight &);
    ~DensCalLight();

    void setReflectionValue(int reflectionValue);
    int reflectionValue() const;

    void setTransmissionValue(int transmissionValue);
    int transmissionValue() const;

    bool isValid() const;

private:
    QSharedDataPointer<DensCalLightData> data;
};

class DensCalGain
{
public:
    DensCalGain();
    DensCalGain(const DensCalGain &);
    DensCalGain &operator=(const DensCalGain &);
    ~DensCalGain();

    void setLow0(float low0);
    float low0() const;

    void setLow1(float low1);
    float low1() const;

    void setMed0(float med0);
    float med0() const;

    void setMed1(float med1);
    float med1() const;

    void setHigh0(float high0);
    float high0() const;

    void setHigh1(float high1);
    float high1() const;

    void setMax0(float max0);
    float max0() const;

    void setMax1(float max1);
    float max1() const;

    bool isValid() const;

private:
    QSharedDataPointer<DensCalGainData> data;
};

class DensUvVisCalGain
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

    DensUvVisCalGain();
    DensUvVisCalGain(const DensUvVisCalGain &);
    DensUvVisCalGain &operator=(const DensUvVisCalGain &);
    ~DensUvVisCalGain();

    bool isEmpty() const;

    static float gainSpecValue(GainLevel gainLevel);
    float gainValue(GainLevel gainLevel) const;
    void setGainValue(GainLevel gainLevel, float value);

    bool isValid() const;

private:
    QSharedDataPointer<DensUvVisCalGainData> data;
};


class DensCalSlope
{
public:
    DensCalSlope();
    DensCalSlope(const DensCalSlope &);
    DensCalSlope &operator=(const DensCalSlope &);
    ~DensCalSlope();

    void setZ(float z);
    float z() const;

    void setB0(float b0);
    float b0() const;

    void setB1(float b1);
    float b1() const;

    void setB2(float b2);
    float b2() const;

    bool isValid() const;

private:
    QSharedDataPointer<DensCalSlopeData> data;
};

class DensCalTemperature
{
public:
    DensCalTemperature();
    DensCalTemperature(const DensCalTemperature &);
    DensCalTemperature &operator=(const DensCalTemperature &);
    ~DensCalTemperature();

    void setB0(CoefficientSet values);
    CoefficientSet b0() const;

    void setB1(CoefficientSet values);
    CoefficientSet b1() const;

    void setB2(CoefficientSet values);
    CoefficientSet b2() const;

    bool isValid() const;

private:
    QSharedDataPointer<DensCalTemperatureData> data;
};

class DensCalTarget
{
public:
    DensCalTarget();
    DensCalTarget(const DensCalTarget &);
    DensCalTarget &operator=(const DensCalTarget &);
    ~DensCalTarget();

    void setLoDensity(float loDensity);
    float loDensity() const;

    void setLoReading(float loValue);
    float loReading() const;

    void setHiDensity(float hiDensity);
    float hiDensity() const;

    void setHiReading(float hiValue);
    float hiReading() const;

    bool isValidReflection() const;
    bool isValidTransmission() const;

    bool isValid() const;

private:
    QSharedDataPointer<DensCalTargetData> data;
};

#endif // DENSCALVALUES_H
