#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"

#define VL53L1X_I2C_ADDR 0x29

// Selected register addresses (16-bit) adapted from public docs/libraries
#define VL53L1X_SOFT_RESET                 0x0000
#define VL53L1X_OSC_MEASURED__FAST_OSC__FREQUENCY 0x0006
#define VL53L1X_SYSTEM__INTERRUPT_CLEAR    0x0086
#define VL53L1X_SYSTEM__MODE_START         0x0087
#define VL53L1X_RESULT__INTERRUPT_STATUS   0x0088
#define VL53L1X_RESULT__RANGE_STATUS       0x0089
#define VL53L1X_RESULT__STREAM_COUNT       0x008B
#define VL53L1X_RESULT__FINAL_RANGE_MM_SD0 0x0096
#define VL53L1X_SYSTEM__INTERMEASUREMENT_PERIOD 0x006C
#define VL53L1X_RESULT__OSC_CALIBRATE_VAL  0x00DE

// System/grouped parameter hold and seed config
#define VL53L1X_SYSTEM__GROUPED_PARAMETER_HOLD_0 0x0071
#define VL53L1X_SYSTEM__GROUPED_PARAMETER_HOLD_1 0x007C
#define VL53L1X_SYSTEM__SEED_CONFIG             0x0077

// Dynamic config (Short/Medium/Long presets)
#define VL53L1X_SD_CONFIG__WOI_SD0              0x0078
#define VL53L1X_SD_CONFIG__WOI_SD1              0x0079
#define VL53L1X_SD_CONFIG__INITIAL_PHASE_SD0    0x007A
#define VL53L1X_SD_CONFIG__INITIAL_PHASE_SD1    0x007B

// Timing config for VCSEL periods and thresholds
#define VL53L1X_RANGE_CONFIG__VCSEL_PERIOD_A    0x0060
#define VL53L1X_RANGE_CONFIG__VCSEL_PERIOD_B    0x0063
#define VL53L1X_RANGE_CONFIG__VALID_PHASE_HIGH  0x0069
#define VL53L1X_RANGE_CONFIG__SIGMA_THRESH      0x0064
#define VL53L1X_RANGE_CONFIG__MIN_COUNT_RATE_RTN_LIMIT_MCPS 0x0066

// Timeout registers (16-bit encoded timeouts)
#define VL53L1X_RANGE_CONFIG__TIMEOUT_MACROP_A  0x005F
#define VL53L1X_RANGE_CONFIG__TIMEOUT_MACROP_B  0x0061

#ifdef __cplusplus
extern "C" {
#endif

bool vl53l1x_write8(i2c_inst_t *i2c, uint8_t addr, uint16_t reg, uint8_t val);
bool vl53l1x_write16(i2c_inst_t *i2c, uint8_t addr, uint16_t reg, uint16_t val);
bool vl53l1x_write32(i2c_inst_t *i2c, uint8_t addr, uint16_t reg, uint32_t val);
bool vl53l1x_read8(i2c_inst_t *i2c, uint8_t addr, uint16_t reg, uint8_t *val);
bool vl53l1x_read16(i2c_inst_t *i2c, uint8_t addr, uint16_t reg, uint16_t *val);

bool vl53l1x_init(i2c_inst_t *i2c, uint8_t addr);
bool vl53l1x_start_continuous(i2c_inst_t *i2c, uint8_t addr, uint32_t period_ms);
bool vl53l1x_data_ready(i2c_inst_t *i2c, uint8_t addr);
bool vl53l1x_read_distance_mm(i2c_inst_t *i2c, uint8_t addr, uint16_t *out_mm);
bool vl53l1x_read_range_block(i2c_inst_t *i2c, uint8_t addr, uint16_t *out_mm, uint8_t *out_status, uint8_t *out_stream);

#ifdef __cplusplus
}
#endif
