#include "app_registry.h"

#include "calculator.h"
#include "abacus.h"
//#include "llm.h"
#include "llm0.h"
#include "dino.h"
#include "terminal.h"
#include "gsx.h"
#include "nts0.h"

// Unified registry combining metadata, config, and task info.
const AppDescriptor APP_REGISTRY[] = {
    {_CALCULATOR, "CALCULATOR", APP_CALCULATOR, &appcfg_CALCULATOR, CLASSIC_INPUT, none, false},
    {_ABACUS,     "ABACUS",     APP_ABACUS,     &appcfg_ABACUS,     CLASSIC_INPUT, none, false},
    {_LLM,        "LLM",        APP_LLM,        &appcfg_LLM,        ABX,           none, false},
    {_DINO,       "DINO",       APP_DINO,       &appcfg_DINO,       GSX,           none, false},
    {_TERMINAL,   "TERMINAL",   APP_TERMINAL,   &appcfg_TERMINAL,   ABX,           none, false},
    {_GSX,        "GSX",        APP_GSX,        &appcfg_GSX,        GSX,           none, false},
    {_NTS,        "NTS",        APP_NTS,        &appcfg_NTS,        CLASSIC_INPUT, bluetooth_a2dp, false},
};

const size_t APP_COUNT = sizeof(APP_REGISTRY) / sizeof(APP_REGISTRY[0]);
