#ifndef ADS7846_h
#define ADS7846_h

#include "EventHandler.h"
#include <stdint.h>

#define CAL_POINT_X1 (20)
#define CAL_POINT_Y1 (20)
#define CAL_POINT1   {CAL_POINT_X1,CAL_POINT_Y1}

#define CAL_POINT_X2 (300)
#define CAL_POINT_Y2 (120)
#define CAL_POINT2   {CAL_POINT_X2,CAL_POINT_Y2}

#define CAL_POINT_X3 (160)
#define CAL_POINT_Y3 (220)
#define CAL_POINT3   {CAL_POINT_X3,CAL_POINT_Y3}

#define MIN_REASONABLE_PRESSURE 9  // depends on position :-(( even slight touch gives more than this
#define MAX_REASONABLE_PRESSURE 110 // Greater means panel not connected
/*
 * without oversampling data is very noisy i.e oversampling of 2 is not suitable for drawing (it gives x +/-1 and y +/-2 pixel noise)
 * since Y value has much more noise than X, Y is oversampled 2 times.
 * 4 is reasonable, 8 is pretty good y +/-1 pixel
 */
#define ADS7846_READ_OVERSAMPLING_DEFAULT 4

// A/D input channel for readChannel()
#define CMD_TEMP0       (0x00)
// 2,5V reference 2,1 mV/Celsius 600 mV at 25 Celsius 12 Bit
// 25 Celsius reads 983 / 0x3D7 and 1 celsius is 3,44064 => Celsius is 897 / 0x381
#define CMD_X_POS       (0x10)
#define CMD_BATT        (0x20) // read Vcc /4
#define CMD_Z1_POS      (0x30)
#define CMD_Z2_POS      (0x40)
#define CMD_Y_POS       (0x50)
#define CMD_AUX	       	(0x60)
#define CMD_TEMP1       (0x70)

#define CMD_START       (0x80)
#define CMD_12BIT       (0x00)
#define CMD_8BIT        (0x08)
#define CMD_DIFF        (0x00)
#define CMD_SINGLE      (0x04)

typedef struct {
    long x;
    long y;
} CAL_POINT; // only for calibrating purposes

typedef struct {
    long a;
    long b;
    long c;
    long d;
    long e;
    long f;
    long div;
} CAL_MATRIX;

#define ADS7846_CHANNEL_COUNT 8 // The number of ADS7846 channel
extern const char * const ADS7846ChannelStrings[ADS7846_CHANNEL_COUNT];
extern const char ADS7846ChannelChars[ADS7846_CHANNEL_COUNT];
// Channel number to text mapping
extern unsigned char ADS7846ChannelMapping[ADS7846_CHANNEL_COUNT];

class ADS7846 {
public:

    struct XYPosition mTouchActualPosition; // calibrated (screen) position
    struct XYPosition mTouchLastPosition; // for move detection
    uint8_t mPressure = 0; // touch panel pressure

    volatile bool ADS7846TouchActive = false; // is true as long as touch lasts
    volatile bool ADS7846TouchStart = false; // is true once for every touch - independent from calling mLongTouchCallback

    ADS7846();
    void init(void);

    void doCalibration(bool aCheckRTC);

    int getXRaw(void);
    int getYRaw(void);
    int getXActual(void);
    int getYActual(void);

    int getPressure(void);
    void rd_data(void);
    void rd_data(int aOversampling);
    uint16_t readChannel(uint8_t channel, bool use12Bit, bool useDiffMode, int numberOfReadingsToIntegrate);

    bool wasTouched(void);

private:
    bool setCalibration(CAL_POINT *lcd, CAL_POINT *tp);
    struct XYPosition mTouchActualPositionRaw; // raw pos (touch panel)
    struct XYPosition mTouchLastCalibratedPositionRaw; // last calibrated raw pos - to avoid calibrating the same position twice
    CAL_MATRIX tp_matrix; // calibrate matrix

    void writeCalibration(CAL_MATRIX aMatrix);
    void readCalibration(CAL_MATRIX *aMatrix);
    void calibrate(void);
};

// The instance provided by the class itself
extern ADS7846 TouchPanel;

#endif //ADS7846_h
