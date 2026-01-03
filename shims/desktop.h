// Example program
#include <iostream>
#include <string>
#include <bitset>

// ---- Arduino-style constants (desktop stub) ----
#define OUTPUT 1
#define INPUT 2
#define INPUT_PULLDOWN 3

#define HIGH 1
#define LOW  0

#define BIN 2
#define HEX 16

#define ESP_EXT1_WAKEUP_ANY_HIGH 1



class SerialClass {
public:
    void begin(unsigned long baud);

    // Print strings
    void print(const char* s);
    void println(const char* s);

    // Print integers
    void print(int v);
    void println(int v);

    // Print 64-bit / masks etc.
    void print(uint64_t v, int base = 10);
    void println(uint64_t v, int base = 10);
};




SerialClass Serial;

void SerialClass::begin(unsigned long) {}

void SerialClass::print(const char* s) { std::cout << s; }
void SerialClass::println(const char* s) { std::cout << s << std::endl; }

void SerialClass::print(int v) { std::cout << v; }
void SerialClass::println(int v) { std::cout << v << std::endl; }

void SerialClass::print(uint64_t v, int base) {
    if (base == 2)      std::cout << std::bitset<64>(v);
    else if (base == 16) std::cout << std::hex << v << std::dec;
    else                std::cout << v;
}
void SerialClass::println(uint64_t v, int base) { print(v, base); std::cout << std::endl; }


const char* modeToString(int mode) {
    switch (mode) {
        case OUTPUT:         return "OUTPUT";
        case INPUT:          return "INPUT";
        case INPUT_PULLDOWN: return "INPUT_PULLDOWN";
        default:             return "UNKNOWN_MODE";
    }
}

const char* levelToString(int v) {
    switch (v) {
        case HIGH: return "HIGH";
        case LOW:  return "LOW";
        default:   return "UNKNOWN_LEVEL";
    }
}

void pinMode(int pin, int mode) {
    std::cout << "[pinMode] pin " << pin
              << " -> " << modeToString(mode) << std::endl;
}

void digitalWrite(int pin, int value) {
    std::cout << "[digitalWrite] pin " << pin
              << " = " << levelToString(value) << std::endl;
}

void esp_sleep_enable_ext1_wakeup(uint64_t mask, int mode) {
    std::cout << "[sleep_wakeup] mask=0x" << std::hex << mask
              << std::dec << " mode=" << mode << std::endl;
}


