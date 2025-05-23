#include "slopecalibrationdialog.h"
#include "ui_slopecalibrationdialog.h"

#include <QClipboard>
#include <QMimeData>
#include <QStyledItemDelegate>
#include <QThread>
#include <QSettings>
#include <QRegularExpression>
#include <QDebug>
#include <cmath>
#include "floatitemdelegate.h"
#include "util.h"

SlopeCalibrationDialog::SlopeCalibrationDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SlopeCalibrationDialog),
    enableReflReadings_(false),
    calValues_{qSNaN(), qSNaN(), qSNaN()}
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

    connect(ui->actionCut, &QAction::triggered, this, &SlopeCalibrationDialog::onActionCut);
    connect(ui->actionCopy, &QAction::triggered, this, &SlopeCalibrationDialog::onActionCopy);
    connect(ui->actionPaste, &QAction::triggered, this, &SlopeCalibrationDialog::onActionPaste);
    connect(ui->actionDelete, &QAction::triggered, this, &SlopeCalibrationDialog::onActionDelete);

    connect(ui->calculatePushButton, &QPushButton::clicked, this, &SlopeCalibrationDialog::onCalculateResults);
    connect(ui->clearPushButton, &QPushButton::clicked, this, &SlopeCalibrationDialog::onClearReadings);

    model_ = new QStandardItemModel(22, 2, this);
    model_->setHorizontalHeaderLabels(QStringList() << tr("Density") << tr("Raw Reading"));

    QStringList verticalLabels;
    for (int i = 0; i < model_->rowCount(); i++) {
        verticalLabels.append(QString::number(i));
    }
    model_->setVerticalHeaderLabels(verticalLabels);

    ui->tableView->setModel(model_);
    ui->tableView->setColumnWidth(0, 80);
    ui->tableView->setColumnWidth(1, 150);
    ui->tableView->setItemDelegateForColumn(0, new FloatItemDelegate(0.0, 5.0, 2));
    ui->tableView->setItemDelegateForColumn(1, new FloatItemDelegate(0.0, 1000.0, 6));

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    // Preload calibrated numbers for the step wedge, with basic validation,
    // if they have been stored in app settings. As this is primarily intended
    // to help with device manufacturing use cases, no UI is currently provided
    // for populating this data.
    QSettings settings;
    QVariantList scaleList = settings.value("slope_calibration/scale").toList();
    if (!scaleList.isEmpty()) {
        int i = 0;
        for (const QVariant &entry : std::as_const(scaleList)) {
            bool ok;
            const QString entryStr = entry.toString();
            const float entryNum = entryStr.toFloat(&ok);
            if (ok && entryNum >= 0.0F && entryNum <= 5.0F) {
                model_->setItem(i++, 0, new QStandardItem(entryStr));
            }
        }
    }

    connect(ui->wedgePrecSpinBox, &QSpinBox::valueChanged, this, &SlopeCalibrationDialog::wedgePrecValueChanged);
    connect(ui->wedgeCountSpinBox, &QSpinBox::valueChanged, this, &SlopeCalibrationDialog::wedgeCountValueChanged);
}

SlopeCalibrationDialog::SlopeCalibrationDialog(DensInterface *densInterface, QWidget *parent)
    : SlopeCalibrationDialog(parent)
{
    if (densInterface) {
        connect(densInterface, &DensInterface::densityReading, this, &SlopeCalibrationDialog::onDensityReading);
    }
}

SlopeCalibrationDialog::SlopeCalibrationDialog(DensiStickRunner *stickRunner, QWidget *parent)
    : SlopeCalibrationDialog(parent)
{
    if (stickRunner) {
        connect(stickRunner, &DensiStickRunner::targetDensity, this, &SlopeCalibrationDialog::onTargetMeasurement);
    }
    enableReflReadings_ = true;
}

SlopeCalibrationDialog::~SlopeCalibrationDialog()
{
    delete ui;
}

std::tuple<float, float, float> SlopeCalibrationDialog::calValues() const
{
    return calValues_;
}

void SlopeCalibrationDialog::wedgePrecValueChanged(int value)
{
    QList<double> rawValues;
    for (int row = 0; row < model_->rowCount(); row++) {
        float density = itemValueAsFloat(row, 0);
        rawValues.append(density);
    }

    ui->tableView->setItemDelegateForColumn(0, new FloatItemDelegate(0.0, 5.0, value));

    for (int row = 0; row < model_->rowCount(); row++) {
        QString numStr;
        if (!qIsNaN(rawValues[row])) {
            numStr = QString::number(rawValues[row], 'f', value);
        }
        model_->setItem(row, 0, new QStandardItem(numStr));
    }
}

void SlopeCalibrationDialog::wedgeCountValueChanged(int value)
{
    int maxRows = value + 1;
    if (maxRows > model_->rowCount()) {
        int startIndex = model_->rowCount();
        model_->setRowCount(maxRows);
        for (int i = startIndex; i < maxRows; i++) {
            model_->setVerticalHeaderItem(i, new QStandardItem(QString::number(i)));
        }
    } else if (maxRows < model_->rowCount()) {
        model_->setRowCount(maxRows);
    }
}

void SlopeCalibrationDialog::onDensityReading(DensInterface::DensityType type, float dValue, float dZero, float rawValue, float corrValue)
{
    Q_UNUSED(dValue)
    Q_UNUSED(dZero)
    Q_UNUSED(corrValue)

    // Only using VIS transmission readings for this
    if (type != DensInterface::DensityTransmission) {
        return;
    }

    addRawMeasurement(rawValue);
}

void SlopeCalibrationDialog::onTargetMeasurement(float basicReading)
{
    addRawMeasurement(basicReading);
}

void SlopeCalibrationDialog::addRawMeasurement(float rawValue)
{
    if (qIsNaN(rawValue) || rawValue < 0.0F) {
        return;
    }

    QPair<int, int> ulIndex = upperLeftActiveIndex();
    int row = ulIndex.first;
    if (row < 0) { row = 0; }

    QString numStr = QString::number(rawValue, 'f', 6);
    model_->setItem(row, 1, new QStandardItem(numStr));

    if (row < model_->rowCount() - 1) {
        QModelIndex index = model_->index(row + 1, 1);
        ui->tableView->setCurrentIndex(index);
    }
}

void SlopeCalibrationDialog::onActionCut()
{
    onActionCopy();
    onActionDelete();
}

void SlopeCalibrationDialog::onActionCopy()
{
    QModelIndexList selected = ui->tableView->selectionModel()->selectedIndexes();
    std::sort(selected.begin(), selected.end());

    int curRow = -1;
    QString num1;
    QString num2;
    QVector<QPair<QString,QString>> numList;
    bool hasCol1 = false;
    bool hasCol2 = false;

    for (const QModelIndex &index : std::as_const(selected)) {
        if (curRow != index.row()) {
            if (curRow != -1 && (!num1.isEmpty() || !num2.isEmpty())) {
                numList.append(qMakePair(num1, num2));
            }
            num1 = QString();
            num2 = QString();
            curRow = index.row();
        }

        QStandardItem *item = model_->itemFromIndex(index);
        if (item && index.column() == 0) {
            num1 = item->text();
            hasCol1 = true;
        } else if (item && index.column() == 1) {
            num2 = item->text();
            hasCol2 = true;
        }
    }
    if (curRow != -1 && (!num1.isEmpty() || !num2.isEmpty())) {
        numList.append(qMakePair(num1, num2));
    }

    QString copiedText;
    for (const auto &numElement : numList) {
        if ((hasCol1 || hasCol2) && !copiedText.isEmpty()) {
#if defined(Q_OS_WIN)
            copiedText.append(QLatin1String("\r\n"));
#else
            copiedText.append(QLatin1String("\n"));
#endif
        }
        if (hasCol1 && hasCol2) {
            copiedText.append(numElement.first);
            copiedText.append(QChar('\t'));
            copiedText.append(numElement.second);
        } else if (hasCol1) {
            copiedText.append(numElement.first);
        } else if (hasCol2) {
            copiedText.append(numElement.second);
        }
    }

    // Move to the clipboard
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(copiedText, QClipboard::Clipboard);

    if (clipboard->supportsSelection()) {
        clipboard->setText(copiedText, QClipboard::Selection);
    }

#if defined(Q_OS_UNIX)
    QThread::msleep(1);
#endif
}

void SlopeCalibrationDialog::onActionPaste()
{
    static QRegularExpression reLine("\n|\r\n|\r");
    static QRegularExpression reRow("[,;]\\s*|\\s+");

    // Capture and split the text to be pasted
    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();
    QList<QPair<float,float>> numList;
    if (mimeData->hasText()) {
        const QString text = mimeData->text();
        const QStringList elements = text.split(reLine, Qt::SkipEmptyParts);
        for (const QString& element : elements) {
            QStringList rowElements = element.split(reRow, Qt::SkipEmptyParts);
            bool ok;
            float num1 = qQNaN();
            float num2 = qQNaN();
            if (rowElements.size() > 0) {
                num1 = rowElements.at(0).toFloat(&ok);
                if (!ok) { num1 = qQNaN(); }
            }
            if (rowElements.size() > 1) {
                num2 = rowElements.at(1).toFloat(&ok);
                if (!ok) { num2 = qQNaN(); }
            }
            if (!qIsNaN(num1) || !qIsNaN(num2)) {
                numList.append(qMakePair(num1, num2));
            }
        }
    }

    // Find the upper-left corner of the paste area
    QPair<int, int> ulIndex = upperLeftActiveIndex();
    int row = ulIndex.first;
    int col = ulIndex.second;

    // Get the precision of the step wedge density values
    int wedgeDecimals;
    FloatItemDelegate *delegate = qobject_cast<FloatItemDelegate *>(ui->tableView->itemDelegateForColumn(0));
    if (delegate) {
        wedgeDecimals = delegate->decimals();
    } else {
        wedgeDecimals = 2;
    }

    // Paste the values
    if (!numList.isEmpty() && row >= 0 && col >= 0) {
        for (auto numElement : numList) {
            QString numStr;
            if (col == 0) {
                if (!qIsNaN(numElement.first)) {
                    numStr = QString::number(numElement.first, 'f', wedgeDecimals);
                    model_->setItem(row, col, new QStandardItem(numStr));
                }
                if (!qIsNaN(numElement.second)) {
                    numStr = QString::number(numElement.second, 'f', 6);
                    model_->setItem(row, col + 1, new QStandardItem(numStr));
                }
            } else {
                if (!qIsNaN(numElement.first)) {
                    numStr = QString::number(numElement.first, 'f', 6);
                    model_->setItem(row, col, new QStandardItem(numStr));
                } else if (!qIsNaN(numElement.second)) {
                    numStr = QString::number(numElement.second, 'f', 6);
                    model_->setItem(row, col, new QStandardItem(numStr));
                }
            }
            row++;
            if (row >= model_->rowCount()) { break; }
        }
    }
}

void SlopeCalibrationDialog::onActionDelete()
{
    QModelIndexList selected = ui->tableView->selectionModel()->selectedIndexes();
    for (const QModelIndex &index : std::as_const(selected)) {
        QStandardItem *item = model_->itemFromIndex(index);
        if (item) {
            item->clearData();
        }
    }
}

void SlopeCalibrationDialog::onCalculateResults()
{
    qDebug() << "Calculate Results";
    QList<float> xList;
    QList<float> yList;
    float base_measurement = qSNaN();

    for (int row = 0; row < model_->rowCount(); row++) {
        float density = itemValueAsFloat(row, 0);
        float measurement = itemValueAsFloat(row, 1);
        if (qIsNaN(density) || qIsNaN(measurement)) {
            break;
        }
        if (row == 0) {
            if (enableReflReadings_) {
                // For reflection readings, the base measurement is calculated
                // by removing the density of the first patch from the first
                // reading.
                // Because of this, there will be no difference between
                // actual and expected values for the first two data points,
                // and there will be one more element in the set than in the
                // provided input.
                base_measurement = measurement * std::pow(10.0F, density);
                float logBase = std::log10(base_measurement);
                xList.append(logBase);
                yList.append(logBase);

                float logMeas = std::log10(measurement);
                xList.append(logMeas);
                yList.append(logMeas);
            } else {
                if (density < 0.0F || density > 0.001F) {
                    qDebug() << "First row density must be zero:" << density;
                    break;
                }

                float x = std::log10(measurement);
                xList.append(x);
                yList.append(x);
                base_measurement = measurement;
            }
        } else {
            float x = std::log10(measurement);
            float y = std::log10(base_measurement / std::pow(10.0F, density));
            xList.append(x);
            yList.append(y);
        }
    }
    qDebug() << "Have" << xList.size() << "rows of data";
    if (xList.size() < 5) {
        qDebug() << "Not enough rows of data";
        return;
    }

    auto beta = util::polyfit(xList, yList);

    ui->b0LineEdit->setText(QString::number(std::get<0>(beta), 'f'));
    ui->b1LineEdit->setText(QString::number(std::get<1>(beta), 'f'));
    ui->b2LineEdit->setText(QString::number(std::get<2>(beta), 'f'));
    calValues_ = beta;
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void SlopeCalibrationDialog::onClearReadings()
{
    for (int i = 0; i < model_->rowCount(); i++) {
        model_->setItem(i, 1, nullptr);
    }
    QModelIndex index = model_->index(0, 0);
    ui->tableView->setCurrentIndex(index);
    ui->tableView->selectionModel()->clearSelection();
    ui->tableView->setColumnWidth(0, 80);
    ui->tableView->setColumnWidth(1, 150);
    ui->tableView->scrollToTop();
}

QPair<int, int> SlopeCalibrationDialog::upperLeftActiveIndex() const
{
    int row = -1;
    int col = -1;
    QModelIndexList selected = ui->tableView->selectionModel()->selectedIndexes();
    selected.append(ui->tableView->selectionModel()->currentIndex());
    for (const QModelIndex &index : std::as_const(selected)) {
        if (row == -1 || index.row() < row) {
            row = index.row();
        }
        if (col == -1 || index.column() < col) {
            col = index.column();
        }
    }
    return qMakePair(row, col);
}

float SlopeCalibrationDialog::itemValueAsFloat(int row, int col) const
{
    float value;
    bool ok = false;
    QStandardItem *item = model_->item(row, col);
    if (item) {
        value = item->text().toFloat(&ok);
    }

    if (!ok) {
        value = qQNaN();
    }
    return value;
}
