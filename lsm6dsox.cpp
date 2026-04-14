// =============================================================================
// CBST - Cricket Bowling Sensor Tracking
// LSM6DSOX SPI Driver Implementation
// =============================================================================

#include "lsm6dsox.h"

// ---- Constructor ----
LSM6DSOX::LSM6DSOX(uint8_t csPin)
    : _csPin(csPin), _spi(nullptr), _spiClock(1000000) {
}

// ---- Initialisation ----
bool LSM6DSOX::begin(SPIClass &spi, uint32_t spiClock) {
    _spi      = &spi;
    _spiClock = spiClock;
    _spiSettings = SPISettings(_spiClock, MSBFIRST, SPI_MODE3);
    // LSM6DSOX uses SPI Mode 3: CPOL=1, CPHA=1
    // Clock idles HIGH, data sampled on falling edge.

    // Configure CS pin as output and deselect the device
    pinMode(_csPin, OUTPUT);
    csDeselect();

    // Small delay after power-up to let the sensor boot
    delay(10);

    // ---- Verify WHO_AM_I ----
    uint8_t id = readWhoAmI();
    if (id != LSM6DSOX_WHO_AM_I_VAL) {
        return false;   // Sensor did not respond correctly
    }

    // ---- Software reset ----
    // Set SW_RESET bit (bit 0) in CTRL3_C to reset all registers
    writeRegister(LSM6DSOX_CTRL3_C, 0x01);
    delay(20);  // Wait for reset to complete

    // ---- Configure CTRL3_C ----
    // BDU = 1 (Block Data Update — output registers not updated until 
    //          both MSB and LSB are read, prevents reading half-updated data)
    // IF_INC = 1 (auto-increment register address during multi-byte reads)
    // SPI mode is 4-wire by default after reset (bit 3 = 0), which is correct.
    writeRegister(LSM6DSOX_CTRL3_C, 0x44);  // BDU | IF_INC

    return true;
}

// ---- WHO_AM_I ----
uint8_t LSM6DSOX::readWhoAmI() {
    return readRegister(LSM6DSOX_WHO_AM_I);
}

// ---- Accelerometer Configuration ----
void LSM6DSOX::configAccel(uint8_t odr, uint8_t fs) {
    // CTRL1_XL: [7:4] = ODR, [3:2] = FS, [1:0] = filter bandwidth
    // We leave the filter bandwidth at default (bits [1:0] = 00)
    uint8_t val = odr | fs;
    writeRegister(LSM6DSOX_CTRL1_XL, val);
}

// ---- Gyroscope Configuration ----
void LSM6DSOX::configGyro(uint8_t odr, uint8_t fs) {
    // CTRL2_G: [7:4] = ODR, [3:1] = FS, [0] = FS_125 enable
    // FS_125 = 0 (we're using the standard full-scale settings)
    uint8_t val = odr | fs;
    writeRegister(LSM6DSOX_CTRL2_G, val);
}

// ---- Data Ready Checks ----
bool LSM6DSOX::accelDataReady() {
    uint8_t status = readRegister(LSM6DSOX_STATUS_REG);
    return (status & LSM6DSOX_STATUS_XLDA) != 0;
}

bool LSM6DSOX::gyroDataReady() {
    uint8_t status = readRegister(LSM6DSOX_STATUS_REG);
    return (status & LSM6DSOX_STATUS_GDA) != 0;
}

// ---- Read Raw Accelerometer Data ----
void LSM6DSOX::readAccelRaw(int16_t &x, int16_t &y, int16_t &z) {
    uint8_t buf[6];
    readRegisters(LSM6DSOX_OUTX_L_A, buf, 6);
    x = (int16_t)(buf[1] << 8 | buf[0]);
    y = (int16_t)(buf[3] << 8 | buf[2]);
    z = (int16_t)(buf[5] << 8 | buf[4]);
}

// ---- Read Raw Gyroscope Data ----
void LSM6DSOX::readGyroRaw(int16_t &x, int16_t &y, int16_t &z) {
    uint8_t buf[6];
    readRegisters(LSM6DSOX_OUTX_L_G, buf, 6);
    x = (int16_t)(buf[1] << 8 | buf[0]);
    y = (int16_t)(buf[3] << 8 | buf[2]);
    z = (int16_t)(buf[5] << 8 | buf[4]);
}

// ---- Read All 6-Axis Data in One Burst ----
// Reads 12 bytes from 0x22 to 0x2D: Gyro XYZ then Accel XYZ
// This is more efficient than separate reads as it requires only
// one CS toggle and one address transmission.
void LSM6DSOX::readAllRaw(int16_t &gx, int16_t &gy, int16_t &gz,
                           int16_t &ax, int16_t &ay, int16_t &az) {
    uint8_t buf[12];
    readRegisters(LSM6DSOX_OUTX_L_G, buf, 12);
    
    // Gyroscope: bytes 0-5
    gx = (int16_t)(buf[1]  << 8 | buf[0]);
    gy = (int16_t)(buf[3]  << 8 | buf[2]);
    gz = (int16_t)(buf[5]  << 8 | buf[4]);
    
    // Accelerometer: bytes 6-11
    ax = (int16_t)(buf[7]  << 8 | buf[6]);
    ay = (int16_t)(buf[9]  << 8 | buf[8]);
    az = (int16_t)(buf[11] << 8 | buf[10]);
}


// =============================================================================
// Low-Level SPI Register Access
// =============================================================================
// The LSM6DSOX SPI protocol:
//   - Byte 1: R/W bit (bit 7) | Register address (bits 6:0)
//     - Read:  bit 7 = 1 (0x80 | address)
//     - Write: bit 7 = 0 (address only)
//   - Byte 2+: Data (MSB first for multi-byte)
//   - With IF_INC=1, the address auto-increments for burst reads.
// =============================================================================

uint8_t LSM6DSOX::readRegister(uint8_t reg) {
    uint8_t val;
    _spi->beginTransaction(_spiSettings);
    csSelect();
    _spi->transfer(0x80 | reg);     // Set read bit
    val = _spi->transfer(0x00);     // Clock out data
    csDeselect();
    _spi->endTransaction();
    return val;
}

void LSM6DSOX::writeRegister(uint8_t reg, uint8_t value) {
    _spi->beginTransaction(_spiSettings);
    csSelect();
    _spi->transfer(reg);            // Write bit is 0 (MSB clear)
    _spi->transfer(value);
    csDeselect();
    _spi->endTransaction();
}

void LSM6DSOX::readRegisters(uint8_t startReg, uint8_t *buffer, uint8_t count) {
    _spi->beginTransaction(_spiSettings);
    csSelect();
    _spi->transfer(0x80 | startReg);    // Read bit + start address
    for (uint8_t i = 0; i < count; i++) {
        buffer[i] = _spi->transfer(0x00);  // Clock out each byte
    }
    csDeselect();
    _spi->endTransaction();
}

// ---- Chip Select Helpers ----
void LSM6DSOX::csSelect() {
    digitalWrite(_csPin, LOW);
    delayMicroseconds(1);   // Brief settling time
}

void LSM6DSOX::csDeselect() {
    digitalWrite(_csPin, HIGH);
    delayMicroseconds(1);   // Minimum CS high time between transactions
}
