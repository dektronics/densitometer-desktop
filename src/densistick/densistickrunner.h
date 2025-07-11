#ifndef DENSISTICKRUNNER_H
#define DENSISTICKRUNNER_H

#include <QObject>

#include "densistickinterface.h"
#include "peripheralcalvalues.h"

class DensiStickRunner : public QObject
{
    Q_OBJECT
public:
    explicit DensiStickRunner(DensiStickInterface *stickInterface, QObject *parent = nullptr);

    DensiStickInterface *stickInterface();

    void setEnabled(bool enabled);
    bool enabled() const;

public slots:
    void reloadCalibration();

signals:
    void targetMeasurement(float basicReading);
    void targetDensity(float density);

private slots:
    void onButtonEvent(bool pressed);
    void onSensorReading(const DensiStickReading& reading);

private:
    void startMeasurement();
    void finishMeasurement();
    DensiStickInterface *stickInterface_;
    bool enabled_ = false;
    bool measuring_ = false;
    QList<DensiStickReading> readingList_;
    DensiStickCalibration calData_;
    qint64 measStartTime_;
    int agcStep_;
};

#endif // DENSISTICKRUNNER_H
