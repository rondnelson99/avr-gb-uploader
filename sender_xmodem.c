#include "buffer.h"
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define SOH 0x01
#define ACK 0x06
#define NAK 0x15
#define EOT 0x04
#define ENQ 0x05

#define STARTING_BLOCK_NUMBER 1
#define PACKET_LENGTH 128

// This is an adaptation of XMODEM to SPI, where the receiver is the master and sender is slave
// ENQ is used by the slave to show that it's listening, but has nothing to send.
// Similarly ENQ is sent by the master to poll the slave/sender without sending any information
// WAI

enum TransferState {
    NOT_STARTED,
    SENDING_SOH,
    SENDING_BLOCK_NUM,
    SENDING_INVERSE_BLOCK_NUM,
    SENDING_DATA,
    SENDING_CHECKSUM,
    WAITING_FOR_ACK,
    WAITING_FOR_BUFFER
};

volatile enum TransferState transferState;
volatile uint8_t sendIndex;
volatile uint8_t blockNumber;
uint8_t checksum;

void sender_init() {
    transferState = NOT_STARTED;
    blockNumber = STARTING_BLOCK_NUMBER;
    // Interrupt on, SPI on, MSB first, slave, Mode 3
    SPCR = (1<<SPIE) | (1<<SPE) | (3 << CPHA);
    SPDR = ENQ; // We're listening but not sending until the master requests us to
}

ISR (SPI_STC_VECT) {
    uint8_t newByte = SPDR;
    switch (transferState) {
        case NOT_STARTED:
            if (newByte == NAK) {
                // Check if we have data to send
                if (buffer_read_ready()) {
                    SPDR = SOH;
                    transferState = SENDING_SOH;
                }
                else {
                    // Don't init trandfer yet but keep listening
                    SPDR = ENQ;
                }
            }
            break;

        case SENDING_SOH:
            if (newByte == ENQ) {
                SPDR = blockNumber;
                transferState = SENDING_BLOCK_NUM;
            }
            else {
                // abort
                transferState = NOT_STARTED;
            }
            break;

        case SENDING_BLOCK_NUM:
            if (newByte == ENQ) {
                SPDR = 255 - blockNumber;
                transferState = SENDING_INVERSE_BLOCK_NUM;
            }
            else {
                // abort
                transferState = NOT_STARTED;
            }
            break;
        
        case SENDING_INVERSE_BLOCK_NUM:
            if (newByte == ENQ) {
                uint8_t dataByte = buffer_read_byte();
                SPDR = dataByte;
                transferState = SENDING_DATA;
                sendIndex = 1;
                checksum = dataByte;
            }
            else {
                // abort
                transferState = NOT_STARTED;
            }
            break;
        
        case SENDING_DATA:
            if (newByte == ENQ) {
                if (sendIndex < PACKET_LENGTH) {
                    uint8_t dataByte = buffer_read_byte();
                    SPDR = dataByte;
                    checksum += dataByte;
                    sendIndex++;
                }
                else {
                    SPDR = checksum;
                    transferState = SENDING_CHECKSUM;
                }
            }               
            else {
                // abort
                transferState = NOT_STARTED;
            }      
            break;

        case SENDING_CHECKSUM:
            if (newByte == ENQ) {
                SPDR = ENQ;
                transferState = WAITING_FOR_ACK;
            }
            else {
                // abort
                transferState = NOT_STARTED;
            }
            break;

        case WAITING_FOR_ACK:
            if (newByte == ACK) {
                // Check if we can send the next packet
                if (buffer_read_block_advance()) {
                    SPDR = SOH;
                    transferState = SENDING_SOH;
                }
                else{
                    SPDR = ENQ;
                    transferState = WAITING_FOR_BUFFER;
                }
            }
            else {
                // abort
                transferState = NOT_STARTED;
            }
            break;  

        case WAITING_FOR_BUFFER:
            if (newByte == ENQ) {
                // Check if we can send the next packet
                if (buffer_read_block_advance()) {
                    SPDR = SOH;
                    transferState = SENDING_SOH;
                }
                else{
                    SPDR = ENQ;
                    transferState = WAITING_FOR_BUFFER;
                }
            }
            else {
                // abort
                transferState = NOT_STARTED;
            }
            break; 
    }
}


