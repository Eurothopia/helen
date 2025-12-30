#include <Arduino.h>
//custom logger class to print to multiple serial interfaces
class SERIAL_MUX {
public:
    //SerialLogger() enabled=true;
    SERIAL_MUX(bool enabled=true) : logging_enabled(enabled) {}

    void begin(long baud0, long baud1) {
        Serial.begin(baud0);
        Serial.begin(baud1);
    }
    void enable() { logging_enabled=true; }
    void disable() { logging_enabled=false; }

    //overloading log()
    template <typename T>
    void log(const T& msg) {
        if (logging_enabled) {
            switch (SERIAL_MODE) {
                case 0:
                    Serial.print(msg);
                    break;
                case 1:
                    Serial1.print(msg);   
                    break;
                case 2:
                    Serial.print(msg);
                    Serial1.print(msg);                    
            }
        }
    }
    /*void log(const char* msg) {
        if (logging_enabled) {
            Serial.print(msg);
            Serial1.print(msg);
        }
    }
    void log(const String& msg) {
        if (logging_enabled) {
            Serial.print(msg);
            Serial1.print(msg);
        }
    }*/
    void ln() {
        if (logging_enabled) {
            Serial.println();
            Serial1.println();
        }
    }
private: 
    bool logging_enabled;

}; //which motherfucker made a semicolon after a class so fucking cocky.. compiler dumb as hell he no tell me what was up