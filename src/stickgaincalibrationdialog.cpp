#include "stickgaincalibrationdialog.h"
#include "ui_stickgaincalibrationdialog.h"

#include <QScrollBar>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QDebug>

namespace
{
static const tsl2585_gain_t MAX_GAIN = TSL2585_GAIN_256X;
static const quint16 SAMPLE_TIME = 719;

static const quint16 LED_SAMPLE_COUNT = 99;
static const int LED_SKIP_READINGS = 2;
static const int LED_SAMPLE_READINGS = 2;

static const quint16 GAIN_SAMPLE_COUNT = 199;
static const int GAIN_SKIP_READINGS = 2;
static const int GAIN_SAMPLE_READINGS = 5;
static const int GAIN_DELAY_COUNT = 200;
}

StickGainCalibrationDialog::StickGainCalibrationDialog(DensiStickInterface *stickInterface, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::StickGainCalibrationDialog),
    stickInterface_(stickInterface),
    started_(false), running_(false), success_(false),
    lastStatus_(-1), lastParam_(-1),
    timerId_(-1),
    step_(0), stepNew_(false), stepGain_(0), stepBrightness_(0), skipCount_(0), delayCount_(0),
    upperGain_(false), captureReadings_(false)
{
    ui->setupUi(this);
    ui->plainTextEdit->document()->setMaximumBlockCount(100);
    ui->plainTextEdit->setReadOnly(true);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &StickGainCalibrationDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &StickGainCalibrationDialog::reject);
    ui->buttonBox->button(QDialogButtonBox::Close)->setEnabled(false);

    connect(stickInterface_, &DensiStickInterface::sensorReading, this, &StickGainCalibrationDialog::onSensorReading);

    /*
     * This class needs to actually implement gain calibration, rather than just monitor it.
     * Datasheet seems to use 128x as a baseline, so it may make sense to do that here as well.
     *
     * - Turn LED on
     * - For each gain, find the LED setting that doesn't saturate it (and hopefully isn't too high, perhaps <2M counts)
     * - Turn LED off
     * - For each gain, 256x through 1x (downward)
     *   - Turn LED on at selected value
     *   - Measure at gain
     *   - Turn LED off
     *   - Wait
     *   - Turn LED on
     *   - Measure at gain - 1
     *   - Turn LED off
     *   - loop to next gain
     * - Do math on all these pairs to get the gain table
     */
}

StickGainCalibrationDialog::~StickGainCalibrationDialog()
{
    delete ui;
}

bool StickGainCalibrationDialog::success() const
{
    return success_;
}

QMap<int, float> StickGainCalibrationDialog::gainMeasurements() const
{
    return gainMeasurements_;
}

void StickGainCalibrationDialog::accept()
{
    if (running_) { return; }

    if (timerId_ != -1) { killTimer(timerId_); }
    QDialog::accept();
}

void StickGainCalibrationDialog::reject()
{
    if (running_) { return; }

    if (timerId_ != -1) { killTimer(timerId_); timerId_ = -1; }
    QDialog::reject();
}

void StickGainCalibrationDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    if (timerId_ != -1) { killTimer(timerId_); timerId_ = -1; }
    step_ = 1;
    stepNew_ = true;
    started_ = true;
    running_ = true;
    timerId_ = startTimer(10);
}

void StickGainCalibrationDialog::timerEvent(QTimerEvent *event)
{
    if (event->timerId() != timerId_) { return; }
    if (step_ == 1) {
        // Initialize finding gain measurement brightness values
        if (stepNew_) { addText(tr("Initializing...")); stepNew_ = false; }
        stepGain_ = MAX_GAIN;
        stepBrightness_ = 0;
        skipCount_ = LED_SKIP_READINGS;
        stickInterface_->setLightBrightness(stepBrightness_);
        stickInterface_->setLightEnable(true);
        stickInterface_->setSensorAgcDisable();
        stickInterface_->setSensorGain(stepGain_);
        stickInterface_->setSensorIntegration(SAMPLE_TIME, LED_SAMPLE_COUNT);
        stickInterface_->sensorStart();
        captureReadings_ = true;
        step_++; stepNew_ = true;
    } else if (step_ == 2) {
        // Find gain measurement brightness values
        if (stepNew_) {
            addText(tr("Finding measurement brightness for %1")
                        .arg(TSL2585::gainString(static_cast<tsl2585_gain_t>(stepGain_))));
            stepNew_ = false;
        }
        if (readingList_.size() < LED_SAMPLE_READINGS) { return; }
        const DensiStickReading reading = readingList_.last();
        if (reading.status() == DensiStickReading::ResultSaturated || reading.status() == DensiStickReading::ResultOverflow) {
            stepBrightness_ = ((stepBrightness_ + 1) << 1) - 1;
            qDebug() << "Changing brightness to" << stepBrightness_;
            stickInterface_->setLightBrightness(stepBrightness_);
        } else if (reading.reading() > 1000000) {
            stepBrightness_--;
            qDebug() << "Changing brightness to" << stepBrightness_;
            stickInterface_->setLightBrightness(stepBrightness_);
        } else {
            gainBrightness_.insert(stepGain_, stepBrightness_);
            qDebug() << "Gain" << TSL2585::gainString(static_cast<tsl2585_gain_t>(stepGain_))
                     << "brightness set to" << stepBrightness_;
            addText(tr("Selected %1 for measuring %2")
                        .arg(stepBrightness_)
                        .arg(TSL2585::gainString(static_cast<tsl2585_gain_t>(stepGain_))));
            stepGain_--;
            stepNew_ = true;
            if (stepGain_ <= 0) {
                step_ = 3;
            } else {
                stepBrightness_ = 0;
                stickInterface_->setLightBrightness(stepBrightness_);
                stickInterface_->setSensorGain(stepGain_);
                stickInterface_->setSensorIntegration(SAMPLE_TIME, LED_SAMPLE_COUNT);
            }
        }

        skipCount_ = LED_SKIP_READINGS;
        readingList_.clear();
    } else if (step_ == 3) {
        // Initialize measuring gain pairs
        captureReadings_ = false;
        stepGain_ = MAX_GAIN;
        skipCount_ = GAIN_SKIP_READINGS;
        readingList_.clear();
        stickInterface_->setLightEnable(false);
        delayCount_ = GAIN_DELAY_COUNT;
        upperGain_ = true;
        step_++;
        stepNew_ = true;
    } else if (step_ == 4) {
        // Wait a while then start a measurement
        if (delayCount_ > 0) {
            delayCount_--;
            return;
        }

        if (stepNew_) {
            if (upperGain_) {
                addText(tr("Measuring %1 vs %2")
                            .arg(TSL2585::gainString(static_cast<tsl2585_gain_t>(stepGain_)),
                                 TSL2585::gainString(static_cast<tsl2585_gain_t>(stepGain_ - 1)))
                        );
            }
            stepNew_ = false;
        }

        qDebug() << "Starting" << TSL2585::gainString(static_cast<tsl2585_gain_t>(stepGain_)) << (upperGain_ ? "upper" : "lower");

        stepBrightness_ = gainBrightness_.value(stepGain_);

        stickInterface_->setLightBrightness(stepBrightness_);
        stickInterface_->setLightEnable(true);
        stickInterface_->setSensorGain(
            upperGain_ ? stepGain_ : stepGain_ - 1);
        stickInterface_->setSensorIntegration(
            SAMPLE_TIME, GAIN_SAMPLE_COUNT);
        skipCount_ = GAIN_SKIP_READINGS;
        readingList_.clear();
        captureReadings_ = true;
        step_++; stepNew_ = true;
    } else if (step_ == 5) {
        if (readingList_.size() < GAIN_SAMPLE_READINGS) { return; }
        double sum = 0;
        size_t count = 0;
        for (const DensiStickReading& reading : readingList_) {
            if (reading.status() == DensiStickReading::ResultValid) {
                sum += (double)reading.reading();
                count++;
            }
        }
        double value = sum / (double)count;
        qDebug() << "Reading" << TSL2585::gainString(static_cast<tsl2585_gain_t>(stepGain_)) << (upperGain_ ? "upper" : "lower") << value;
        if (upperGain_) {
            gainReadingUpper_.insert(stepGain_, value);
        } else {
            gainReadingLower_.insert(stepGain_, value);
        }

        stickInterface_->setLightEnable(false);
        readingList_.clear();
        captureReadings_ = false;
        delayCount_ = GAIN_DELAY_COUNT;

        if (upperGain_) {
            upperGain_ = false;
        } else {
            stepGain_--;
            upperGain_ = true;
        }
        if (stepGain_ <= 0) {
            step_++;
            stepNew_ = true;
        } else {
            step_ = 4;
            stepNew_ = true;
        }
    } else if (step_ == 6) {
        // Calculate gain values
        for (int i = TSL2585_GAIN_256X; i > 0; --i) {
            const double upper = gainReadingUpper_.value(i);
            const double lower = gainReadingLower_.value(i);

            if (i == TSL2585_GAIN_256X) {
                gainMeasurements_.insert(i - 1, 128.0F);
                gainMeasurements_.insert(i, (float)(128.0 * (upper / lower)));
            } else {
                const double prev = gainMeasurements_.value(i);
                gainMeasurements_.insert(i - 1, (float)(prev * (lower / upper)));
            }
        }

        addText(tr("Finished!"));
        for (int i = 0; i <= MAX_GAIN; i++) {
            addText(tr("Gain %1 => %2")
                        .arg(TSL2585::gainString(static_cast<tsl2585_gain_t>(i)))
                        .arg(gainMeasurements_.value(i)));
        }

        if (timerId_ != -1) {
            killTimer(timerId_);
            timerId_ = -1;
        }

        step_++;
        onCalGainCalFinished();
    }
}

void StickGainCalibrationDialog::onSensorReading(const DensiStickReading& reading)
{
    if (!started_ || !running_ || !captureReadings_) { return; }
    if (skipCount_ > 0) {
        skipCount_--;
    } else {
        readingList_.append(reading);
    }
}

void StickGainCalibrationDialog::onCalGainCalStatus(int status, int param)
{
    if (status == lastStatus_ && param == lastParam_) {
        return;
    }

    switch (status) {
    case 0:
        if (param == 0) {
            addText(tr("Initializing..."));
        }
        break;
    case 1:
        addText(tr("Measuring medium gain... [%1]").arg(gainParamText(param)));
        break;
    case 2:
        addText(tr("Measuring high gain... [%1]").arg(gainParamText(param)));
        break;
    case 3:
        addText(tr("Measuring maximum gain... [%1]").arg(gainParamText(param)));
        break;
    case 5:
        addText(tr("Finding gain measurement brightness... [%1]").arg(lightParamText(param)));
        break;
    case 6:
        if (param == 0) {
            addText(tr("Waiting between measurements..."));
        }
        break;
    }

    lastStatus_ = status;
    lastParam_ = param;
}

QString StickGainCalibrationDialog::gainParamText(int param)
{
    if (param == 0) {
        return tr("lower");
    } else if (param == 1) {
        return tr("higher");
    } else {
        return QString::number(param);
    }
}

QString StickGainCalibrationDialog::lightParamText(int param)
{
    if (param == 0) {
        return tr("init");
    } else {
        return QString::number(param);
    }
}

void StickGainCalibrationDialog::onCalGainCalFinished()
{
    addText(tr("Gain calibration complete!"));
    stickInterface_->setLightEnable(false);
    stickInterface_->sensorStop();
    running_ = false;
    success_ = true;
    ui->buttonBox->button(QDialogButtonBox::Close)->setEnabled(true);
}

void StickGainCalibrationDialog::onCalGainCalError()
{
    addText(tr("Gain calibration failed!"));
    stickInterface_->setLightEnable(false);
    stickInterface_->sensorStop();
    running_ = false;
    success_ = false;
    ui->buttonBox->button(QDialogButtonBox::Close)->setEnabled(true);
}

void StickGainCalibrationDialog::addText(const QString &text)
{
    ui->plainTextEdit->insertPlainText(text);
    ui->plainTextEdit->insertPlainText("\n");
    QScrollBar *bar = ui->plainTextEdit->verticalScrollBar();
    bar->setValue(bar->maximum());
}
