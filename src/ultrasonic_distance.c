#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <serial.h>

#define TRIGGER_ECHO_PIN PD0 // Shared Trigger and Echo pin connected to PD0 (INT0)

volatile uint16_t timer_count = 0; // Timer count for timer3
volatile uint8_t echo_received = 0; // Flag to indicate state of echo reception
volatile uint8_t measurement_ready = 0; // Flag to indicate measurement is ready to be processed
volatile uint8_t read_ready = 0; // Flag to indicate ready to read echo
volatile uint8_t timeout_counter = 0; // Timer count for overflow
volatile uint8_t state = 0; // State variable for timer1 compare match ISR

char buffer[20];

void init_interrupts() {
  EICRA |= (1 << ISC01) | (1 << ISC00); // ISC01 = 1, ISC00 = 1 -> Rising edge
  EIMSK |= (1 << INT0); // Enable INT0
}

void init_timer1() {
  TCCR1A = 0; // Normal mode
  TCCR1B = (1 << WGM12) | (1 << CS11); // Prescaler = 8
  OCR1A = 37000; // compare for 18.5ms holdoff
  TIMSK1 |= (1 << OCIE1A); // Enable compare match interrupt
}

void init_timer3() {
  TCCR3A = 0; // Normal mode
  TCCR3B = (1 << CS31); // Prescaler = 8
  TCNT3 = 0; // Reset timer count
}

void init_timer0() {
  TCCR0A = 0; // Normal mode
  TCCR0B = (1 << CS01) | (1 << CS00); // Prescaler = 64
  TIMSK0 |= (1 << TOIE0); // Enable overflow interrupt
}

ISR(TIMER1_COMPA_vect) {
  if (state == 0) {
    DDRD |= (1 << TRIGGER_ECHO_PIN); // Set the pin as output to send the trigger pulse
    PORTD |= (1 << TRIGGER_ECHO_PIN); // Start the pulse
    OCR1A = 10; // compare for 5us pulse
    state = 1;
  }
  else {
    PORTD &= ~(1 << TRIGGER_ECHO_PIN); // End the pulse
    DDRD &= ~(1 << TRIGGER_ECHO_PIN); // Set the pin as input to listen for the echo
    timeout_counter = 0; // Reset timeout counter
    echo_received = 0; // Reset echo received to 'no'
    TCNT3 = 0; // Reset timer count
    read_ready = 1; // Set flag to indicate ready to read echo
    OCR1A = 37000; // compare for 18.5ms holdoff before listening for echo
    state = 0;
  }
}

ISR(TIMER0_OVF_vect) {
  if (read_ready == 0) // Only count timeouts when waiting for echo
  return;
  timeout_counter += 1; // Increment timer
  if (timeout_counter >= 19) { // Approximately 19 * 1.024 ms = 19.456 ms, which is above the max expected time for a valid reading
    timeout_counter = 0; // Reset timeout counter
    read_ready = 0; // Reset read ready to prevent further processing of echo
    if (measurement_ready == 0) { // If measurement is not already ready
      timer_count = 0; // Set timer count to 0 to indicate timeout
      measurement_ready = 1; // Set measurement ready to allow for new reading
    }
  }
}

ISR(INT0_vect) {
  if (read_ready == 0) // Ensure not reading trigger output
  return;
  if (echo_received == 0) {
    TCNT3 = 0; // Reset Timer3
    echo_received = 1; // Set flag to indicate echo received
    EIMSK &= ~(1 << INT0); // Disable INT0 to prevent further interrupts while processing
    EICRA &= ~(1 << ISC00); // ISC01 = 1, ISC00 = 0 -> Falling edge
    EIFR |= (1 << INTF0); // Clear any pending INT0 interrupts
    EIMSK |= (1 << INT0); // Enable INT0
  }
  else {
    uint16_t t = TCNT3; // Capture the timer count for distance calculation
    echo_received = 0; // Reset echo received for next measurement
    read_ready = 0; // Reset read ready for next measurement
    EIMSK &= ~(1 << INT0); // Disable INT0 to prevent further interrupts while processing
    EICRA |= (1 << ISC00); // ISC01 = 1, ISC00 = 1 -> Rising edge
    EIFR |= (1 << INTF0); // Clear any pending INT0 interrupts
    EIMSK |= (1 << INT0); // Enable INT0
    if (measurement_ready == 0) { // If measurement is not already ready
      timer_count = t; // Set timer count to captured value
      measurement_ready = 1; // Set measurement ready to allow for processing in main loop
    }
  }
}

uint16_t calculate_distance(uint16_t ticks) {
  // Timer3 clock = 16MHz / 8 = 2MHz -> 0.5us per tick
  // Distance = (ticks * 0.5us) / 58 (speed of sound in cm/us)
  return (ticks / 2) / 58;
}

int main() {
  DDRD &= ~(1 << TRIGGER_ECHO_PIN); // Configure the shared pin as input initially
  PORTD &= ~(1 << TRIGGER_ECHO_PIN); // Ensure the pin is low
  // Initialize peripherals
  init_interrupts();
  init_timer1();
  init_timer3();
  init_timer0();
  serial0_init();
  sei(); // Enable global interrupts
  while (1) {
    if (measurement_ready) {
      uint16_t t;
      cli();
      t = timer_count;
      measurement_ready = 0;
      sei();
      if (t == 0 || t < 115 || t > 18500) {
        serial0_print_string("Out of range\n");
      } else {
        uint16_t distance = calculate_distance(t);
        sprintf(buffer, "%u\n", distance);
        serial0_print_string(buffer);
      }
    }
  }
}