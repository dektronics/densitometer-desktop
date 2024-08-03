#ifndef STICKREADING_H
#define STICKREADING_H

#include <QSharedDataPointer>

#include "tsl2585.h"

class StickReadingData;

class StickReading
{
public:
    enum Status {
        ResultInvalid,
        ResultValid,
        ResultSaturated,
        ResultOverflow
    };

    StickReading();
    StickReading(StickReading::Status status, tsl2585_gain_t gain, uint32_t reading);
    StickReading(const StickReading &);
    StickReading &operator=(const StickReading &);
    ~StickReading();

    StickReading::Status status() const;
    tsl2585_gain_t gain() const;
    uint32_t reading() const;

private:
    QSharedDataPointer<StickReadingData> data;
};

#endif // STICKREADING_H
