// =============================================================================
// CBST - Cricket Bowling Sensor Tracking
// Block 1: ESP32-S3 SPI Initialisation & Dual LSM6DSOX Verification
// =============================================================================
// 
// This firmware initialises the SPI bus on the ESP32-S3 ProS3 and establishes
// communication with two LSM6DSOX IMU sensors (shoulder and wrist).
//
// What this block validates (from the Testing Matrix):
//   COM1 — IMU 1 detected by ESP32 (WHO_AM_I check)
//   COM2 — IMU 2 detected by ESP32 (WHO_AM_I check)
//   COM3 — SPI communication functions correctly (register read/write)
//   COM4 — Chip select correctly isolates sensors (independent reads)
//
// Serial output at 115200 baud — open the PlatformIO Serial Monitor to see
// the initialisation results and live sensor data.
// =============================================================================

#include <Arduino.h>
#include <SPI.h>
#include "cbst_pins.h"
#include "lsm6dsox.h"

// ---- Create two IMU instances, one per chip select line ----
LSM6DSOX imuShoulder(PIN_CS_IMU1);   // Shoulder sensor (IMU1)
LSM6DSOX imuWrist(PIN_CS_IMU2);      // Wrist sensor (IMU2)

// ---- SPI Bus Instance ----
// The ESP32-S3 has multiple SPI peripherals. We use HSPI (SPI2) which is
// the standard user-accessible SPI bus. FSPI (SPI0/1) is reserved for flash.
SPIClass spiIMU(HSPI);

// ---- Configuration ----
// These settings determine the sensor behaviour for bowling capture.
// ODR of 833 Hz gives ~1.2 ms between samples — sufficient for the 
// bowling motion which takes roughly 0.3-0.5 seconds from top of action 
// to release. At 833 Hz that's 250-415 samples per delivery.
//
// Accelerometer at ±16g: bowling arm deceleration at release can exceed 
// 10g, so ±16g prevents clipping.
//
// Gyroscope at ±2000 °/s: the bowling arm can rotate at >1000 °/s 
// during the delivery stride, so ±2000 °/s gives headroom.

const uint8_t ACCEL_ODR = LSM6DSOX_ODR_833_HZ;
const uint8_t ACCEL_FS  = LSM6DSOX_XL_FS_16G;
const uint8_t GYRO_ODR  = LSM6DSOX_ODR_833_HZ;
const uint8_t GYRO_FS   = LSM6DSOX_GY_FS_2000DPS;

// ---- Sensitivity Constants ----
// Used to convert raw 16-bit counts to physical units.
// From the LSM6DSOX datasheet Table 2 and Table 3:
//   ±16g  → 0.488 mg/LSB  → multiply by 0.000488 for g
//   ±2000 °/s → 70.0 mdps/LSB → multiply by 0.070 for °/s
const float ACCEL_SENSITIVITY = 0.000488f;   // g per LSB (±16g range)
const float GYRO_SENSITIVITY  = 0.070f;      // °/s per LSB (±2000 °/s range)

// ---- Timing ----
unsigned long lastPrintTime = 0;
const unsigned long PRINT_INTERVAL_MS = 100;  // Print at 10 Hz for readability

// ---- Forward declarations ----
void printSensorData();
void printInitResult(const char* name, bool success, uint8_t whoAmI);
void runCOM4Test();


// =============================================================================
// setup() — Runs once at power-on
// =============================================================================
void setup() {
    // ---- Serial for debugging ----
    pinMode(17, OUTPUT);
    digitalWrite(17, HIGH);

    Serial.begin(115200);
    while (!Serial && millis() < 3000) {
        // Wait up to 3 seconds for serial connection (USB CDC on ESP32-S3)
    }
    delay(10000);

    Serial.println();
    Serial.println("==============================================");
    Serial.println("  CBST — Cricket Bowling Sensor Tracking");
    Serial.println("  Block 1: SPI Init & Dual IMU Verification");
    Serial.println("==============================================");
    Serial.println();

    // ---- Drive CS pins HIGH before anything else ----
    // The LSM6DSOX latches its communication interface (SPI vs I2C) at
    // power-on based on the CS pin state. If CS floats or is LOW during
    // boot the sensor initialises in I2C mode and will never respond to SPI.
    // Driving HIGH here before spiIMU.begin() guarantees SPI mode is selected.
    pinMode(PIN_CS_IMU1, OUTPUT);
    digitalWrite(PIN_CS_IMU1, HIGH);
    pinMode(PIN_CS_IMU2, OUTPUT);
    digitalWrite(PIN_CS_IMU2, HIGH);

    // ---- Initialise SPI bus ----
    // Assign our custom pins to the HSPI peripheral.
    // The ESP32-S3 allows remapping SPI pins via the GPIO matrix.
    spiIMU.begin(PIN_SPI_SCLK, PIN_SPI_MISO, PIN_SPI_MOSI);
    Serial.println("[SPI] Bus initialised");
    Serial.printf("  SCLK = IO%d, MISO = IO%d, MOSI = IO%d\n",
                  PIN_SPI_SCLK, PIN_SPI_MISO, PIN_SPI_MOSI);
    Serial.printf("  Clock speed = %lu Hz\n", (unsigned long)SPI_CLOCK_HZ);
    Serial.println();

    // ---- Initialise IMU 1 (Shoulder) ----
    Serial.println("[IMU1] Initialising shoulder sensor...");
    Serial.printf("  CS pin = IO%d\n", PIN_CS_IMU1);
    bool imu1OK = imuShoulder.begin(spiIMU, SPI_CLOCK_HZ);
    uint8_t id1 = imuShoulder.readWhoAmI();
    printInitResult("IMU1 (Shoulder)", imu1OK, id1);

    // ---- Initialise IMU 2 (Wrist) ----
    Serial.println("[IMU2] Initialising wrist sensor...");
    Serial.printf("  CS pin = IO%d\n", PIN_CS_IMU2);
    bool imu2OK = imuWrist.begin(spiIMU, SPI_CLOCK_HZ);
    uint8_t id2 = imuWrist.readWhoAmI();
    printInitResult("IMU2 (Wrist)", imu2OK, id2);

    // ---- Check if required sensors are operational ----
    // IMU1 (shoulder) must be present. IMU2 (wrist) is optional during
    // single-sensor bring-up — remove the imu2OK guard once both are wired.
    if (!imu1OK) {
        Serial.println();
        Serial.println("!!! INIT FAILED — IMU1 (Shoulder) not responding !!!");
        Serial.println("    Troubleshooting steps:");
        Serial.println("    1. Verify 3.3V power to VDDIO and VDD on IMU1");
        Serial.println("    2. Check GND connection");
        Serial.println("    3. Confirm SCLK, MOSI, MISO wiring matches cbst_pins.h");
        Serial.println("    4. Verify CS line is connected to IO7");
        Serial.println("    5. Ensure SDO/SA0 pin on IMU is connected to MISO");
        Serial.println("       (not left floating or tied to VDD/GND)");
        Serial.println();
        Serial.println("Halting. Fix the hardware issue and reset.");
        while (true) { delay(1000); }  // Halt — no point continuing
    }
    if (!imu2OK) {
        Serial.println("    [WARN] IMU2 (Wrist) not detected — continuing with IMU1 only");
    }

    // ---- Configure both sensors ----
    Serial.println();
    Serial.println("[CONFIG] Setting sensor parameters...");

    imuShoulder.configAccel(ACCEL_ODR, ACCEL_FS);
    imuShoulder.configGyro(GYRO_ODR, GYRO_FS);
    Serial.println("  IMU1: Accel ±16g @ 833 Hz, Gyro ±2000 °/s @ 833 Hz");

    imuWrist.configAccel(ACCEL_ODR, ACCEL_FS);
    imuWrist.configGyro(GYRO_ODR, GYRO_FS);
    Serial.println("  IMU2: Accel ±16g @ 833 Hz, Gyro ±2000 °/s @ 833 Hz");

    // ---- Verify configuration was written correctly ----
    uint8_t ctrl1_1 = imuShoulder.readRegister(LSM6DSOX_CTRL1_XL);
    uint8_t ctrl2_1 = imuShoulder.readRegister(LSM6DSOX_CTRL2_G);
    uint8_t ctrl1_2 = imuWrist.readRegister(LSM6DSOX_CTRL1_XL);
    uint8_t ctrl2_2 = imuWrist.readRegister(LSM6DSOX_CTRL2_G);

    Serial.printf("  IMU1 CTRL1_XL = 0x%02X (expected 0x%02X)\n",
                  ctrl1_1, ACCEL_ODR | ACCEL_FS);
    Serial.printf("  IMU1 CTRL2_G  = 0x%02X (expected 0x%02X)\n",
                  ctrl2_1, GYRO_ODR | GYRO_FS);
    Serial.printf("  IMU2 CTRL1_XL = 0x%02X (expected 0x%02X)\n",
                  ctrl1_2, ACCEL_ODR | ACCEL_FS);
    Serial.printf("  IMU2 CTRL2_G  = 0x%02X (expected 0x%02X)\n",
                  ctrl2_2, GYRO_ODR | GYRO_FS);

    bool configOK = (ctrl1_1 == (ACCEL_ODR | ACCEL_FS)) &&
                    (ctrl2_1 == (GYRO_ODR | GYRO_FS)) &&
                    (ctrl1_2 == (ACCEL_ODR | ACCEL_FS)) &&
                    (ctrl2_2 == (GYRO_ODR | GYRO_FS));

    if (configOK) {
        Serial.println("  [PASS] Register verification successful");
    } else {
        Serial.println("  [FAIL] Register values do not match — SPI write issue?");
    }

    // ---- Run COM4 test: verify CS isolation ----
    Serial.println();
    runCOM4Test();

    // ---- Ready ----
    Serial.println();
    Serial.println("==============================================");
    Serial.println("  INIT COMPLETE — Streaming sensor data...");
    Serial.println("  (printing at 10 Hz for readability)");
    Serial.println("==============================================");
    Serial.println();
    Serial.println("  IMU1 (Shoulder)                    |  IMU2 (Wrist)");
    Serial.println("  AX     AY     AZ    GX    GY    GZ |  AX     AY     AZ    GX    GY    GZ");
    Serial.println("  (g)    (g)    (g)  (°/s) (°/s) (°/s)| (g)    (g)    (g)  (°/s) (°/s) (°/s)");
    Serial.println("  ----   ----   ----  ----  ----  ---- |  ----   ----   ----  ----  ----  ----");

    lastPrintTime = millis();
}


// =============================================================================
// loop() — Runs repeatedly
// =============================================================================
void loop() {
    // For Block 1 verification, we just print data at a readable rate.
    // In Block 2, this will be replaced with high-speed acquisition into
    // PSRAM arrays with proper timestamping.

    unsigned long now = millis();
    if (now - lastPrintTime >= PRINT_INTERVAL_MS) {
        lastPrintTime = now;
        printSensorData();
    }
}


// =============================================================================
// Helper Functions
// =============================================================================

void printInitResult(const char* name, bool success, uint8_t whoAmI) {
    if (success) {
        Serial.printf("  [PASS] %s — WHO_AM_I = 0x%02X (correct)\n",
                      name, whoAmI);
    } else {
        Serial.printf("  [FAIL] %s — WHO_AM_I = 0x%02X (expected 0x%02X)\n",
                      name, whoAmI, LSM6DSOX_WHO_AM_I_VAL);
    }
}


// COM4 Validation: Chip select isolation test
// Writes a known value to IMU1, reads it back, then checks that IMU2
// was NOT affected (i.e. its register still holds a different value).
void runCOM4Test() {
    Serial.println("[COM4] Chip select isolation test...");

    // Read current CTRL5_C from both sensors (should be 0x00 after reset)
    uint8_t imu1_before = imuShoulder.readRegister(LSM6DSOX_CTRL5_C);
    uint8_t imu2_before = imuWrist.readRegister(LSM6DSOX_CTRL5_C);

    // Write a test value to IMU1 only
    uint8_t testVal = 0x40;  // Set bit 6 (rounding status)
    imuShoulder.writeRegister(LSM6DSOX_CTRL5_C, testVal);

    // Read back from both sensors
    uint8_t imu1_after = imuShoulder.readRegister(LSM6DSOX_CTRL5_C);
    uint8_t imu2_after = imuWrist.readRegister(LSM6DSOX_CTRL5_C);

    // IMU1 should have the test value, IMU2 should be unchanged
    bool isolated = (imu1_after == testVal) && (imu2_after == imu2_before);

    Serial.printf("  IMU1 CTRL5_C: 0x%02X → 0x%02X (wrote 0x%02X)\n",
                  imu1_before, imu1_after, testVal);
    Serial.printf("  IMU2 CTRL5_C: 0x%02X → 0x%02X (should be unchanged)\n",
                  imu2_before, imu2_after);

    if (isolated) {
        Serial.println("  [PASS] CS lines correctly isolate each sensor");
    } else {
        Serial.println("  [FAIL] CS isolation failed — both sensors may be");
        Serial.println("         sharing the same CS line, or wiring is crossed");
    }

    // Restore the register to its original value
    imuShoulder.writeRegister(LSM6DSOX_CTRL5_C, imu1_before);
}


// Read and print data from both sensors side-by-side
void printSensorData() {
    int16_t ax1, ay1, az1, gx1, gy1, gz1;
    int16_t ax2, ay2, az2, gx2, gy2, gz2;

    // Burst read all 6 axes from each sensor
    imuShoulder.readAllRaw(gx1, gy1, gz1, ax1, ay1, az1);
    imuWrist.readAllRaw(gx2, gy2, gz2, ax2, ay2, az2);

    // Convert to physical units
    float axf1 = ax1 * ACCEL_SENSITIVITY;
    float ayf1 = ay1 * ACCEL_SENSITIVITY;
    float azf1 = az1 * ACCEL_SENSITIVITY;
    float gxf1 = gx1 * GYRO_SENSITIVITY;
    float gyf1 = gy1 * GYRO_SENSITIVITY;
    float gzf1 = gz1 * GYRO_SENSITIVITY;

    float axf2 = ax2 * ACCEL_SENSITIVITY;
    float ayf2 = ay2 * ACCEL_SENSITIVITY;
    float azf2 = az2 * ACCEL_SENSITIVITY;
    float gxf2 = gx2 * GYRO_SENSITIVITY;
    float gyf2 = gy2 * GYRO_SENSITIVITY;
    float gzf2 = gz2 * GYRO_SENSITIVITY;

    // Print formatted output
    Serial.printf("  %+6.2f %+6.2f %+6.2f %+7.1f %+7.1f %+7.1f | "
                  "%+6.2f %+6.2f %+6.2f %+7.1f %+7.1f %+7.1f\n",
                  axf1, ayf1, azf1, gxf1, gyf1, gzf1,
                  axf2, ayf2, azf2, gxf2, gyf2, gzf2);
}
