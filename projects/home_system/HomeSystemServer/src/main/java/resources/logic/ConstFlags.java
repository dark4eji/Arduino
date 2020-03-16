package resources.logic;

import lombok.Data;

@Data
class ConstFlags {
    static final short TANK_HEIGHT = 165;
    static final short TANK_FULL = 130;
    static final short TANK_HALF = 82;
    static final short TANK_EMPTY = 41;

    static final String GREEN = "#23C48E";
    static final String RED = "#D3435C";

    static byte full = 0;
    static byte half = 0;
    static byte empty = 0;
}
