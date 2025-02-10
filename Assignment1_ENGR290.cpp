// Libraries
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <math.h>
#include <stdio.h>

// Define Pins
#define TRIGGER_PIN 5   // PB5
#define ECHO_PIN 3      // PD3 (INT1 - External Interrupt)
#define IR_PIN 0        // PC0 (ADC Channel)
#define L_LED 5         // PB5 (Warning LED)
#define D3_LED 4        // PB4 (PWM LED)

// UART Setup
#define BAUD 9600       //serial communication speed to 9600 bps
#define F_CPU 16000000UL //CPU clock speed is 16MHz
#define UBRR F_CPU/16/BAUD-1 ///formula for baud rate setting

// Distances
#define D1 12
#define D2 46

// Function Prototypes
void pinModeC(uint8_t pin, uint8_t mode);
void init();
void sendTriggerPulse();
float readEchoPulse();
uint16_t read_ADC(uint8_t channel);
float ADC_to_distance(uint16_t adc_value);
void uart_transmit(unsigned char data);
void uart_print(const char* str);
void uart_print_float(float num, int decimal_places);
void set_LED_brightness(float distance);
void flash_L_LED(float distance);

// Timing Variables
volatile uint16_t pulseStart = 0;
volatile uint16_t pulseEnd = 0;
volatile uint16_t pulseDuration = 0;
volatile uint8_t pulseMeasured = 0;

// Interrupt Service Routine
ISR(INT1_vect) {
    if (PIND & (1 << ECHO_PIN)) { //eacho high reset timer 0
        //TCNT1 = 0;  
        pulseStart = TCNT1;
    } else {
        pulseEnd = TCNT1;  //echo low-recordtime pulseDuration = pulseEnd - pulseStart;
        pulseDuration = pulseEnd - pulseStart;
        pulseMeasured = 1; //for valid reading
    }
}

// Main function!!!

int main(void) {
    float IR_adc_result, IR_distance, IR_voltage;
    float US_distance;

    init();

    while (1) {
        sendTriggerPulse();
        if (pulseMeasured) {  
            US_distance = readEchoPulse();
            uart_print("\nUltrasonic Pulse Length: ");
            uart_print_float((pulseDuration / 2.0), 2);
            uart_print(" us");

            uart_print("\nUltrasonic Sensor Distance: ");
            uart_print_float(US_distance, 2);
            uart_print(" cm");
            pulseMeasured = 0;
        }

        IR_adc_result = read_ADC(0);
        IR_voltage = (IR_adc_result / 1023.0) * 5;
        IR_distance = 29 * pow(IR_voltage, -1.17); //according to datasheet

        uart_print("\nIR ADC Value: ");
        uart_print_float(IR_adc_result, 1);
        uart_print("\nIR Sensor Distance: ");
        uart_print_float(IR_distance, 2);
        uart_print(" cm");

        set_LED_brightness(IR_distance);
        flash_L_LED(IR_distance);

        _delay_ms(500);
    }
}

// Function to set pin mode using registers
void pinModeC(uint8_t pin, uint8_t mode) { //it is replace pinMode()
    uint8_t port = pin / 8;
    uint8_t bit = pin % 8;

    volatile uint8_t *ddr, *port_reg;

    switch (port) {
        case 0: ddr = &DDRB; port_reg = &PORTB; break;
        case 1: ddr = &DDRC; port_reg = &PORTC; break;
        case 2: ddr = &DDRD; port_reg = &PORTD; break;
        default: return;
    }

    if (mode == 1) { // OUTPUT
        *ddr |= (1 << bit);
    } else if (mode == 0) { // INPUT
        *ddr &= ~(1 << bit);
    } else if (mode == 2) { // INPUT_PULLUP
        *ddr &= ~(1 << bit);
        *port_reg |= (1 << bit);
    }
}

void init() {
    // Use pinModeC() instead of direct DDR manipulation
    pinModeC(TRIGGER_PIN, 1);  // OUTPUT for ultrasonic trigger
    pinModeC(ECHO_PIN, 0);     // INPUT for ultrasonic echo
    pinModeC(IR_PIN, 0);       // INPUT for IR sensor
    pinModeC(D3_LED, 1);       // OUTPUT for LED
    pinModeC(L_LED, 1);        // OUTPUT for warning LED

    // Initialize Timer1 for Pulse Measurement
    TCCR1A = 0;
    TCCR1B = (1 << CS11);

    // Initialize Timer2 for Variable LED Brightness
    TCCR2A = (1 << COM2A1) | (1 << WGM20);
    TCCR2B = (1 << CS21);

    // Enable External Interrupt on INT1 (PD3)
    EICRA |= (1 << ISC10);
    EIMSK |= (1 << INT1);

    // Initialize ADC
    ADMUX = (1 << REFS0);
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

    // UART Initialization
    uint16_t ubrr_value = UBRR;
    UBRR0H = (uint8_t)(ubrr_value >> 8);
    UBRR0L = (uint8_t)ubrr_value;
    UCSR0B = (1 << TXEN0) | (1 << RXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);

    sei();
}

void sendTriggerPulse() {
    PORTB &= ~(1 << TRIGGER_PIN);
    _delay_us(2);
    PORTB |= (1 << TRIGGER_PIN);
    _delay_us(10); //generate a 10 microsecond pulse to trigger the ultrasonic sensor
    PORTB &= ~(1 << TRIGGER_PIN);
}

float readEchoPulse() {
    float pulseDurationMicroseconds = pulseDuration / 2.0;
    return pulseDurationMicroseconds / 58.0;
}

uint16_t read_ADC(uint8_t channel) {
    ADMUX = (ADMUX & 0xF8) | (channel & 0x07);
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    return ADC;
}

//float ADC_to_distance_3V3(uint16_t adc_value) {
   // return  3 * ((3000.0 / (adc_value - 15)) - 1);
//}

void set_LED_brightness(float distance) {
    if (distance <= D1) {
        OCR2A = 255;
    } else if (distance >= D2) {
        OCR2A = 26;
    } else {
        OCR2A = (uint8_t)(255 - ((distance - D1) / (D2 - D1)) * (255-26));
    }
}

void flash_L_LED(float distance) {
    if (distance < D1 || distance > D2) {
        PORTB ^= (1 << L_LED);
        _delay_ms(500);
    } else {
        PORTB &= ~(1 << L_LED);
    }
}

void uart_transmit(unsigned char data) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = data;
}

void uart_print(const char* str) {
    while (*str) {
        uart_transmit(*str++);
    }
}

void uart_print_float(float num, int decimal_places) {
    char buffer[32];
    dtostrf(num, 6, decimal_places, buffer);
    uart_print(buffer);
}
