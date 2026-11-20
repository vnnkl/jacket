// Fixed firmware constants. Everything user-adjustable lives in settings (admin page).
#pragma once
#include <stdint.h>

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "dev"
#endif
#ifndef BOOT_PIN
#define BOOT_PIN 0
#endif

constexpr uint8_t kPaymentQueue = 20;        // bursts beyond this are dropped
constexpr uint32_t kWifiJoinTimeoutMs = 30000;
constexpr uint32_t kWsReconnectMs = 5000;
constexpr uint32_t kWsPingMs = 20000;
constexpr uint32_t kBleScanSeconds = 10;
constexpr uint32_t kBleRetryMs = 5000;
constexpr uint32_t kRestartDelayMs = 1500;   // let the HTTP response go out first
constexpr uint8_t kBootPin = BOOT_PIN;
constexpr const char* kJacketNamePrefix = "Hello Fairy";
constexpr const char* kHotspotName = "Jacket-Setup";
constexpr const char* kHostname = "jacket";
constexpr const char* kAdminUser = "admin";
