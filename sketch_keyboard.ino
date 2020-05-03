
// Config -------------------------------------------------------------------------

//#define EABLE_SERIAL

#define ENABLE_KEYBOAD
//#define ENABLE_MOUSE
#define ENABLE_JOYSTICK

//#define DEBUG

// --------------------------------------------------------------------------------

#ifdef ENABLE_KEYBOAD
#include "Keyboard.h"
#endif // ENABLE_KEYBOAD

#ifdef ENABLE_MOUSE
#include "Mouse.h"
#endif // ENABLE_MOUSE

#ifdef ENABLE_JOYSTICK
#include "Joystick.h"
#endif // ENABLE_JOYSTICK

#define XMACRODATA

#ifdef DEBUG
#define ENABLE_SERIAL
#define LOG(...) do { Serial.print("DEBUG "); Serial.print(__FILE__); Serial.print(":"); Serial.print(__LINE__); Serial.print(" "); char __L[256]; snprintf(__L,255,__VA_ARGS__); __L[255] = '\0'; Serial.print(__L); } while(0)
#else // DEBUG
#define LOG(...)
#endif // DEBUG

typedef enum {
  SEND_DISABLED = 0,
  SEND_KEYBOARD,
  SEND_MOUSE,
  SEND_JOY_BUTTON,
  SEND_JOY_HAT,
  SEND_JOY_AXIS,
} send_mode_t;

#ifdef ENABLE_JOYSTICK

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID,JOYSTICK_TYPE_GAMEPAD,
  16, 0,                 // Button Count, Hat Switch Count
  true, true, false,     // X and Y, but no Z Axis
  false, false, false,   // No Rx, Ry, or Rz
  false, false,          // No rudder or throttle
  false, false, false);  // No accelerator, brake, or steering

#endif // ENABLE_JOYSTICK

void setup_common() {

#ifdef ENABLE_SERIAL
  Serial.begin(9600);
#endif // ENABLE_SERIAL

#ifdef ENABLE_KEYBOARD
  Keyboard.begin();
#endif // ENABLE_KEYBOARD

#ifdef ENABLE_MOUSE
  Mouse.begin();
#endif // ENABLE_MOUSE

#ifdef ENABLE_JOYSTICK
  Joystick.begin();
  Joystick.setXAxisRange(-1, 1);
  Joystick.setYAxisRange(-1, 1);
#endif // ENABLE_JOYSTICK

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
#ifdef ENABLE_KEYBOAD

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
    LOG("value change keyb_key 0x%x [%c] %d -> %d\n", key, key, last_status[key], new_status);
    last_status[key] = new_status;
  }
#endif // ENABLE_KEYBOAD
  return 0;
}

static int update_mouse_status(int key, int new_status){
#ifdef ENABLE_MOUSE
  // TODO : implement
#endif // ENABLE_MOUSE
  return 0;
}

static int update_joy_button_status(unsigned char key, unsigned char new_status){
#ifdef ENABLE_JOYSTICK

  static int inited = 0;
  static unsigned char last_status[32];

  if (key >= sizeof(last_status)/sizeof(last_status[0]))
    return 0;

  if (!inited){
    inited = 1;
    for (int k = 0; k < sizeof(last_status)/sizeof(last_status[0]); k += 1)
      last_status[k] = LOW;
  }

  if (new_status != last_status[key]) {
    if (new_status == HIGH) Joystick.setButton(key, LOW);
    else Joystick.setButton(key, HIGH);
    LOG("value change joy_button 0x%x [%c] %d -> %d\n", key, key, last_status[key], new_status);
    last_status[key] = new_status;
  }
#endif // ENABLE_JOYSTICK
  return 0;
}

static int update_joy_hat_status(unsigned char key, int new_status){
#ifdef ENABLE_JOYSTICK
  // TODO : implement
#endif // ENABLE_JOYSTICK
  return 0;
}

static int update_joy_axis_status(unsigned char key, int new_status){
#ifdef ENABLE_JOYSTICK
  // TODO : implement
#endif // ENABLE_JOYSTICK
  return 0;
}

static int update_status(int key, int mode, int new_status){
  switch (mode) {

  break;case SEND_KEYBOARD: return update_key_status(key, new_status);
  break;case SEND_MOUSE:    return update_mouse_status(key, new_status);
  break;case SEND_JOY_BUTTON: return update_joy_button_status(key, new_status);
  break;case SEND_JOY_HAT: return update_joy_hat_status(key, new_status);
  break;case SEND_JOY_AXIS: return update_joy_axis_status(key, new_status);
  break;default: break;
  }
}

static int delta_value_low(int new_value) {
  static int can_switch = 0;
  int old_switch = can_switch;
  can_switch = (new_value == HIGH);
  return old_switch && (new_value == LOW);
}

// Key mode 0 ---------------------------------------------------------------------

#undef XMACRODATA
// XMACRO(PIN, MODE, KEY)
#define XMACRODATA          \
  XMACRO(2, SEND_KEYBOARD, KEY_ESC)        \
  XMACRO(3, SEND_KEYBOARD, KEY_DOWN_ARROW) \
  XMACRO(4, SEND_KEYBOARD, KEY_UP_ARROW)   \
  XMACRO(5, SEND_KEYBOARD, KEY_RETURN)     \
  XMACRO(6, SEND_KEYBOARD, KEY_F12)        \
//END XMACRODATA

static void setup_mode_0(void){

  // Set all the pins defined in XMACRODATA to pullup input
#define XMACRO(P, M, K) pinMode(P, INPUT_PULLUP);
  XMACRODATA;
#undef XMACRO

  // Additional pin for mode switch
  pinMode(7, INPUT_PULLUP);
}

static int loop_mode_0(void) {

  // Call update_status for each PIN/KEY pair in XMACRODATA
#define XMACRO(P, M, K) update_status(K, M, digitalRead(P));
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
// XMACRO(PIN, MODE, KEY)
#define XMACRODATA         \
  XMACRO(2, SEND_JOY_BUTTON, 0) \
  XMACRO(3, SEND_JOY_BUTTON, 1) \
  XMACRO(4, SEND_JOY_BUTTON, 3) \
//END XMACRODATA

static void setup_mode_1(void){

  // Set all the pins defined in XMACRODATA to pullup input
#define XMACRO(P, M, K) pinMode(P, INPUT_PULLUP);
  XMACRODATA;
#undef XMACRO

  // Additional pin for mode switch
  pinMode(7, INPUT_PULLUP);
}

static int loop_mode_1(void) {

  // Call update_status for each PIN/KEY pair in XMACRODATA
#define XMACRO(P, M, K) update_status(K, M, digitalRead(P));
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
