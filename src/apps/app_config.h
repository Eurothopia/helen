#pragma once
#include <Arduino.h>

// Represents configurable properties for each app.
struct AppConfig {
    bool vsync = false;          // Whether the app wants display sync.
    bool fullscreen = true;      // Whether the app occupies the full display.
    uint8_t target_fps = 20;
    uint32_t refresh_ms = 50;    // Preferred refresh/update interval.
    uint16_t stack_size = 4096;  // Suggested task stack size.
    UBaseType_t priority = 1;    // Suggested task priority.
    bool needs_network = false;  // Whether Wi-Fi is required.
    bool wakelock = false;       // Whether the app requests wake lock.
};

// Helper to build configs with a lambda for readability in C++17.
template <typename Fn>
constexpr AppConfig make_app_config(Fn fn) {
    AppConfig cfg{};
    fn(cfg);
    return cfg;
}

// Convenience default config instance.
inline constexpr AppConfig DefaultAppConfig{};

// Macro to declare an inline constexpr AppConfig for an app.
#define DEFINE_APP_CONFIG(NAME, INIT_EXPR) inline constexpr AppConfig appcfg_##NAME = INIT_EXPR;
