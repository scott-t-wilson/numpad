#include "elapsedMillis.h"
#include <vector>

extern std::vector<int> row_pins;
extern std::vector<int> col_pins;

enum switch_state {
    SWITCH_UP = 0,
    SWITCH_DOWN = 1,
    SWITCH_BOUNCE = 2
};

class keyswitch {
private:
    elapsedMillis ms_since_change;
    int row;
    int col;
    int value;
    switch_state state = SWITCH_BOUNCE;

public:
    keyswitch(int row, int col, int value)
        : row(row)
        , col(col)
        , value(value){};

    int get_value(void) const { return value; }
    bool is_modifier(void) const { return (value & 0x8000) == 0x8000; }
    bool is_media(void) const { return ((value & 0xC000) == 0x0000) && ((value & 0xFF) != 0); }
    switch_state get_state(void);
};