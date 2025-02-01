#ifndef DENSISTICKREADING_H
#define DENSISTICKREADING_H

#include <QSharedDataPointer>
#include <QDebug>

#include "tsl2585.h"

class DensiStickReadingData;

class DensiStickReading
{
public:
    enum Status {
        ResultInvalid,
        ResultValid,
        ResultSaturated,
        ResultOverflow
    };

    DensiStickReading();
    DensiStickReading(DensiStickReading::Status status, tsl2585_gain_t gain, uint32_t reading);
    DensiStickReading(const DensiStickReading &);
    DensiStickReading &operator=(const DensiStickReading &);
    ~DensiStickReading();

    DensiStickReading::Status status() const;
    tsl2585_gain_t gain() const;
    uint32_t reading() const;

private:
    QSharedDataPointer<DensiStickReadingData> data;
};

QDebug operator<<(QDebug debug, const DensiStickReading &reading);

#endif // DENSISTICKREADING_H
