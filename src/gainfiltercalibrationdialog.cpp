#include "gainfiltercalibrationdialog.h"
#include "ui_gainfiltercalibrationdialog.h"

#include <QStyleHints>

#include "floatitemdelegate.h"
#include "intitemdelegate.h"
#include "util.h"

namespace
{
const int MAX_GAIN = 9;
}

GainFilterCalibrationDialog::GainFilterCalibrationDialog(DensInterface *densInterface, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::GainFilterCalibrationDialog)
    , densInterface_(densInterface)
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

    connect(ui->actionCut, &QAction::triggered, this, &GainFilterCalibrationDialog::onActionCut);
    connect(ui->actionCopy, &QAction::triggered, this, &GainFilterCalibrationDialog::onActionCopy);
    connect(ui->actionPaste, &QAction::triggered, this, &GainFilterCalibrationDialog::onActionPaste);
    connect(ui->actionDelete, &QAction::triggered, this, &GainFilterCalibrationDialog::onActionDelete);

    connect(densInterface_, &DensInterface::systemRemoteControl, this, &GainFilterCalibrationDialog::onSystemRemoteControl);
    connect(densInterface_, &DensInterface::diagLightMaxChanged, this, &GainFilterCalibrationDialog::onDiagLightMaxChanged);
    connect(densInterface_, &DensInterface::diagSensorUvInvokeReading, this, &GainFilterCalibrationDialog::onDiagSensorUvInvokeReading);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &GainFilterCalibrationDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &GainFilterCalibrationDialog::reject);

    ui->measTableWidget->setItemDelegate(new IntItemDelegate(0, std::numeric_limits<int>::max()));
    ui->gainRatioTableWidget->setItemDelegate(new FloatItemDelegate(0.0, std::numeric_limits<float>::infinity(), -1));
    ui->gainValueTableWidget->setItemDelegate(new FloatItemDelegate(0.0, std::numeric_limits<float>::infinity(), 6));

    ui->measTableWidget->clearContents();
    ui->gainRatioTableWidget->clearContents();
    ui->gainValueTableWidget->clearContents();

    ui->measTableWidget->setRowCount(1);

    ui->measTableWidget->setColumnCount(MAX_GAIN + 1);
    ui->gainRatioTableWidget->setRowCount(MAX_GAIN);
    ui->gainValueTableWidget->setRowCount(MAX_GAIN + 1);

    if (QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark) {
        ui->measTableWidget->setStyleSheet("QTableWidget::item:disabled { selection-color: black; }");
    } else {
        ui->measTableWidget->setStyleSheet("QTableWidget::item:disabled { selection-color: white; }");
    }

    connect(ui->scanPushButton, &QPushButton::clicked, this, &GainFilterCalibrationDialog::onScanPushButtonClicked);
    connect(ui->clearMeasPushButton, &QPushButton::clicked, this, &GainFilterCalibrationDialog::onClearMeasTable);
    connect(ui->addMeasRowPushButton, &QPushButton::clicked, this, &GainFilterCalibrationDialog::onAddMeasTableRow);
    connect(ui->removeMeasRowPushButton, &QPushButton::clicked, this, &GainFilterCalibrationDialog::onRemoveMeasTableRow);
    connect(ui->calcPushButton, &QPushButton::clicked, this, &GainFilterCalibrationDialog::onCalcPushButtonClicked);
    connect(ui->calcValuesPushButton, &QPushButton::clicked, this, &GainFilterCalibrationDialog::onCalcValuesPushButtonClicked);

    ui->measTableWidget->setCurrentCell(0, 0);

    connect(ui->measTableWidget->model(), &QAbstractItemModel::dataChanged, this, &GainFilterCalibrationDialog::onMeasTableWidgetDataChanged);
    connect(ui->measTableWidget, &QTableWidget::currentCellChanged, this, &GainFilterCalibrationDialog::onMeasTableCurrentCellChanged);
    connect(ui->gainRatioTableWidget->model(), &QAbstractItemModel::dataChanged, this, &GainFilterCalibrationDialog::onGainRatioTableWidgetDataChanged);
    connect(ui->gainValueTableWidget->model(), &QAbstractItemModel::dataChanged, this, &GainFilterCalibrationDialog::onGainValueTableWidgetDataChanged);

    onMeasTableCurrentCellChanged(0, 0, 0, 0);
    refreshButtonState();
}

GainFilterCalibrationDialog::~GainFilterCalibrationDialog()
{
    delete ui;
}

QList<float> GainFilterCalibrationDialog::gainValues() const
{
    QList<float> result;

    for (int i = 0; i < ui->gainValueTableWidget->rowCount(); i++) {
        QTableWidgetItem *item = ui->gainValueTableWidget->item(i, 0);
        if (!item) { return QList<float>(); }

        bool ok;
        double gainValue = item->text().toFloat(&ok);
        if (!ok) { return QList<float>(); }

        result.append(gainValue);
    }
    return result;
}

void GainFilterCalibrationDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    if (densInterface_->connected()) {
        densInterface_->sendInvokeSystemRemoteControl(true);
    } else {
        // Start in offline mode
        started_ = true;
        offline_ = true;
        refreshButtonState();
    }
}

void GainFilterCalibrationDialog::done(int r)
{
    const QVariant doneCode = property("doneCode");
    if (!doneCode.isValid() && densInterface_->connected() && densInterface_->remoteControlEnabled()) {
        setProperty("doneCode", r);
        densInterface_->sendInvokeSystemRemoteControl(false);
    } else {
        QDialog::done(r);
    }
}

void GainFilterCalibrationDialog::onSystemRemoteControl(bool enabled)
{
    qDebug() << "Remote control:" << enabled;
    if (enabled) {
        densInterface_->sendGetDiagLightMax();
    } else {
        const QVariant doneCode = property("doneCode");
        if (doneCode.isValid()) {
            QDialog::done(doneCode.toInt());
        }
    }
}

void GainFilterCalibrationDialog::onDiagLightMaxChanged()
{
    densInterface_->sendSetSystemDisplayText("Gain\nCalibration");
    started_ = true;
    refreshButtonState();
}

void GainFilterCalibrationDialog::onActionCut()
{
    QTableWidget *tableWidget = qobject_cast<QTableWidget *>(focusWidget());
    if (tableWidget) {
        util::tableWidgetCut(tableWidget);
    }
}

void GainFilterCalibrationDialog::onActionCopy()
{
    QTableWidget *tableWidget = qobject_cast<QTableWidget *>(focusWidget());
    if (tableWidget) {
        util::tableWidgetCopy(tableWidget);
    }
}

void GainFilterCalibrationDialog::onActionPaste()
{
    QTableWidget *tableWidget = qobject_cast<QTableWidget *>(focusWidget());
    if (tableWidget) {
        util::tableWidgetPaste(tableWidget);
    }
}

void GainFilterCalibrationDialog::onActionDelete()
{
    QTableWidget *tableWidget = qobject_cast<QTableWidget *>(focusWidget());
    if (tableWidget) {
        util::tableWidgetDelete(tableWidget);
    }
}

void GainFilterCalibrationDialog::refreshButtonState()
{
    ui->scanPushButton->setEnabled(densInterface_->connected() && !offline_ && !running_);
    ui->clearMeasPushButton->setEnabled(!running_);

    if (running_) {
        ui->calcPushButton->setEnabled(false);
        ui->calcValuesPushButton->setEnabled(false);
        ui->measTableWidget->setEnabled(false);
        ui->gainRatioTableWidget->setEnabled(false);
        ui->gainValueTableWidget->setEnabled(false);
    } else {
        onMeasTableWidgetDataChanged();
        onGainRatioTableWidgetDataChanged();
        onGainValueTableWidgetDataChanged();
        ui->measTableWidget->setEnabled(true);
        ui->gainRatioTableWidget->setEnabled(true);
        ui->gainValueTableWidget->setEnabled(true);
    }
}

void GainFilterCalibrationDialog::onScanPushButtonClicked()
{
    if (selectedMeasRow_ < 0) { return; }
    running_ = true;
    refreshButtonState();

    ui->measTableWidget->selectRow(selectedMeasRow_);
    util::tableWidgetDelete(ui->measTableWidget, false);

    ui->statusLabel->setText(tr("Scanning..."));

    currentGain_ = 0;
    measureNextGain();
}

void GainFilterCalibrationDialog::measureNextGain()
{
    densInterface_->sendInvokeUvDiagRead(
        DensInterface::SensorLightTransmission, densInterface_->diagLightMax(),
        /*Visual*/ 1, currentGain_, 719, 199);
}

void GainFilterCalibrationDialog::onDiagSensorUvInvokeReading(unsigned int reading)
{
    bool measError = false;

    if (!running_) { return; }
    if (reading < std::numeric_limits<unsigned int>::max()) {
        QTableWidgetItem *item = util::tableWidgetItem(ui->measTableWidget, selectedMeasRow_, currentGain_);
        item->setText(QString::number(reading));
        currentGain_++;
    } else {
        measError = true;
    }

    if (currentGain_ < ui->measTableWidget->columnCount() && !measError) {
        measureNextGain();
    } else {
        running_ = false;
        ui->statusLabel->setText(tr("Ready"));
        ui->measTableWidget->clearSelection();

        if (selectedMeasRow_ + 1 == ui->measTableWidget->rowCount()) {
            onAddMeasTableRow();
        }

        if (selectedMeasRow_ + 1 < ui->measTableWidget->rowCount()) {
            ui->measTableWidget->setCurrentCell(selectedMeasRow_ + 1, 0);
        }
        refreshButtonState();
    }
}

void GainFilterCalibrationDialog::onClearMeasTable()
{
    disconnect(ui->measTableWidget->model(), &QAbstractItemModel::dataChanged, this, &GainFilterCalibrationDialog::onMeasTableWidgetDataChanged);
    selectedMeasRow_ = 0;
    ui->measTableWidget->clearContents();
    connect(ui->measTableWidget->model(), &QAbstractItemModel::dataChanged, this, &GainFilterCalibrationDialog::onMeasTableWidgetDataChanged);
    ui->calcPushButton->setEnabled(false);
}

void GainFilterCalibrationDialog::onAddMeasTableRow()
{
    const int oldRowCount = ui->measTableWidget->rowCount();
    ui->measTableWidget->setRowCount(oldRowCount + 1);
    ui->measTableWidget->setVerticalHeaderItem(oldRowCount, new QTableWidgetItem(tr("Patch %1").arg(oldRowCount)));
    refreshButtonState();
}

void GainFilterCalibrationDialog::onRemoveMeasTableRow()
{
    const int oldRowCount = ui->measTableWidget->rowCount();
    if (oldRowCount > 1) {
        ui->measTableWidget->setRowCount(oldRowCount - 1);
    }
    refreshButtonState();
}

void GainFilterCalibrationDialog::onMeasTableWidgetDataChanged()
{
    bool enableCalculate = true;

    // Simpler version of the table iteration done during the calculation
    // process to decide if there's enough data to even try.
    for (int i = 0; i < ui->gainRatioTableWidget->rowCount(); i++) {
        bool hasGainPair = false;
        for (int j = 0; j < ui->measTableWidget->rowCount(); j++) {
            QTableWidgetItem *itemHigher = ui->measTableWidget->item(j, i + 1);
            QTableWidgetItem *itemLower = ui->measTableWidget->item(j, i);

            if (itemHigher && itemLower && !itemHigher->text().isEmpty() && !itemLower->text().isEmpty()) {
                hasGainPair = true;
                break;
            }
        }
        if (!hasGainPair) {
            enableCalculate = false;
            break;
        }
    }

    ui->calcPushButton->setEnabled(enableCalculate);
}

void GainFilterCalibrationDialog::onMeasTableCurrentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn)
{
    if (currentRow < 0) { currentRow = 0; }
    ui->scanPushButton->setText(tr("Scan %1").arg(ui->measTableWidget->verticalHeaderItem(currentRow)->text()));
    selectedMeasRow_ = currentRow;
}

void GainFilterCalibrationDialog::onGainRatioTableWidgetDataChanged()
{
    bool hasEmpty = util::tableWidgetHasEmptyCells(ui->gainRatioTableWidget);
    ui->calcValuesPushButton->setEnabled(!hasEmpty);
}

void GainFilterCalibrationDialog::onGainValueTableWidgetDataChanged()
{
    bool hasEmpty = util::tableWidgetHasEmptyCells(ui->gainValueTableWidget);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!hasEmpty);
}

void GainFilterCalibrationDialog::onCalcPushButtonClicked()
{
    bool ok;
    QList<double> gainRatios(ui->gainRatioTableWidget->rowCount());

    int baseGainIndex = 0;

    // Calculate the gain ratios
    for (int i = 0; i < gainRatios.size(); i++) {
        unsigned int higherValue = 0;
        unsigned int lowerValue = 0;

        for (int j = 0; j < ui->measTableWidget->rowCount(); j++) {
            QTableWidgetItem *itemHigher = ui->measTableWidget->item(j, i + 1);
            QTableWidgetItem *itemLower = ui->measTableWidget->item(j, i);

            if (itemHigher && itemLower) {
                higherValue = itemHigher->text().toUInt(&ok);
                if (!ok) { higherValue = 0; }

                lowerValue = itemLower->text().toUInt(&ok);
                if (!ok) { lowerValue = 0; }
            }

            if (higherValue > 0 && lowerValue > 0) {
                if (j == 0) {
                    baseGainIndex = i + 1;
                }
                break;
            }
        }

        if (higherValue == 0 || lowerValue == 0) {
            qWarning() << "Unable to calculate due to missing data for ratio" << i;
            return;
        }
        gainRatios[i] = static_cast<double>(higherValue) / static_cast<double>(lowerValue);
    }

    // Update the base gain setting
    ui->baseGainComboBox->setCurrentIndex(baseGainIndex);

    // Update the gain ratio table
    disconnect(ui->gainRatioTableWidget->model(), &QAbstractItemModel::dataChanged, this, &GainFilterCalibrationDialog::onGainRatioTableWidgetDataChanged);

    for (int i = 0; i < gainRatios.size(); i++) {
        QTableWidgetItem *item = util::tableWidgetItem(ui->gainRatioTableWidget, i, 0);
        item->setText(QString::number(gainRatios[i], 'f', 10));
    }

    connect(ui->gainRatioTableWidget->model(), &QAbstractItemModel::dataChanged, this, &GainFilterCalibrationDialog::onGainRatioTableWidgetDataChanged);
    onGainRatioTableWidgetDataChanged();
    onCalcValuesPushButtonClicked();
}

void GainFilterCalibrationDialog::onCalcValuesPushButtonClicked()
{
    bool ok;
    QTableWidgetItem *item;

    // Collect the base gain value
    const int baseGainIndex = ui->baseGainComboBox->currentIndex();
    const DensUvVisCalGain::GainLevel baseGain = static_cast<DensUvVisCalGain::GainLevel>(baseGainIndex);
    const float baseGainValue = DensUvVisCalGain::gainSpecValue(baseGain);


    // Collect the list of gain ratios
    QList<double> gainRatios(ui->gainRatioTableWidget->rowCount());
    for (int i = 0; i < gainRatios.size(); i++) {
        QTableWidgetItem *item = ui->gainRatioTableWidget->item(i, 0);
        if (!item) { return; }

        double gainRatio = item->text().toDouble(&ok);
        if (!ok) { return; }

        gainRatios[i] = gainRatio;
    }

    QList<double> gainValues(ui->gainValueTableWidget->rowCount());

    // Set the base gain value
    gainValues[baseGainIndex] = baseGainValue;

    // Calculate gain values above the base gain
    for (int i = baseGainIndex + 1; i < gainValues.size(); i++) {
        gainValues[i] = gainValues[i - 1] * gainRatios[i - 1];
    }

    // Calculate gain values below the base gain
    for (int i = baseGainIndex - 1; i >= 0; i--) {
        gainValues[i] = gainValues[i + 1] / gainRatios[i];
    }

    // Set the gain values to the table
    disconnect(ui->gainValueTableWidget->model(), &QAbstractItemModel::dataChanged, this, &GainFilterCalibrationDialog::onGainValueTableWidgetDataChanged);
    for (int i = 0; i < ui->gainValueTableWidget->rowCount(); i++) {
        item = util::tableWidgetItem(ui->gainValueTableWidget, i, 0);
        item->setText(QString::number(gainValues[i], 'f'));
    }
    connect(ui->gainValueTableWidget->model(), &QAbstractItemModel::dataChanged, this, &GainFilterCalibrationDialog::onGainValueTableWidgetDataChanged);
    onGainValueTableWidgetDataChanged();
}
