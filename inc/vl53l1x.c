#include "inc/vl53l1x.h"
#include "pico/stdlib.h"

static bool write_addr(i2c_inst_t *i2c, uint8_t addr, const uint8_t *buf, size_t len) {
    int ret = i2c_write_blocking(i2c, addr, buf, len, false);
    return ret == (int)len;
}

static bool write_reg8(i2c_inst_t *i2c, uint8_t addr, uint16_t reg, uint8_t val) {
    uint8_t buf[3] = { (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF), val };
    return write_addr(i2c, addr, buf, 3);
}

static bool write_reg16(i2c_inst_t *i2c, uint8_t addr, uint16_t reg, uint16_t val) {
    uint8_t buf[4] = { (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF), (uint8_t)(val >> 8), (uint8_t)(val & 0xFF) };
    return write_addr(i2c, addr, buf, 4);
}

static bool write_reg32(i2c_inst_t *i2c, uint8_t addr, uint16_t reg, uint32_t val) {
    uint8_t buf[6] = { (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF),
                       (uint8_t)(val >> 24), (uint8_t)(val >> 16), (uint8_t)(val >> 8), (uint8_t)(val & 0xFF) };
    return write_addr(i2c, addr, buf, 6);
}

static bool read_reg(i2c_inst_t *i2c, uint8_t addr, uint16_t reg, uint8_t *buf, size_t len) {
    uint8_t regbuf[2] = { (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF) };
    int wr = i2c_write_blocking(i2c, addr, regbuf, 2, true);
    if (wr != 2) return false;
    int rd = i2c_read_blocking(i2c, addr, buf, len, false);
    return rd == (int)len;
}

bool vl53l1x_write8(i2c_inst_t *i2c, uint8_t addr, uint16_t reg, uint8_t val) { return write_reg8(i2c, addr, reg, val); }
bool vl53l1x_write16(i2c_inst_t *i2c, uint8_t addr, uint16_t reg, uint16_t val) { return write_reg16(i2c, addr, reg, val); }
bool vl53l1x_write32(i2c_inst_t *i2c, uint8_t addr, uint16_t reg, uint32_t val) { return write_reg32(i2c, addr, reg, val); }

bool vl53l1x_read8(i2c_inst_t *i2c, uint8_t addr, uint16_t reg, uint8_t *val) {
    return read_reg(i2c, addr, reg, val, 1);
}

bool vl53l1x_read16(i2c_inst_t *i2c, uint8_t addr, uint16_t reg, uint16_t *val) {
    uint8_t buf[2];
    if (!read_reg(i2c, addr, reg, buf, 2)) return false;
    *val = ((uint16_t)buf[0] << 8) | buf[1];
    return true;
}

// Cached oscillator values used for timing
static uint16_t s_fast_osc_frequency = 0;
static uint16_t s_osc_calibrate_val = 0;

// Encode sequence step timeout from macro periods to register value
// based on VL53L1_encode_timeout()
static uint16_t encode_timeout(uint32_t timeout_mclks) {
    // (LSByte * 2^MSByte) + 1 = timeout_mclks
    if (timeout_mclks == 0) return 0;
    uint32_t ls_byte = timeout_mclks - 1;
    uint16_t ms_byte = 0;
    while ((ls_byte & 0xFFFFFF00u) > 0) {
        ls_byte >>= 1;
        ms_byte++;
    }
    return (uint16_t)((ms_byte << 8) | (ls_byte & 0xFF));
}

// Calculate macro period in microseconds (12.12 format) with given VCSEL period
// based on VL53L1_calc_macro_period_us()
static uint32_t calc_macro_period_us(uint8_t vcsel_period) {
    if (s_fast_osc_frequency == 0) return 0;
    uint32_t pll_period_us = ((uint32_t)1 << 30) / s_fast_osc_frequency; // 0.24 format
    uint8_t vcsel_period_pclks = (vcsel_period + 1) << 1;
    uint32_t macro_period_us = (uint32_t)2304 * pll_period_us; // 12.12 format scaling
    macro_period_us >>= 6;
    macro_period_us *= vcsel_period_pclks;
    macro_period_us >>= 6;
    return macro_period_us;
}

// Convert timeout from microseconds to macro periods (12.12 format)
static uint32_t timeout_us_to_mclks(uint32_t timeout_us, uint32_t macro_period_us) {
    // from VL53L1_calc_timeout_mclks
    return (uint32_t)(((uint64_t)timeout_us << 12) + (macro_period_us >> 1)) / macro_period_us;
}

bool vl53l1x_init(i2c_inst_t *i2c, uint8_t addr) {
    // Soft reset
    if (!vl53l1x_write8(i2c, addr, VL53L1X_SOFT_RESET, 0x00)) return false;
    sleep_ms(10);
    if (!vl53l1x_write8(i2c, addr, VL53L1X_SOFT_RESET, 0x01)) return false;
    sleep_ms(10);

    // Clear any pending interrupt
    if (!vl53l1x_write8(i2c, addr, VL53L1X_SYSTEM__INTERRUPT_CLEAR, 0x01)) return false;

    // Read oscillator data used for timing scaling
    if (!vl53l1x_read16(i2c, addr, VL53L1X_OSC_MEASURED__FAST_OSC__FREQUENCY, &s_fast_osc_frequency)) return false;
    if (!vl53l1x_read16(i2c, addr, VL53L1X_RESULT__OSC_CALIBRATE_VAL, &s_osc_calibrate_val)) return false;

    // Grouped parameter hold while writing configuration
    if (!vl53l1x_write8(i2c, addr, VL53L1X_SYSTEM__GROUPED_PARAMETER_HOLD_0, 0x01)) return false;
    if (!vl53l1x_write8(i2c, addr, VL53L1X_SYSTEM__GROUPED_PARAMETER_HOLD_1, 0x01)) return false;

    // Seed config and sequence config (low power auto sequence: VHV, PHASECAL, DSS1, RANGE)
    if (!vl53l1x_write8(i2c, addr, VL53L1X_SYSTEM__SEED_CONFIG, 0x01)) return false;

    // Distance mode: Short preset (per Pololu/ST ULD)
    if (!vl53l1x_write8(i2c, addr, VL53L1X_RANGE_CONFIG__VCSEL_PERIOD_A, 0x07)) return false;
    if (!vl53l1x_write8(i2c, addr, VL53L1X_RANGE_CONFIG__VCSEL_PERIOD_B, 0x05)) return false;
    if (!vl53l1x_write8(i2c, addr, VL53L1X_RANGE_CONFIG__VALID_PHASE_HIGH, 0x38)) return false;
    if (!vl53l1x_write8(i2c, addr, VL53L1X_SD_CONFIG__WOI_SD0, 0x07)) return false;
    if (!vl53l1x_write8(i2c, addr, VL53L1X_SD_CONFIG__WOI_SD1, 0x05)) return false;
    if (!vl53l1x_write8(i2c, addr, VL53L1X_SD_CONFIG__INITIAL_PHASE_SD0, 6)) return false;
    if (!vl53l1x_write8(i2c, addr, VL53L1X_SD_CONFIG__INITIAL_PHASE_SD1, 6)) return false;

    // Release grouped parameter hold
    if (!vl53l1x_write8(i2c, addr, VL53L1X_SYSTEM__GROUPED_PARAMETER_HOLD_0, 0x00)) return false;
    if (!vl53l1x_write8(i2c, addr, VL53L1X_SYSTEM__GROUPED_PARAMETER_HOLD_1, 0x00)) return false;

    // Apply a minimum timing budget for Short mode (20 ms)
    uint8_t vcsel_a = 0x07;
    uint8_t vcsel_b = 0x05;
    uint32_t macro_a = calc_macro_period_us(vcsel_a);
    uint32_t macro_b = calc_macro_period_us(vcsel_b);
    if (macro_a == 0 || macro_b == 0) return false;
    uint32_t budget_us = 20000; // 20 ms minimum for Short
    // Range timeout A/B are each ~budget/2 minus guard; we use a simple split
    uint32_t timeout_a_us = (budget_us / 2);
    uint32_t timeout_b_us = (budget_us / 2);
    uint32_t mclks_a = timeout_us_to_mclks(timeout_a_us, macro_a);
    uint32_t mclks_b = timeout_us_to_mclks(timeout_b_us, macro_b);
    uint16_t enc_a = encode_timeout(mclks_a);
    uint16_t enc_b = encode_timeout(mclks_b);
    if (!vl53l1x_write16(i2c, addr, VL53L1X_RANGE_CONFIG__TIMEOUT_MACROP_A, enc_a)) return false;
    if (!vl53l1x_write16(i2c, addr, VL53L1X_RANGE_CONFIG__TIMEOUT_MACROP_B, enc_b)) return false;

    // Set inter-measurement period to 50 ms scaled by osc_calibrate_val in start_continuous

    return true;
}

bool vl53l1x_start_continuous(i2c_inst_t *i2c, uint8_t addr, uint32_t period_ms) {
    // Update period and start timed ranging
    uint32_t scaled = (s_osc_calibrate_val == 0) ? period_ms : (period_ms * (uint32_t)s_osc_calibrate_val);
    if (!vl53l1x_write32(i2c, addr, VL53L1X_SYSTEM__INTERMEASUREMENT_PERIOD, scaled)) return false;
    if (!vl53l1x_write8(i2c, addr, VL53L1X_SYSTEM__INTERRUPT_CLEAR, 0x01)) return false;
    if (!vl53l1x_write8(i2c, addr, VL53L1X_SYSTEM__MODE_START, 0x40)) return false; // timed mode
    return true;
}

bool vl53l1x_data_ready(i2c_inst_t *i2c, uint8_t addr) {
    uint8_t isr = 0;
    if (!vl53l1x_read8(i2c, addr, VL53L1X_RESULT__INTERRUPT_STATUS, &isr)) return false;
    // bit0 == 1 indicates new range
    return (isr & 0x01) != 0;
}

bool vl53l1x_read_distance_mm(i2c_inst_t *i2c, uint8_t addr, uint16_t *out_mm) {
    uint16_t raw = 0;
    if (!vl53l1x_read16(i2c, addr, VL53L1X_RESULT__FINAL_RANGE_MM_SD0, &raw)) return false;
    *out_mm = raw; // raw is already in mm for SD0
    // clear interrupt for next measurement
    vl53l1x_write8(i2c, addr, VL53L1X_SYSTEM__INTERRUPT_CLEAR, 0x01);
    return true;
}

bool vl53l1x_read_range_block(i2c_inst_t *i2c, uint8_t addr, uint16_t *out_mm, uint8_t *out_status, uint8_t *out_stream) {
    // Read 17 bytes starting at RESULT__RANGE_STATUS (0x0089)
    uint8_t regbuf[2] = { (uint8_t)(VL53L1X_RESULT__RANGE_STATUS >> 8), (uint8_t)(VL53L1X_RESULT__RANGE_STATUS & 0xFF) };
    int wr = i2c_write_blocking(i2c, addr, regbuf, 2, true);
    if (wr != 2) return false;
    uint8_t buf[17];
    int rd = i2c_read_blocking(i2c, addr, buf, sizeof buf, false);
    if (rd != (int)sizeof buf) return false;

    uint8_t range_status = buf[0];
    uint8_t stream_count = buf[2];
    uint16_t final_range = ((uint16_t)buf[13] << 8) | buf[14];

    // Apply gain factor ~2011/2048 as in Pololu
    uint16_t scaled = (uint32_t)final_range * 2011u;
    scaled = (scaled + 0x0400u) >> 11; // divide by 2048 with rounding

    if (out_status) *out_status = range_status;
    if (out_stream) *out_stream = stream_count;
    if (out_mm) *out_mm = scaled;

    // clear interrupt
    vl53l1x_write8(i2c, addr, VL53L1X_SYSTEM__INTERRUPT_CLEAR, 0x01);
    return true;
}
