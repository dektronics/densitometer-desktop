#ifndef SLOPECALIBRATIONDIALOG_H
#define SLOPECALIBRATIONDIALOG_H

#include <QDialog>
#include <QStandardItemModel>
#include <QList>
#include <QPair>
#include <tuple>
#include "densinterface.h"
#include "stickrunner.h"

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
    explicit SlopeCalibrationDialog(StickRunner *stickRunner, QWidget *parent = nullptr);
    ~SlopeCalibrationDialog();

    void setCalculateZeroAdjustment(bool enable);

    std::tuple<float, float, float> calValues() const;
    float zeroAdjustment() const;

private slots:
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
    std::tuple<float, float, float> polyfit(const QList<float> &xList, const QList<float> &yList);
    void gaussEliminationLS(int m, int n, double **a /*[m][n]*/, double *x /*[n-1]*/);
    QPair<int, int> upperLeftActiveIndex() const;
    float itemValueAsFloat(int row, int col) const;

    Ui::SlopeCalibrationDialog *ui;
    QStandardItemModel *model_;
    bool enableZeroAdj_;
    bool enableReflReadings_;
    std::tuple<float, float, float> calValues_;
    float zeroAdj_;
};

#endif // SLOPECALIBRATIONDIALOG_H
