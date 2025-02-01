#ifndef STICKGAINCALIBRATIONDIALOG_H
#define STICKGAINCALIBRATIONDIALOG_H

#include <QDialog>
#include <QMap>
#include "densistickinterface.h"

namespace Ui {
class StickGainCalibrationDialog;
}

class StickGainCalibrationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StickGainCalibrationDialog(DensiStickInterface *stickInterface, QWidget *parent = nullptr);
    ~StickGainCalibrationDialog();

    bool success() const;

    QMap<int, float> gainMeasurements() const;

public slots:
    virtual void accept() override;
    virtual void reject() override;

protected:
    void showEvent(QShowEvent *event) override;
    void timerEvent(QTimerEvent *event) override;

private slots:
    void onSensorReading(const DensiStickReading& reading);
    void onCalGainCalStatus(int status, int param);
    void onCalGainCalFinished();
    void onCalGainCalError();

private:
    QString gainParamText(int param);
    QString lightParamText(int param);
    void addText(const QString &text);
    Ui::StickGainCalibrationDialog *ui;
    DensiStickInterface *stickInterface_;
    bool started_;
    bool running_;
    bool success_;
    int lastStatus_;
    int lastParam_;
    int timerId_;
    int step_;
    bool stepNew_;
    int stepGain_;
    quint8 stepBrightness_;
    int skipCount_;
    int delayCount_;
    bool captureReadings_;
    bool upperGain_;
    QList<DensiStickReading> readingList_;
    QMap<int, quint8> gainBrightness_;
    QMap<int, double> gainReadingUpper_;
    QMap<int, double> gainReadingLower_;
    QMap<int, float> gainMeasurements_;
};

#endif // STICKGAINCALIBRATIONDIALOG_H
