#include "densisticksettings.h"

#include <QByteArray>
#include <QDebug>

#include "m24c08.h"
#include "../util.h"

/*
 * The first 256B are reserved for FT260 configuration and shall never be
 * modified by this code. If the format is documented, then it can be read
 * for some of the device ID information. However, that can also be read
 * out of the USB device properties.
 */
#define PAGE_RESERVED          0x000UL
#define PAGE_RESERVED_SIZE     (256UL)

/* Meter Probe Memory Header (16B) */
#define PAGE_HEADER            0x100UL
#define PAGE_HEADER_SIZE       (16UL)

#define HEADER_MAGIC             0 /* 3B = {'D', 'P', 'D'} */
#define HEADER_VERSION           3 /* 1B (uint8_t) */
#define HEADER_DEV_TYPE          4 /* 1B (uint8_t) */
#define HEADER_DEV_REV_MAJOR     5 /* 1B (uint8_t) */
#define HEADER_DEV_REV_MINOR     6 /* 1B (uint8_t) */

#define LATEST_CAL_TSL2585_VERSION 1

#define PAGE_CAL               0x110UL
#define PAGE_CAL_SIZE          (112UL)

/* TSL2585 Data Format Header (16B) */
#define CAL_TSL2585_VERSION      0 /* 4B (uint32_t) */
#define CAL_TSL2585_RESERVED0    4 /* 12B (for page alignment) */

/* TSL2585 Gain Calibration (48B) */
#define CAL_TSL2585_GAIN_0_5X   16 /* 4B (float) */
#define CAL_TSL2585_GAIN_1X     20 /* 4B (float) */
#define CAL_TSL2585_GAIN_2X     24 /* 4B (float) */
#define CAL_TSL2585_GAIN_4X     28 /* 4B (float) */
#define CAL_TSL2585_GAIN_8X     32 /* 4B (float) */
#define CAL_TSL2585_GAIN_16X    36 /* 4B (float) */
#define CAL_TSL2585_GAIN_32X    40 /* 4B (float) */
#define CAL_TSL2585_GAIN_64X    44 /* 4B (float) */
#define CAL_TSL2585_GAIN_128X   48 /* 4B (float) */
#define CAL_TSL2585_GAIN_256X   52 /* 4B (float) */
#define CAL_TSL2585_RESERVED1   56 /* 4B (for page alignment) */
#define CAL_TSL2585_GAIN_CRC    60 /* 4B (uint32_t) */

/* TSL2585 Slope Calibration (16B) */
#define CAL_TSL2585_SLOPE_RESERVED 64 /* 16B (unused) */

/* TSL2585 Target Calibration (32B) */
#define CAL_TSL2585_TARGET_LO_DENSITY  80 /* 4B (float) */
#define CAL_TSL2585_TARGET_LO_READING  84 /* 4B (float) */
#define CAL_TSL2585_TARGET_HI_DENSITY  88 /* 4B (float) */
#define CAL_TSL2585_TARGET_HI_READING  92 /* 4B (float) */
#define CAL_TSL2585_RESERVED2          88 /* 12B (for page alignment) */
#define CAL_TSL2585_TARGET_CRC        108 /* 4B (uint32_t) */

DensiStickSettings::DensiStickSettings(M24C08 *eeprom) : eeprom_(eeprom), headerValid_(false)
{
}

bool DensiStickSettings::init()
{
    const QByteArray headerPage = eeprom_->readBuffer(PAGE_HEADER, PAGE_HEADER_SIZE);
    if (headerPage.isEmpty() || headerPage.size() != PAGE_HEADER_SIZE) { return false; }

    // Verify header page prefix: 'D', 'P', 'D', 0x01
    if (static_cast<quint8>(headerPage[0]) == 'D'
        && static_cast<quint8>(headerPage[1]) == 'P'
        && static_cast<quint8>(headerPage[2]) == 'D'
        && static_cast<quint8>(headerPage[3]) == 0x01) {
        headerValid_ = true;
        headerPage_ = headerPage;
    } else {
        qWarning() << "Header prefix is invalid:" << headerPage.left(4).toHex();
        headerValid_ = false;

        // Initialize a clean header
        headerPage_ = QByteArray(PAGE_HEADER_SIZE, 0);
        headerPage_[0] = 'D';
        headerPage_[1] = 'P';
        headerPage_[2] = 'D';
        headerPage_[3] = 0x01;
    }

    return true;
}

bool DensiStickSettings::headerValid() const
{
    return headerValid_;
}

quint8 DensiStickSettings::probeType() const
{
    const uint8_t *data = reinterpret_cast<const uint8_t *>(headerPage_.constData());
    return data[HEADER_DEV_TYPE];
}

void DensiStickSettings::setProbeType(quint8 probeType)
{
    uint8_t *data = reinterpret_cast<uint8_t *>(headerPage_.data());
    data[HEADER_DEV_TYPE] = probeType;
}

quint8 DensiStickSettings::probeRevisionMajor() const
{
    const uint8_t *data = reinterpret_cast<const uint8_t *>(headerPage_.constData());
    return data[HEADER_DEV_REV_MAJOR];
}

quint8 DensiStickSettings::probeRevisionMinor() const
{
    const uint8_t *data = reinterpret_cast<const uint8_t *>(headerPage_.constData());
    return data[HEADER_DEV_REV_MINOR];
}

void DensiStickSettings::setProbeRevision(quint8 major, quint8 minor)
{
    uint8_t *data = reinterpret_cast<uint8_t *>(headerPage_.data());
    data[HEADER_DEV_REV_MAJOR] = major;
    data[HEADER_DEV_REV_MINOR] = minor;
}

bool DensiStickSettings::writeHeaderPage()
{
    qDebug() << "Saving header page:" << headerPage_.toHex();

    bool result = eeprom_->writeBuffer(PAGE_HEADER, headerPage_);
    if (result) {
        headerValid_ = true;
    }
    return result;
}

DensiStickCalibration DensiStickSettings::readCalibration()
{
    uint32_t version;
    uint32_t crc;
    uint32_t calculated_crc;

    // Read the full buffer from the EEPROM
    QByteArray buf = eeprom_->readBuffer(PAGE_CAL, PAGE_CAL_SIZE);

    if (buf.isEmpty()) { return DensiStickCalibration(); }

    uint8_t *data = reinterpret_cast<uint8_t *>(buf.data());

    // Get the version
    version = util::copy_to_u32(data + CAL_TSL2585_VERSION);

    if (version != LATEST_CAL_TSL2585_VERSION) {
        qWarning() << "Unexpected TSL2585 cal version:" << version << "!=" << LATEST_CAL_TSL2585_VERSION;
        return DensiStickCalibration();
    }

    DensiStickCalibration calibrationData;

    // Validate the gain data CRC
    crc = util::copy_to_u32(data + CAL_TSL2585_GAIN_CRC);
    calculated_crc = util::calculateStmCrc32(
        reinterpret_cast<uint32_t *>(data + CAL_TSL2585_GAIN_0_5X),
        (CAL_TSL2585_GAIN_CRC - CAL_TSL2585_GAIN_0_5X) / 4UL);
    if (crc == calculated_crc) {
        // Parse the gain data
        PeripheralCalGain calGain;
        calGain.setGainValue(PeripheralCalGain::Gain0_5X, util::copy_to_f32(data + CAL_TSL2585_GAIN_0_5X));
        calGain.setGainValue(PeripheralCalGain::Gain1X, util::copy_to_f32(data + CAL_TSL2585_GAIN_1X));
        calGain.setGainValue(PeripheralCalGain::Gain2X, util::copy_to_f32(data + CAL_TSL2585_GAIN_2X));
        calGain.setGainValue(PeripheralCalGain::Gain4X, util::copy_to_f32(data + CAL_TSL2585_GAIN_4X));
        calGain.setGainValue(PeripheralCalGain::Gain8X, util::copy_to_f32(data + CAL_TSL2585_GAIN_8X));
        calGain.setGainValue(PeripheralCalGain::Gain16X, util::copy_to_f32(data + CAL_TSL2585_GAIN_16X));
        calGain.setGainValue(PeripheralCalGain::Gain32X, util::copy_to_f32(data + CAL_TSL2585_GAIN_32X));
        calGain.setGainValue(PeripheralCalGain::Gain64X, util::copy_to_f32(data + CAL_TSL2585_GAIN_64X));
        calGain.setGainValue(PeripheralCalGain::Gain128X, util::copy_to_f32(data + CAL_TSL2585_GAIN_128X));
        calGain.setGainValue(PeripheralCalGain::Gain256X, util::copy_to_f32(data + CAL_TSL2585_GAIN_256X));
        calibrationData.setGainCalibration(calGain);
    } else {
        qWarning() << "Invalid TSL2585 gain cal CRC:" << Qt::hex << crc << "!=" << calculated_crc;
    }

    // Validate the target data CRC
    crc = util::copy_to_u32(data + CAL_TSL2585_TARGET_CRC);
    calculated_crc = util::calculateStmCrc32(
        reinterpret_cast<uint32_t *>(data + CAL_TSL2585_TARGET_LO_DENSITY),
        (CAL_TSL2585_TARGET_CRC - CAL_TSL2585_TARGET_LO_DENSITY) / 4UL);
    if (crc == calculated_crc) {
        PeripheralCalDensityTarget calTarget;
        calTarget.setLoDensity(util::copy_to_f32(data + CAL_TSL2585_TARGET_LO_DENSITY));
        calTarget.setLoReading(util::copy_to_f32(data + CAL_TSL2585_TARGET_LO_READING));
        calTarget.setHiDensity(util::copy_to_f32(data + CAL_TSL2585_TARGET_HI_DENSITY));
        calTarget.setHiReading(util::copy_to_f32(data + CAL_TSL2585_TARGET_HI_READING));
        calibrationData.setTargetCalibration(calTarget);
    } else {
        qWarning() << "Invalid TSL2585 target cal CRC:" << Qt::hex << crc << "!=" << calculated_crc;
    }

    return calibrationData;
}

bool DensiStickSettings::writeCalibration(const DensiStickCalibration &calibrationData)
{
    uint32_t crc;

    QByteArray buf(PAGE_CAL_SIZE, '\0');
    uint8_t *data = reinterpret_cast<uint8_t *>(buf.data());

    // Populate the version header
    util::copy_from_u32(data + CAL_TSL2585_VERSION, LATEST_CAL_TSL2585_VERSION);

    // Populate the gain data
    const PeripheralCalGain calGain = calibrationData.gainCalibration();
    util::copy_from_f32(data + CAL_TSL2585_GAIN_0_5X, calGain.gainValue(PeripheralCalGain::Gain0_5X));
    util::copy_from_f32(data + CAL_TSL2585_GAIN_1X, calGain.gainValue(PeripheralCalGain::Gain1X));
    util::copy_from_f32(data + CAL_TSL2585_GAIN_2X, calGain.gainValue(PeripheralCalGain::Gain2X));
    util::copy_from_f32(data + CAL_TSL2585_GAIN_4X, calGain.gainValue(PeripheralCalGain::Gain4X));
    util::copy_from_f32(data + CAL_TSL2585_GAIN_8X, calGain.gainValue(PeripheralCalGain::Gain8X));
    util::copy_from_f32(data + CAL_TSL2585_GAIN_16X, calGain.gainValue(PeripheralCalGain::Gain16X));
    util::copy_from_f32(data + CAL_TSL2585_GAIN_32X, calGain.gainValue(PeripheralCalGain::Gain32X));
    util::copy_from_f32(data + CAL_TSL2585_GAIN_64X, calGain.gainValue(PeripheralCalGain::Gain64X));
    util::copy_from_f32(data + CAL_TSL2585_GAIN_128X, calGain.gainValue(PeripheralCalGain::Gain128X));
    util::copy_from_f32(data + CAL_TSL2585_GAIN_256X, calGain.gainValue(PeripheralCalGain::Gain256X));

    crc = util::calculateStmCrc32(
        reinterpret_cast<uint32_t *>(data + CAL_TSL2585_GAIN_0_5X),
        (CAL_TSL2585_GAIN_CRC - CAL_TSL2585_GAIN_0_5X) / 4UL);
    util::copy_from_u32(data + CAL_TSL2585_GAIN_CRC, crc);

    // Populate the target data
    const PeripheralCalDensityTarget calTarget = calibrationData.targetCalibration();
    util::copy_from_f32(data + CAL_TSL2585_TARGET_LO_DENSITY, calTarget.loDensity());
    util::copy_from_f32(data + CAL_TSL2585_TARGET_LO_READING, calTarget.loReading());
    util::copy_from_f32(data + CAL_TSL2585_TARGET_HI_DENSITY, calTarget.hiDensity());
    util::copy_from_f32(data + CAL_TSL2585_TARGET_HI_READING, calTarget.hiReading());

    crc = util::calculateStmCrc32(
        reinterpret_cast<uint32_t *>(data + CAL_TSL2585_TARGET_LO_DENSITY),
        (CAL_TSL2585_TARGET_CRC - CAL_TSL2585_TARGET_LO_DENSITY) / 4UL);
    util::copy_from_u32(data + CAL_TSL2585_TARGET_CRC, crc);

    // Write the buffer to the EEPROM
    return eeprom_->writeBuffer(PAGE_CAL, buf);
}
