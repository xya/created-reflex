#define BUTTONS 3
int led_state;
int button_pins[BUTTONS] = {0, 1, 2};
int current_state[BUTTONS];
int previous_state[BUTTONS];


// the setup function runs once when you press reset or power the board
void setup() {
  led_state = 0;
  for (int i = 0; i < BUTTONS; i++) {
    previous_state[i] = 1;
    pinMode(button_pins[i], INPUT_PULLUP);
  }
  pinMode(4, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  for (int i = 0; i < BUTTONS; i++) {
    current_state[i] = digitalRead(button_pins[i]);
  }

  for (int i = 0; i < BUTTONS; i++) {
    if (current_state[i] == 0 && previous_state[i] == 1) {
      led_state ^= 1;
    }
  }
  
  for (int i = 0; i < BUTTONS; i++) {
    previous_state[i] = current_state[i];
  }
  
  if (led_state == 0) {
    digitalWrite(4, LOW);    
  } else {
    digitalWrite(4, HIGH);
  }
  
  delay(10);                       // wait for a second
}
