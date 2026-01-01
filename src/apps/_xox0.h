#pragma once

#include <apps/calculator.h>
#include <apps/abacus.h>
#include <apps/llm.h>
#include <apps/dino.h>
#include <apps/terminal.h>
#include <apps/gsx.h>
#include <apps/nts.h>

typedef void (*fX)(void*);

struct APP_initdata {
    char name[20];
    fX function;
    int stack_size;
    uint8_t priority;
};

inline APP_initdata applist_init[] = {
    {"CALCULATOR", APP_CALCULATOR, 2048, 2},
    {"ABACUS", APP_ABACUS, 1536, 2},
    {"LLM", APP_LLM, 8192, 2},
    {"DINO", APP_DINO, 3584, 1},
    {"TERMINAL", APP_TERMINAL, 2048, 1},
    {"GSX", APP_GSX, 1024, 2},
    {"NTS", APP_NTS, 2048, 2}
};
//inline const size_t APP_COUNT = sizeof(applist_init) / sizeof(applist_init[0]);
