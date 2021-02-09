
/***********************************************************************************

This is free and unencumbered work released into the public domain.

Should the previous statement, for any reason, be judged legally invalid or
ineffective under applicable law, the work is made avaiable under the terms of one
of the following licences:

- Apache License 2.0 https://www.apache.org/licenses/LICENSE-2.0
- MIT License https://opensource.org/licenses/MIT
- CC0 https://creativecommons.org/publicdomain/zero/1.0/legalcode
- Unlicense https://unlicense.org/

The "C64 datasette button reader"'s Author, 2021

***********************************************************************************/

// Config -------------------------------------------------------------------------

//#define EABLE_SERIAL
//#define DEBUG

// --------------------------------------------------------------------------------

#include "Keyboard.h"

#define XMACRODATA

#ifdef DEBUG
#define ENABLE_SERIAL
#define LOG(...) do { Serial.print("DEBUG "); Serial.print(__FILE__); Serial.print(":"); Serial.print(__LINE__); Serial.print(" "); char __L[256]; snprintf(__L,255,__VA_ARGS__); __L[255] = '\0'; Serial.print(__L); } while(0)
#define DASSERT(C, R, ...) do{ if (!(C)){ LOG(__VA_ARGS__); return (R); } }while(0)
#else // DEBUG
#define LOG(...)
#define DASSERT(...)
#endif // DEBUG

static unsigned long now_us = 0;
static void next_time_step(void) { now_us = micros(); }
static unsigned long current_time_step(void)  { return now_us; }

static int last_value[32];
static unsigned long last_change_us[32];

static int init_status_check() {
  for (int k = 0; k < sizeof(last_value)/sizeof(*last_value); k += 1){
    last_change_us[k] = current_time_step();
    last_value[k] = digitalRead(k);
  }
}

static int chack_status_change(int pin, int* value) {

  DASSERT(pin >= 0 && pin < sizeof(last_value)/sizeof(*last_value), 0,
    "ERROR trying to read unsupported pin %d\n", pin);

  if (now_us - last_change_us[pin] < 1000) { // Debouncer
    *value = last_value[pin];
    return 0;
  }
  last_change_us[pin] = current_time_step();

  *value = digitalRead(pin);
  if (last_value[pin] != *value) {
    LOG("pin %d value change %d -> %d\n", pin, last_value[pin], *value);
    last_value[pin] = *value;
    return 1;
  }

  return 0;
}

// Basic handling -----------------------------------------------------------------

void setup_first() {

#ifdef ENABLE_SERIAL
  Serial.begin(9600);
#endif // ENABLE_SERIAL

  Keyboard.begin();
}

void setup_last() {

  next_time_step();
  init_status_check();

  // shutdown annoying leds
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_BUILTIN_RX, OUTPUT);
  pinMode(LED_BUILTIN_TX, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(LED_BUILTIN_RX, HIGH);
  digitalWrite(LED_BUILTIN_TX, HIGH);
}

static void loop_common(void){

  next_time_step();

  // the bootloader continously turn on the annoying leds, shutdown
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(LED_BUILTIN_RX, HIGH);
  digitalWrite(LED_BUILTIN_TX, HIGH);
}

// Key mode 0 ---------------------------------------------------------------------

#define C64BTN_RECORD (6)
#define C64BTN_PLAY   (5)
#define C64BTN_REWIND (4)
#define C64BTN_FFWD   (3)
#define C64BTN_STOP   (2)

static void setup_mode_0(void){

  pinMode(C64BTN_RECORD, INPUT_PULLUP);
  pinMode(C64BTN_PLAY, INPUT_PULLUP);
  pinMode(C64BTN_REWIND, INPUT_PULLUP);
  pinMode(C64BTN_FFWD, INPUT_PULLUP);
  pinMode(C64BTN_STOP, INPUT_PULLUP);
}

typedef enum {

  // single key sequences

  SEQ_F12 = KEY_F12,         // F12 to MiSTer = open main menu
  SEQ_F11 = KEY_F11,         // F11 to MiSTer = pair bluetooth controller
  SEQ_SPACE = ' ',           // SPACE to MiSTer = skip the binding of a single button
  SEQ_ENTER = KEY_RETURN,    // ENTER to MiSTer = select menu item
  SEQ_UP = KEY_UP_ARROW,     // UP to MiSTer = next menu item
  SEQ_DOWN = KEY_DOWN_ARROW, // DOWN to MiSTer = previous menu item
  SEQ_ESCAPE = KEY_ESC,      // ESC to MiSTer = go back in the menu

  // multiple key sequences

  SEQ_WIN_F12,               // Win + F12 to MiSTer = open main menu in some core (e.g. ao486)
  SEQ_ALT_F12,               // ALT + F12 to MiSTer = open core-selection menu

} key_seq_t;

static void send_single_key_seq(int key){
  Keyboard.press(key);
  delay(10);
  Keyboard.release(key);
}

static void send_seq(int seq){
  switch (seq) {
    break; default: send_single_key_seq(seq);

    break; case SEQ_ALT_F12:
      Keyboard.press(KEY_LEFT_ALT);
      delay(10);
      Keyboard.press(KEY_F12);
      delay(10);
      Keyboard.release(KEY_F12);
      Keyboard.release(KEY_LEFT_ALT);

    break; case SEQ_WIN_F12:
      // KEY_LEFT_GUI = Windows key in arduino Keyboard lib
      Keyboard.press(KEY_LEFT_GUI);
      delay(10);
      Keyboard.press(KEY_F12);
      delay(10);
      Keyboard.release(KEY_F12);
      Keyboard.release(KEY_LEFT_GUI);

  }
}

static int check_and_send_seq(int pin, int seq){
  int status = 0;
  if (chack_status_change(pin, &status)) {
    if (LOW == status) {
      send_seq(seq);
      return 1;
    }
  }
  return 0;
}

static int loop_mode_0(void) {

  static int mode_sent = 0;

  int mode;
  if (chack_status_change(C64BTN_RECORD, &mode)) {
    if (HIGH == mode) {
      if (mode_sent <= 0) send_seq(KEY_F12);
      mode_sent = 0;
    }
  }

  if (LOW == mode) {
    mode_sent += check_and_send_seq(C64BTN_PLAY, SEQ_SPACE);
    mode_sent += check_and_send_seq(C64BTN_REWIND, SEQ_F11);
    mode_sent += check_and_send_seq(C64BTN_FFWD, SEQ_WIN_F12);
    mode_sent += check_and_send_seq(C64BTN_STOP, SEQ_ALT_F12);
  } else {
    check_and_send_seq(C64BTN_PLAY, SEQ_ENTER);
    check_and_send_seq(C64BTN_REWIND, SEQ_UP);
    check_and_send_seq(C64BTN_FFWD, SEQ_DOWN);
    check_and_send_seq(C64BTN_STOP, SEQ_ESCAPE);
  }

  return 0;
}

// dispatcher ---------------------------------------------------------------------

void setup() {
  setup_first();
  setup_mode_0();
  setup_last();
}

void loop(){
  static int mode = -1;
  loop_common();
  loop_mode_0();
}
