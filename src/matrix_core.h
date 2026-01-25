#include <Arduino.h>

//#include <matrix_core.h>
#include <ns/matrix_ns.h>

bool matrix_scan(uint8_t *KEY_ARR_OUT, bool *KEY_ARR_BOOL, uint8_t *KEY_ARR_COUNT) {
  //KEY_ARR_OUT={};
  *KEY_ARR_COUNT=0;
  //static int KEY_ARR_INTERNAL[24] = {};
  
  for (size_t i_drive = 0; i_drive < PIN_COUNT; i_drive++)
  {
    if (!PINMAP_ALT[i_drive].output_capable) continue;
    //Serial.print("runningline: ");
    //Serial.println(i_drive);

    //prepare island line
    pinMode(PINMAP_ALT[i_drive].pin, OUTPUT);
    digitalWrite(PINMAP_ALT[i_drive].pin, HIGH);

    //sets up pins beforehand, giving it time to electrically settle, could be technically omitted later
    for (size_t ix = 0; ix < SCAN_PERLINE_PTR[i_drive]; ix++) {
      uint8_t pin_idx = SCAN_PERLINE[i_drive][ix];
      //(PINMAP_ALT[SCAN_PERLINE[ix]].output_capable) ? pinMode(PINMAP_ALT[SCAN_PERLINE[ix]].pin, INPUT_PULLDOWN) : pinMode(PINMAP_ALT[SCAN_PERLINE[ix]].pin, INPUT);
      if (PINMAP_ALT[pin_idx].has_pulldown) {
         pinMode(PINMAP_ALT[pin_idx].pin, INPUT_PULLDOWN);
      } else pinMode(PINMAP_ALT[pin_idx].pin, INPUT);
    }

    for (size_t i_sense = 0; i_sense < SCAN_PERLINE_PTR[i_drive]; i_sense++)
    {
      if (digitalRead(PINMAP_ALT[SCAN_PERLINE[i_drive][i_sense]].pin)) {
        //defer the pointer.. wtfx
        KEY_ARR_OUT[(*KEY_ARR_COUNT)++] = so_what_key_was_it_actually[i_drive][i_sense]; //finish this

        KEY_ARR_BOOL[so_what_key_was_it_actually[i_drive][i_sense]]=true;
        //if (so_what_key_was_it_actually[i_drive][i_sense]==20) {Serial.print(i_drive);Serial.print(":drive <> sense:");Serial.println(i_sense);}
      } else KEY_ARR_BOOL[so_what_key_was_it_actually[i_drive][i_sense]]=false;
      //KEY_ARR_BOOL[so_what_key_was_it_actually[i_drive][i_sense]]=digitalRead(PINMAP_ALT[SCAN_PERLINE[i_drive][i_sense]].pin);
    }
    //KEY_ARR_OUT = KEY_ARR_INTERNAL;
    //reset
    pinMode(PINMAP_ALT[i_drive].pin, INPUT_PULLDOWN);
    //(KEY_ARR_COUNT>0) ? return true : return false;
    //if (*KEY_ARR_COUNT>0) return true;
    //return false; fucking hell im dumb 
  } 
  return (*KEY_ARR_COUNT>0);
}

void matrix_reset() {
  for (size_t i = 0; i < PIN_COUNT; i++)
  {
    if(PINMAP_ALT[i].has_pulldown) {
      pinMode(PINMAP_ALT[i].pin, INPUT_PULLDOWN);
    } else pinMode(PINMAP_ALT[i].pin, INPUT);
  }
  
}

String matrix_state() {
  String output = "";
  for (size_t i = 0; i < PIN_COUNT; i++)
  {
    pinMode(PINMAP_ALT[i].pin, INPUT);
    output += String(digitalRead(PINMAP_ALT[i].pin),0);
  }
  matrix_reset();
  return output;
}


int MATRIX_SCAN_V1() {
  int KEY = -1;
  //Serial.print("matrix ");
  for (size_t i0 = 0; i0 < PIN_COUNT; i0++)
  {
    //Serial.print(i0);
    //carrier="       ";
    switch (i0){
    case 0: //ON, OFF
      pinMode(PINMAP[0], OUTPUT);
      digitalWrite(PINMAP[0],HIGH);
      pinMode(PINMAP[1],INPUT_PULLDOWN);
      pinMode(PINMAP[2],INPUT_PULLDOWN);

      if(digitalRead(PINMAP[2])) {
        KEY=1;
      } else if(digitalRead(PINMAP[1])) {
        KEY=0;
      } //else { carrier = " "; }
      break;
      pinMode(PINMAP[0], INPUT_PULLDOWN);

    case 1: 
      pinMode(PINMAP[1], OUTPUT);
      digitalWrite(PINMAP[1],HIGH);

      //pinMode(PINMAP[0],INPUT_PULLDOWN);
      pinMode(PINMAP[3],INPUT);
      pinMode(PINMAP[6],INPUT_PULLDOWN);
      //pinMode(PINMAP[8],INPUT_PULLDOWN);
      /*if(digitalRead(PINMAP[0])) {
        KEY=0;
      } else */if(digitalRead(PINMAP[3])) {
        //constant positive read: it got better
        KEY=6;
      } else if(digitalRead(PINMAP[6])) {
        KEY=23;
      }
      pinMode(PINMAP[1], INPUT_PULLDOWN);
      break;

    case 2:
      pinMode(PINMAP[2], OUTPUT);
      digitalWrite(PINMAP[2],HIGH);
      //pinMode(PINMAP[0],INPUT_PULLDOWN);
      pinMode(PINMAP[3],INPUT);
      pinMode(PINMAP[5],INPUT_PULLDOWN);
      pinMode(PINMAP[6],INPUT_PULLDOWN);
      //pinMode(PINMAP[10],INPUT_PULLDOWN);
      
      if(digitalRead(PINMAP[3])) {
        //keeps misfiring
        KEY=7;
      } else if(digitalRead(PINMAP[5])) {
        KEY=11;
      } else if(digitalRead(PINMAP[6])) {
        KEY=9;
      } /*else if(digitalRead(PINMAP[10])) {
        //bugs out after a while, PRIOTIY
        KEY=16;
      }*/
      pinMode(PINMAP[2], INPUT_PULLDOWN);
      break;

    case 3: //special care, input only
      
      break;

    case 4: //special care, input only

      break;

    case 5:
      pinMode(PINMAP[5], OUTPUT);
      digitalWrite(PINMAP[5],HIGH);
      //bugfix?
      pinMode(PINMAP[0],INPUT_PULLDOWN);

      pinMode(PINMAP[1],INPUT_PULLDOWN);
      //pinMode(PINMAP[2],INPUT_PULLDOWN);
      pinMode(PINMAP[4],INPUT);
      
      if(digitalRead(PINMAP[1])) {
        //bugs out when pressed with ON
        KEY=10;
      } else if(digitalRead(PINMAP[4])) {
        KEY=21;
      }
      pinMode(PINMAP[5], INPUT_PULLDOWN);
      break;

    case 6:
      pinMode(PINMAP[6], OUTPUT);
      digitalWrite(PINMAP[6],HIGH);

      pinMode(PINMAP[4],INPUT);
      pinMode(PINMAP[8],INPUT_PULLDOWN);
      pinMode(PINMAP[9],INPUT_PULLDOWN);
      
      if(digitalRead(PINMAP[4])) {
        KEY=2;
      } else if(digitalRead(PINMAP[8])) {
        KEY=22;
      } else if(digitalRead(PINMAP[9])) {
        KEY=8;
      }
      pinMode(PINMAP[6], INPUT_PULLDOWN);
      break;

    case 7:
      pinMode(PINMAP[7], OUTPUT);
      digitalWrite(PINMAP[7],HIGH);

      pinMode(PINMAP[1],INPUT_PULLDOWN);
      pinMode(PINMAP[6],INPUT_PULLDOWN);
      pinMode(PINMAP[9],INPUT_PULLDOWN);
      
      if(digitalRead(PINMAP[1])) {
        KEY=19;
      } else if(digitalRead(PINMAP[6])) {
        KEY=22;
      } else if(digitalRead(PINMAP[9])) {
        KEY=18;
      }
      pinMode(PINMAP[7], INPUT_PULLDOWN);
      break;

    case 8:
      pinMode(PINMAP[8], OUTPUT);
      digitalWrite(PINMAP[8],HIGH);

      //pinMode(PINMAP[1],INPUT_PULLDOWN);
      pinMode(PINMAP[3],INPUT);
      pinMode(PINMAP[5],INPUT_PULLDOWN);
      pinMode(PINMAP[7],INPUT_PULLDOWN);
      /*if(digitalRead(PINMAP[1])) {
        //desnt detect
        Serial.println("internal KEYx0");
        KEY=20;
      } else */if(digitalRead(PINMAP[3])) {
        KEY=5;
      } else if(digitalRead(PINMAP[5])) {
        KEY=14;
      } else if(digitalRead(PINMAP[7])) {
        KEY=4;
      }
      pinMode(PINMAP[8], INPUT_PULLDOWN);
      break;

    case 9:
      pinMode(PINMAP[9], OUTPUT);
      digitalWrite(PINMAP[9],HIGH);
      //bugfix? pinMode(PINMAP[0],INPUT_PULLDOWN);

      pinMode(PINMAP[3],INPUT);
      pinMode(PINMAP[5],INPUT_PULLDOWN);
      pinMode(PINMAP[10],INPUT_PULLDOWN);
      
      if(digitalRead(PINMAP[3])) {
        KEY=13;
      } else if(digitalRead(PINMAP[5])) {
        KEY=12;
      } else if(digitalRead(PINMAP[10])) {
        KEY=17;
      }
      pinMode(PINMAP[9], INPUT_PULLDOWN);
      break;

    case 10:
      pinMode(PINMAP[10], OUTPUT);
      digitalWrite(PINMAP[10],HIGH);
      //bugfix? pinMode(PINMAP[0],INPUT_PULLDOWN);

      pinMode(PINMAP[1],INPUT_PULLDOWN);
      pinMode(PINMAP[2],INPUT_PULLDOWN);
      pinMode(PINMAP[4],INPUT);
      pinMode(PINMAP[8],INPUT_PULLDOWN);
      
      if(digitalRead(PINMAP[1])) {
        KEY=15;
      } else if(digitalRead(PINMAP[2])) {
        KEY=16;
      } else if(digitalRead(PINMAP[4])) {
        KEY=3;
      } else if(digitalRead(PINMAP[8])) {
        KEY=20;
      }
      pinMode(PINMAP[10], INPUT_PULLDOWN);
      break;

    default:
      Serial.print(i0);
      Serial.println(" whatthefuck");
    }
  }
      return KEY;
}

