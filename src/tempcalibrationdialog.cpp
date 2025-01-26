#include "tempcalibrationdialog.h"
#include "ui_tempcalibrationdialog.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

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

    ui->visInputTableWidget->setItemDelegateForColumn(0, new FloatItemDelegate(-20, 100, 2));
    for (int i = 1; i < ui->visInputTableWidget->columnCount(); i++) {
        ui->visInputTableWidget->setItemDelegateForColumn(i, new FloatItemDelegate(0, 1000, 6));
    }

    ui->visResultsTableWidget->setItemDelegate(new FloatItemDelegate());
    ui->visResultsTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

    ui->uvInputTableWidget->setItemDelegateForColumn(0, new FloatItemDelegate(-20, 100, 2));
    for (int i = 1; i < ui->uvInputTableWidget->columnCount(); i++) {
        ui->uvInputTableWidget->setItemDelegateForColumn(i, new FloatItemDelegate(0, 1000, 6));
    }

    ui->uvResultsTableWidget->setItemDelegate(new FloatItemDelegate());
    ui->uvResultsTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

    onClearClicked();
}

TempCalibrationDialog::~TempCalibrationDialog()
{
    delete ui;
}

void TempCalibrationDialog::setUniqueId(const QString &uniqueId)
{
    uniqueId_ = uniqueId;
}

bool TempCalibrationDialog::hasVisValues() const
{
    return hasVisValues_;
}

CoefficientSet TempCalibrationDialog::b0VisValues() const
{
    return coefficientSetCollectRow(ui->visResultsTableWidget, 0);
}

CoefficientSet TempCalibrationDialog::b1VisValues() const
{
    return coefficientSetCollectRow(ui->visResultsTableWidget, 1);
}

CoefficientSet TempCalibrationDialog::b2VisValues() const
{
    return coefficientSetCollectRow(ui->visResultsTableWidget, 2);
}

bool TempCalibrationDialog::hasUvValues() const
{
    return hasUvValues_;
}

CoefficientSet TempCalibrationDialog::b0UvValues() const
{
    return coefficientSetCollectRow(ui->uvResultsTableWidget, 0);
}

CoefficientSet TempCalibrationDialog::b1UvValues() const
{
    return coefficientSetCollectRow(ui->uvResultsTableWidget, 1);
}

CoefficientSet TempCalibrationDialog::b2UvValues() const
{
    return coefficientSetCollectRow(ui->uvResultsTableWidget, 2);
}

void TempCalibrationDialog::onActionCut()
{
    QTableWidget *tableWidget = qobject_cast<QTableWidget *>(focusWidget());
    if (tableWidget == ui->visInputTableWidget || tableWidget == ui->uvInputTableWidget) {
        util::tableWidgetCut(tableWidget);
    }
}

void TempCalibrationDialog::onActionCopy()
{
    QTableWidget *tableWidget = qobject_cast<QTableWidget *>(focusWidget());
    if (tableWidget) {
        util::tableWidgetCopy(tableWidget);
    }
}

void TempCalibrationDialog::onActionPaste()
{
    QTableWidget *tableWidget = qobject_cast<QTableWidget *>(focusWidget());
    if (tableWidget == ui->visInputTableWidget || tableWidget == ui->uvInputTableWidget) {
        util::tableWidgetPaste(tableWidget);
    }
}

void TempCalibrationDialog::onActionDelete()
{
    QTableWidget *tableWidget = qobject_cast<QTableWidget *>(focusWidget());
    if (tableWidget == ui->visInputTableWidget || tableWidget == ui->uvInputTableWidget) {
        util::tableWidgetDelete(tableWidget);
    }
}

void TempCalibrationDialog::onImportClicked()
{
    QFileDialog fileDialog(this, tr("Import Temperature Data"));
    fileDialog.setDefaultSuffix(".dat");
    fileDialog.setNameFilters(
        QStringList() << tr("Matching Data Files (*%1*.dat)").arg(uniqueId_)
                      << tr("All Data Files (*.dat)"));
    fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
    if (fileDialog.exec() && !fileDialog.selectedFiles().isEmpty()) {
        QString filename = fileDialog.selectedFiles().constFirst();
        if (!filename.isEmpty()) {
            QFile importFile(filename);

            if (!importFile.open(QIODevice::ReadOnly)) {
                qWarning() << "Couldn't open import file.";
                QMessageBox::warning(this, tr("Import Error"), tr("Unable to open import file"));
                return;
            }

            QByteArray importData = importFile.readAll();
            importFile.close();

            if (!processImportData(importData)) {
                QMessageBox::warning(this, tr("Import Error"), tr("Unable to process import file"));
            }
        }
    }
}

bool TempCalibrationDialog::processImportData(const QByteArray &importData)
{
    QJsonDocument doc = QJsonDocument::fromJson(importData);
    if (doc.isEmpty()) {
        return false;
    }

    const QJsonObject root = doc.object();

    QString uniqueId;
    QDateTime timestamp;
    if (root.contains("header") && root["header"].isObject()) {
        const QJsonObject jsonHeader = root["header"].toObject();

        if (jsonHeader.contains("uniqueId")) {
            uniqueId = jsonHeader["uniqueId"].toString();
        }
        if (jsonHeader.contains("timestamp")) {
            timestamp = QDateTime::fromString(jsonHeader["timestamp"].toString(), Qt::ISODate);
        }
    }

    if (uniqueId.isEmpty() || !timestamp.isValid()) {
        return false;
    }

    if (!uniqueId_.isEmpty() && uniqueId_ != uniqueId) {
        qDebug() << uniqueId_ << "!=" << uniqueId;
        if (QMessageBox::warning(
                this,
                tr("Device Mismatch"),
                tr("The selected data file contains temperature data for a different device, "
                   "and will not result in accurate correction calculations. Are you sure you "
                   "still want to import it?"),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No) != QMessageBox::Yes) {
            return true;
        }
    }

    qDebug() << "Import data captured on:" << timestamp.toString();

    if (root.contains("sequenceVis") && root["sequenceVis"].isArray()) {
        const QJsonArray sequenceVis = root["sequenceVis"].toArray();
        populateImportDataSequence(ui->visInputTableWidget, sequenceVis);
        ui->visResultsTableWidget->clearContents();
    }

    if (root.contains("sequenceUv") && root["sequenceUv"].isArray()) {
        const QJsonArray sequenceUv = root["sequenceUv"].toArray();
        populateImportDataSequence(ui->uvInputTableWidget, sequenceUv);
        ui->uvResultsTableWidget->clearContents();
    }

    return true;
}

void TempCalibrationDialog::populateImportDataSequence(QTableWidget *inputTableWidget, const QJsonArray &sequence)
{
    inputTableWidget->clearContents();
    int row = 0;

    for (const QJsonValue &entry : std::as_const(sequence)) {
        if (!entry.isObject()) { continue; }
        const QJsonObject entryObj = entry.toObject();

        float temperature = qSNaN();
        QList<float> readings;

        if (entryObj.contains("temperature") && entryObj["temperature"].isDouble()) {
            temperature = entryObj["temperature"].toDouble(qSNaN());
        }

        if (entryObj.contains("readings") && entryObj["readings"].isArray()) {
            const QJsonArray readingArray = entryObj["readings"].toArray();
            for (const QJsonValue &readingEntry : std::as_const(readingArray)) {
                float reading = readingEntry.toDouble(qSNaN());
                readings.append(reading);
            }
        }

        if (qIsNaN(temperature) || readings.isEmpty()) {
            continue;
        }

        QTableWidgetItem *item = inputTableWidget->item(row, 0);
        if (!item) {
            item = new QTableWidgetItem();
            inputTableWidget->setItem(row, 0, item);
        }
        item->setText(QString::number(temperature, 'f', 1));

        for (qsizetype i = 0; i < readings.size(); i++) {
            item = inputTableWidget->item(row, i + 1);
            if (!item) {
                item = new QTableWidgetItem();
                inputTableWidget->setItem(row, i + 1, item);
            }
            item->setText(QString::number(readings[i]));
        }
        row++;
    }
}

void TempCalibrationDialog::onClearClicked()
{
    hasVisValues_ = false;
    hasUvValues_ = false;
    ui->visInputTableWidget->clearContents();
    ui->visResultsTableWidget->clearContents();
    ui->uvInputTableWidget->clearContents();
    ui->uvResultsTableWidget->clearContents();
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}

void TempCalibrationDialog::onCalculateClicked()
{
    QList<QList<double>> tableData;
    int refTempRow;

    ui->visResultsTableWidget->clearContents();
    ui->uvResultsTableWidget->clearContents();
    hasVisValues_ = false;
    hasUvValues_ = false;

    // Calculate VIS corrections
    tableData = collectInputData(ui->visInputTableWidget);
    refTempRow = findReferenceRow(tableData);

    if (tableData.isEmpty() || refTempRow < 0) {
        QMessageBox::warning(this, tr("Invalid Values"), tr("Cannot calculate VIS corrections from invalid or incomplete data."));
    } else {
        calculateCorrections(tableData, refTempRow, ui->visResultsTableWidget);
        hasVisValues_ = true;
    }

    // Calculate UV corrections
    tableData = collectInputData(ui->uvInputTableWidget);
    refTempRow = findReferenceRow(tableData);

    if (tableData.isEmpty() || refTempRow < 0) {
        QMessageBox::warning(this, tr("Invalid Values"), tr("Cannot calculate UV corrections from invalid or incomplete data."));
    } else {
        calculateCorrections(tableData, refTempRow, ui->uvResultsTableWidget);
        hasUvValues_ = true;
    }

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(hasVisValues_ || hasUvValues_);
}

QList<QList<double>> TempCalibrationDialog::collectInputData(QTableWidget *inputTableWidget)
{
    QList<QList<double>> tableData;
    bool valid = true;

    for (int row = 0; row < inputTableWidget->rowCount(); row++) {
        QList<double> rowData;
        for (int col = 0; col < inputTableWidget->columnCount(); col++) {
            QTableWidgetItem *item = inputTableWidget->item(row, col);
            if (!item || item->text().isEmpty()) {
                valid = false;
                break;
            }
            double val = item->text().toDouble(&valid);
            if (!valid) { break; }

            rowData.append(val);
        }
        if (!valid) { break; }
        tableData.append(rowData);
    }

    if (!valid) {
        tableData.clear();
    }
    return tableData;
}

int TempCalibrationDialog::findReferenceRow(const QList<QList<double>> &tableData)
{
    int refTempRow = -1;
    for (qsizetype r = 0; r < tableData.size(); r++) {
        const QList<double> rowData = tableData[r];

        if (refTempRow == -1 && rowData[0] > 18.0 && rowData[0] < 22.0) {
            refTempRow = r;
            break;
        }
    }
    return refTempRow;
}

void TempCalibrationDialog::calculateCorrections(const QList<QList<double>> &tableData, int refTempRow, QTableWidget *resultsTableWidget)
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
    coefficientSetAssignRow(resultsTableWidget, 0, b0Result);
    coefficientSetAssignRow(resultsTableWidget, 1, b1Result);
    coefficientSetAssignRow(resultsTableWidget, 2, b2Result);

    qDebug() << "Reference temp:" << QString("%1°C").arg(QString::number(tableData[refTempRow][0], 'f', 1));
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
