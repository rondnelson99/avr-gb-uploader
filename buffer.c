#define BLOCK_SIZE 128
#define NUM_BLOCKS 12
#include <stdbool.h>
#include "buffer.h"
#include "fetcher.h"

enum BlockState {
    FREE,
    FILLING, // selected for filling
    READY, // full, marked as complete
    SENDING
};

volatile uint8_t buffer[NUM_BLOCKS][BLOCK_SIZE];
volatile uint8_t blockStates[NUM_BLOCKS];
uint8_t storeBlock = 0;
uint8_t storeIndex = 0;
uint8_t readBlock = 0;
uint8_t readIndex = 0;

void buffer_init() {
    for (int i = 0; i < NUM_BLOCKS; i++) {
        blockStates[i] = FREE;
    }
}

void buffer_push_byte(uint8_t byte) {
    buffer[storeBlock][storeIndex] = byte;
    storeIndex++;
}

// Redo a push block, useful if a checksum fails and it needs to be resent
void buffer_restart_push_block() {
    storeIndex = 0;
}

// Returns True if block advanced, false if buffer full
// Also marks block as ready
bool buffer_push_block_advance() {
    uint8_t nextBlockState = blockStates[storeBlock + 1];
    if (nextBlockState == FREE) {
        blockStates[storeBlock] = READY;
        storeBlock++;
        blockStates[storeBlock] = FILLING;
        return true;
    }
    return false;
}

uint8_t buffer_read_byte() {
    uint8_t byte = buffer[readBlock][readIndex];
    readIndex++;
    return byte;
}

void buffer_restart_read_block() {
    readIndex = 0;
}

// Returns True if block advanced, false if buffer not ready
// Also marks block as ready, and tells fetcher that a block is available
bool buffer_read_block_advance() {
    uint8_t nextBlockState = blockStates[readBlock + 1];
    if (nextBlockState == READY) {
        blockStates[readBlock] = FREE;
        readBlock++;
        blockStates[readBlock] = SENDING;
        fetcher_buffer_ready();
        return true;
    }
    return false;
}

