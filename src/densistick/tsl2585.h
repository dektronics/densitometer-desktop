#ifndef TSL2585_H
#define TSL2585_H

#include <QString>
#include <stdint.h>

#define TSL2585_SAMPLE_TIME_BASE 1.388889F /*!< Sample time base in microseconds */

typedef struct {
    uint8_t devId;
    uint8_t revId;
    uint8_t auxId;
} tsl2585_ident_t;

typedef enum {
    SENSOR_TYPE_UNKNOWN = 0,
    SENSOR_TYPE_TSL2585,
    SENSOR_TYPE_TSL2520,
    SENSOR_TYPE_TSL2521,
    SENSOR_TYPE_TSL2522,
    SENSOR_TYPE_TCS3410
} tsl2585_sensor_type_t;

typedef enum {
    TSL2585_MOD_NONE = 0x00,
    TSL2585_MOD0 = 0x01,
    TSL2585_MOD1 = 0x02,
    TSL2585_MOD2 = 0x04
} tsl2585_modulator_t;

inline tsl2585_modulator_t operator|(tsl2585_modulator_t a, tsl2585_modulator_t b)
{
    return static_cast<tsl2585_modulator_t>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

typedef enum {
    TSL2585_STEP0 = 0x01,
    TSL2585_STEP1 = 0x02,
    TSL2585_STEP2 = 0x04,
    TSL2585_STEP3 = 0x08,
    TSL2585_STEPS_NONE = 0x00,
    TSL2585_STEPS_ALL = 0x0F
} tsl2585_step_t;

typedef enum {
    TSL2585_PHD_0 = 0, /*!< IR */
    TSL2585_PHD_1,     /*!< Photopic */
    TSL2585_PHD_2,     /*!< IR */
    TSL2585_PHD_3,     /*!< UV-A */
    TSL2585_PHD_4,     /*!< UV-A */
    TSL2585_PHD_5,     /*!< Photopic */
    TSL2585_PHD_MAX
} tsl2585_photodiode_t;

typedef enum {
    TSL2585_GAIN_0_5X  = 0,
    TSL2585_GAIN_1X    = 1,
    TSL2585_GAIN_2X    = 2,
    TSL2585_GAIN_4X    = 3,
    TSL2585_GAIN_8X    = 4,
    TSL2585_GAIN_16X   = 5,
    TSL2585_GAIN_32X   = 6,
    TSL2585_GAIN_64X   = 7,
    TSL2585_GAIN_128X  = 8,
    TSL2585_GAIN_256X  = 9,
    TSL2585_GAIN_512X  = 10,
    TSL2585_GAIN_1024X = 11,
    TSL2585_GAIN_2048X = 12,
    TSL2585_GAIN_4096X = 13,
    TSL2585_GAIN_MAX   = 14
} tsl2585_gain_t;

inline tsl2585_gain_t operator+(tsl2585_gain_t a, int b)
{
    int gainVal = (static_cast<uint8_t>(a) + b) % TSL2585_GAIN_MAX;
    return static_cast<tsl2585_gain_t>(gainVal);
}

inline tsl2585_gain_t operator-(tsl2585_gain_t a, int b)
{
    int gainVal = (static_cast<uint8_t>(a) - b) % TSL2585_GAIN_MAX;
    return static_cast<tsl2585_gain_t>(gainVal);
}

typedef struct {
    bool overflow;
    bool underflow;
    uint16_t level;
} tsl2585_fifo_status_t;

typedef enum {
    TSL2585_ALS_FIFO_16BIT = 0x00,
    TSL2585_ALS_FIFO_24BIT = 0x01,
    TSL2585_ALS_FIFO_32BIT = 0x03
} tsl2585_als_fifo_data_format_t;

typedef enum {
    TSL2585_TRIGGER_OFF      = 0x00, /*!< Off */
    TSL2585_TRIGGER_NORMAL   = 0x01, /*!< 2.844ms * WTIME */
    TSL2585_TRIGGER_LONG     = 0x02, /*!< 45.511ms * WTIME */
    TSL2585_TRIGGER_FAST     = 0x03, /*!< 88.889us * WTIME */
    TSL2585_TRIGGER_FASTLONG = 0x04, /*!< 1.422ms * WTIME */
    TSL2585_TRIGGER_VSYNC    = 0x05  /*!< One VSYNC per WTIME step */
} tsl2585_trigger_mode_t;

/* ENABLE register values */
#define TSL2585_ENABLE_FDEN 0x40 /*!< Flicker detection enable */
#define TSL2585_ENABLE_AEN  0x02 /*!< ALS Enable */
#define TSL2585_ENABLE_PON  0x01 /*!< Power on */

/* INTENAB register values */
#define TSL2585_INTENAB_MIEN 0x80 /*!< Modulator Interrupt Enable*/
#define TSL2585_INTENAB_AIEN 0x08 /*!< ALS Interrupt Enable */
#define TSL2585_INTENAB_FIEN 0x04 /*!< FIFO Interrupt Enable */
#define TSL2585_INTENAB_SIEN 0x01 /*!< System Interrupt Enable */

/* STATUS register values */
#define TSL2585_STATUS_MINT 0x80 /*!< Modulator Interrupt (STATUS2 for details) */
#define TSL2585_STATUS_AINT 0x08 /*!< ALS Interrupt (STATUS3 for details) */
#define TSL2585_STATUS_FINT 0x04 /*!< FIFO Interrupt */
#define TSL2585_STATUS_SINT 0x01 /*!< System Interrupt (STATUS5 for details) */

/* STATUS2 register values */
#define TSL2585_STATUS2_ALS_DATA_VALID         0x40 /*!< ALS Data Valid */
#define TSL2585_STATUS2_ALS_DIGITAL_SATURATION 0x10 /*!< ALS Digital Saturation */
#define TSL2585_STATUS2_FD_DIGITAL_SATURATION  0x08 /*!< Flicker Detect Digital Saturation */
#define TSL2585_STATUS2_MOD_ANALOG_SATURATION2 0x04 /*!< ALS Analog Saturation of modulator 2 */
#define TSL2585_STATUS2_MOD_ANALOG_SATURATION1 0x02 /*!< ALS Analog Saturation of modulator 1 */
#define TSL2585_STATUS2_MOD_ANALOG_SATURATION0 0x01 /*!< ALS Analog Saturation of modulator 0 */

/* STATUS3 register values */
#define TSL2585_STATUS3_AINT_HYST_STATE_VALID 0x80 /*!< Indicates that the ALS interrupt hysteresis state is valid */
#define TSL2585_STATUS3_AINT_HYST_STATE_RD    0x40 /*!< Indicates the state in the hysteresis defined with AINT_AILT and AINT_AIHT */
#define TSL2585_STATUS3_AINT_AIHT             0x20 /*!< ALS Interrupt High */
#define TSL2585_STATUS3_AINT_AILT             0x10 /*!< ALS Interrupt Low */
#define TSL2585_STATUS3_VSYNC_LOST            0x08 /*!< Indicates that synchronization is out of sync with clock provided at the VSYNC pin */
#define TSL2585_STATUS3_OSC_CALIB_SATURATION  0x02 /*!< Indicates that oscillator calibration is out of range */
#define TSL2585_STATUS3_OSC_CALIB_FINISHED    0x01 /*!< Indicates that oscillator calibration is finished */

/* STATUS4 register values */
#define TSL2585_STATUS4_MOD_SAMPLE_TRIGGER_ERROR 0x08 /*!< Measured data is corrupted */
#define TSL2585_STATUS4_MOD_TRIGGER_ERROR        0x04 /*!< WTIME is too short for the programmed configuration */
#define TSL2585_STATUS4_SAI_ACTIVE               0x02 /*!< Sleep After Interrupt Active */
#define TSL2585_STATUS4_INIT_BUSY                0x01 /*!< Initialization Busy */

/* ALS_STATUS register values */
#define TSL2585_MEAS_SEQR_STEP 0xC0 /*< Mask for bits that contains the sequencer step where ALS data was measured */
#define TSL2585_ALS_DATA0_ANALOG_SATURATION_STATUS 0x20 /*< Indicates analog saturation of ALS data0 in data registers ALS_ADATA0 */
#define TSL2585_ALS_DATA1_ANALOG_SATURATION_STATUS 0x10 /*< Indicates analog saturation of ALS data1 in data registers ALS_ADATA1 */
#define TSL2585_ALS_DATA2_ANALOG_SATURATION_STATUS 0x08 /*< Indicates analog saturation of ALS data2 in data registers ALS_ADATA2 */
#define TSL2585_ALS_DATA0_SCALED_STATUS 0x04 /*< Indicates if ALS data0 needs to be multiplied */
#define TSL2585_ALS_DATA1_SCALED_STATUS 0x02 /*< Indicates if ALS data1 needs to be multiplied */
#define TSL2585_ALS_DATA2_SCALED_STATUS 0x01 /*< Indicates if ALS data2 needs to be multiplied */

/* VSYNC_CFG register values */
#define TSL2585_VSYNC_CFG_OSC_CALIB_DISABLED  0x00 /*!< Oscillator calibration disabled */
#define TSL2585_VSYNC_CFG_OSC_CALIB_AFTER_PON 0x40 /*!< Oscillator calibration after PON */
#define TSL2585_VSYNC_CFG_OSC_CALIB_ALWAYS    0x80 /*!< Oscillator calibration always on */
#define TSL2585_VSYNC_CFG_VSYNC_MODE          0x04 /*!< Select trigger signal (0=VSYNC/GPIO/INT, 1=SW_VSYNC_TRIGGER) */
#define TSL2585_VSYNC_CFG_VSYNC_SELECT        0x02 /*!< Select trigger pin (0=VSYNC/GPIO, 1=INT) */
#define TSL2585_VSYNC_CFG_VSYNC_INVERT        0x01 /*!< If set to "1" the VSYNC input signal is inverted */

/* VSYNC_GPIO_INT register values */
#define TSL2585_GPIO_INT_INT_INVERT        0x40 /*!< Set to invert the INT pin output */
#define TSL2585_GPIO_INT_INT_IN_EN         0x20 /*!< Set to configure the INT pin as input */
#define TSL2585_GPIO_INT_INT_IN            0x10 /*!< External HIGH or LOW value applied to INT pin */
#define TSL2585_GPIO_INT_VSYNC_GPIO_INVERT 0x08 /*!< Set to invert the VSYNC/GPIO output */
#define TSL2585_GPIO_INT_VSYNC_GPIO_IN_EN  0x04 /*!< Set to configure the VSYNC/GPIO pin as input */
#define TSL2585_GPIO_INT_VSYNC_GPIO_OUT    0x02 /*!< Set the VSYNC/GPIO pin HI or LOW */
#define TSL2585_GPIO_INT_VSYNC_GPIO_IN     0x01 /*!< External HIGH or LOW value applied to the VSYNC/GPIO pin */

class Ft260;

class TSL2585
{
public:
    explicit TSL2585(Ft260 *ft260);

    bool init(tsl2585_ident_t *ident = nullptr);

    bool setEnable(uint8_t value);
    bool enable();
    bool disable();

    bool setInterruptEnable(uint8_t value);

    bool softReset();
    bool clearFifo();

    bool enableModulators(tsl2585_modulator_t mods);

    bool getStatus(uint8_t *status);
    bool setStatus(uint8_t status);

    bool getStatus2(uint8_t *status);
    bool getStatus3(uint8_t *status);
    bool getStatus4(uint8_t *status);
    bool getStatus5(uint8_t *status);

    bool setModGainTableSelect(bool alternate);
    bool setMaxModGain(tsl2585_gain_t gain);
    bool setModGain(tsl2585_modulator_t mod, tsl2585_step_t step, tsl2585_gain_t gain);

    bool setModResidualEnable(tsl2585_modulator_t mod, tsl2585_step_t steps);

    bool setModPhotodiodeSmux(tsl2585_step_t step, const tsl2585_modulator_t phd_mod[TSL2585_PHD_MAX]);

    bool setCalibrationNthIteration(uint8_t iteration);

    bool setSampleTime(uint16_t value);
    bool setAlsNumSamples(uint16_t value);

    bool setAlsInterruptPersistence(uint8_t value);

    bool getAlsStatus(uint8_t *status);
    bool getAlsScale(uint8_t *scale);

    bool setAgcNumSamples(uint16_t value);
    bool setAgcCalibration(bool enabled);

    bool setFifoAlsStatusWriteEnable(bool enable);
    bool setFifoAlsDataFormat(tsl2585_als_fifo_data_format_t format);

    bool getAlsMsbPosition(uint8_t *position);
    bool setAlsMsbPosition(uint8_t position);

    bool getTriggerMode(tsl2585_trigger_mode_t *mode);
    bool setTriggerMode(tsl2585_trigger_mode_t mode);

    bool getAlsData0(uint16_t *data);
    bool getAlsData1(uint16_t *data);
    bool getAlsData2(uint16_t *data);

    bool setFifoDataWriteEnable(tsl2585_modulator_t mod, bool enable);
    bool setFifoThreshold(uint16_t threshold);
    bool getFifoStatus(tsl2585_fifo_status_t *status);

    QByteArray readFifo(uint16_t len);

    bool getVSyncPeriod(uint16_t *period);
    bool setVSyncPeriodTarget(uint16_t periodTarget, bool useFastTiming);
    bool setVSyncControl(uint8_t value);
    bool setVSyncConfig(uint8_t value);
    bool setVSyncGpioInt(uint8_t value);

    static tsl2585_sensor_type_t sensorType(const tsl2585_ident_t *ident);
    static QString sensorTypeName(tsl2585_sensor_type_t sensorType);
    static QString gainString(tsl2585_gain_t gain);
    static float gainValue(tsl2585_gain_t gain);
    static float integrationTimeMs(uint16_t sampleTime, uint16_t numSamples);

private:
    static bool gainRegister(tsl2585_modulator_t mod, tsl2585_step_t step, uint8_t *reg, bool *upper);

    Ft260 *ft260_;
};

#endif // TSL2585_H
