#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "hardware/i2c.h"

#define MAX30101_I2C_ADDR 0x57

// Minimal register map
#define MAX30101_REG_INT_STATUS1   0x00
#define MAX30101_REG_INT_STATUS2   0x01
#define MAX30101_REG_INT_ENABLE1   0x02
#define MAX30101_REG_INT_ENABLE2   0x03
#define MAX30101_REG_FIFO_WR_PTR   0x04
#define MAX30101_REG_OVF_COUNTER   0x05
#define MAX30101_REG_FIFO_RD_PTR   0x06
#define MAX30101_REG_FIFO_DATA     0x07
#define MAX30101_REG_FIFO_CONFIG   0x08
#define MAX30101_REG_MODE_CONFIG   0x09
#define MAX30101_REG_SPO2_CONFIG   0x0A
#define MAX30101_REG_LED1_PA       0x0C // RED
#define MAX30101_REG_LED2_PA       0x0D // IR
#define MAX30101_REG_PART_ID       0xFF

#ifdef __cplusplus
extern "C" {
#endif

bool max30101_init(i2c_inst_t *i2c, uint8_t addr);
bool max30101_read_part_id(i2c_inst_t *i2c, uint8_t addr, uint8_t *out_id);
bool max30101_read_ir(i2c_inst_t *i2c, uint8_t addr, uint32_t *ir_out);

#ifdef __cplusplus
}
#endif
