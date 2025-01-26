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
    CoefficientSet b0VisValues() const;
    CoefficientSet b1VisValues() const;
    CoefficientSet b2VisValues() const;

    bool hasUvValues() const;
    CoefficientSet b0UvValues() const;
    CoefficientSet b1UvValues() const;
    CoefficientSet b2UvValues() const;

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
    QList<QList<double>> collectInputData(QTableWidget *inputTableWidget);
    int findReferenceRow(const QList<QList<double>> &tableData);
    void calculateCorrections(const QList<QList<double>> &tableData, int refTempRow, QTableWidget *resultsTableWidget);
    QTableWidgetItem *tableWidgetItem(QTableWidget *table, int row, int column);
    void coefficientSetAssignRow(QTableWidget *table, int row, const CoefficientSet &sourceValues);
    CoefficientSet coefficientSetCollectRow(const QTableWidget *table, int row) const;

    QString uniqueId_;
    bool hasVisValues_ = false;
    bool hasUvValues_ = false;
    Ui::TempCalibrationDialog *ui;
};

#endif // TEMPCALIBRATIONDIALOG_H
