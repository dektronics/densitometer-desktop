#ifndef TEMPCALIBRATIONDIALOG_H
#define TEMPCALIBRATIONDIALOG_H

#include <QDialog>
#include <QList>
#include <QJsonArray>

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

    void setUniqueId(const QString &uniqueId);

    bool hasVisValues() const;
    DensCalTemperature visValues() const;

    bool hasUvValues() const;
    DensCalTemperature uvValues() const;

private slots:
    void onActionCut();
    void onActionCopy();
    void onActionPaste();
    void onActionDelete();
    void onImportClicked();
    void onClearClicked();
    void onCalculateClicked();

private:
    bool processImportData(const QByteArray &importData);
    void populateImportDataSequence(QTableWidget *inputTableWidget, const QJsonArray &sequence);
    int findMaxValidColumns(QTableWidget *inputTableWidget);
    QList<QList<double>> collectInputData(QTableWidget *inputTableWidget, int maxColumns);
    int findReferenceRow(const QList<QList<double>> &tableData);
    void calculateCorrections(const QList<QList<double>> &tableData, int refTempRow,
                              QTableWidget *resultsTableWidget, int resultsCol);
    QTableWidgetItem *tableWidgetItem(QTableWidget *table, int row, int column);
    void coefficientSetAssignColumn(QTableWidget *table, int col, const DensCalTemperature &sourceValues);
    DensCalTemperature coefficientSetCollectColumn(const QTableWidget *table, int col) const;

    QString uniqueId_;
    bool hasVisValues_ = false;
    bool hasUvValues_ = false;
    Ui::TempCalibrationDialog *ui;
};

#endif // TEMPCALIBRATIONDIALOG_H
