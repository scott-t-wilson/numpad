#include "keyswitch.h"
#include "elapsedMillis.h"
#include <vector>

std::vector<int> row_pins = { 13, 14, 15, 16, 17, 18 };
std::vector<int> col_pins = { 19, 20, 21, 22 };

switch_state keyswitch::get_state()
{
    if (state == SWITCH_BOUNCE && (ms_since_change < 5)) {
        // If we're bouncing and it hasn't be 5ms, don't bother checking
        return SWITCH_BOUNCE;
    }
    // Set the column high, then check our row
    digitalWrite(col_pins[col], 1);
    int pin = digitalRead(row_pins[row]);
    digitalWrite(col_pins[col], 0);
    if (state == SWITCH_BOUNCE) {
        // We were previously bounced, but waited long enough, save and return
        // the newly read statek
        state = pin ? SWITCH_DOWN : SWITCH_UP;
    } else if (pin != state) {
        // We read a different state than we had saved
        // set state to bounce and reset timer
        state = SWITCH_BOUNCE;
        ms_since_change = 0;
    }
    return state;
}
