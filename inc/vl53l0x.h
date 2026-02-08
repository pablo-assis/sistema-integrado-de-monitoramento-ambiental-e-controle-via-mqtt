#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "hardware/i2c.h"

#define VL53L0X_I2C_ADDR 0x29

// VL53L0X 8-bit register addresses
#define VL53L0X_REG_IDENTIFICATION_MODEL_ID   0xC0
#define VL53L0X_REG_SYSRANGE_START            0x00
#define VL53L0X_REG_RESULT_RANGE_STATUS       0x14
#define VL53L0X_REG_RESULT_RANGE_MM           0x1E

#ifdef __cplusplus
extern "C" {
#endif

bool vl53l0x_init(i2c_inst_t *i2c);
bool vl53l0x_read_distance_mm(i2c_inst_t *i2c, uint16_t *out_mm);

#ifdef __cplusplus
}
#endif
