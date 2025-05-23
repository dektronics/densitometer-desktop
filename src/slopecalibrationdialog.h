#ifndef SLOPECALIBRATIONDIALOG_H
#define SLOPECALIBRATIONDIALOG_H

#include <QDialog>
#include <QStandardItemModel>
#include <QList>
#include <QPair>
#include <tuple>
#include "densinterface.h"
#include "densistickrunner.h"

namespace Ui {
class SlopeCalibrationDialog;
}

class SlopeCalibrationDialog : public QDialog
{
    Q_OBJECT

private:
    explicit SlopeCalibrationDialog(QWidget *parent);
public:
    explicit SlopeCalibrationDialog(DensInterface *densInterface, QWidget *parent = nullptr);
    explicit SlopeCalibrationDialog(DensiStickRunner *stickRunner, QWidget *parent = nullptr);
    ~SlopeCalibrationDialog();

    std::tuple<float, float, float> calValues() const;

private slots:
    void wedgePrecValueChanged(int value);
    void wedgeCountValueChanged(int value);
    void onDensityReading(DensInterface::DensityType type, float dValue, float dZero, float rawValue, float corrValue);
    void onTargetMeasurement(float basicReading);
    void onActionCut();
    void onActionCopy();
    void onActionPaste();
    void onActionDelete();
    void onCalculateResults();
    void onClearReadings();

private:
    void addRawMeasurement(float rawValue);
    QPair<int, int> upperLeftActiveIndex() const;
    float itemValueAsFloat(int row, int col) const;

    Ui::SlopeCalibrationDialog *ui;
    QStandardItemModel *model_;
    bool enableReflReadings_;
    std::tuple<float, float, float> calValues_;
};

#endif // SLOPECALIBRATIONDIALOG_H
