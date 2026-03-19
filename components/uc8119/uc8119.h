#pragma once
// UC8119 EPD Segment Display Driver
// SPDX-License-Identifier: MIT
//
// Generic driver — no display-specific segment mapping or layout.
// Use this as a base for any product with a UC8119 controller.

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace uc8119 {

// Registers (reverse-engineered from Shelly H&T Gen3, compatible with UC81xx family)
static const uint8_t REG_PSR  = 0x00;  // Panel Setting
static const uint8_t REG_PWR  = 0x01;  // Power Setting
static const uint8_t REG_POF  = 0x02;  // Power OFF
static const uint8_t REG_PFS  = 0x03;  // Mode: 0x00=Clear, 0x06=Normal
static const uint8_t REG_PON  = 0x04;  // Power ON
static const uint8_t REG_DSLP = 0x07;  // Deep Sleep (data=0xA5, wake by HW reset only)
static const uint8_t REG_DRF  = 0x12;  // Display Refresh Trigger
static const uint8_t REG_VCOM = 0x15;  // VCOM / Timing Config
static const uint8_t REG_FB   = 0x18;  // Framebuffer Write (new data)
static const uint8_t REG_OLDFB= 0x1C;  // Previous Framebuffer (for diff update)
static const uint8_t REG_LUTC = 0x20;  // LUT VCOM
static const uint8_t REG_LUTWB= 0x23;  // LUT White-to-Black
static const uint8_t REG_LUTBB= 0x24;  // LUT Black-to-Black
static const uint8_t REG_LUT3 = 0x25;  // LUT Extra
static const uint8_t REG_LUTBD= 0x26;  // LUT Border
static const uint8_t REG_PLL  = 0x30;  // PLL Control

static const uint8_t FB_DATA_SIZE  = 17;   // 17 data bytes = 136 segments
static const uint8_t FB_TOTAL_SIZE = 18;   // + 0x80 terminator
static const uint8_t LUT_SIZE     = 15;

class UC8119 : public Component, public i2c::I2CDevice {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::LATE; }

  // Config
  void set_reset_pin(GPIOPin *pin) { this->reset_pin_ = pin; }
  void set_busy_pin(GPIOPin *pin) { this->busy_pin_ = pin; }
  void set_enable_pin(GPIOPin *pin) { this->enable_pin_ = pin; }
  void set_full_update_every(uint32_t v) { this->full_update_every_ = v; }
  void set_ghost_clear_interval(uint32_t ms) { this->ghost_clear_interval_ms_ = ms; }

  // ─── Framebuffer access (inverted: 0=ON, 1=OFF) ──────────────

  /// Set a single segment by bit index (0-135)
  void set_segment(uint8_t index, bool state);

  /// Get segment state
  bool get_segment(uint8_t index) const;

  /// Clear display (all segments OFF)
  void clear();

  /// Fill display (all segments ON)
  void fill();

  /// Direct framebuffer access (17 bytes, inverted)
  void set_framebuffer(const uint8_t *data, uint8_t len);
  const uint8_t *get_framebuffer() const { return this->framebuffer_; }

  // ─── Refresh control ──────────────────────────────────────────

  /// Immediate refresh: full (with ghost clearing) or partial
  void refresh(bool full = false);

  /// Smart commit: only refreshes if framebuffer changed, handles ghost clear timing
  /// Returns true if a refresh was performed
  bool commit();

  /// Force ghost clear on next commit
  void request_ghost_clear() { this->ghost_clear_requested_ = true; }

  /// Check if framebuffer differs from what's on the display
  bool is_dirty() const;

  /// Check if the driver has been initialized
  bool is_ready() const { return this->initialized_; }

  // ─── Power management ─────────────────────────────────────────

  /// Power on: enable pin HIGH → hardware reset → wait busy
  /// Called automatically in setup(), but can be used manually after power_off()
  void power_on();

  /// Power off: POF command → enable pin LOW (if configured)
  /// Call this before ESP32 deep sleep to fully power down the display.
  /// The current framebuffer is saved to RTC memory for partial refresh on next wake.
  void power_off();

  /// Send UC8119 deep sleep command (register 0x07, check code 0xA5)
  /// Alternative to power_off() for boards WITHOUT enable pin.
  /// Wake requires hardware reset (power_on() or full reboot).
  void deep_sleep_cmd();

 protected:
  GPIOPin *reset_pin_{nullptr};
  GPIOPin *busy_pin_{nullptr};
  GPIOPin *enable_pin_{nullptr};
  uint32_t full_update_every_{10};
  uint32_t ghost_clear_interval_ms_{1800000};

  uint8_t framebuffer_[FB_DATA_SIZE]{};
  uint8_t committed_fb_[FB_DATA_SIZE]{};
  bool initialized_{false};
  bool has_old_fb_{false};  // True if old FB restored from RTC memory

  uint32_t update_count_{0};
  uint32_t last_ghost_clear_ms_{0};
  bool ghost_clear_requested_{false};

  // Low-level
  void hardware_reset_();
  void wait_busy_(uint32_t timeout_ms = 5000);
  void write_register_(uint8_t reg);
  void write_register_(uint8_t reg, uint8_t value);
  void write_register_(uint8_t reg, const uint8_t *data, uint8_t len);

  // Protocol sequences
  void send_config_preamble_();
  void send_clear_luts_();
  void send_normal_luts_();
  void send_framebuffer_with_terminator_(uint8_t reg, const uint8_t *fb);
  void do_full_refresh_();
  void do_partial_refresh_();
  void do_partial_refresh_with_old_fb_(const uint8_t *old_fb);
  void pof_cmd_();

  // RTC memory
  void save_to_rtc_();
  bool load_from_rtc_();
};

}  // namespace uc8119
}  // namespace esphome
