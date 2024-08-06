#ifndef STICKRUNNER_H
#define STICKRUNNER_H

#include <QObject>

#include "stickinterface.h"
#include "tsl2585calibration.h"

class StickRunner : public QObject
{
    Q_OBJECT
public:
    explicit StickRunner(StickInterface *stickInterface, QObject *parent = nullptr);

    StickInterface *stickInterface();

    void setEnabled(bool enabled);
    bool enabled() const;

    void reloadCalibration();

signals:
    void targetMeasurement(float basicReading);
    void targetDensity(float density);

private slots:
    void onButtonEvent(bool pressed);
    void onSensorReading(const StickReading& reading);

private:
    void startMeasurement();
    void finishMeasurement();
    StickInterface *stickInterface_;
    bool enabled_ = false;
    bool measuring_ = false;
    int skipCount_ = 0;
    QList<StickReading> readingList_;
    Tsl2585Calibration calData_;
};

#endif // STICKRUNNER_H
