#include "drivers/uart.h"

#include "drivers/terminal.h"

// For avoiding unused argument compiler warnings cleanly
// TODO: Remove after these empty functions are done
#define UNUSED_ARG __attribute__((unused))

void init_terminal() {}

void echo_char(uint16_t c) {
    uart_print((char*) &c);
}

void echo_raw(const char* data, UNUSED_ARG size_t len) {
    uart_print(data);
}

void terminal_clear() {}
