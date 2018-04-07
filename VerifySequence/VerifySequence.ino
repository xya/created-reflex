#define LEDS 3
#define SEQ_LEN 3
// Red, blue, green
int led_pins[LEDS] = {4, 6, 5};
int sequence[SEQ_LEN];
int sequence_next = 0;
int time_on = 250;
int time_off = 250;

// the setup function runs once when you press reset or power the board
void setup() {
  for (int i = 0; i < LEDS; i++) {
    pinMode(led_pins[i], OUTPUT);
    digitalWrite(led_pins[i], LOW);
  }

  // Generate a random sequence.
  randomSeed(analogRead(0));
  for (int i = 0; i < SEQ_LEN; i++) {
    sequence[i] = random(0, LEDS);
  }
}

// the loop function runs over and over again forever
void loop() {
  if (sequence_next < SEQ_LEN) {
    int led = sequence[sequence_next];
    int pin = led_pins[led];
    digitalWrite(pin, HIGH);
    delay(time_on);
    digitalWrite(pin, LOW);
    delay(time_off);
    sequence_next++;
  } else {
    delay(1000);
  }
}
