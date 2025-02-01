#include "densisticksettings.h"

#include <QByteArray>
#include <QDebug>

#include "tsl2585.h"
#include "m24c08.h"
#include "util.h"

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
#define CAL_TSL2585_SLOPE_B0    64 /* 4B (float) */
#define CAL_TSL2585_SLOPE_B1    68 /* 4B (float) */
#define CAL_TSL2585_SLOPE_B2    72 /* 4B (float) */
#define CAL_TSL2585_SLOPE_CRC   76 /* 4B (uint32_t) */

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

Tsl2585Calibration DensiStickSettings::readCalTsl2585()
{
    uint32_t version;
    uint32_t crc;
    uint32_t calculated_crc;

    // Read the full buffer from the EEPROM
    QByteArray buf = eeprom_->readBuffer(PAGE_CAL, PAGE_CAL_SIZE);

    if (buf.isEmpty()) { return Tsl2585Calibration(); }

    uint8_t *data = reinterpret_cast<uint8_t *>(buf.data());

    // Get the version
    version = util::copy_to_u32(data + CAL_TSL2585_VERSION);

    if (version != LATEST_CAL_TSL2585_VERSION) {
        qWarning() << "Unexpected TSL2585 cal version:" << version << "!=" << LATEST_CAL_TSL2585_VERSION;
        return Tsl2585Calibration();
    }

    Tsl2585Calibration calData;

    // Validate the gain data CRC
    crc = util::copy_to_u32(data + CAL_TSL2585_GAIN_CRC);
    calculated_crc = util::calculateStmCrc32(
        reinterpret_cast<uint32_t *>(data + CAL_TSL2585_GAIN_0_5X),
        (CAL_TSL2585_GAIN_CRC - CAL_TSL2585_GAIN_0_5X) / 4UL);
    if (crc == calculated_crc) {
        // Parse the gain data
        calData.setGainCalibration(TSL2585_GAIN_0_5X, util::copy_to_f32(data + CAL_TSL2585_GAIN_0_5X));
        calData.setGainCalibration(TSL2585_GAIN_1X, util::copy_to_f32(data + CAL_TSL2585_GAIN_1X));
        calData.setGainCalibration(TSL2585_GAIN_2X, util::copy_to_f32(data + CAL_TSL2585_GAIN_2X));
        calData.setGainCalibration(TSL2585_GAIN_4X, util::copy_to_f32(data + CAL_TSL2585_GAIN_4X));
        calData.setGainCalibration(TSL2585_GAIN_8X, util::copy_to_f32(data + CAL_TSL2585_GAIN_8X));
        calData.setGainCalibration(TSL2585_GAIN_16X, util::copy_to_f32(data + CAL_TSL2585_GAIN_16X));
        calData.setGainCalibration(TSL2585_GAIN_32X, util::copy_to_f32(data + CAL_TSL2585_GAIN_32X));
        calData.setGainCalibration(TSL2585_GAIN_64X, util::copy_to_f32(data + CAL_TSL2585_GAIN_64X));
        calData.setGainCalibration(TSL2585_GAIN_128X, util::copy_to_f32(data + CAL_TSL2585_GAIN_128X));
        calData.setGainCalibration(TSL2585_GAIN_256X, util::copy_to_f32(data + CAL_TSL2585_GAIN_256X));
    } else {
        qWarning() << "Invalid TSL2585 gain cal CRC:" << Qt::hex << crc << "!=" << calculated_crc;
    }

    // Validate the slope data CRC
    crc = util::copy_to_u32(data + CAL_TSL2585_SLOPE_CRC);
    calculated_crc = util::calculateStmCrc32(
        reinterpret_cast<uint32_t *>(data + CAL_TSL2585_SLOPE_B0),
        (CAL_TSL2585_SLOPE_CRC - CAL_TSL2585_SLOPE_B0) / 4UL);
    if (crc == calculated_crc) {
        // Parse the slope data
        Tsl2585CalSlope calSlope;
        calSlope.setB0(util::copy_to_f32(data + CAL_TSL2585_SLOPE_B0));
        calSlope.setB1(util::copy_to_f32(data + CAL_TSL2585_SLOPE_B1));
        calSlope.setB2(util::copy_to_f32(data + CAL_TSL2585_SLOPE_B2));
        calData.setSlopeCalibration(calSlope);
    } else {
        qWarning() << "Invalid TSL2585 slope cal CRC:" << Qt::hex << crc << "!=" << calculated_crc;
    }

    // Validate the target data CRC
    crc = util::copy_to_u32(data + CAL_TSL2585_TARGET_CRC);
    calculated_crc = util::calculateStmCrc32(
        reinterpret_cast<uint32_t *>(data + CAL_TSL2585_TARGET_LO_DENSITY),
        (CAL_TSL2585_TARGET_CRC - CAL_TSL2585_TARGET_LO_DENSITY) / 4UL);
    if (crc == calculated_crc) {
        Tsl2585CalTarget calTarget;
        calTarget.setLoDensity(util::copy_to_f32(data + CAL_TSL2585_TARGET_LO_DENSITY));
        calTarget.setLoReading(util::copy_to_f32(data + CAL_TSL2585_TARGET_LO_READING));
        calTarget.setHiDensity(util::copy_to_f32(data + CAL_TSL2585_TARGET_HI_DENSITY));
        calTarget.setHiReading(util::copy_to_f32(data + CAL_TSL2585_TARGET_HI_READING));
        calData.setTargetCalibration(calTarget);
    } else {
        qWarning() << "Invalid TSL2585 target cal CRC:" << Qt::hex << crc << "!=" << calculated_crc;
    }

    return calData;
}

bool DensiStickSettings::writeCalTsl2585(const Tsl2585Calibration &calData)
{
    uint32_t crc;

    QByteArray buf(PAGE_CAL_SIZE, '\0');
    uint8_t *data = reinterpret_cast<uint8_t *>(buf.data());

    // Populate the version header
    util::copy_from_u32(data + CAL_TSL2585_VERSION, LATEST_CAL_TSL2585_VERSION);

    // Populate the gain data
    util::copy_from_f32(data + CAL_TSL2585_GAIN_0_5X, calData.gainCalibration(TSL2585_GAIN_0_5X));
    util::copy_from_f32(data + CAL_TSL2585_GAIN_1X, calData.gainCalibration(TSL2585_GAIN_1X));
    util::copy_from_f32(data + CAL_TSL2585_GAIN_2X, calData.gainCalibration(TSL2585_GAIN_2X));
    util::copy_from_f32(data + CAL_TSL2585_GAIN_4X, calData.gainCalibration(TSL2585_GAIN_4X));
    util::copy_from_f32(data + CAL_TSL2585_GAIN_8X, calData.gainCalibration(TSL2585_GAIN_8X));
    util::copy_from_f32(data + CAL_TSL2585_GAIN_16X, calData.gainCalibration(TSL2585_GAIN_16X));
    util::copy_from_f32(data + CAL_TSL2585_GAIN_32X, calData.gainCalibration(TSL2585_GAIN_32X));
    util::copy_from_f32(data + CAL_TSL2585_GAIN_64X, calData.gainCalibration(TSL2585_GAIN_64X));
    util::copy_from_f32(data + CAL_TSL2585_GAIN_128X, calData.gainCalibration(TSL2585_GAIN_128X));
    util::copy_from_f32(data + CAL_TSL2585_GAIN_256X, calData.gainCalibration(TSL2585_GAIN_256X));

    crc = util::calculateStmCrc32(
        reinterpret_cast<uint32_t *>(data + CAL_TSL2585_GAIN_0_5X),
        (CAL_TSL2585_GAIN_CRC - CAL_TSL2585_GAIN_0_5X) / 4UL);
    util::copy_from_u32(data + CAL_TSL2585_GAIN_CRC, crc);

    // Populate the slope data
    util::copy_from_f32(data + CAL_TSL2585_SLOPE_B0, calData.slopeCalibration().b0());
    util::copy_from_f32(data + CAL_TSL2585_SLOPE_B1, calData.slopeCalibration().b1());
    util::copy_from_f32(data + CAL_TSL2585_SLOPE_B2, calData.slopeCalibration().b2());

    crc = util::calculateStmCrc32(
        reinterpret_cast<uint32_t *>(data + CAL_TSL2585_SLOPE_B0),
        (CAL_TSL2585_SLOPE_CRC - CAL_TSL2585_SLOPE_B0) / 4UL);
    util::copy_from_u32(data + CAL_TSL2585_SLOPE_CRC, crc);

    // Populate the target data
    util::copy_from_f32(data + CAL_TSL2585_TARGET_LO_DENSITY, calData.targetCalibration().loDensity());
    util::copy_from_f32(data + CAL_TSL2585_TARGET_LO_READING, calData.targetCalibration().loReading());
    util::copy_from_f32(data + CAL_TSL2585_TARGET_HI_DENSITY, calData.targetCalibration().hiDensity());
    util::copy_from_f32(data + CAL_TSL2585_TARGET_HI_READING, calData.targetCalibration().hiReading());

    crc = util::calculateStmCrc32(
        reinterpret_cast<uint32_t *>(data + CAL_TSL2585_TARGET_LO_DENSITY),
        (CAL_TSL2585_TARGET_CRC - CAL_TSL2585_TARGET_LO_DENSITY) / 4UL);
    util::copy_from_u32(data + CAL_TSL2585_TARGET_CRC, crc);

    // Write the buffer to the EEPROM
    return eeprom_->writeBuffer(PAGE_CAL, buf);
}
