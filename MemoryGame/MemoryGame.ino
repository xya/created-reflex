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
#define STATE_SHOW_SCORE 4
#define STATE_END_ROUND 5
#define STATE_END_GAME 6
int state;
int max_score;
#define MAX_ROUNDS 25
int round_id;
int cur_seq_len;
int cur_attempt;

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
const int button_pins_p1[BUTTONS] = {
  4, // Red
  3, // Blue
  2  // Green
};

const int button_pins_p2[BUTTONS] = {
  10, // Red
  9, // Blue
  8  // Green
};

struct button {
  int id;
  int pin;
  int state;
  int prev_state;
  int event;
  int is_pending;
  long pending_timestamp;
};

const int debounce_time = 10;

// Sequences
#define MAX_SEQ_LEN 10
struct sequence {
  int count;
  int values[MAX_SEQ_LEN];
};

struct sequence current_seq;
int current_seq_pos;

#define SCORE_LIMIT 100
struct player {
  const int *button_pins;
  struct button buttons[BUTTONS];
  struct sequence seq;  
  int score;
};

struct player p1;
struct player p2;

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

  round_id = 0;
  for (int i = 0; i < BUTTONS; i++) {
    p1.buttons[i].pin = button_pins_p1[i];
  }
  for (int i = 0; i < BUTTONS; i++) {
    p2.buttons[i].pin = button_pins_p2[i];
  }
  p1.score = p2.score = 0;
  max_score = 0;
  cur_seq_len = 3;
  cur_attempt = -1;
  newRound();
}

void newRound() {
  round_id++;
  cur_attempt++;
  if (cur_attempt == cur_seq_len) {
    cur_attempt = 0;
    if (cur_seq_len < MAX_SEQ_LEN) {
      cur_seq_len++;
    }
  }
  state = STATE_GENERATE_SEQ;
  resetButtons(p1.buttons);
  resetButtons(p2.buttons);
  current_seq_pos = 0;
  current_seq.count = 0;
  p1.seq.count = 0;
  p2.seq.count = 0;
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Round ");
  display.println(round_id);
  display.print("  x ");
  display.println(cur_seq_len);
  display.display();
}

void resetButtons(struct button *buttons) {
  for (int i = 0; i < BUTTONS; i++) {
    struct button *b = &buttons[i];
    b->id = i;
    b->state = b->prev_state = 0;
    b->event = BUTTON_EVENT_NONE;
    b->is_pending = 0;
    b->pending_timestamp = 0;
    pinMode(b->pin, INPUT_PULLUP);
  }
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
  b->state = ~digitalRead(b->pin) & 1;
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
  for (int i = 0; i < cur_seq_len; i++) {
    seq->values[i] = random(0, LEDS);
    seq->count++;
  }
}

void showSequence(struct sequence *seq, int *current_index) {
  if (*current_index < cur_seq_len) {
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

void inputSequenceValue(struct sequence *seq, struct button *buttons) {
  for (int i = 0; i < BUTTONS; i++) {
    readButton(&buttons[i]);
  }
  for (int i = 0; i < BUTTONS; i++) {
    if (buttons[i].event == BUTTON_EVENT_PRESSED) {
      seq->values[seq->count] = i;
      seq->count++;
      break;
    }
  }
}

void inputSequence() {
  int p1_done = (p1.seq.count == cur_seq_len);
  int p2_done = (p2.seq.count == cur_seq_len);
  if (p1_done && p2_done) {
    state = STATE_VERIFY_SEQ;
    return;
  }
  if (!p1_done) {
    inputSequenceValue(&p1.seq, p1.buttons);
  }
  if (!p2_done) {
    inputSequenceValue(&p2.seq, p2.buttons);
  }
}

int compareSequences(struct sequence *seq_a, struct sequence *seq_b) {
  if (seq_a->count != seq_b->count) {
    return seq_a->count - seq_b->count;
  }
  int num_mistakes = 0;
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
      num_mistakes++;
    }
  }
  return num_mistakes;
}

void printFeedback(int num_correct, int total) {
  if (num_correct == total) {
    display.println("All good!");
  } else {
    display.print(num_correct);
    display.print(" of ");
    display.println(total);
  }
}

void verifySequence() {
  int num_correct_p1 = current_seq.count - compareSequences(&current_seq, &p1.seq);
  int num_correct_p2 = current_seq.count - compareSequences(&current_seq, &p2.seq);
  p1.score += num_correct_p1;
  p2.score += num_correct_p2;

  int best_score = p1.score > p2.score ? p1.score : p2.score;
  max_score += current_seq.count;
  
  display.clearDisplay();
  display.setCursor(0, 0);
  printFeedback(num_correct_p1, current_seq.count);
  printFeedback(num_correct_p2, current_seq.count);
  display.display();

  delay(1500);

  if ((best_score < SCORE_LIMIT) && (round_id < MAX_ROUNDS)) {
    state = STATE_SHOW_SCORE;
  } else {
    state = STATE_END_GAME;
  }
}

void showScore() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("P1:");
  display.println(p1.score);
  display.print("P2:");
  display.println(p2.score);
  display.display();
  delay(2000);
  state = STATE_END_ROUND;
}

void showFinalScore() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("P1:");
  display.print(p1.score);
  display.print("/");
  display.println(max_score);
  display.print("P2:");
  display.print(p2.score);
  display.print("/");
  display.println(max_score);
  display.display();
  delay(2000);
}

void loop() {
  switch (state) {
  default:
  case STATE_END_ROUND:
    delay(2000);
    newRound();
    break;
  case STATE_GENERATE_SEQ:
    generateSequence(&current_seq);
    state = STATE_SHOW_SEQ;
    break;
  case STATE_SHOW_SEQ:
    showSequence(&current_seq, &current_seq_pos);
    break;
  case STATE_INPUT_SEQ:
    inputSequence();
    break;
  case STATE_VERIFY_SEQ:
    verifySequence();
    break;
  case STATE_SHOW_SCORE:
    showScore();
    break;
  case STATE_END_GAME:
    showFinalScore();
    break;
  } 
}
