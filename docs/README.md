# CBST Project Documents

Read this file first to determine which documents are relevant to your current task.

---

## Documents

### CBST_AI_Outline.pdf
**Read when:** You need to understand the overall project — goals, system architecture, hardware selection rationale, accuracy requirements, or the full scope of what is being built.
**Contains:** Project brief, hardware BOM, software architecture, metric definitions (run-up speed, release angle, release speed), success criteria, and budget constraints.

### LSM6DSOX.pdf
**Read when:** You are writing or debugging IMU code — initialisation, register configuration, SPI communication, data rate selection, accelerometer/gyroscope range settings, or interrupt configuration.
**Contains:** Full datasheet for the LSM6DSOX 6-DoF IMU. Includes register maps, electrical characteristics, SPI/I2C timing, and FIFO configuration.

### ESP32 Dataset.pdf
**Read when:** You are working with low-level ESP32-S3 peripherals — SPI, I2C, UART, GPIO, ADC, timers, PWM (LEDC), power modes, or memory layout.
**Contains:** Technical reference data for the ESP32-S3 SoC. Use alongside the PlatformIO board definition for the UM ProS3.

### schematic-pros3d-p1.pdf
**Read when:** You need to verify pin assignments, check power rail connections, or understand how the UM ProS3D board routes its peripherals.
**Contains:** Full schematic for the UM ProS3D development board. Cross-reference with `cbst_pins.h` when assigning GPIO pins.

---

## Quick Reference

| Task | Read |
|---|---|
| Understanding project goals or requirements | CBST_AI_Outline.pdf |
| IMU driver, SPI config, register access | LSM6DSOX.pdf |
| ESP32 peripheral config (SPI, PWM, timers) | ESP32 Dataset.pdf |
| Pin assignments, power rails | schematic-pros3d-p1.pdf |
| All of the above | Start with CBST_AI_Outline.pdf, then the relevant datasheet |
