#include "buffer.h"
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define INIT_TOKEN 42 // sent by the GB
#define RECEIVED_TOKEN 43 // received by the GB 2nd
#define START_TOKEN 44 // sent by the GB 2nd
#define WAIT_TOKEN 45 // sent by the MCU instead of RECIEVED_TOKEN when it needs more time to download stuff from the computer.
#define READ_BLOCK_SIZE 128
#define SEND_BLOCK_SIZE 256

enum TransferState {
    NOT_STARTED,
    WAITING_FOR_START_TOKEN,
    SENDING
};

volatile enum TransferState transferState;
volatile uint16_t sendIndex;

void sender_init() {
    transferState = NOT_STARTED;
    buffer_read_block_advance(); // just hope this works I guess
    // Interrupt on, SPI on, MSB first, slave, Mode 3
    SPCR = (1<<SPIE) | (1<<SPE) | (3 << CPHA);
}

ISR (SPI_STC_vect) {
    uint8_t newByte = SPDR;
    switch (transferState) {
        case NOT_STARTED:
            if (newByte == INIT_TOKEN) {
                SPDR = RECEIVED_TOKEN;
                transferState = WAITING_FOR_START_TOKEN;
            }
            break;
        case WAITING_FOR_START_TOKEN:
            if (newByte == START_TOKEN) {
                sendIndex = 0;
                SPDR = buffer_read_byte();
                sendIndex++;
            }
            else {
                // abort
                transferState = NOT_STARTED;
            }
            break;
        case SENDING:
            SPDR = buffer_read_byte();
            sendIndex++;
            if (sendIndex == READ_BLOCK_SIZE) {
                buffer_read_block_advance(); // just hope this works I guess
            }
            if (sendIndex == SEND_BLOCK_SIZE) {
                sendIndex = 0;
                buffer_read_block_advance();
                transferState = NOT_STARTED; // I guess we just completely restart the transfer every 256 bytes
            }
    }
}


