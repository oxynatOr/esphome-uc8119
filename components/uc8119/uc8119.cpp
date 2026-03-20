#include "uc8119.h"
#include "esphome/core/log.h"
#include <cstring>

// RTC memory survives deep sleep but not power cycle
#ifdef USE_ESP32
static RTC_DATA_ATTR uint32_t rtc_magic;
static RTC_DATA_ATTR uint8_t  rtc_framebuffer[18];  // 17 data + 1 padding
#endif
static const uint32_t RTC_MAGIC_VALUE = 0x8119CAFE;

namespace esphome {
namespace uc8119 {

static const char *const TAG = "uc8119";

// LUT tables (captured from Shelly H&T Gen3 firmware)
static const uint8_t LUT_CLEAR_VCOM[] = {0x5E,0xBC,0x01,0x9E,0x7C,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const uint8_t LUT_CLEAR_WB[]   = {0x9E,0xBC,0x01,0x5E,0x7C,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const uint8_t LUT_CLEAR_BD[]   = {0x9E,0x7C,0x01,0x5E,0xBC,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const uint8_t LUT_NORM_VCOM[]  = {0x68,0xA8,0x01,0x81,0x00,0x01,0x41,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00};
static const uint8_t LUT_NORM_WB[]    = {0x68,0xA8,0x01,0x41,0x00,0x01,0x41,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00};
static const uint8_t LUT_NORM_BB[]    = {0xA8,0xA8,0x01,0x81,0x00,0x01,0x41,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00};
static const uint8_t LUT_NORM_3[]     = {0x68,0x68,0x01,0x81,0x00,0x01,0x41,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00};
static const uint8_t LUT_NORM_BD[]    = {0x68,0xA8,0x01,0x81,0x00,0x01,0x81,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00};

// ── I2C ─────────────────────────────────────────────────────────

void UC8119::write_register_(uint8_t reg) { this->write(&reg, 1); }
void UC8119::write_register_(uint8_t reg, uint8_t val) { this->write_byte(reg, val); }
void UC8119::write_register_(uint8_t reg, const uint8_t *d, uint8_t len) { this->write_bytes(reg, d, len); }

// ── Hardware ────────────────────────────────────────────────────

void UC8119::hardware_reset_() {
  if (!this->reset_pin_) return;
  this->reset_pin_->digital_write(false); delay(10);
  this->reset_pin_->digital_write(true);  delay(1);
}

void UC8119::wait_busy_(uint32_t timeout_ms) {
  if (!this->busy_pin_) { delay(2000); return; }
  uint32_t start = millis();
  while (!this->busy_pin_->digital_read()) {
    if (millis() - start > timeout_ms) { ESP_LOGW(TAG, "BUSY timeout"); return; }
    delay(10);
  }
}

// ── RTC memory ──────────────────────────────────────────────────

void UC8119::save_to_rtc_() {
#ifdef USE_ESP32
  memcpy(rtc_framebuffer, this->committed_fb_, FB_DATA_SIZE);
  rtc_magic = RTC_MAGIC_VALUE;
  ESP_LOGD(TAG, "FB saved to RTC");
#endif
}

bool UC8119::load_from_rtc_() {
#ifdef USE_ESP32
  if (rtc_magic != RTC_MAGIC_VALUE) return false;
  memcpy(this->committed_fb_, rtc_framebuffer, FB_DATA_SIZE);
  ESP_LOGD(TAG, "FB restored from RTC");
  return true;
#else
  return false;  // No RTC memory — always full refresh
#endif
}

// ── Protocol sequences ──────────────────────────────────────────

void UC8119::send_config_preamble_() {
  this->write_register_(REG_PON); this->wait_busy_();
  this->write_register_(REG_PSR, 0x0F);
  uint8_t pwr[] = {0x46, 0x46};
  this->write_register_(REG_PWR, pwr, 2);
  this->write_register_(REG_PLL, 0x07);
}

void UC8119::send_clear_luts_() {
  this->write_register_(REG_LUTC, LUT_CLEAR_VCOM, LUT_SIZE);
  this->write_register_(REG_LUTWB, LUT_CLEAR_WB, LUT_SIZE);
  this->write_register_(REG_LUTBD, LUT_CLEAR_BD, LUT_SIZE);
}

void UC8119::send_normal_luts_() {
  this->write_register_(REG_LUTC, LUT_NORM_VCOM, LUT_SIZE);
  this->write_register_(REG_LUTWB, LUT_NORM_WB, LUT_SIZE);
  this->write_register_(REG_LUTBB, LUT_NORM_BB, LUT_SIZE);
  this->write_register_(REG_LUT3, LUT_NORM_3, LUT_SIZE);
  this->write_register_(REG_LUTBD, LUT_NORM_BD, LUT_SIZE);
}

void UC8119::send_framebuffer_with_terminator_(uint8_t reg, const uint8_t *fb) {
  uint8_t buf[FB_TOTAL_SIZE];
  memcpy(buf, fb, FB_DATA_SIZE);
  buf[FB_DATA_SIZE] = 0x80;
  this->write_bytes(reg, buf, FB_TOTAL_SIZE);
}

void UC8119::do_full_refresh_() {
  ESP_LOGI(TAG, "Full refresh");
  uint8_t empty[FB_DATA_SIZE]; memset(empty, 0xFF, FB_DATA_SIZE);

  // Phase 1: Clear
  this->send_config_preamble_();
  this->write_register_(REG_PFS, 0x00);
  this->send_clear_luts_();
  this->send_framebuffer_with_terminator_(REG_FB, empty);
  this->write_register_(REG_DRF); this->wait_busy_(); this->pof_cmd_();

  // Phase 2: White flash
  this->send_config_preamble_();
  this->write_register_(REG_PFS, 0x06);
  uint8_t vcom[] = {0x00, 0x87, 0x00};
  this->write_register_(REG_VCOM, vcom, 3);
  this->send_normal_luts_();
  this->send_framebuffer_with_terminator_(REG_FB, empty);
  this->write_register_(REG_DRF); this->wait_busy_();

  // Phase 3: Content (skip if empty)
  if (memcmp(this->framebuffer_, empty, FB_DATA_SIZE) != 0) {
    this->send_framebuffer_with_terminator_(REG_FB, this->framebuffer_);
    this->write_register_(REG_DRF); this->wait_busy_();
  }

  this->pof_cmd_();
  memcpy(this->committed_fb_, this->framebuffer_, FB_DATA_SIZE);
  this->last_ghost_clear_ms_ = millis();
  this->ghost_clear_requested_ = false;
  this->has_old_fb_ = false;
}

void UC8119::do_partial_refresh_() {
  ESP_LOGD(TAG, "Partial refresh");
  this->send_config_preamble_();
  this->write_register_(REG_PFS, 0x06);
  uint8_t vcom[] = {0x00, 0x87, 0x00};
  this->write_register_(REG_VCOM, vcom, 3);
  this->send_normal_luts_();
  this->send_framebuffer_with_terminator_(REG_FB, this->framebuffer_);
  this->write_register_(REG_DRF); this->wait_busy_(); this->pof_cmd_();
  memcpy(this->committed_fb_, this->framebuffer_, FB_DATA_SIZE);
}

void UC8119::do_partial_refresh_with_old_fb_(const uint8_t *old_fb) {
  ESP_LOGI(TAG, "Partial refresh with old FB");
  this->send_config_preamble_();
  this->write_register_(REG_PFS, 0x06);
  uint8_t vcom[] = {0x00, 0x87, 0x00};
  this->write_register_(REG_VCOM, vcom, 3);
  this->send_normal_luts_();
  // Write old FB to 0x1C so the UC8119 knows what was on the display
  this->send_framebuffer_with_terminator_(REG_OLDFB, old_fb);
  // Write new FB to 0x18
  this->send_framebuffer_with_terminator_(REG_FB, this->framebuffer_);
  this->write_register_(REG_DRF); this->wait_busy_(); this->pof_cmd_();
  memcpy(this->committed_fb_, this->framebuffer_, FB_DATA_SIZE);
  this->has_old_fb_ = false;
}

void UC8119::pof_cmd_() {
  this->write_register_(REG_POF, 0x03); this->wait_busy_();
}

// ── Framebuffer access (inverted: 0=ON, 1=OFF) ─────────────────

void UC8119::set_segment(uint8_t i, bool state) {
  if (i >= FB_DATA_SIZE * 8) return;
  uint8_t bi = 7 - (i % 8);
  if (state) this->framebuffer_[i/8] &= ~(1 << bi);
  else       this->framebuffer_[i/8] |=  (1 << bi);
}

bool UC8119::get_segment(uint8_t i) const {
  if (i >= FB_DATA_SIZE * 8) return false;
  return !((this->framebuffer_[i/8] >> (7 - (i % 8))) & 1);
}

void UC8119::clear() { memset(this->framebuffer_, 0xFF, FB_DATA_SIZE); }
void UC8119::fill()  { memset(this->framebuffer_, 0x00, FB_DATA_SIZE); }

void UC8119::set_framebuffer(const uint8_t *d, uint8_t len) {
  if (len > FB_DATA_SIZE) len = FB_DATA_SIZE;
  memcpy(this->framebuffer_, d, len);
}

// ── Refresh control ─────────────────────────────────────────────

void UC8119::refresh(bool full) {
  if (full) this->do_full_refresh_(); else this->do_partial_refresh_();
}

bool UC8119::is_dirty() const {
  return memcmp(this->framebuffer_, this->committed_fb_, FB_DATA_SIZE) != 0;
}

bool UC8119::commit() {
  if (!this->initialized_) return false;
  if (!this->is_dirty() && !this->ghost_clear_requested_) return false;

  bool need_gc = this->ghost_clear_requested_;
  if (this->ghost_clear_interval_ms_ > 0 &&
      (millis() - this->last_ghost_clear_ms_ >= this->ghost_clear_interval_ms_))
    need_gc = true;

  if (need_gc) {
    this->do_full_refresh_();
  } else if (this->has_old_fb_) {
    // First commit after deep sleep wake: use old FB from RTC for diff update
    this->do_partial_refresh_with_old_fb_(this->committed_fb_);
  } else {
    this->do_partial_refresh_();
  }

  return true;
}

// ── Power management ────────────────────────────────────────────

void UC8119::power_on() {
  if (this->enable_pin_) {
    this->enable_pin_->digital_write(true);
    delay(5);
    ESP_LOGD(TAG, "Enable HIGH");
  }
  this->hardware_reset_();
  this->wait_busy_();
}

void UC8119::power_off() {
  // Save framebuffer to RTC memory before power down
  this->save_to_rtc_();
  // Send power-off command
  this->pof_cmd_();
  // Cut power via enable pin (if configured)
  if (this->enable_pin_) {
    this->enable_pin_->digital_write(false);
    ESP_LOGD(TAG, "Enable LOW");
  }
}

void UC8119::deep_sleep_cmd() {
  // Save framebuffer to RTC memory before deep sleep
  this->save_to_rtc_();
  // UC8119 software deep sleep — wake requires hardware reset
  this->write_register_(REG_DSLP, 0xA5);
  ESP_LOGD(TAG, "Deep sleep cmd sent");
}

// ── Lifecycle ───────────────────────────────────────────────────

void UC8119::setup() {
  ESP_LOGI(TAG, "Initializing");

  // Setup GPIO pins
  if (this->enable_pin_) this->enable_pin_->setup();
  if (this->reset_pin_)  this->reset_pin_->setup();
  if (this->busy_pin_)   this->busy_pin_->setup();

  // Power on hardware
  this->power_on();

  // Check RTC memory for saved framebuffer (deep sleep wake)
  if (this->load_from_rtc_()) {
    // Wake from deep sleep — we know what was on the display.
    // Skip full refresh; first commit() will use partial with old FB.
    this->has_old_fb_ = true;
    this->clear();
    this->last_ghost_clear_ms_ = millis();
    this->initialized_ = true;
    ESP_LOGI(TAG, "Ready (wake from sleep)");
  } else {
    // First boot or power cycle — no old FB available, must do full refresh.
    this->clear();
    memset(this->committed_fb_, 0xFF, FB_DATA_SIZE);
    this->do_full_refresh_();
    this->initialized_ = true;
    ESP_LOGI(TAG, "Ready (first boot)");
  }
}

void UC8119::dump_config() {
  ESP_LOGCONFIG(TAG,
                "UC8119 EPD Segment Driver:\n"
                "  Address: 0x%02X\n"
                "  Segments: %d\n"
                "  Ghost clear: %u min",
                this->address_, FB_DATA_SIZE * 8,
                this->ghost_clear_interval_ms_ / 60000);
  LOG_PIN("  Reset: ", this->reset_pin_);
  LOG_PIN("  Busy: ", this->busy_pin_);
  LOG_PIN("  Enable: ", this->enable_pin_);
}

}  // namespace uc8119
}  // namespace esphome
