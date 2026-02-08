#include "ssd1306.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64

static i2c_inst_t *g_i2c = NULL;
static uint8_t g_addr = 0x3C; // padrão
static uint8_t fb[SSD1306_WIDTH * SSD1306_HEIGHT / 8];

static inline int clamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

static void ssd1306_write_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};
    i2c_write_blocking(g_i2c, g_addr, buf, 2, false);
}

static void ssd1306_write_data(const uint8_t *data, size_t len) {
    // Envia em blocos com control byte 0x40
    const size_t chunk = 16;
    uint8_t out[1 + chunk];
    out[0] = 0x40;
    while (len) {
        size_t n = len > chunk ? chunk : len;
        for (size_t i = 0; i < n; ++i) out[1 + i] = data[i];
        i2c_write_blocking(g_i2c, g_addr, out, 1 + n, false);
        data += n;
        len -= n;
    }
}

bool ssd1306_init(i2c_inst_t *i2c, uint8_t addr) {
    g_i2c = i2c;
    g_addr = addr;

    // Init sequence (internal VCC, horizontal addressing)
    ssd1306_write_cmd(0xAE); // display off
    ssd1306_write_cmd(0xD5); ssd1306_write_cmd(0x80); // clock
    ssd1306_write_cmd(0xA8); ssd1306_write_cmd(0x3F); // multiplex 1/64
    ssd1306_write_cmd(0xD3); ssd1306_write_cmd(0x00); // display offset
    ssd1306_write_cmd(0x40); // start line = 0
    ssd1306_write_cmd(0x8D); ssd1306_write_cmd(0x14); // charge pump on
    ssd1306_write_cmd(0x20); ssd1306_write_cmd(0x00); // memory mode horizontal
    ssd1306_write_cmd(0xA1); // segment remap
    ssd1306_write_cmd(0xC8); // COM scan dec
    ssd1306_write_cmd(0xDA); ssd1306_write_cmd(0x12); // compins
    ssd1306_write_cmd(0x81); ssd1306_write_cmd(0xCF); // contrast
    ssd1306_write_cmd(0xD9); ssd1306_write_cmd(0xF1); // precharge
    ssd1306_write_cmd(0xDB); ssd1306_write_cmd(0x40); // vcom detect
    ssd1306_write_cmd(0xA4); // resume RAM
    ssd1306_write_cmd(0xA6); // normal display
    ssd1306_write_cmd(0x2E); // deactivate scroll

    // set column/page range
    ssd1306_write_cmd(0x21); ssd1306_write_cmd(0x00); ssd1306_write_cmd(SSD1306_WIDTH - 1);
    ssd1306_write_cmd(0x22); ssd1306_write_cmd(0x00); ssd1306_write_cmd((SSD1306_HEIGHT/8) - 1);

    ssd1306_clear();
    ssd1306_update();
    ssd1306_write_cmd(0xAF); // display on
    return true;
}

void ssd1306_clear(void) {
    for (size_t i = 0; i < sizeof(fb); ++i) fb[i] = 0x00;
}

void ssd1306_update(void) {
    ssd1306_write_cmd(0x21); ssd1306_write_cmd(0x00); ssd1306_write_cmd(SSD1306_WIDTH - 1);
    ssd1306_write_cmd(0x22); ssd1306_write_cmd(0x00); ssd1306_write_cmd((SSD1306_HEIGHT/8) - 1);
    ssd1306_write_data(fb, sizeof(fb));
}

void ssd1306_set_pixel(int x, int y, bool on) {
    if (x < 0 || x >= SSD1306_WIDTH || y < 0 || y >= SSD1306_HEIGHT) return;
    int page = y >> 3;
    int bit = y & 7;
    size_t idx = page * SSD1306_WIDTH + x;
    if (on) fb[idx] |= (1u << bit); else fb[idx] &= ~(1u << bit);
}

void ssd1306_draw_line(int x0, int y0, int x1, int y1, bool on) {
    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int sx = (x0 < x1) ? 1 : -1;
    int dy = -((y1 > y0) ? (y1 - y0) : (y0 - y1));
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;
    while (true) {
        ssd1306_set_pixel(x0, y0, on);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void ssd1306_draw_circle(int cx, int cy, int r, bool on) {
    int x = r, y = 0;
    int err = 0;
    while (x >= y) {
        ssd1306_set_pixel(cx + x, cy + y, on);
        ssd1306_set_pixel(cx + y, cy + x, on);
        ssd1306_set_pixel(cx - y, cy + x, on);
        ssd1306_set_pixel(cx - x, cy + y, on);
        ssd1306_set_pixel(cx - x, cy - y, on);
        ssd1306_set_pixel(cx - y, cy - x, on);
        ssd1306_set_pixel(cx + y, cy - x, on);
        ssd1306_set_pixel(cx + x, cy - y, on);
        y++;
        if (err <= 0) { err += 2*y + 1; } else { x--; err -= 2*x + 1; }
    }
}

// Desenha bitmap monocromático: bytes em ordem horizontal, 8 pixels por byte (MSB primeiro)
void ssd1306_draw_bitmap(int x, int y, const uint8_t *bmp, int w, int h, bool white) {
    if (!bmp || w <= 0 || h <= 0) return;
    int x0 = x;
    int y0 = y;
    int bytes_per_row = (w + 7) / 8;
    for (int row = 0; row < h; ++row) {
        const uint8_t *row_ptr = bmp + row * bytes_per_row;
        for (int col = 0; col < w; ++col) {
            int byte_index = col >> 3;             // col / 8
            int bit_index = 7 - (col & 7);         // MSB primeiro
            uint8_t b = row_ptr[byte_index];
            bool on = ((b >> bit_index) & 0x1) ? white : !white;
            ssd1306_set_pixel(x0 + col, y0 + row, on);
        }
    }
}
