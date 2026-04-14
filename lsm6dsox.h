// =============================================================================
// CBST - Cricket Bowling Sensor Tracking
// LSM6DSOX Register Map & Driver Header
// =============================================================================
// Reference: ST LSM6DSOX Datasheet (DocID 029464 Rev 9)
// Only registers needed for Block 1 are included. Additional registers
// (FIFO, interrupts, embedded functions) will be added in later blocks.
// =============================================================================

#ifndef LSM6DSOX_H
#define LSM6DSOX_H

#include <Arduino.h>
#include <SPI.h>

// ---- LSM6DSOX Register Addresses ----

// Identification
#define LSM6DSOX_WHO_AM_I       0x0F    // Device ID register
#define LSM6DSOX_WHO_AM_I_VAL   0x6C    // Expected response (decimal 108)

// Control registers
#define LSM6DSOX_CTRL1_XL       0x10    // Accelerometer ODR & full-scale
#define LSM6DSOX_CTRL2_G        0x11    // Gyroscope ODR & full-scale
#define LSM6DSOX_CTRL3_C        0x12    // Control register 3 (BDU, IF_INC, etc.)
#define LSM6DSOX_CTRL4_C        0x13    // Control register 4 (SPI mode, etc.)
#define LSM6DSOX_CTRL5_C        0x14    // Control register 5 (rounding, self-test)
#define LSM6DSOX_CTRL6_C        0x15    // Control register 6 (accel performance)
#define LSM6DSOX_CTRL7_G        0x16    // Control register 7 (gyro performance)
#define LSM6DSOX_CTRL8_XL       0x17    // Control register 8 (accel filtering)

// Status
#define LSM6DSOX_STATUS_REG     0x1E    // Data ready flags

// Temperature output
#define LSM6DSOX_OUT_TEMP_L     0x20    // Temperature data low byte
#define LSM6DSOX_OUT_TEMP_H     0x21    // Temperature data high byte

// Gyroscope output
#define LSM6DSOX_OUTX_L_G       0x22    // Gyro X low byte
#define LSM6DSOX_OUTX_H_G       0x23    // Gyro X high byte
#define LSM6DSOX_OUTY_L_G       0x24    // Gyro Y low byte
#define LSM6DSOX_OUTY_H_G       0x25    // Gyro Y high byte
#define LSM6DSOX_OUTZ_L_G       0x26    // Gyro Z low byte
#define LSM6DSOX_OUTZ_H_G       0x27    // Gyro Z high byte

// Accelerometer output
#define LSM6DSOX_OUTX_L_A       0x28    // Accel X low byte
#define LSM6DSOX_OUTX_H_A       0x29    // Accel X high byte
#define LSM6DSOX_OUTY_L_A       0x2A    // Accel Y low byte
#define LSM6DSOX_OUTY_H_A       0x2B    // Accel Y high byte
#define LSM6DSOX_OUTZ_L_A       0x2C    // Accel Z low byte
#define LSM6DSOX_OUTZ_H_A       0x2D    // Accel Z high byte

// ---- ODR (Output Data Rate) Settings ----
// Bits [7:4] of CTRL1_XL and CTRL2_G set the ODR.
// For fast bowling capture, we need high rates. 
// 833 Hz gives ~1.2 ms between samples — good starting point.
// 1.66 kHz is available if needed for higher temporal resolution.
#define LSM6DSOX_ODR_OFF        0x00
#define LSM6DSOX_ODR_12_5_HZ    0x10
#define LSM6DSOX_ODR_26_HZ      0x20
#define LSM6DSOX_ODR_52_HZ      0x30
#define LSM6DSOX_ODR_104_HZ     0x40
#define LSM6DSOX_ODR_208_HZ     0x50
#define LSM6DSOX_ODR_416_HZ     0x60
#define LSM6DSOX_ODR_833_HZ     0x70
#define LSM6DSOX_ODR_1660_HZ    0x80
#define LSM6DSOX_ODR_3330_HZ    0x90
#define LSM6DSOX_ODR_6660_HZ    0xA0

// ---- Accelerometer Full-Scale Selection ----
// Bits [3:2] of CTRL1_XL. Bowling involves high g-forces at release.
// ±16g is safest to avoid clipping during the bowling action.
#define LSM6DSOX_XL_FS_2G       0x00
#define LSM6DSOX_XL_FS_4G       0x08
#define LSM6DSOX_XL_FS_8G       0x0C
#define LSM6DSOX_XL_FS_16G      0x04    // Note: 0x04, not 0x0C (per datasheet)

// ---- Gyroscope Full-Scale Selection ----
// Bits [3:1] of CTRL2_G. Fast arm rotation can exceed 1000 °/s.
// ±2000 °/s prevents clipping during rapid bowling arm swing.
#define LSM6DSOX_GY_FS_250DPS   0x00
#define LSM6DSOX_GY_FS_500DPS   0x04
#define LSM6DSOX_GY_FS_1000DPS  0x08
#define LSM6DSOX_GY_FS_2000DPS  0x0C

// ---- Status Register Bit Masks ----
#define LSM6DSOX_STATUS_XLDA    0x01    // Accelerometer new data available
#define LSM6DSOX_STATUS_GDA     0x02    // Gyroscope new data available
#define LSM6DSOX_STATUS_TDA     0x04    // Temperature new data available


// =============================================================================
// LSM6DSOX SPI Driver Class
// =============================================================================
// Handles low-level SPI register reads/writes for a single IMU.
// Each instance is bound to a specific CS pin.
// =============================================================================

class LSM6DSOX {
public:
    // Constructor — takes the chip select GPIO pin number
    LSM6DSOX(uint8_t csPin);

    // Initialise the sensor: verify WHO_AM_I, configure ODR and full-scale
    // Returns true if the sensor responded correctly, false otherwise
    bool begin(SPIClass &spi, uint32_t spiClock);

    // Read the WHO_AM_I register to verify sensor identity
    uint8_t readWhoAmI();

    // Configure accelerometer ODR and full-scale range
    // odr: one of LSM6DSOX_ODR_* defines
    // fs:  one of LSM6DSOX_XL_FS_* defines
    void configAccel(uint8_t odr, uint8_t fs);

    // Configure gyroscope ODR and full-scale range
    // odr: one of LSM6DSOX_ODR_* defines
    // fs:  one of LSM6DSOX_GY_FS_* defines
    void configGyro(uint8_t odr, uint8_t fs);

    // Check if new data is available
    bool accelDataReady();
    bool gyroDataReady();

    // Read raw 16-bit sensor data
    // Accelerometer output in raw counts (convert with sensitivity later)
    void readAccelRaw(int16_t &x, int16_t &y, int16_t &z);

    // Gyroscope output in raw counts
    void readGyroRaw(int16_t &x, int16_t &y, int16_t &z);

    // Read all 12 bytes of accel+gyro data in one burst for efficiency
    // Order: gx, gy, gz, ax, ay, az (register layout 0x22–0x2D)
    void readAllRaw(int16_t &gx, int16_t &gy, int16_t &gz,
                    int16_t &ax, int16_t &ay, int16_t &az);

    // Read a single register
    uint8_t readRegister(uint8_t reg);

    // Write a single register
    void writeRegister(uint8_t reg, uint8_t value);

    // Read multiple consecutive registers (burst read)
    void readRegisters(uint8_t startReg, uint8_t *buffer, uint8_t count);

private:
    uint8_t     _csPin;
    SPIClass   *_spi;
    uint32_t    _spiClock;
    SPISettings _spiSettings;

    // Pull CS low to begin SPI transaction
    void csSelect();
    // Pull CS high to end SPI transaction
    void csDeselect();
};

#endif // LSM6DSOX_H
