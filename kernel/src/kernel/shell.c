#include "kernel/shell.h"
#include "kernel/heap.h"
#include "kernel/pmm.h"
#include <stdint.h>

#include "drivers/console.h"
#include "kernel/timer.h"

// Keep Unicode codepoints directly so Turkish letters occupy one buffer slot.
static uint32_t command_buffer[64];
static uint64_t current_buffer_length;

void shell_init() {
    // Start a new, empty command line.
    current_buffer_length = 0;
    console_write("KBM> ");
}

static int shell_command_is(const char *command) {
    // Typed letters are codepoints; command names are simple ASCII.
    uint64_t index = 0;
    while (command[index] != '\0') {
        if (index >= current_buffer_length) {
            return 0;
        }
        if (command_buffer[index] != (uint32_t)(uint8_t)command[index]) {
            return 0;
        }
        index++;
    }

    return index == current_buffer_length;
}

static int shell_command_starts_with(const char *command) {
    uint64_t index = 0;
    while (command[index] != '\0') {
        if (index >= current_buffer_length) {
            return 0;
        }
        if (command_buffer[index] != (uint32_t)(uint8_t)command[index]) {
            return 0;
        }
        index++;
    }
    return index == current_buffer_length || command_buffer[index] == ' ';
}

void shell_handle_character(uint32_t character) {
    if (character == '\b') {
        if (current_buffer_length != 0) {
            current_buffer_length--;
            console_backspace();
        }
        return;
    }

    // Enter runs the command. Other characters are shown on screen.
    if (character == '\n') {
        console_write_char('\n');
        // Leave one visual gap between the entered command and its result.
        console_write_char('\n');
        if (shell_command_is("KOCAELI")) {
            console_write("BELEDİYE MÜZESİ\n");
        } else if (shell_command_is("CLEAR")) {
            // CLEAR should put the next prompt at the top-left.
            console_clear();
            shell_init();
            return;
        } else if (shell_command_is("MEM")) {
            // Report both PMM frame availability and bump-heap consumption.

            console_write("MEMORY:\n");
            console_write("TOTAL_FRAMES: ");
            console_write_decimal(pmm_get_frame_count());
            console_write("\n");

            console_write("FREE_FRAMES: ");
            console_write_decimal(pmm_get_free_frame_count());
            console_write("\n");

            console_write("FREE_MEMORY_KIB: ");
            console_write_hex(pmm_get_free_frame_count() * 4);
            console_write("\n");

            console_write("\nHEAP:\n");

            console_write("HEAP_FRAMES: ");
            console_write_hex(heap_get_frame_count());
            console_write("\n");

            console_write("HEAP_USED_BYTES: ");
            console_write_hex(heap_get_used_bytes());
            console_write("\n");

            console_write("\nPMM_METADATA:\n");
            console_write("BITMAP_BYTES: ");
            console_write_hex(pmm_get_bitmap_byte_count());
            console_write("\n");

        } else if (shell_command_is("TICKS")) {
            console_write("TICKS: ");
            console_write_decimal(timer_get_ticks());
            console_write("\n");
        } else if (shell_command_is("UPTIME")) {
            console_write("UPTIME: ");
            uint64_t seconds = timer_get_ticks() / 100;
            console_write_decimal(seconds);
            console_write(" SECONDS\n");
        } else if (shell_command_starts_with("ECHO")) {
            if (current_buffer_length > 4) {
                uint64_t index = 5;
                while (index < current_buffer_length) {
                    console_write_codepoint(command_buffer[index]);
                    index++;
                }
            }
            console_write_char('\n');
        } else if (shell_command_is("HELP")) {
            console_write("COMMANDS:\n");
            console_write("HELP\n");
            console_write("CLEAR\n");
            console_write("MEM\n");
            console_write("TICKS\n");
            console_write("UPTIME\n");
            console_write("ECHO <TEXT>\n");
        } else {
            console_write("UNKNOWN COMMAND\n");
        }

        // Separate one command result from the next KBM prompt.
        console_write_char('\n');
        shell_init();
        return;
    }

    if (current_buffer_length < 63) {
        command_buffer[current_buffer_length] = character;
        current_buffer_length++;
        console_write_codepoint(character);
    }
}
