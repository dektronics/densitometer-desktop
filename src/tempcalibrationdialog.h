#ifndef TEMPCALIBRATIONDIALOG_H
#define TEMPCALIBRATIONDIALOG_H

#include <QDialog>
#include <QList>

#include "denscalvalues.h"

class QTableWidget;
class QTableWidgetItem;

namespace Ui {
class TempCalibrationDialog;
}

class TempCalibrationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TempCalibrationDialog(QWidget *parent = nullptr);
    ~TempCalibrationDialog();

    void setModeVis();
    void setModeUv();

    CoefficientSet b0Values() const;
    CoefficientSet b1Values() const;
    CoefficientSet b2Values() const;

private slots:
    void onActionCut();
    void onActionCopy();
    void onActionPaste();
    void onActionDelete();
    void onImportClicked();
    void onClearClicked();
    void onCalculateClicked();

private:
    void calculateCorrections(const QList<QList<double>> &tableData, int refTempRow);
    QTableWidgetItem *tableWidgetItem(QTableWidget *table, int row, int column);
    void coefficientSetAssignRow(QTableWidget *table, int row, const CoefficientSet &sourceValues);
    CoefficientSet coefficientSetCollectRow(const QTableWidget *table, int row) const;

    Ui::TempCalibrationDialog *ui;
};

#endif // TEMPCALIBRATIONDIALOG_H
