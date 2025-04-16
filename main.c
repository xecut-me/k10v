#include <avr/io.h>
#include <util/delay.h>
#include <stddef.h>

// UART Configuration
#define TX_PIN PD2
#define BAUD_DELAY_US 104 // 9600 baud
#define BITS_IN_BYTE 8

#define LED_PORT PORTB
#define LED_DDR DDRB
#define LED_MASK (1 << PB0 | 1 << PB1 | 1 << PB4 | 1 << PB5 )

// Keyboard Matrix Configuration
#define ROW_PORT PORTC
#define ROW_DDR DDRC
#define ROW_MASK ((1 << PC0) | (1 << PC1) | (1 << PC2) | (1 << PC3) | (1 << PC4) | (1<<PC5))

#define COL_PORTB PINB
#define COL_PORTD PIND
#define COL_MASKB ((1 << PB6) | (1 << PB7)) // Columns on PB6-PB7
#define COL_MASKD ((1 << PD5)|(1 << PD6))   // Column on PD5-PD6

// Key Mapping
const char BOARD[] = "P123T456M789BC0EOSLA";
// const uint8_t  LED_COLS[4] = { PB0, PB1 ,PB4 ,PB5 };
const uint8_t  LED_COLS[4] = {0b00110010, 0b00110001 ,0b00100011,0b00010011 };

// Buffer Configuration
#define BUFFER_SIZE 8
char buffer[BUFFER_SIZE];
uint8_t buffer_len = 0;

void uart_transmit(char data) {
    // Start bit
    PORTD &= ~(1 << TX_PIN);
    _delay_us(BAUD_DELAY_US);

    // Data bits
    for (uint8_t i = 0; i < BITS_IN_BYTE; i++) {
        if (data & (1 << i)) {
            PORTD |= (1 << TX_PIN);
        } else {
            PORTD &= ~(1 << TX_PIN);
        }
        _delay_us(BAUD_DELAY_US);
    }

    // Stop bit
    PORTD |= (1 << TX_PIN);
    _delay_us(BAUD_DELAY_US);
}

void send_buffer() {
    if (!buffer_len) return;
    // uint8_t leds_save = LED_PORT & LED_MASK;
    // LED_PORT |= LED_MASK;          // LED off
    for (uint8_t i = 0; i < buffer_len; i++) {
        uart_transmit(buffer[i]);
    }
    uart_transmit('E'); // End marker
    buffer_len = 0;     // Clear buffer
    // LED_PORT &= leds_save;
}

void clear_buffer() {
    buffer_len = 0; // Simply reset the buffer length
}

int main() {
    // Initialize UART TX, rows, and LED
    LED_DDR = LED_MASK;   // LED as output
    ROW_DDR = ROW_MASK;  // Rows as output
    DDRD = (1 << TX_PIN) | (1<<PD0) | (1<<PD1); // UART TX as output

    LED_PORT = 0;          // LED off
    ROW_PORT = ~0;      // Pull-ups for rows
    PORTD = 1 << TX_PIN ; // UART TX high (idle)

    // Track previous states for each row
    uint8_t prev_pinb[5] = {0, 0, 0, 0, 0};
    uint8_t prev_pind[5] = {0, 0, 0, 0, 0};

    _delay_ms(500);

    // Send "HELLO" over UART
    uart_transmit('I');
    uart_transmit('N');
    uart_transmit('I');
    uart_transmit('T');
    uart_transmit('E');

    while (1) {
        for (uint8_t row = 0; row < 5; row++) {
            ROW_PORT = ~(1 << row); // Activate the current row
            _delay_us(100);         // Allow signals to stabilize

            // Read column states
            uint8_t pinb = COL_PORTB & COL_MASKB;
            uint8_t pind = COL_PORTD & COL_MASKD;

            char key = 0;
            int8_t col = -1;
            do {
                // Unwrapped column checking
                if ((pinb & (1 << PB6)) && !(prev_pinb[row] & (1 << PB6))) {
                    col = 0;
                    break;
                }
                if ((pinb & (1 << PB7)) && !(prev_pinb[row] & (1 << PB7))) {
                    col = 1;
                    break;
                }
                if ((pind & (1 << PD5)) && !(prev_pind[row] & (1 << PD5))) {
                    col = 2;
                    break;
                }
                if ((pind & (1 << PD6)) && !(prev_pind[row] & (1 << PD6))) {
                    col = 3;
                    break;
                }
            }while(0);

            uint8_t leds_save = LED_PORT;
            if (col != -1){
                key = BOARD[row * 4 + col];
                LED_PORT = LED_COLS[col];
            }
            switch (key) {
                case 0:
                    break;
                case 'E':
                    send_buffer();
                    break;
                case 'C':
                    clear_buffer();
                    break;
                default:
                    if (buffer_len < BUFFER_SIZE) {
                        buffer[buffer_len++] = key; // Add key to buffer
                    }
            }
            if (buffer_len==BUFFER_SIZE){
                send_buffer();
            }else if (key){
                _delay_us(BAUD_DELAY_US*8);
            }

            // Update previous states for the current row
            prev_pinb[row] = pinb;
            prev_pind[row] = pind;
            LED_PORT = leds_save;
        }

        LED_PORT = 0;
        ROW_PORT = ROW_MASK; // Deactivate all rows
        _delay_ms(40); // Debounce delay
    }

    return 0;
}
