// #define USB_DEBUG

// Display
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

// General state
#define STATE_GENERATE_SEQ 0
#define STATE_SHOW_SEQ 1
#define STATE_INPUT_SEQ 2
#define STATE_VERIFY_SEQ 3
#define STATE_END 4
int state;

// LEDs
#define LEDS 3
#define LED_RED 0
#define LED_BLUE 1
#define LED_GREEN 2
const int led_pins[LEDS] = {
  5, // Red
  6, // Blue
  7  // Green
};

// Buttons
#define BUTTONS 3
#define BUTTON_EVENT_NONE 0
#define BUTTON_EVENT_PRESSED 1
#define BUTTON_EVENT_RELEASED 2
int button_pins[BUTTONS] = {
  4, // Red
  3, // Blue
  2  // Green
};

struct button {
  int id;
  int state;
  int prev_state;
  int event;
  int is_pending;
  long pending_timestamp;
};

struct button buttons[BUTTONS];
int debounce_time = 10;

// Sequences
#define SEQ_LEN 3
struct sequence {
  int count;
  int values[SEQ_LEN];
};

struct sequence current_seq;
int current_seq_pos;

struct sequence user_seq;

const int time_on = 250;
const int time_off = 250;

void setup() {
  for (int i = 0; i < LEDS; i++) {
    pinMode(led_pins[i], OUTPUT);
    digitalWrite(led_pins[i], LOW);
  }
  randomSeed(analogRead(0));

#ifdef DEBUG_USB
  Serial.begin(9600);
#endif

  // Init the display.
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextSize(2);
  display.setTextColor(WHITE);

  reset();
}

void reset() {
  state = STATE_GENERATE_SEQ;
  for (int i = 0; i < BUTTONS; i++) {
    struct button *b = &buttons[i];
    b->id = i;
    b->state = b->prev_state = 0;
    b->event = BUTTON_EVENT_NONE;
    b->is_pending = 0;
    b->pending_timestamp = 0;
    pinMode(button_pins[i], INPUT_PULLUP);
  }
  current_seq_pos = 0;
  current_seq.count = 0;
  user_seq.count = 0;
  display.clearDisplay();
  display.display();
}

void showColor(int color) {
  int levels[3] = {LOW, LOW, LOW};
  switch (color) {
  default:
    return;
  case LED_RED:
    levels[0] = HIGH;
    break;
  case LED_GREEN:
    levels[1] = HIGH;
    break;
  case LED_BLUE:
    levels[2] = HIGH;
    break;
  }
  for (int i = 0; i < LEDS; i++) {
    digitalWrite(led_pins[i], levels[i]);
  }
}

void hideColor() {
  for (int i = 0; i < LEDS; i++) {
    digitalWrite(led_pins[i], 0);
  }
}

void readButton(struct button *b) {
  b->state = ~digitalRead(button_pins[b->id]) & 1;
  b->event = BUTTON_EVENT_NONE;
  if (b->is_pending) {
    long elapsed = millis() - b->pending_timestamp;
    if (elapsed > debounce_time) {
      if (!b->state && b->prev_state) {
        b->event = BUTTON_EVENT_PRESSED;
        b->prev_state = b->state;
      } else if (b->state && !b->prev_state) {
        b->event = BUTTON_EVENT_RELEASED;
        b->prev_state = b->state;
      }
      b->is_pending = 0;
    }
  } else if (b->state != b->prev_state) {
    b->is_pending = 1;
    b->pending_timestamp = millis();
  }
}

void generateSequence(struct sequence *seq) {
  for (int i = 0; i < SEQ_LEN; i++) {
    seq->values[i] = random(0, LEDS);
    seq->count++;
  }
}

void showSequence(struct sequence *seq, int *current_index) {
  if (*current_index < SEQ_LEN) {
    int led = seq->values[*current_index];
    showColor(led);
    delay(time_on);
    hideColor();
    delay(time_off);
    *current_index += 1;
  } else {
    state = STATE_INPUT_SEQ;
  }
}

void inputSequence(struct sequence *seq) {
  if (seq->count < SEQ_LEN) {
    for (int i = 0; i < BUTTONS; i++) {
      readButton(&buttons[i]);
    }
    for (int i = 0; i < BUTTONS; i++) {
      if (buttons[i].event == BUTTON_EVENT_PRESSED) {
        seq->values[seq->count] = i;
        seq->count++;
#ifdef DEBUG_USB
        Serial.write("Button pressed: ");
        Serial.println(i);
#endif
        break;
      }
    }
  } else {
    state = STATE_VERIFY_SEQ;
  }
}

int compareSequences(struct sequence *seq_a, struct sequence *seq_b) {
  if (seq_a->count != seq_b->count) {
    return seq_a->count - seq_b->count;
  }
  for (int i = 0; i < seq_a->count; i++) {
    if (seq_a->values[i] != seq_b->values[i]) {
#ifdef DEBUG_USB
      Serial.print("Different value at ");
      Serial.print(i);
      Serial.print(": expected ");
      Serial.print(seq_a->values[i]);
      Serial.print(", actual ");
      Serial.println(seq_b->values[i]);
#endif
      return seq_a->values[i] - seq_b->values[i];
    }
  }
  return 0;
}

void verifySequence() {
  int cmp = compareSequences(&current_seq, &user_seq);
  int color;
  display.clearDisplay();
  display.setCursor(0, 0);
  if (cmp == 0) {
    display.println("Correct!");
    color = LED_GREEN;
  } else {
    display.println("Incorrect");
    color = LED_RED;
  }
  display.display();
  for (int i = 0; i < 10; i++) {
    showColor(color);
    delay(100);
    hideColor();
    delay(100);
  }
  state = STATE_END;
}

void loop() {
  switch (state) {
  default:
    delay(2000);
    reset();
    break;
  case STATE_GENERATE_SEQ:
    generateSequence(&current_seq);
    state = STATE_SHOW_SEQ;
    break;
  case STATE_SHOW_SEQ:
    showSequence(&current_seq, &current_seq_pos);
    break;
  case STATE_INPUT_SEQ:
    inputSequence(&user_seq);
    break;
  case STATE_VERIFY_SEQ:
    verifySequence();
    break;
  } 
}
