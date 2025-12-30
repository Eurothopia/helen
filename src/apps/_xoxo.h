#ifndef xoxo
#define xoxo

#include <apps/calculator.h>
#include <apps/abacus.h>
#include <apps/llm.h>
#include <apps/dino.h>
#include <apps/terminal.h>
#include <apps/nts.h>

typedef void (*fX)(void*);

enum bluetooth_type {none, bluetooth_low_energy, bluetooth_classic, bluetooth_a2dp};
struct APP_metadata {
    char name[20];
    fX function;
    int stack_size;
    uint8_t priority;
    bluetooth_type requires_bluetooth;
    bool requires_wifi;
    bool vsync;
    bool wakelock;
};

APP_metadata applist[] = {
    {"CALCULATOR", APP_CALCULATOR, 8192, 2, none, false, false},
    {"ABACUS", APP_ABACUS, 16384, 2, none, false, false},
    {"LLM", APP_LLM, 8192, 2, none, false, false},
    {"DINO", APP_DINO, 8192, 1, none, false, false},
    {"TERMINAL", APP_TERMINAL, 2048, 1, none, false, false},
    {"NTS", APP_NTS, 8912, 2, bluetooth_a2dp, true, false, true}
};
const size_t APP_COUNT = sizeof(applist) / sizeof(applist[0]);



/*
APP(CALCULATOR, 8192, 2)
APP(ABACUS,    16384, 2)
APP(LLM,        8192, 2)
APP(DINO,       8192, 1)
APP(TERMINAL,   2048, 1)

#pragma once
#include <Arduino.h>

enum AppID {
#define APP(name) _##name,
#include "apps.h"
#undef APP
};

static const char* AppName[] = {
#define APP(name) #name,
#include "apps.h"
#undef APP
};

volatile inline AppID FOCUSED_APP = _CALCULATOR;
*/

#endif