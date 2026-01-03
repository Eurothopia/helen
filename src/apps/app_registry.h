#pragma once
#include "app_config.h"
#include "../struct.h"

// Global registry of apps. Definitions are in app_registry.cpp
extern const AppDescriptor APP_REGISTRY[];
extern const size_t APP_COUNT;

inline const AppDescriptor &getApp(AppID id) {
    return APP_REGISTRY[static_cast<size_t>(id)];
}

inline const AppConfig &getAppConfig(AppID id) {
    return *APP_REGISTRY[static_cast<size_t>(id)].config;
}
