#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "fetcher.h"
#include "buffer.h"
#include "lib/iopins.h"

#define SOH 0x01
#define ACK 0x06
#define NAK 0x15
#define EOT 0x04
#define STARTING_BLOCK_NUMBER 1
#define PACKET_LENGTH 128
#define BAUD_CONSTANT_2X_115200 16
#define BAUD_CONSTANT_2X_500000 3

enum TransferState {
    NOT_STARTED,
    WAITING_FOR_HEADER,
    WAITING_FOR_BLOCK_NUM,
    WAITING_FOR_INVERSE_BLOCK_NUM,
    RECEIVING,
    WAITING_FOR_CHECKSUM,
    WAITING_FOR_BUFFER
};

volatile enum TransferState transferState;
volatile uint8_t sendIndex;
volatile uint8_t blockNumber;
uint8_t checksum;

void send_byte(uint8_t byte) {
    /* Wait for empty transmit buffer */
    // This should never happen under normal circumstance since we send very little
    while (!(UCSR0A & (1<<UDRE0))) {
        pin_toggle(13);
    }
    /* Put data into buffer, sends the data */
    UDR0 = byte;
}

void fetcher_init() {
    UCSR0A = 1 << U2X0; // double speed for more accurate 115200 baud
    UCSR0C = 0b11 << UCSZ00; // 8-N-1
    UBRR0 = BAUD_CONSTANT_2X_500000; // from manual
    UCSR0B = (1 << RXCIE0) | (1 << RXEN0) | (1 << TXEN0); // Enable TX, RX, RX interrupt

    if(buffer_push_block_advance()) {
        blockNumber++;
        transferState = WAITING_FOR_HEADER;
        send_byte(NAK);
    }
    else {
        transferState = WAITING_FOR_BUFFER;
    }
}
void fetcher_start() {
    send_byte(NAK);
}

bool fetcher_check_started() {
    return (transferState != NOT_STARTED) && (transferState != WAITING_FOR_HEADER);
}

void fetcher_buffer_ready() {
    if (transferState == WAITING_FOR_BUFFER) {
        buffer_push_block_advance();
        blockNumber++;
        send_byte(ACK); // tell sender to continue sending
        transferState = WAITING_FOR_HEADER;
    }
}

ISR (USART_RX_vect) {
    /*send_byte(transferState);
    send_byte(blockNumber);
    send_byte(sendIndex);*/
    uint8_t newByte = UDR0;
    switch (transferState) {
        case NOT_STARTED:
            // Do nothing???
            break;
        
        case WAITING_FOR_HEADER:
            if (newByte == SOH) {
                transferState = WAITING_FOR_BLOCK_NUM;
            }
            else {
                // abort
                transferState = NOT_STARTED;
            }
            break;

        case WAITING_FOR_BLOCK_NUM:
            if (newByte == blockNumber) {
                transferState = WAITING_FOR_INVERSE_BLOCK_NUM;
            }
            else {
                // abort
                transferState = NOT_STARTED;
            }
            break;

        case WAITING_FOR_INVERSE_BLOCK_NUM:
            if (newByte == 255 - blockNumber) {
                transferState = RECEIVING;
                sendIndex = 0;
                checksum = 0;
            }
            else {
                // abort
                transferState = NOT_STARTED;
            }
            break;

        case RECEIVING:
            buffer_push_byte(newByte);
            checksum += newByte;
            sendIndex++;
            if (sendIndex == PACKET_LENGTH) {
                transferState = WAITING_FOR_CHECKSUM;          
            }
            break;

        case WAITING_FOR_CHECKSUM:
            if (newByte == checksum) {
                // try to advance buffer block
                if (buffer_push_block_advance()) {
                    blockNumber++;
                    send_byte(ACK);
                    transferState = WAITING_FOR_HEADER;
                }
                else {
                    transferState = WAITING_FOR_BUFFER;
                    // we will ACK later once we have buffer
                }
            }
            else {
                // abort
                transferState = NOT_STARTED;
            }

            break;

        default:
            // abort
            transferState = NOT_STARTED;
    }   

    
}




