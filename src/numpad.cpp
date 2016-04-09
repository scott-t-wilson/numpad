#include "WProgram.h"
#include "elapsedMillis.h"
#include "keyswitch.h"
#include "usb_desc.h"
#include "usb_keyboard.h"
#include "usb_serial.h"

#include <algorithm>
#include <list>
#include <vector>

#define PIN_SCLK 4
#define PIN_RCLK 5
#define PIN_SER 12
#define PIN_PWM 23

std::vector<keyswitch> switch_matrix = {
    keyswitch(0, 0, KEY_ESC),
    keyswitch(0, 1, KEY_MEDIA_MUTE),
    keyswitch(0, 2, KEY_MEDIA_VOLUME_DEC),
    keyswitch(0, 3, KEY_MEDIA_VOLUME_INC),
    keyswitch(1, 0, KEY_NUM_LOCK),
    keyswitch(1, 1, KEYPAD_SLASH),
    keyswitch(1, 2, KEYPAD_ASTERIX),
    keyswitch(1, 3, KEYPAD_MINUS),
    keyswitch(2, 0, KEYPAD_7),
    keyswitch(2, 1, KEYPAD_8),
    keyswitch(2, 2, KEYPAD_9),
    // keyswitch(2, 3, 0), // Not used in this layout
    keyswitch(3, 0, KEYPAD_4),
    keyswitch(3, 1, KEYPAD_5),
    keyswitch(3, 2, KEYPAD_6),
    keyswitch(3, 3, KEYPAD_PLUS),
    keyswitch(4, 0, KEYPAD_1),
    keyswitch(4, 1, KEYPAD_2),
    keyswitch(4, 2, KEYPAD_3),
    // keyswitch(4, 3, 0), // Not used in this layout
    keyswitch(5, 0, KEYPAD_0),
    // keyswitch(5, 1, 0), // Not used in this layout
    keyswitch(5, 2, KEYPAD_PERIOD),
    keyswitch(5, 3, KEYPAD_ENTER),
};

std::list<int> keys_down;
bool keys_changed = false;

uint32_t leds = 0x000000; // bit 23 is MX1's LED, LSB is MX24's LED

void scan(void)
{
    int new_modifiers = 0;
    int new_media = 0;

    for (auto& the_switch : switch_matrix) {
        switch_state state = the_switch.get_state();
        int key_value = the_switch.get_value();
        if (the_switch.is_modifier()) {
            // Rebuild the modifier value
            if (state == SWITCH_DOWN) {
                new_modifiers |= key_value;
            }
        } else if (the_switch.is_media()) {
            // Rebuild the media value
            if (state == SWITCH_DOWN) {
                new_media |= key_value;
            }
        } else {
            // Check to see if the key is currently in the keys_down list
            bool in_key_down = std::any_of(keys_down.begin(), keys_down.end(), [&](int i) { return i == key_value; });
            // Key is pressed, but isn't in our keys_down list, add it to the end
            if (state == SWITCH_DOWN && !in_key_down) {
                keys_changed = true;
                keys_down.push_back(key_value);
            }
            // key isn't pressed anymore, but its in the list, remove it
            if (state == SWITCH_UP && in_key_down) {
                keys_changed = true;
                keys_down.remove(key_value);
            }
        }
    }
    // If the modifier keys have changed, update them and mark for resend
    if (new_modifiers != keyboard_modifier_keys) {
        keys_changed = true;
        keyboard_modifier_keys = new_modifiers;
    }
    // If the media keys have changed, update them and mark for resend
    if (new_media != keyboard_media_keys) {
        keys_changed = true;
        keyboard_media_keys = new_media;
    }
    // Rebuild the usb key list using the list of down keys
    // if there are less than 6 down, set the rest to 0s
    // if there are more than 6 down, only the first 6 will be used
    std::list<int>::iterator key_iter = keys_down.begin();
    for (int i = 0; i < 6; i++) {
        keyboard_keys[i] = 0;
        if (key_iter != keys_down.end()) {
            keyboard_keys[i] = *key_iter;
            key_iter++;
        }
    }
}

// This assumes an array 3 bytes long is passed
void shift_out(uint32_t leds)
{
    // Clock out all the data
    for (int bit_shift = 0; bit_shift <= 16; bit_shift += 8) {
        shiftOut_lsbFirst(PIN_SER, PIN_SCLK, (leds >> bit_shift) & 0xFF);
    }
    // Refresh the registers
    digitalWrite(PIN_RCLK, 1);
    digitalWrite(PIN_RCLK, 0);
}

// Right now this just sequentially lights one led - moving from MX1 to MX24
// then starting over
void update_leds()
{
    shift_out(leds);
    leds = leds >> 1;
    if (!(leds & 0xFFFFFF)) {
        leds = 0x800000;
    }
}

void init_pins(void)
{
    // Rows
    for (auto& row : row_pins) {
        pinMode(row, INPUT_PULLDOWN);
    }
    // Cols
    for (auto& col : col_pins) {
        pinMode(col, OUTPUT);
        digitalWrite(col, 0);
    }
    // shift registers
    pinMode(PIN_SCLK, OUTPUT);
    pinMode(PIN_RCLK, OUTPUT);
    pinMode(PIN_SER, OUTPUT);
    pinMode(PIN_PWM, OUTPUT);
    digitalWrite(PIN_SCLK, 0);
    digitalWrite(PIN_RCLK, 0);
    digitalWrite(PIN_SER, 0);
    analogWrite(PIN_PWM, 192); // Just a random starting brightness
}

extern "C" int main(void)
{
    init_pins();
    elapsedMillis waiting;
    while (1) {
        scan();
        if (keys_changed) {
            usb_keyboard_send();
            keys_changed = false;
        }
        if (waiting >= 50) {
            waiting = 0;
            update_leds();
        }
    }
}
