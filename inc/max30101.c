#include "inc/max30101.h"
#include "pico/stdlib.h"

static bool write_reg(i2c_inst_t *i2c, uint8_t addr, uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    int ret = i2c_write_blocking(i2c, addr, buf, 2, false);
    return ret == 2;
}

static bool read_reg(i2c_inst_t *i2c, uint8_t addr, uint8_t reg, uint8_t *val) {
    int ret = i2c_write_blocking(i2c, addr, &reg, 1, true);
    if (ret != 1) return false;
    ret = i2c_read_blocking(i2c, addr, val, 1, false);
    return ret == 1;
}

static bool read_regs(i2c_inst_t *i2c, uint8_t addr, uint8_t reg, uint8_t *buf, size_t len) {
    int ret = i2c_write_blocking(i2c, addr, &reg, 1, true);
    if (ret != 1) return false;
    ret = i2c_read_blocking(i2c, addr, buf, len, false);
    return ret == (int)len;
}

bool max30101_read_part_id(i2c_inst_t *i2c, uint8_t addr, uint8_t *out_id) {
    return read_reg(i2c, addr, MAX30101_REG_PART_ID, out_id);
}

bool max30101_init(i2c_inst_t *i2c, uint8_t addr) {
    // Reset device
    if (!write_reg(i2c, addr, MAX30101_REG_MODE_CONFIG, 0x40)) return false;
    sleep_ms(10);

    // Clear FIFO pointers
    if (!write_reg(i2c, addr, MAX30101_REG_FIFO_WR_PTR, 0x00)) return false;
    if (!write_reg(i2c, addr, MAX30101_REG_OVF_COUNTER, 0x00)) return false;
    if (!write_reg(i2c, addr, MAX30101_REG_FIFO_RD_PTR, 0x00)) return false;

    // FIFO config: sample avg = 4 (0b010 << 5), rollover disabled, almost full = 0x0F
    if (!write_reg(i2c, addr, MAX30101_REG_FIFO_CONFIG, (0x02 << 5) | 0x0F)) return false;

    // SPO2 config: ADC range 4096nA (0x3 << 5), SR=100Hz (0x3 << 2), LED_PW=411us/18-bit (0x3)
    if (!write_reg(i2c, addr, MAX30101_REG_SPO2_CONFIG, (0x3 << 5) | (0x3 << 2) | 0x3)) return false;

    // LED currents (tune as needed)
    if (!write_reg(i2c, addr, MAX30101_REG_LED1_PA, 0x24)) return false; // RED
    if (!write_reg(i2c, addr, MAX30101_REG_LED2_PA, 0x24)) return false; // IR

    // Mode: SpO2 (RED+IR)
    if (!write_reg(i2c, addr, MAX30101_REG_MODE_CONFIG, 0x03)) return false;

    return true;
}

bool max30101_read_ir(i2c_inst_t *i2c, uint8_t addr, uint32_t *ir_out) {
    // Read one pair (RED, IR) = 6 bytes
    uint8_t data[6];
    if (!read_regs(i2c, addr, MAX30101_REG_FIFO_DATA, data, sizeof data)) return false;

    // IR is bytes 3..5 in SpO2 mode
    uint32_t ir = ((uint32_t)(data[3] & 0x03) << 16) | ((uint32_t)data[4] << 8) | data[5];
    *ir_out = ir;
    return true;
}
