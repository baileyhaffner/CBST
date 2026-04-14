// =============================================================================
// CBST - Cricket Bowling Sensor Tracking
// Pin Definitions (derived from Schematic Rev 1A, 3/12/2026)
// =============================================================================
//
// IMPORTANT: Verify these pin assignments against your breadboard wiring.
// The values below are traced from the schematic drawn by Connor & Bailey.
// If any pin does not match your physical setup, update it here — every 
// other file references this header, so a single change propagates everywhere.
// =============================================================================

#ifndef CBST_PINS_H
#define CBST_PINS_H

// ---- SPI Bus (shared between both IMUs) ----
// These three lines form the shared SPI bus. Both IMUs share SCLK, MOSI 
// and MISO, and are individually selected via their CS pins below.
#define PIN_SPI_SCLK    36    // IO36 → SCX (SPI Clock)
#define PIN_SPI_MOSI    35    // IO35 → SDX (SPI Data In to IMU)
#define PIN_SPI_MISO    37    // IO37 → SDO/SA0 (SPI Data Out from IMU)

// ---- IMU Chip Select Lines ----
// Each IMU has its own CS line so they can be addressed independently.
// Active LOW — pull LOW to select, HIGH to deselect.
#define PIN_CS_IMU1     7    // IO7 → IMU1 CS (shoulder sensor)
#define PIN_CS_IMU2     6    // IO6 → IMU2 CS (wrist sensor)

// ---- Push Button (ball-release) ----
// Connected to IO6 with a 10k pull-down resistor to GND.
// Reads HIGH when pressed, LOW when released.
#define PIN_BUTTON      42     // IO42 → Push button

// ---- Motor Driver (TC78H660FTG) — reserved for Block 6 ----
// Digital motor signal lines from schematic (IO1–IO5 region)
// These are defined here for reference but not used in Block 1.
#define PIN_MOTOR_IN1A  1     // IO1 → IN1A / PHASE_A
#define PIN_MOTOR_IN2A  2     // IO2 → IN2A / PHASE_B
#define PIN_MOTOR_IN1B  3     // IO3 → IN1B / ENABLE_A
#define PIN_MOTOR_IN2B  4     // IO4 → IN2B / ENABLE_B
#define PIN_MOTOR_STBY  5     // IO5 → STBY (standby control)

// ---- SPI Bus Configuration ----
// The LSM6DSOX supports SPI clock speeds up to 10 MHz.
// Start conservatively at 1 MHz for initial bring-up, then increase
// once communication is verified stable.
#define SPI_CLOCK_HZ    1000000   // 1 MHz (increase to 4-8 MHz once stable)

#endif // CBST_PINS_H
