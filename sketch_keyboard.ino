
//#define DEBUG

#include "Keyboard.h"
#include "Mouse.h"

#define XMACRODATA

#ifdef DEBUG
#define LOG(...) do { Serial.print("DEBUG "); Serial.print(__FILE__); Serial.print(":"); Serial.print(__LINE__); Serial.print(" "); char __L[256]; snprintf(__L,255,__VA_ARGS__); __L[255] = '\0'; Serial.print(__L); } while(0)
#else // DEBUG
#define LOG(...)
#endif // DEBUG

void setup_common() {

#ifdef DEBUG
  Serial.begin(9600);
#endif // DEBUG
  Mouse.begin();
  Keyboard.begin();

  // shutdown annoying led
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_BUILTIN_RX, OUTPUT);
  pinMode(LED_BUILTIN_TX, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(LED_BUILTIN_RX, HIGH);
  digitalWrite(LED_BUILTIN_TX, HIGH);
}

static void loop_common(void){

  // the bootloader continously turn on the annoying led, shutdown
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(LED_BUILTIN_RX, HIGH);
  digitalWrite(LED_BUILTIN_TX, HIGH);
}

static int update_key_status(unsigned char key, unsigned char new_status){
  static int inited = 0;
  static unsigned char last_status[256];
  if (!inited){
    inited = 1;
    for (int k = 0; k < sizeof(last_status)/sizeof(last_status[0]); k += 1)
      last_status[k] = LOW;
  }
  if (new_status != last_status[key]) {
    if (new_status == HIGH) Keyboard.release(key);
    else Keyboard.press(key);
    LOG("value change 0x%x [%c] %d -> %d\n", key, key, last_status[key], new_status);
    last_status[key] = new_status;
  }
  return 0;
}

static int delta_value_low(int new_value) {
  static int can_switch = 0;
  int old_switch = can_switch;
  can_switch = (new_value == HIGH);
  return old_switch && (new_value == LOW);
}

// Key mode 0 ---------------------------------------------------------------------

#undef XMACRODATA
// XMACRO(PIN, KEY)
#define XMACRODATA          \
  XMACRO(2, KEY_ESC)        \
  XMACRO(3, KEY_DOWN_ARROW) \
  XMACRO(4, KEY_UP_ARROW)   \
  XMACRO(5, KEY_RETURN)     \
  XMACRO(6, KEY_F12)        \
//END XMACRODATA

static void setup_mode_0(void){

  // Set all the pins defined in XMACRODATA to pullup input
#define XMACRO(P, K) pinMode(P, INPUT_PULLUP);
  XMACRODATA;
#undef XMACRO

  // Additional pin for mode switch
  pinMode(7, INPUT_PULLUP);
}

static int loop_mode_0(void) {

  // Call update_key_status for each PIN/KEY pair in XMACRODATA
#define XMACRO(P, K) update_key_status(K, digitalRead(P));
  XMACRODATA;
#undef XMACRO

  // switch to mode_1 when pin 7 is fired
  if (delta_value_low(digitalRead(7))){
    LOG("switching to mode 1\n");
    return 1;
  }

  return 0; // Stay in mode 0
}

// Key mode 1 ---------------------------------------------------------------------

#undef XMACRODATA
// XMACRO(PIN, KEY)
#define XMACRODATA         \
  XMACRO(2, KEY_ESC)       \
  XMACRO(3, KEY_PAGE_DOWN) \
  XMACRO(4, KEY_PAGE_UP)   \
//END XMACRODATA

static void setup_mode_1(void){

  // Set all the pins defined in XMACRODATA to pullup input
#define XMACRO(P, K) pinMode(P, INPUT_PULLUP);
  XMACRODATA;
#undef XMACRO

  // Additional pin for mode switch
  pinMode(7, INPUT_PULLUP);
}

static int loop_mode_1(void) {

  // Call update_key_status for each PIN/KEY pair in XMACRODATA
#define XMACRO(P, K) update_key_status(K, digitalRead(P));
  XMACRODATA;
#undef XMACRO

  // switch to mode_0 when pin 7 is fired
  if (delta_value_low(digitalRead(7))) {
    LOG("switching to mode 0\n");
    return 0;
  }

  return 1; // Stay in mode 1
}

// dispatcher ---------------------------------------------------------------------

void setup() {
  // NOTE: all hte modes must be compatible at pin-level
  setup_common();
  setup_mode_0();
  setup_mode_1();
}

void loop(){
  static int mode = 0;

  loop_common();

  switch (mode) {
  break;default: mode = loop_mode_0();
  break;case  0: mode = loop_mode_0();
  break;case  1: mode = loop_mode_1();
  }
}

