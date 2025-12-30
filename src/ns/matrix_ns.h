//#include <Arduino.h>
#ifndef MATRIX_NS_H
#define MATRIX_NS_H

#define KEYS_COUNT 24

inline const int PIN_COUNT=11;

struct PIN_CONFIG {
  int pin;
  bool output_capable;
  bool has_pulldown;
};

inline const int PINMAP[PIN_COUNT] = {15,26,32,12,35,14,27,2,25,33,13};
inline const PIN_CONFIG PINMAP_ALT[PIN_COUNT] = {
    {15, true,  true},
    {26, true,  true},
    {32, true,  true},
    {12, false, true}, // input capable but will be kept this way, pulldown
    {35, false, false}, // input only, no pulldown
    {14, true,  true},
    {27, true,  true},
    {2,  true,  true},
    {25, true,  true},
    {33, true,  true},
    {13, true,  true}
};

//should be generated but wtv i was lazy
inline const uint8_t SCAN_PERLINE[PIN_COUNT][6] = {{1,2},{3,6},{3,5,6},{},{},/*bugfix pulldown zero*/{1,4},{4,8,9},{1,9},{3,5,7},{3,5,10},{1,2,4,8}};
inline const uint8_t SCAN_PERLINE_PTR[PIN_COUNT] = {2,2,3,0,0,2,3,2,3,3,4};

inline uint8_t SCAN_PERLINE_GENERATED[PIN_COUNT][6];
inline uint8_t SCAN_PERLINE_GENERATED_PTR;

//dynamic scan lookup table (lazy search)
inline const uint8_t so_what_key_was_it_actually[PIN_COUNT][6] = {{0,1},{6,23},{7,11,9},{},{},{10,21},{2,22,8},{19,18},{5,14,4},{13,12,17},{15,16,3,20}};
                                                                             //what
//extern const int PINMAP_ALT[PIN_COUNT] = {15,26,32,-34,-35,14,27,2,25,33,13};
inline const int SCAN1[] = {0,1,2,5,6,7,8,9};
//const int SCAN2[][] = {}
inline const int MATRIXMAP[KEYS_COUNT][2] = {{0,1}, {0,2}, {4,6}, {10,4},{7,8},
                                           {3,8}, {3,1}, {3,2}, {6,9}, {6,2},
                                           {1,5}, {5,2}, {5,9}, {9,3}, {8,5},
                                           {1,10},{2,10},{9,10},{9,7}, {7,1},
                                           {8,10}, {5,4}, {8,6},/*{ },*/{1,6}};

//////////////////////////////////////////////////////////////////////////////// 
inline const String SYMBOL_MAP[KEYS_COUNT] = {"ON", "OFF","+/-","sqrt","%",
                                              "7",  "8",  "9",  "x",   "/",
                                              "4",  "5",  "6",  "-",   "MRC",
                                              "1",  "2",  "3",  "+",   "M-",
                                              "0",  ".",  "=",/*"+",*/ "M+"};

inline const String SYMBOL_MAP_ALT[KEYS_COUNT] = {"ON", "OFF","+/-","^",   "%",
                                                  "7",  "8",  "9",  "LEFT","RIGHT",
                                                  "4",  "5",  "6",  "-",   "MRC",
                                                  "1",  "2",  "3",  "+",   "M-",
                                                  "0",  "#",  "=",/*"+",*/ "M+"};
////////////////////////////////////////////////////////////////////////////////         

//////////////////////////////////////////////////////////////////////////////// 
inline const char* ABC_MAP[KEYS_COUNT] = {"a","b","c","d","e",
                                          "f","g","h","i","j",
                                          "k","l","m","n","o",
                                          "p","r","s","t","u",
                                          "v","#"," ",    "y"};

inline const char* ABC_MAP_ALT[KEYS_COUNT] = {"", "" ,"" ,"",     "",
                                              "7","8","9","LEFT", "RIGHT",
                                              "4","5","6","CLEAR","w",
                                              "1","2","3","ENTER","q",
                                              "0","#","z",        "x"};
//////////////////////////////////////////////////////////////////////////////// 

//////////////////////////////////////////////////////////////////////////////// 
inline const String T9_MAP[KEYS_COUNT] = {"",    "",    "",     "",  "",
                                          "",    "abc", "def",  "",  "",
                                          "ghi", "jkl", "mno",  "",  "",
                                          "pqrs","tuv", "wxyz", "ENTER",  "",
                                          "#",    "#",   " ",          " "};
//////////////////////////////////////////////////////////////////////////////// 

//////////////////////////////////////////////////////////////////////////////// 
inline const char* GSX_MAP[KEYS_COUNT] =  {"START","",  "",     "OPT","START",
                                          "",    "UP",  "",     "",   "",
                                          "LEFT","DOWN","RIGHT","",   "",
                                          "",    "",    "",     "A",  "B",
                                          "",    "",    "",           ""};
//////////////////////////////////////////////////////////////////////////////// 

//////////////////////////////////////////////////////////////////////////////// 
inline const char* CHIP8_MAP[KEYS_COUNT] = {"",  "",  "",  "",  "",
                                            "1", "2", "3", "C", "",
                                            "4", "5", "6", "D", "",
                                            "7", "8", "9", "E", "",
                                            "A", "0", "B",      "F"};
//////////////////////////////////////////////////////////////////////////////// 

//////////////////////////////////////////////////////////////////////////////// 
inline const char* MESSAGEASE_MAP[KEYS_COUNT] ={"",  "",  "",  "",  "",
                                                "a", "n", "i", "", "",
                                                "h", "o", "r", "", "",
                                                "t", "e", "s", "", "",
                                                "", " ", "",      "F"};
 //////////////////////////////////////////////////////////////////////////////// 
inline const int SCAN1LENGTH = sizeof(SCAN1)/sizeof(SCAN1[0]);

//extern const int PIN_COUNT = sizeof(PINMAP)/sizeof(PINMAP[0]);
enum keys {KEY_ON, KEY_OFF, KEY_PLUSMINUS, KEY_SQRT, KEY_MOD, KEY_7, KEY_8, KEY_9, KEY_X, KEY_DIVIDE, KEY_4, KEY_FIVE, KEY_SIX, KEY_MINUS, KEY_MRC,
KEY_1, KEY_2, KEY_3, KEY_PLUS, KEY_M_MINUS, KEY_0, KEY_DOT, KEY_EQUALS, KEY_M_PLUS};

#define key_ON 0
#define key_OFF 1
#define key_plusminus 2
#define key_sqrt 3
#define key_mod 4
#define key_seven 5
#define key_eight 6
#define key_nine 7
#define key_x 8
#define key_divide 9
#define key_four 10
#define key_five 11
#define key_six 12
#define key_minus 13
#define key_mrc 14
#define key_one 15
#define key_two 16 
#define key_three 17
#define key_plus 18
#define key_memminus 19
#define key_zero 20
#define key_dot 21
#define key_equals 22
#define key_memplus 23

#endif