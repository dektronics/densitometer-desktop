#include "tsl2585.h"

#include <QThread>
#include <QDebug>

#include "ft260.h"

// I2C device address
static const uint8_t TSL2585_ADDRESS = 0x39;

/*
 * Photodiode numbering:
 * 0 - IR        {1, x, 2, x}
 * 1 - Photopic  {0, 0, 0, 0}
 * 2 - IR        {1, x, x, 1}
 * 3 - UV-A      {0, 1, x, x}
 * 4 - UV-A      {1, 2, x, 2}
 * 5 - Photopic  {0, x, 1, x}
 *
 * Startup configuration:
 * Sequencer step 0:
 * - Photodiode 0(IR)  -> Modulator 1
 * - Photodiode 1(Pho) -> Modulator 0
 * - Photodiode 2(IR)  -> Modulator 1
 * - Photodiode 3(UVA) -> Modulator 0
 * - Photodiode 4(UVA) -> Modulator 1
 * - Photodiode 5(Pho) -> Modulator 0
 *
 * Sequencer step 1:
 * - Photodiode 0(IR)  -> NC
 * - Photodiode 1(Pho) -> Modulator 0
 * - Photodiode 2(IR)  -> NC
 * - Photodiode 3(UVA) -> Modulator 1
 * - Photodiode 4(UVA) -> Modulator 2
 * - Photodiode 5(Pho) -> NC
 *
 * Sequencer step 2:
 * - Photodiode 0(IR)  -> Modulator 2
 * - Photodiode 1(Pho) -> Modulator 0
 * - Photodiode 2(IR)  -> NC
 * - Photodiode 3(UVA) -> NC
 * - Photodiode 4(UVA) -> NC
 * - Photodiode 5(Pho) -> Modulator 1
 *
 * Sequencer step 3:
 * - Photodiode 0(IR)  -> NC
 * - Photodiode 1(Pho) -> Modulator 0
 * - Photodiode 2(IR)  -> Modulator 1
 * - Photodiode 3(UVA) -> NC
 * - Photodiode 4(UVA) -> Modulator 2
 * - Photodiode 5(Pho) -> NC
 *
 * Default gain: 128x
 * Sample time: 179 (250μs)
 * ALSIntegrationTimeStep = (SAMPLE_TIME+1) * 1.388889μs
 */

/* Registers */
#define TSL2585_UV_CALIB         0x08 /*!< UV calibration factor */
#define TSL2585_MOD_CHANNEL_CTRL 0x40 /*!< Modulator channel control */
#define TSL2585_ENABLE           0x80 /*!< Enables device states */
#define TSL2585_MEAS_MODE0       0x81 /*!< Measurement mode settings 0 */
#define TSL2585_MEAS_MODE1       0x82 /*!< Measurement mode settings 1 */
#define TSL2585_SAMPLE_TIME0     0x83 /*!< Flicker sample time settings 0 [7:0] */
#define TSL2585_SAMPLE_TIME1     0x84 /*!< Flicker sample time settings 1 [10:8] */
#define TSL2585_ALS_NR_SAMPLES0  0x85 /*!< ALS measurement time settings 0 [7:0] */
#define TSL2585_ALS_NR_SAMPLES1  0x86 /*!< ALS measurement time settings 1 [10:8] */
#define TSL2585_FD_NR_SAMPLES0   0x87 /*!< Flicker number of samples 0 [7:0] */
#define TSL2585_FD_NR_SAMPLES1   0x88 /*!< Flicker number of samples 1 [10:8] */
#define TSL2585_WTIME            0x89 /*!< Wait time */
#define TSL2585_AILT0            0x8A /*!< ALS Interrupt Low Threshold [7:0] */
#define TSL2585_AILT1            0x8B /*!< ALS Interrupt Low Threshold [15:8] */
#define TSL2585_AILT2            0x8C /*!< ALS Interrupt Low Threshold [23:16] */
#define TSL2585_AIHT0            0x8D /*!< ALS Interrupt High Threshold [7:0] */
#define TSL2585_AIHT1            0x8E /*!< ALS Interrupt High Threshold [15:8] */
#define TSL2585_AIHT2            0x8F /*!< ALS interrupt High Threshold [23:16] */
#define TSL2585_AUX_ID           0x90 /*!< Auxiliary identification */
#define TSL2585_REV_ID           0x91 /*!< Revision identification */
#define TSL2585_ID               0x92 /*!< Device identification */
#define TSL2585_STATUS           0x93 /*!< Device status information 1 */
#define TSL2585_ALS_STATUS       0x94 /*!< ALS Status information 1 */
#define TSL2585_ALS_DATA0_L      0x95 /*!< ALS data channel 0 low byte [7:0] */
#define TSL2585_ALS_DATA0_H      0x96 /*!< ALS data channel 0 high byte [15:8] */
#define TSL2585_ALS_DATA1_L      0x97 /*!< ALS data channel 1 low byte [7:0] */
#define TSL2585_ALS_DATA1_H      0x98 /*!< ALS data channel 1 high byte [15:8] */
#define TSL2585_ALS_DATA2_L      0x99 /*!< ALS data channel 2 low byte [7:0] */
#define TSL2585_ALS_DATA2_H      0x9A /*!< ALS data channel 2 high byte [15:8] */
#define TSL2585_ALS_STATUS2      0x9B /*!< ALS Status information 2 */
#define TSL2585_ALS_STATUS3      0x9C /*!< ALS Status information 3 */
#define TSL2585_STATUS2          0x9D /*!< Device Status information 2 */
#define TSL2585_STATUS3          0x9E /*!< Device Status information 3 */
#define TSL2585_STATUS4          0x9F /*!< Device Status information 4 */
#define TSL2585_STATUS5          0xA0 /*!< Device Status information 5 */
#define TSL2585_CFG0             0xA1 /*!< Configuration 0 */
#define TSL2585_CFG1             0xA2 /*!< Configuration 1 */
#define TSL2585_CFG2             0xA3 /*!< Configuration 2 */
#define TSL2585_CFG3             0xA4 /*!< Configuration 3 */
#define TSL2585_CFG4             0xA5 /*!< Configuration 4 */
#define TSL2585_CFG5             0xA6 /*!< Configuration 5 */
#define TSL2585_CFG6             0xA7 /*!< Configuration 6 */
#define TSL2585_CFG7             0xA8 /*!< Configuration 7 */
#define TSL2585_CFG8             0xA9 /*!< Configuration 8 */
#define TSL2585_CFG9             0xAA /*!< Configuration 9 */
#define TSL2585_AGC_NR_SAMPLES_L 0xAC /*!< Number of samples for measurement with AGC low [7:0] */
#define TSL2585_AGC_NR_SAMPLES_H 0xAD /*!< Number of samples for measurement with AGC high [10:8] */
#define TSL2585_TRIGGER_MODE     0xAE /*!< Wait Time Mode */
#define TSL2585_CONTROL          0xB1 /*!< Device control settings */
#define TSL2585_INTENAB          0xBA /*!< Enable interrupts */
#define TSL2585_SIEN             0xBB /*!< Enable saturation interrupts */
#define TSL2585_MOD_COMP_CFG1    0xCE /*!< Adjust AutoZero range */
#define TSL2585_MEAS_SEQR_FD_0                  0xCF /*!< Flicker measurement with sequencer on modulator0 */
#define TSL2585_MEAS_SEQR_ALS_FD_1              0xD0 /*!< ALS measurement with sequencer on all modulators */
#define TSL2585_MEAS_SEQR_APERS_AND_VSYNC_WAIT  0xD1 /*!< Defines the measurement sequencer pattern */
#define TSL2585_MEAS_SEQR_RESIDUAL_0            0xD2 /*!< Residual measurement configuration with sequencer on modulator0 and modulator1 */
#define TSL2585_MEAS_SEQR_RESIDUAL_1_AND_WAIT   0xD3 /*!< Residual measurement configuration with sequencer on modulator2 and wait time configuration for all sequencers */
#define TSL2585_MEAS_SEQR_STEP0_MOD_GAINX_L     0xD4 /*!< Gain of modulator0 and modulator1 for sequencer step 0 */
#define TSL2585_MEAS_SEQR_STEP0_MOD_GAINX_H     0xD5 /*!< Gain of modulator2 for sequencer step 0 */
#define TSL2585_MEAS_SEQR_STEP1_MOD_GAINX_L     0xD6 /*!< Gain of modulator0 and modulator1 for sequencer step 1 */
#define TSL2585_MEAS_SEQR_STEP1_MOD_GAINX_H     0xD7 /*!< Gain of modulator2 for sequencer step 1 */
#define TSL2585_MEAS_SEQR_STEP2_MOD_GAINX_L     0xD8 /*!< Gain of modulator0 and modulator1 for sequencer step 2 */
#define TSL2585_MEAS_SEQR_STEP2_MOD_GAINX_H     0xD9 /*!< Gain of modulator2 for sequencer step 2 */
#define TSL2585_MEAS_SEQR_STEP3_MOD_GAINX_L     0xDA /*!< Gain of modulator0 and modulator1 for sequencer step 3 */
#define TSL2585_MEAS_SEQR_STEP3_MOD_GAINX_H     0xDB /*!< Gain of modulator2 for sequencer step 3 */
#define TSL2585_MEAS_SEQR_STEP0_MOD_PHDX_SMUX_L 0xDC /*!< Photodiode 0-3 to modulator mapping through multiplexer for sequencer step 0 */
#define TSL2585_MEAS_SEQR_STEP0_MOD_PHDX_SMUX_H 0xDD /*!< Photodiode 4-5 to modulator mapping through multiplexer for sequencer step 0 */
#define TSL2585_MEAS_SEQR_STEP1_MOD_PHDX_SMUX_L 0xDE /*!< Photodiode 0-3 to modulator mapping through multiplexer for sequencer step 1 */
#define TSL2585_MEAS_SEQR_STEP1_MOD_PHDX_SMUX_H 0xDF /*!< Photodiode 4-5 to modulator mapping through multiplexer for sequencer step 1 */
#define TSL2585_MEAS_SEQR_STEP2_MOD_PHDX_SMUX_L 0xE0 /*!< Photodiode 0-3 to modulator mapping through multiplexer for sequencer step 2 */
#define TSL2585_MEAS_SEQR_STEP2_MOD_PHDX_SMUX_H 0xE1 /*!< Photodiode 4-5 to modulator mapping through multiplexer for sequencer step 2 */
#define TSL2585_MEAS_SEQR_STEP3_MOD_PHDX_SMUX_L 0xE2 /*!< Photodiode 0-3 to modulator mapping through multiplexer for sequencer step 3 */
#define TSL2585_MEAS_SEQR_STEP3_MOD_PHDX_SMUX_H 0xE3 /*!< Photodiode 4-5 to modulator mapping through multiplexer for sequencer step 3 */
#define TSL2585_MOD_CALIB_CFG0   0xE4 /*!< Modulator calibration config0 */
#define TSL2585_MOD_CALIB_CFG2   0xE6 /*!< Modulator calibration config2 */
#define TSL2585_MOD_GAIN_H       0xED /*!< Modulator gain table selection (undocumented, from app note) */
#define TSL2585_VSYNC_PERIOD_L        0xF2 /*!< Measured VSYNC period */
#define TSL2585_VSYNC_PERIOD_H        0xF3 /*!< Read and clear measured VSYNC period */
#define TSL2585_VSYNC_PERIOD_TARGET_L 0xF4 /*!< Targeted VSYNC period */
#define TSL2585_VSYNC_PERIOD_TARGET_H 0xF5 /*!< Alternative target VSYNC period */
#define TSL2585_VSYNC_CONTROL         0xF6 /*!< Control of VSYNC period */
#define TSL2585_VSYNC_CFG             0xF7 /*!< Configuration of VSYNC input */
#define TSL2585_VSYNC_GPIO_INT        0xF8 /*!< Configuration of GPIO pin */
#define TSL2585_MOD_FIFO_DATA_CFG0 0xF9 /*!< Configuration of FIFO access for modulator 0 */
#define TSL2585_MOD_FIFO_DATA_CFG1 0xFA /*!< Configuration of FIFO access for modulator 1 */
#define TSL2585_MOD_FIFO_DATA_CFG2 0xFB /*!< Configuration of FIFO access for modulator 2 */
#define TSL2585_FIFO_THR         0xFC /*!< Configuration of FIFO threshold interrupt */
#define TSL2585_FIFO_STATUS0     0xFD /*!< FIFO status information 0 */
#define TSL2585_FIFO_STATUS1     0xFE /*!< FIFO status information 1 */
#define TSL2585_FIFO_DATA        0xFF /*!< FIFO readout */

/* CFG0 register values */
#define TSL2585_CFG0_SAI                 0x40
#define TSL2585_CFG0_LOWPOWER_IDLE       0x20

/* CONTROL register values */
#define TSL2585_CONTROL_SOFT_RESET       0x08
#define TSL2585_CONTROL_FIFO_CLR         0x02
#define TSL2585_CONTROL_CLEAR_SAI_ACTIVE 0x01

/* MOD_CALIB_CFG2 register values */
#define TSL2585_MOD_CALIB_NTH_ITERATION_RC_ENABLE 0x80
#define TSL2585_MOD_CALIB_NTH_ITERATION_AZ_ENABLE 0x40
#define TSL2585_MOD_CALIB_NTH_ITERATION_AGC_ENABLE 0x20
#define TSL2585_MOD_CALIB_RESIDUAL_ENABLE_AUTO_CALIB_ON_GAIN_CHANGE 0x10

/* MEAS_MODE0 register values */
#define TSL2585_MEAS_MODE0_STOP_AFTER_NTH_ITERATION 0x80
#define TSL2585_MEAS_MODE0_ENABLE_AGC_ASAT_DOUBLE_STEP_DOWN 0x40
#define TSL2585_MEAS_MODE0_MEASUREMENT_SEQUENCER_SINGLE_SHOT_MODE 0x20
#define TSL2585_MEAS_MODE0_MOD_FIFO_ALS_STATUS_WRITE_ENABLE 0x10

TSL2585::TSL2585(Ft260 *ft260) : ft260_(ft260)
{
}

bool TSL2585::init(tsl2585_ident_t *ident)
{
    quint8 data;
    quint8 devId = 0;
    quint8 revId = 0;
    quint8 auxId = 0;


    qDebug() << "Initializing TSL2585";

    if (!ft260_->i2cReadByte(TSL2585_ADDRESS, TSL2585_ID, &devId)) { return false; }

    qDebug() << "Device ID:" << Qt::hex << devId;

    if (devId != 0x5C) {
        qWarning() << "Invalid Device ID";
        return false;
    }

    if (!ft260_->i2cReadByte(TSL2585_ADDRESS, TSL2585_REV_ID, &revId)) { return false; }

    qDebug() << "Revision ID:" << Qt::hex << revId;

    if (!ft260_->i2cReadByte(TSL2585_ADDRESS, TSL2585_AUX_ID, &data)) { return false; }
    auxId = (data & 0x0F);

    qDebug() << "Aux ID:" << Qt::hex << auxId;

    if (ident) {
        ident->devId = devId;
        ident->revId = revId;
        ident->auxId = auxId;
    }

    if (!ft260_->i2cReadByte(TSL2585_ADDRESS, TSL2585_STATUS, &data)) { return false; }

    qDebug() << "Status:" << Qt::hex << data;

    // Power on the sensor
    if (!enable()) {
        qWarning() << "Sensor power on error";
        return false;
    }

    // Perform a soft reset to clear any leftover state
    if (!softReset()) {
        qWarning() << "Sensor soft reset error";
        return false;
    }

    // Short delay to allow the soft reset to complete
    QThread::usleep(1000);

    // Power off the sensor
    if (!disable()) {
        qWarning() << "Sensor power off error";
        return false;
    }

    qDebug() << "TSL2585 Initialized";

    return true;
}

bool TSL2585::setEnable(uint8_t value)
{
    uint8_t data = value & 0x43; /* Mask bits 6,1:0 */
    return ft260_->i2cWriteByte(TSL2585_ADDRESS, TSL2585_ENABLE, data);
}

bool TSL2585::enable()
{
    return setEnable(TSL2585_ENABLE_PON | TSL2585_ENABLE_AEN);
}

bool TSL2585::disable()
{
    return setEnable(0x00);
}

bool TSL2585::setInterruptEnable(uint8_t value)
{
    uint8_t data = value & 0x8D; /* Mask bits 7,3,2,0 */
    return ft260_->i2cWriteByte(TSL2585_ADDRESS, TSL2585_INTENAB, data);
}

bool TSL2585::softReset()
{
    uint8_t data = TSL2585_CONTROL_SOFT_RESET;

    return ft260_->i2cWriteByte(TSL2585_ADDRESS, TSL2585_CONTROL, data);
}

bool TSL2585::clearFifo()
{
    uint8_t data = TSL2585_CONTROL_FIFO_CLR;

    return ft260_->i2cWriteByte(TSL2585_ADDRESS, TSL2585_CONTROL, data);
}

bool TSL2585::enableModulators(tsl2585_modulator_t mods)
{
    /* Mask bits [2:0] and invert since asserting disables the modulators */
    uint8_t data = ~((uint8_t)mods) & 0x03;

    return ft260_->i2cWriteByte(TSL2585_ADDRESS, TSL2585_MOD_CHANNEL_CTRL, data);
}

bool TSL2585::getStatus(uint8_t *status)
{
    uint8_t data;
    if (!ft260_->i2cReadByte(TSL2585_ADDRESS, TSL2585_STATUS, &data)) { return false; }

    if (status) {
        *status = data;
    }
    return true;
}

bool TSL2585::setStatus(uint8_t status)
{
    return ft260_->i2cWriteByte(TSL2585_ADDRESS, TSL2585_STATUS, status);
}

bool TSL2585::getStatus2(uint8_t *status)
{
    uint8_t data;
    if (!ft260_->i2cReadByte(TSL2585_ADDRESS, TSL2585_STATUS2, &data)) { return false; }

    if (status) {
        *status = data;
    }
    return true;
}

bool TSL2585::getStatus3(uint8_t *status)
{
    uint8_t data;
    if (!ft260_->i2cReadByte(TSL2585_ADDRESS, TSL2585_STATUS3, &data)) { return false; }

    if (status) {
        *status = data;
    }
    return true;
}

bool TSL2585::getStatus4(uint8_t *status)
{
    uint8_t data;
    if (!ft260_->i2cReadByte(TSL2585_ADDRESS, TSL2585_STATUS4, &data)) { return false; }

    if (status) {
        *status = data;
    }
    return true;
}

bool TSL2585::getStatus5(uint8_t *status)
{
    uint8_t data;
    if (!ft260_->i2cReadByte(TSL2585_ADDRESS, TSL2585_STATUS5, &data)) { return false; }

    if (status) {
        *status = data;
    }
    return true;
}

bool TSL2585::setModGainTableSelect(bool alternate)
{
    /*
     * Selects the alternate gain table, which determines the number of
     * residual bits, as documented in AN001059.
     * This register is completely undocumented in the actual datasheet.
     */
    uint8_t data;
    if (!ft260_->i2cReadByte(TSL2585_ADDRESS, TSL2585_MOD_GAIN_H, &data)) { return false; }

    data = (data & 0xCF) | (alternate ? 0x30 : 0x00);

    return ft260_->i2cWriteByte(TSL2585_ADDRESS, TSL2585_MOD_GAIN_H, data);
}

bool TSL2585::setMaxModGain(tsl2585_gain_t gain)
{
    uint8_t data;

    if (!ft260_->i2cReadByte(TSL2585_ADDRESS, TSL2585_CFG8, &data)) { return false; }

    data = (data & 0x0F) | (((uint8_t)gain & 0x0F) << 4);

    return ft260_->i2cWriteByte(TSL2585_ADDRESS, TSL2585_CFG8, data);
}

bool TSL2585::gainRegister(tsl2585_modulator_t mod, tsl2585_step_t step, uint8_t *reg, bool *upper)
{
    *reg = 0;
    if (mod == TSL2585_MOD0) {
        if (step == TSL2585_STEP0) {
            *reg = TSL2585_MEAS_SEQR_STEP0_MOD_GAINX_L;
            *upper = false;
        } else if (step == TSL2585_STEP1) {
            *reg = TSL2585_MEAS_SEQR_STEP1_MOD_GAINX_L;
            *upper = false;
        } else if (step == TSL2585_STEP2) {
            *reg = TSL2585_MEAS_SEQR_STEP2_MOD_GAINX_L;
            *upper = false;
        }
    } else if (mod == TSL2585_MOD1) {
        if (step == TSL2585_STEP0) {
            *reg = TSL2585_MEAS_SEQR_STEP0_MOD_GAINX_L;
            *upper = true;
        } else if (step == TSL2585_STEP1) {
            *reg = TSL2585_MEAS_SEQR_STEP1_MOD_GAINX_L;
            *upper = true;
        } else if (step == TSL2585_STEP2) {
            *reg = TSL2585_MEAS_SEQR_STEP2_MOD_GAINX_L;
            *upper = true;
        }
    } else if (mod == TSL2585_MOD2) {
        if (step == TSL2585_STEP0) {
            *reg = TSL2585_MEAS_SEQR_STEP0_MOD_GAINX_H;
            *upper = false;
        } else if (step == TSL2585_STEP1) {
            *reg = TSL2585_MEAS_SEQR_STEP1_MOD_GAINX_H;
            *upper = false;
        } else if (step == TSL2585_STEP2) {
            *reg = TSL2585_MEAS_SEQR_STEP2_MOD_GAINX_H;
            *upper = false;
        }
    }
    return *reg != 0;
}

bool TSL2585::setModGain(tsl2585_modulator_t mod, tsl2585_step_t step, tsl2585_gain_t gain)
{
    uint8_t data;
    uint8_t reg;
    bool upper;

    if (!gainRegister(mod, step, &reg, &upper)) {
        return false;
    }

    if (!ft260_->i2cReadByte(TSL2585_ADDRESS, reg, &data)) { return false; }

    if (upper) {
        data = (data & 0x0F) | ((uint8_t)gain << 4);
    } else {
        data = (data & 0xF0) | (uint8_t)gain;
    }

    return ft260_->i2cWriteByte(TSL2585_ADDRESS, reg, data);
}

bool TSL2585::setModResidualEnable(tsl2585_modulator_t mod, tsl2585_step_t steps)
{
    uint8_t reg;
    bool upper;
    uint8_t data;

    switch (mod) {
    case TSL2585_MOD0:
        reg = TSL2585_MEAS_SEQR_RESIDUAL_0;
        upper = false;
        break;
    case TSL2585_MOD1:
        reg = TSL2585_MEAS_SEQR_RESIDUAL_0;
        upper = true;
        break;
    case TSL2585_MOD2:
        reg = TSL2585_MEAS_SEQR_RESIDUAL_1_AND_WAIT;
        upper = false;
        break;
    default:
        return false;
    }

    if (!ft260_->i2cReadByte(TSL2585_ADDRESS, reg, &data)) { return false; }

    if (upper) {
        data = (data & 0x0F) | (((uint8_t)steps) & 0x0F) << 4;
    } else {
        data = (data & 0xF0) | (((uint8_t)steps) & 0x0F);
    }

    return ft260_->i2cWriteByte(TSL2585_ADDRESS, reg, data);
}

bool TSL2585::setModPhotodiodeSmux(tsl2585_step_t step, const tsl2585_modulator_t phd_mod[TSL2585_PHD_MAX])
{
    QByteArray buf;
    uint8_t reg;
    uint8_t data[2];
    uint8_t phd_mod_vals[TSL2585_PHD_MAX];

    /* Select the appropriate step register */
    switch (step) {
    case TSL2585_STEP0:
        reg = TSL2585_MEAS_SEQR_STEP0_MOD_PHDX_SMUX_L;
        break;
    case TSL2585_STEP1:
        reg = TSL2585_MEAS_SEQR_STEP1_MOD_PHDX_SMUX_L;
        break;
    case TSL2585_STEP2:
        reg = TSL2585_MEAS_SEQR_STEP2_MOD_PHDX_SMUX_L;
        break;
    case TSL2585_STEP3:
        reg = TSL2585_MEAS_SEQR_STEP3_MOD_PHDX_SMUX_L;
        break;
    default:
        return false;
    }

    /* Convert the input array to the right series of 2-bit values */
    for (uint8_t i = 0; i < TSL2585_PHD_MAX; i++) {
        switch(phd_mod[i]) {
        case TSL2585_MOD0:
            phd_mod_vals[i] = 0x01;
            break;
        case TSL2585_MOD1:
            phd_mod_vals[i] = 0x02;
            break;
        case TSL2585_MOD2:
            phd_mod_vals[i] = 0x03;
            break;
        default:
            phd_mod_vals[i] = 0;
            break;
        }
    }

    /* Read the current value */
    buf = ft260_->i2cRead(TSL2585_ADDRESS, reg, 2);
    if (buf.isEmpty()) { return false; }
    data[0] = static_cast<quint8>(buf[0]);
    data[1] = static_cast<quint8>(buf[1]);

    /* Clear everything but the unrelated or reserved bits */
    data[0] = 0;
    data[1] &= 0xF0;


    /* Apply the selected assignments */
    data[0] |= phd_mod_vals[TSL2585_PHD_3] << 6;
    data[0] |= phd_mod_vals[TSL2585_PHD_2] << 4;
    data[0] |= phd_mod_vals[TSL2585_PHD_1] << 2;
    data[0] |= phd_mod_vals[TSL2585_PHD_0];
    data[1] |= phd_mod_vals[TSL2585_PHD_5] << 2;
    data[1] |= phd_mod_vals[TSL2585_PHD_4];

    buf.clear();
    buf.append(static_cast<uint8_t>(data[0]));
    buf.append(static_cast<uint8_t>(data[1]));
    return ft260_->i2cWrite(TSL2585_ADDRESS, reg, buf);
}

bool TSL2585::setCalibrationNthIteration(uint8_t iteration)
{
    return ft260_->i2cWriteByte(TSL2585_ADDRESS, TSL2585_MOD_CALIB_CFG0, iteration);
}

bool TSL2585::setSampleTime(uint16_t value)
{
    QByteArray buf;

    if (value > 0x7FF) {
        return false;
    }

    buf.append(static_cast<uint8_t>(value & 0x0FF));
    buf.append(static_cast<uint8_t>((value & 0x700) >> 8));

    return ft260_->i2cWrite(TSL2585_ADDRESS, TSL2585_SAMPLE_TIME0, buf);
}

bool TSL2585::setAlsNumSamples(uint16_t value)
{
    QByteArray buf;

    if (value > 0x7FF) {
        return false;
    }

    buf.append(static_cast<uint8_t>(value & 0x0FF));
    buf.append(static_cast<uint8_t>((value & 0x700) >> 8));

    return ft260_->i2cWrite(TSL2585_ADDRESS, TSL2585_ALS_NR_SAMPLES0, buf);
}

bool TSL2585::setAlsInterruptPersistence(uint8_t value)
{
    uint8_t data;

    if (!ft260_->i2cReadByte(TSL2585_ADDRESS, TSL2585_CFG5, &data)) { return false; }

    data = (data & 0xF0) | (value & 0x0F);

    return ft260_->i2cWriteByte(TSL2585_ADDRESS, TSL2585_CFG5, data);
}

bool TSL2585::getAlsStatus(uint8_t *status)
{
    return ft260_->i2cReadByte(TSL2585_ADDRESS, TSL2585_ALS_STATUS, status);
}

bool TSL2585::getAlsScale(uint8_t *scale)
{
    uint8_t data;
    if (!ft260_->i2cReadByte(TSL2585_ADDRESS, TSL2585_MEAS_MODE0, &data)) { return false; }

    if (scale) {
        *scale = data & 0x0F;
    }
    return true;
}

bool TSL2585::setAgcNumSamples(uint16_t value)
{
    QByteArray buf;

    if (value > 0x7FF) {
        return false;
    }

    buf.append(static_cast<uint8_t>(value & 0x0FF));
    buf.append(static_cast<uint8_t>((value & 0x700) >> 8));

    return ft260_->i2cWrite(TSL2585_ADDRESS, TSL2585_AGC_NR_SAMPLES_L, buf);
}

bool TSL2585::setAgcCalibration(bool enabled)
{
    uint8_t data;

    if (!ft260_->i2cReadByte(TSL2585_ADDRESS, TSL2585_MOD_CALIB_CFG2, &data)) { return false; }

    data = (data & ~TSL2585_MOD_CALIB_NTH_ITERATION_AGC_ENABLE) | (enabled ? TSL2585_MOD_CALIB_NTH_ITERATION_AGC_ENABLE : 0);

    return ft260_->i2cWriteByte(TSL2585_ADDRESS, TSL2585_MOD_CALIB_CFG2, data);
}

bool TSL2585::setFifoAlsStatusWriteEnable(bool enable)
{
    uint8_t data;

    if (!ft260_->i2cReadByte(TSL2585_ADDRESS, TSL2585_MEAS_MODE0, &data)) { return false; }

    data = (data & 0xEF) | (enable ? 0x10 : 0x00);

    return ft260_->i2cWriteByte(TSL2585_ADDRESS, TSL2585_MEAS_MODE0, data);
}

bool TSL2585::setFifoAlsDataFormat(tsl2585_als_fifo_data_format_t format)
{
    uint8_t data;

    if (!ft260_->i2cReadByte(TSL2585_ADDRESS, TSL2585_CFG4, &data)) { return false; }

    data = (data & 0x03) | format;

    return ft260_->i2cWriteByte(TSL2585_ADDRESS, TSL2585_CFG4, data);
}

bool TSL2585::getAlsMsbPosition(uint8_t *position)
{
    uint8_t data;
    if (!ft260_->i2cReadByte(TSL2585_ADDRESS, TSL2585_MEAS_MODE1, &data)) { return false; }

    if (position) {
        *position = data & 0x1F;
    }
    return true;
}

bool TSL2585::setAlsMsbPosition(uint8_t position)
{
    uint8_t data;

    if (!ft260_->i2cReadByte(TSL2585_ADDRESS, TSL2585_MEAS_MODE1, &data)) { return false; }

    data = (data & 0xE0) | (position & 0x1F);

    return ft260_->i2cWriteByte(TSL2585_ADDRESS, TSL2585_MEAS_MODE1, data);
}

bool TSL2585::getTriggerMode(tsl2585_trigger_mode_t *mode)
{
    uint8_t data;
    if (!ft260_->i2cReadByte(TSL2585_ADDRESS, TSL2585_TRIGGER_MODE, &data)) { return false; }

    if (mode) {
        *mode = static_cast<tsl2585_trigger_mode_t>(data & 0x07);
    }
    return true;
}

bool TSL2585::setTriggerMode(tsl2585_trigger_mode_t mode)
{
    return ft260_->i2cWriteByte(TSL2585_ADDRESS, TSL2585_TRIGGER_MODE, static_cast<uint8_t>(mode));
}

bool TSL2585::getAlsData0(uint16_t *data)
{
    QByteArray buf;
    buf = ft260_->i2cRead(TSL2585_ADDRESS, TSL2585_ALS_DATA0_L, 2);
    if (buf.isEmpty()) { return false; }

    if (data) {
        *data = static_cast<uint8_t>(buf[0]) | static_cast<uint8_t>(buf[1]) << 8;
    }
    return true;
}

bool TSL2585::getAlsData1(uint16_t *data)
{
    QByteArray buf;
    buf = ft260_->i2cRead(TSL2585_ADDRESS, TSL2585_ALS_DATA1_L, 2);
    if (buf.isEmpty()) { return false; }

    if (data) {
        *data = static_cast<uint8_t>(buf[0]) | static_cast<uint8_t>(buf[1]) << 8;
    }
    return true;
}

bool TSL2585::getAlsData2(uint16_t *data)
{
    QByteArray buf;
    buf = ft260_->i2cRead(TSL2585_ADDRESS, TSL2585_ALS_DATA2_L, 2);
    if (buf.isEmpty()) { return false; }

    if (data) {
        *data = static_cast<uint8_t>(buf[0]) | static_cast<uint8_t>(buf[1]) << 8;
    }
    return true;
}

bool TSL2585::setFifoDataWriteEnable(tsl2585_modulator_t mod, bool enable)
{
    uint8_t data;
    uint8_t reg;

    switch (mod) {
    case TSL2585_MOD0:
        reg = TSL2585_MOD_FIFO_DATA_CFG0;
        break;
    case TSL2585_MOD1:
        reg = TSL2585_MOD_FIFO_DATA_CFG1;
        break;
    case TSL2585_MOD2:
        reg = TSL2585_MOD_FIFO_DATA_CFG2;
        break;
    default:
        return false;
    }

    if (!ft260_->i2cReadByte(TSL2585_ADDRESS, reg, &data)) { return false; }

    data = (data & 0x7F) | (enable ? 0x80 : 0x00);

    return ft260_->i2cWriteByte(TSL2585_ADDRESS, reg, data);
}

bool TSL2585::setFifoThreshold(uint16_t threshold)
{
    uint8_t data0;
    uint8_t data1;

    if (threshold > 0x01FF) { return false; }

    if (!ft260_->i2cReadByte(TSL2585_ADDRESS, TSL2585_CFG2, &data0)) { return false; }

    data0 = (data0 & 0xFE) | (threshold & 0x0001);
    data1 = threshold >> 1;

    /* CFG2 contains FIFO_THR[0] */
    if (!ft260_->i2cWriteByte(TSL2585_ADDRESS, TSL2585_CFG2, data0)) { return false; }

    /* FIFO_THR contains FIFO_THR[8:1] */
    if (!ft260_->i2cWriteByte(TSL2585_ADDRESS, TSL2585_FIFO_THR, data1)) { return false; }

    return true;
}

bool TSL2585::getFifoStatus(tsl2585_fifo_status_t *status)
{
    QByteArray buf;
    buf = ft260_->i2cRead(TSL2585_ADDRESS, TSL2585_FIFO_STATUS0, 2);
    if (buf.isEmpty()) { return false; }

    if (status) {
        status->overflow = (static_cast<uint8_t>(buf[1]) & 0x80) ==  0x80;
        status->underflow = (static_cast<uint8_t>(buf[1]) & 0x40) == 0x40;
        status->level = (static_cast<uint16_t>(buf[0]) << 2) | (static_cast<uint16_t>(buf[1]) & 0x03);
    }

    return true;
}

QByteArray TSL2585::readFifo(uint16_t len)
{
    // The USB-ISS adapter imposes a lower length limit than the FIFO size.
    // That limit will be enforced by the actual call to i2cRead().

    if (len == 0 || len > 512) {
        return QByteArray();
    }

    return ft260_->i2cRead(TSL2585_ADDRESS, TSL2585_FIFO_DATA, len);
}

bool TSL2585::getVSyncPeriod(uint16_t *period)
{
    QByteArray buf;
    buf = ft260_->i2cRead(TSL2585_ADDRESS, TSL2585_VSYNC_PERIOD_L, 2);
    if (buf.isEmpty()) { return false; }

    if (period) {
        *period = static_cast<uint8_t>(buf[0]) | static_cast<uint8_t>(buf[1]) << 8;
    }
    return true;
}

bool TSL2585::setVSyncPeriodTarget(uint16_t periodTarget, bool useFastTiming)
{
    QByteArray buf;

    if (periodTarget > 0x7FFF) {
        return false;
    }

    buf.append(static_cast<uint8_t>(periodTarget & 0x00FF));
    buf.append(static_cast<uint8_t>((periodTarget & 0x7F00) >> 8) | (useFastTiming ? 0x80 : 0x00));

    return ft260_->i2cWrite(TSL2585_ADDRESS, TSL2585_VSYNC_PERIOD_TARGET_L, buf);
}

bool TSL2585::setVSyncControl(uint8_t value)
{
    uint8_t data = value & 0x03;

    return ft260_->i2cWriteByte(TSL2585_ADDRESS, TSL2585_VSYNC_CONTROL, data);
}

bool TSL2585::setVSyncConfig(uint8_t value)
{
    uint8_t data = value & 0xC7;

    return ft260_->i2cWriteByte(TSL2585_ADDRESS, TSL2585_VSYNC_CFG, data);
}

bool TSL2585::setVSyncGpioInt(uint8_t value)
{
    uint8_t data = value & 0x7F;

    return ft260_->i2cWriteByte(TSL2585_ADDRESS, TSL2585_VSYNC_GPIO_INT, data);
}

tsl2585_sensor_type_t TSL2585::sensorType(const tsl2585_ident_t *ident)
{
    if (!ident) { return SENSOR_TYPE_UNKNOWN; }

    if (ident->devId == 0x5C && ident->revId == 0x11) {
        if (ident->auxId == 0b0110) {
            return SENSOR_TYPE_TSL2585;
        } else if (ident->auxId == 0b0010) {
            // TSL2520 and TSL2521 identify the same here, so they will need to be distinguished some other
            // way when and if that matters.
            // If the flicker engine is not used, then they can be treated identically.
            return SENSOR_TYPE_TSL2521;
        } else if (ident->auxId == 0b0101) {
            return SENSOR_TYPE_TSL2522;
        } else if (ident->auxId == 0b0001) {
            return SENSOR_TYPE_TCS3410;
        }
    }

    return SENSOR_TYPE_UNKNOWN;
}

QString TSL2585::sensorTypeName(tsl2585_sensor_type_t sensorType)
{
    switch (sensorType) {
    case SENSOR_TYPE_TSL2585:
        return QLatin1String("TSL2585");
    case SENSOR_TYPE_TSL2520:
        return QLatin1String("TSL2520");
    case SENSOR_TYPE_TSL2521:
        return QLatin1String("TSL2521");
    case SENSOR_TYPE_TSL2522:
        return QLatin1String("TSL2522");
    case SENSOR_TYPE_TCS3410:
        return QLatin1String("TCS3410");
    case SENSOR_TYPE_UNKNOWN:
    default:
        return QLatin1String("Unknown");
    }
}

QString TSL2585::gainString(tsl2585_gain_t gain)
{
    static std::array TSL2585_GAIN_STR{
        "0.5x", "1x", "2x", "4x", "8x", "16x", "32x", "64x",
        "128x", "256x", "512x", "1024x", "2048x", "4096x"
    };


    if (gain >= TSL2585_GAIN_0_5X && gain <= TSL2585_GAIN_4096X) {
        return QString(TSL2585_GAIN_STR[gain]);
    } else {
        return QString("?");
    }
}

float TSL2585::gainValue(tsl2585_gain_t gain)
{
    static std::array TSL2585_GAIN_VAL{
        0.5F, 1.0F, 2.0F, 4.0F, 8.0F, 16.0F, 32.0F, 64.0F,
        128.0F, 256.0F, 512.0F, 1024.0F, 2048.0F, 4096.0F
    };

    if (gain >= TSL2585_GAIN_0_5X && gain <= TSL2585_GAIN_4096X) {
        return TSL2585_GAIN_VAL[gain];
    } else {
        return qSNaN();
    }
}

float TSL2585::integrationTimeMs(uint16_t sampleTime, uint16_t numSamples)
{
    return ((numSamples + 1) * (sampleTime + 1) * 1.388889F) / 1000.0F;
}
