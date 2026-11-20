#pragma once
#if __has_include("secrets.h")
#include "secrets.h"
#else
#error "Missing include/secrets.h - run ./flash.sh (or copy include/secrets.example.h to include/secrets.h)"
#endif

constexpr uint8_t kPaymentQueue = 20;        // bursts beyond this are dropped
constexpr uint32_t kWifiRetryMs = 10000;
constexpr uint32_t kWsReconnectMs = 5000;
constexpr uint32_t kWsPingMs = 20000;
constexpr uint32_t kBleScanSeconds = 10;
constexpr uint32_t kBleRetryMs = 5000;
constexpr const char* kJacketNamePrefix = "Hello Fairy";
