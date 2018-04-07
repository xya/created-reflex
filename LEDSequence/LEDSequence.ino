#define LEDS 3
#define SEQ_LEN 3
int led_pins[LEDS] = {4, 5, 6};
int sequence[SEQ_LEN] = {0, 2, 1};
int sequence_next = 0;
int time_on = 500;
int time_off = 500;

// the setup function runs once when you press reset or power the board
void setup() {
  for (int i = 0; i < LEDS; i++) {
    pinMode(led_pins[i], OUTPUT);
    digitalWrite(led_pins[i], LOW);
  }
}

// the loop function runs over and over again forever
void loop() {
  if (sequence_next < SEQ_LEN) {
    int led = led_pins[sequence[sequence_next]];
    digitalWrite(led, HIGH);
    delay(time_on);
    digitalWrite(led, LOW);
    delay(time_off);
    sequence_next++;
  } else {
    delay(1000);
  }
}
