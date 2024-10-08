#include "stickreading.h"
#include "tsl2585.h"

class StickReadingData : public QSharedData
{
public:
    StickReading::Status status = StickReading::ResultInvalid;
    tsl2585_gain_t gain = TSL2585_GAIN_MAX;
    uint32_t reading = 0;
};

StickReading::StickReading()
    : data(new StickReadingData)
{
}

StickReading::StickReading(StickReading::Status status, tsl2585_gain_t gain, uint32_t reading)
    : data(new StickReadingData)
{
    data->status = status;
    data->gain = gain;
    data->reading = reading;
}

StickReading::StickReading(const StickReading &rhs)
    : data{rhs.data}
{
}

StickReading &StickReading::operator=(const StickReading &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

StickReading::~StickReading()
{
}

StickReading::Status StickReading::status() const
{
    return data->status;
}

tsl2585_gain_t StickReading::gain() const
{
    return data->gain;
}

uint32_t StickReading::reading() const
{
    return data->reading;
}

QDebug operator<<(QDebug debug, const StickReading &reading)
{
    QDebugStateSaver saver(debug);
    debug.nospace().noquote() << "StickReading(";
    switch(reading.status()) {
    case StickReading::ResultInvalid:
        debug.nospace() << "ResultInvalid";
        break;
    case StickReading::ResultValid:
        debug.nospace() << "ResultValid";
        break;
    case StickReading::ResultSaturated:
        debug.nospace() << "ResultSaturated";
        break;
    case StickReading::ResultOverflow:
        debug.nospace() << "ResultOverflow";
        break;
    default:
        debug.nospace() << static_cast<int>(reading.status());
        break;
    }

    if (reading.status() == StickReading::ResultValid) {
        debug << ", " << TSL2585::gainString(reading.gain()) << ", " << reading.reading();
    }

    debug << ')';
    return debug;
}
