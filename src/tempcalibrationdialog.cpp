#include "tempcalibrationdialog.h"
#include "ui_tempcalibrationdialog.h"

#include <QMessageBox>

#include "floatitemdelegate.h"
#include "util.h"

TempCalibrationDialog::TempCalibrationDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::TempCalibrationDialog)
{
    ui->setupUi(this);

    ui->actionCut->setShortcut(QKeySequence::Cut);
    ui->actionCopy->setShortcut(QKeySequence::Copy);
    ui->actionPaste->setShortcut(QKeySequence::Paste);
    ui->actionDelete->setShortcut(QKeySequence::Delete);

    addAction(ui->actionCut);
    addAction(ui->actionCopy);
    addAction(ui->actionPaste);
    addAction(ui->actionDelete);

    connect(ui->actionCut, &QAction::triggered, this, &TempCalibrationDialog::onActionCut);
    connect(ui->actionCopy, &QAction::triggered, this, &TempCalibrationDialog::onActionCopy);
    connect(ui->actionPaste, &QAction::triggered, this, &TempCalibrationDialog::onActionPaste);
    connect(ui->actionDelete, &QAction::triggered, this, &TempCalibrationDialog::onActionDelete);

    connect(ui->importPushButton, &QPushButton::clicked, this, &TempCalibrationDialog::onImportClicked);
    connect(ui->clearPushButton, &QPushButton::clicked, this, &TempCalibrationDialog::onClearClicked);
    connect(ui->calculatePushButton, &QPushButton::clicked, this, &TempCalibrationDialog::onCalculateClicked);

    ui->inputTableWidget->setItemDelegateForColumn(0, new FloatItemDelegate(-20, 100, 2));
    for (int i = 1; i < ui->inputTableWidget->columnCount(); i++) {
        ui->inputTableWidget->setItemDelegateForColumn(i, new FloatItemDelegate(0, 1000, 6));
    }

    ui->resultsTableWidget->setItemDelegate(new FloatItemDelegate());

    ui->resultsTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Disabling until this feature is implemented
    ui->importPushButton->setVisible(false);

    onClearClicked();
}

TempCalibrationDialog::~TempCalibrationDialog()
{
    delete ui;
}

void TempCalibrationDialog::setModeVis()
{
    setWindowTitle(tr("Temperature Calibration Tool (VIS Sensor)"));
    ui->modeValueLabel->setText(tr("VIS"));
}

void TempCalibrationDialog::setModeUv()
{
    setWindowTitle(tr("Temperature Calibration Tool (UV Sensor)"));
    ui->modeValueLabel->setText(tr("UV"));
}

CoefficientSet TempCalibrationDialog::b0Values() const
{
    return coefficientSetCollectRow(ui->resultsTableWidget, 0);
}

CoefficientSet TempCalibrationDialog::b1Values() const
{
    return coefficientSetCollectRow(ui->resultsTableWidget, 1);
}

CoefficientSet TempCalibrationDialog::b2Values() const
{
    return coefficientSetCollectRow(ui->resultsTableWidget, 2);
}

void TempCalibrationDialog::onActionCut()
{
    if (focusWidget() == ui->inputTableWidget) {
        util::tableWidgetCut(ui->inputTableWidget);
    }
}

void TempCalibrationDialog::onActionCopy()
{
    const QTableWidget *tableWidget = qobject_cast<const QTableWidget *>(focusWidget());
    if (tableWidget) {
        util::tableWidgetCopy(tableWidget);
    }
}

void TempCalibrationDialog::onActionPaste()
{
    if (focusWidget() == ui->inputTableWidget) {
        util::tableWidgetPaste(ui->inputTableWidget);
    }
}

void TempCalibrationDialog::onActionDelete()
{
    if (focusWidget() == ui->inputTableWidget) {
        util::tableWidgetDelete(ui->inputTableWidget);
    }
}

void TempCalibrationDialog::onImportClicked()
{
    //TODO
}

void TempCalibrationDialog::onClearClicked()
{
    ui->inputTableWidget->clearContents();
    ui->resultsTableWidget->clearContents();
    ui->refTempValueLabel->setText(QString());
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}

void TempCalibrationDialog::onCalculateClicked()
{
    // Collect the data from the input table
    QList<QList<double>> tableData;
    bool valid = true;
    int refTempRow = 0;

    for (int row = 0; row < ui->inputTableWidget->rowCount(); row++) {
        QList<double> rowData;
        for (int col = 0; col < ui->inputTableWidget->columnCount(); col++) {
            QTableWidgetItem *item = ui->inputTableWidget->item(row, col);
            if (!item || item->text().isEmpty()) {
                valid = false;
                break;
            }
            double val = item->text().toDouble(&valid);
            if (!valid) { break; }

            // Try to find a suitable reference temperature row
            if (refTempRow == 0 && col == 0 && val > 18.0 && val < 22.0) {
                refTempRow = row;
            }

            rowData.append(val);
        }
        if (!valid) { break; }
        tableData.append(rowData);
    }

    if (!valid) {
        QMessageBox::warning(this, tr("Invalid Values"), tr("Cannot calculate corrections from invalid or incomplete data."));
        return;
    }

    calculateCorrections(tableData, refTempRow);
}

void TempCalibrationDialog::calculateCorrections(const QList<QList<double>> &tableData, int refTempRow)
{
    // Prepare some initial datasets, which includes the list of measured
    // temperatures and the average reading for each brightness level.
    QList<float> tempList;
    QList<double> readingSums;
    for (qsizetype r = 0; r < tableData.size(); r++) {
        const QList<double> rowData = tableData[r];

        tempList.append(rowData[0]);

        for (qsizetype i = 1; i < rowData.size(); i++) {
            if (r == 0) {
                readingSums.append(0);
            }
            readingSums[i - 1] += rowData[i];
        }
    }

    QList<float> readingList;
    for (qsizetype i = 0; i < readingSums.size(); i++) {
        readingList.append(readingSums[i] / tableData.size());
    }

    // Iterate over each sensor reading block, and calculate block corrections
    QList<float> b0List;
    QList<float> b1List;
    QList<float> b2List;
    for (qsizetype b = 0; b < readingList.size(); b++) {
        QList<float> correctionList;
        const double refReading = tableData[refTempRow][b + 1];

        for (qsizetype r = 0; r < tableData.size(); r++) {
            correctionList.append(refReading / tableData[r][b + 1]);
        }

        std::tuple<float, float, float> polyResult = util::polyfit(tempList, correctionList);

        b0List.append(std::get<0>(polyResult));
        b1List.append(std::get<1>(polyResult));
        b2List.append(std::get<2>(polyResult));
    }

    // Calculate the final results
    std::tuple<float, float, float> b0Result = util::polyfit(readingList, b0List);
    std::tuple<float, float, float> b1Result = util::polyfit(readingList, b1List);
    std::tuple<float, float, float> b2Result = util::polyfit(readingList, b2List);

    // Insert the results into the output table
    coefficientSetAssignRow(ui->resultsTableWidget, 0, b0Result);
    coefficientSetAssignRow(ui->resultsTableWidget, 1, b1Result);
    coefficientSetAssignRow(ui->resultsTableWidget, 2, b2Result);

    ui->refTempValueLabel->setText(QString("%1°C").arg(QString::number(tableData[refTempRow][0], 'f', 1)));
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

QTableWidgetItem *TempCalibrationDialog::tableWidgetItem(QTableWidget *table, int row, int column)
{
    QTableWidgetItem *item = table->item(row, column);
    if (!item) {
        item = new QTableWidgetItem();
        table->setItem(row, column, item);
    }
    return item;
}

void TempCalibrationDialog::coefficientSetAssignRow(QTableWidget *table, int row, const CoefficientSet &sourceValues)
{
    if (!table || table->rowCount() <= row || table->columnCount() < 3) { return; }

    QTableWidgetItem *item;

    item = tableWidgetItem(table, row, 0);
    item->setText(QString::number(std::get<0>(sourceValues)));

    item = tableWidgetItem(table, row, 1);
    item->setText(QString::number(std::get<1>(sourceValues)));

    item = tableWidgetItem(table, row, 2);
    item->setText(QString::number(std::get<2>(sourceValues)));
}

CoefficientSet TempCalibrationDialog::coefficientSetCollectRow(const QTableWidget *table, int row) const
{
    bool ok;
    float v0 = qSNaN();;
    float v1 = qSNaN();;
    float v2 = qSNaN();;

    if (!table || table->rowCount() <= row || table->columnCount() < 3) { return { v0, v1, v2 }; }

    QTableWidgetItem *item;

    item = table->item(row, 0);
    if (item) {
        v0 = item->text().toFloat(&ok);
        if (!ok) { v0 = qSNaN(); }
    }

    item = table->item(row, 1);
    if (item) {
        v1 = item->text().toFloat(&ok);
        if (!ok) { v1 = qSNaN(); }
    }

    item = table->item(row, 2);
    if (item) {
        v2 = item->text().toFloat(&ok);
        if (!ok) { v2 = qSNaN(); }
    }


    return { v0, v1, v2 };
}
