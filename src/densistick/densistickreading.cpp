#include "densistickreading.h"
#include "tsl2585.h"

class DensiStickReadingData : public QSharedData
{
public:
    DensiStickReading::Status status = DensiStickReading::ResultInvalid;
    tsl2585_gain_t gain = TSL2585_GAIN_MAX;
    uint32_t reading = 0;
};

DensiStickReading::DensiStickReading()
    : data(new DensiStickReadingData)
{
}

DensiStickReading::DensiStickReading(DensiStickReading::Status status, tsl2585_gain_t gain, uint32_t reading)
    : data(new DensiStickReadingData)
{
    data->status = status;
    data->gain = gain;
    data->reading = reading;
}

DensiStickReading::DensiStickReading(const DensiStickReading &rhs)
    : data{rhs.data}
{
}

DensiStickReading &DensiStickReading::operator=(const DensiStickReading &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

DensiStickReading::~DensiStickReading()
{
}

DensiStickReading::Status DensiStickReading::status() const
{
    return data->status;
}

tsl2585_gain_t DensiStickReading::gain() const
{
    return data->gain;
}

uint32_t DensiStickReading::reading() const
{
    return data->reading;
}

QDebug operator<<(QDebug debug, const DensiStickReading &reading)
{
    QDebugStateSaver saver(debug);
    debug.nospace().noquote() << "DensiStickReading(";
    switch(reading.status()) {
    case DensiStickReading::ResultInvalid:
        debug.nospace() << "ResultInvalid";
        break;
    case DensiStickReading::ResultValid:
        debug.nospace() << "ResultValid";
        break;
    case DensiStickReading::ResultSaturated:
        debug.nospace() << "ResultSaturated";
        break;
    case DensiStickReading::ResultOverflow:
        debug.nospace() << "ResultOverflow";
        break;
    default:
        debug.nospace() << static_cast<int>(reading.status());
        break;
    }

    if (reading.status() == DensiStickReading::ResultValid) {
        debug << ", " << TSL2585::gainString(reading.gain()) << ", " << reading.reading();
    }

    debug << ')';
    return debug;
}
