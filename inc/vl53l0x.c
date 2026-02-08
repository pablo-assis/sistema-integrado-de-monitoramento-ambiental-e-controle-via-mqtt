#include <stdio.h>
#include "vl53l0x.h"

static bool read_reg8(i2c_inst_t *i2c, uint8_t addr, uint8_t reg, uint8_t *val)
{
    int wr = i2c_write_blocking(i2c, addr, &reg, 1, true);
    if (wr != 1) return false;
    int rd = i2c_read_blocking(i2c, addr, val, 1, false);
    return rd == 1;
}

static bool write_reg8(i2c_inst_t *i2c, uint8_t addr, uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    int wr = i2c_write_blocking(i2c, addr, buf, 2, false);
    return wr == 2;
}

bool vl53l0x_init(i2c_inst_t *i2c)
{
    uint8_t id = 0;
    if (!read_reg8(i2c, VL53L0X_I2C_ADDR, VL53L0X_REG_IDENTIFICATION_MODEL_ID, &id))
        return false;
    // Expected model ID for VL53L0X is 0xEE
    if (id != 0xEE)
        return false;
    return true;
}

bool vl53l0x_read_distance_mm(i2c_inst_t *i2c, uint16_t *out_mm)
{
    if (!out_mm) return false;

    // Start single measurement
    if (!write_reg8(i2c, VL53L0X_I2C_ADDR, VL53L0X_REG_SYSRANGE_START, 0x01))
        return false;

    // Wait for result ready (poll status bit 0)
    for (int i = 0; i < 100; i++) {
        uint8_t status = 0;
        if (!read_reg8(i2c, VL53L0X_I2C_ADDR, VL53L0X_REG_RESULT_RANGE_STATUS, &status))
            return false;
        if (status & 0x01) break;
        sleep_ms(5);
    }

    // Read 2 bytes distance (big-endian)
    uint8_t reg = VL53L0X_REG_RESULT_RANGE_MM;
    uint8_t buffer[2] = {0};
    int wr = i2c_write_blocking(i2c, VL53L0X_I2C_ADDR, &reg, 1, true);
    if (wr != 1) return false;
    int rd = i2c_read_blocking(i2c, VL53L0X_I2C_ADDR, buffer, 2, false);
    if (rd != 2) return false;

    *out_mm = (uint16_t)((buffer[0] << 8) | buffer[1]);
    return true;
}
