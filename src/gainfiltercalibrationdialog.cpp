#include "gainfiltercalibrationdialog.h"
#include "ui_gainfiltercalibrationdialog.h"

#include "floatitemdelegate.h"
#include "util.h"

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
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    ui->measTableWidget->setItemDelegate(new FloatItemDelegate(0.0, 512.0, 6));
    ui->gainRatioTableWidget->setItemDelegate(new FloatItemDelegate(0.0, std::numeric_limits<float>::infinity(), 6));
    ui->gainValueTableWidget->setItemDelegate(new FloatItemDelegate(0.0, std::numeric_limits<float>::infinity(), 6));

    ui->measTableWidget->clearContents();
    ui->gainRatioTableWidget->clearContents();
    ui->gainValueTableWidget->clearContents();

    connect(ui->clearMeasPushButton, &QPushButton::clicked, this, &GainFilterCalibrationDialog::onClearMeasTable);
    connect(ui->calcPushButton, &QPushButton::clicked, this, &GainFilterCalibrationDialog::onCalcPushButtonClicked);
    connect(ui->calcValuesPushButton, &QPushButton::clicked, this, &GainFilterCalibrationDialog::onCalcValuesPushButtonClicked);

    connect(ui->measTableWidget->model(), &QAbstractItemModel::dataChanged, this, &GainFilterCalibrationDialog::onMeasTableWidgetDataChanged);
    connect(ui->gainRatioTableWidget->model(), &QAbstractItemModel::dataChanged, this, &GainFilterCalibrationDialog::onGainRatioTableWidgetDataChanged);

    refreshButtonState();
}

GainFilterCalibrationDialog::~GainFilterCalibrationDialog()
{
    delete ui;
}

bool GainFilterCalibrationDialog::success() const
{
    return success_;
}

void GainFilterCalibrationDialog::accept()
{
    if (running_) { return; }

    if (densInterface_->connected()) {
        densInterface_->sendInvokeSystemRemoteControl(false);
    }
    QDialog::accept();
}

void GainFilterCalibrationDialog::reject()
{
    if (running_) { return; }

    if (densInterface_->connected()) {
        densInterface_->sendInvokeSystemRemoteControl(false);
    }
    QDialog::reject();
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

void GainFilterCalibrationDialog::onSystemRemoteControl(bool enabled)
{
    qDebug() << "Remote control:" << enabled;
    if (enabled) {
        densInterface_->sendGetDiagLightMax();
    }
    // if (enabled && !started_) {
    //     started_ = true;
    //     running_ = true;
    //     //XXX densInterface_->sendInvokeCalGain();
    //     //XXX Doing test gain reading sweep
    //     curGain_ = 0;
    //     nextDiagReading();
    // }
}

void GainFilterCalibrationDialog::onDiagLightMaxChanged()
{
    densInterface_->sendSetSystemDisplayText("Gain\nCalibration");
    started_ = true;
    refreshButtonState();
}

void GainFilterCalibrationDialog::onDiagSensorUvInvokeReading(unsigned int reading)
{
    if (reading < std::numeric_limits<unsigned int>::max()) {
        //TODO Has valid reading
    } else {
        //TODO Error or saturation
    }
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
    onMeasTableWidgetDataChanged();
    onGainRatioTableWidgetDataChanged();
}

void GainFilterCalibrationDialog::onClearMeasTable()
{
    disconnect(ui->measTableWidget->model(), &QAbstractItemModel::dataChanged, this, &GainFilterCalibrationDialog::onMeasTableWidgetDataChanged);
    ui->measTableWidget->clearContents();
    connect(ui->measTableWidget->model(), &QAbstractItemModel::dataChanged, this, &GainFilterCalibrationDialog::onMeasTableWidgetDataChanged);
    ui->calcPushButton->setEnabled(false);
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

void GainFilterCalibrationDialog::onGainRatioTableWidgetDataChanged()
{
    bool hasEmpty = util::tableWidgetHasEmptyCells(ui->gainRatioTableWidget);
    ui->calcValuesPushButton->setEnabled(!hasEmpty);
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
        item->setText(QString::number(gainRatios[i], 'f'));
    }

    connect(ui->gainRatioTableWidget->model(), &QAbstractItemModel::dataChanged, this, &GainFilterCalibrationDialog::onGainRatioTableWidgetDataChanged);
    onGainRatioTableWidgetDataChanged();
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
    for (int i = 0; i < ui->gainValueTableWidget->rowCount(); i++) {
        item = util::tableWidgetItem(ui->gainValueTableWidget, i, 0);
        item->setText(QString::number(gainValues[i], 'f'));
    }
}
