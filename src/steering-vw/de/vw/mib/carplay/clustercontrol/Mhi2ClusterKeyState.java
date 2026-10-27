package de.vw.mib.carplay.clustercontrol;

/*
 * Pure state machine shared by the MHI2 DSI bridge and its host-side tests.
 * It intentionally uses Java 1.4 syntax for the IBM J9 runtime in MHI2.
 */
final class Mhi2ClusterKeyState {
    static final int ACTION_NONE = 0;
    static final int ACTION_ZOOM_IN = 1;
    static final int ACTION_ZOOM_OUT = 2;
    static final int ACTION_MAP_PREVIOUS = 3;
    static final int ACTION_MAP_NEXT = 4;

    static final int KEY_UP = 38;
    static final int KEY_DOWN = 39;
    static final int STATE_RELEASED = 0;
    static final int STATE_PRESSED = 1;
    static final int STATE_LONG_PRESSED = 3;
    static final int STATE_LONG_PRESSED_2 = 4;
    static final int STATE_LONG_PRESSED_3 = 5;
    static final long LONG_PRESS_MILLIS = 2500L;

    private int activeKey = -1;
    private long pressedAtMillis;
    private boolean longActionSent;

    int onKey(int key, int state, long nowMillis) {
        if (key != KEY_UP && key != KEY_DOWN) return ACTION_NONE;

        if (state == STATE_PRESSED) {
            if (activeKey == key) return ACTION_NONE;
            activeKey = key;
            pressedAtMillis = nowMillis;
            longActionSent = false;
            return key == KEY_UP ? ACTION_ZOOM_IN : ACTION_ZOOM_OUT;
        }

        if (state == STATE_RELEASED) {
            if (activeKey == key) {
                activeKey = -1;
                longActionSent = false;
            }
            return ACTION_NONE;
        }

        if (state == STATE_LONG_PRESSED ||
            state == STATE_LONG_PRESSED_2 ||
            state == STATE_LONG_PRESSED_3) {
            if (activeKey < 0) {
                /* A defensive recovery path if the initial press was missed. */
                activeKey = key;
                pressedAtMillis = nowMillis;
                longActionSent = false;
                return ACTION_NONE;
            }
            if (activeKey == key) return onTimer(nowMillis);
        }
        return ACTION_NONE;
    }

    int onTimer(long nowMillis) {
        if (activeKey < 0 || longActionSent) return ACTION_NONE;
        if (nowMillis - pressedAtMillis < LONG_PRESS_MILLIS) {
            return ACTION_NONE;
        }
        longActionSent = true;
        return activeKey == KEY_UP ? ACTION_MAP_PREVIOUS : ACTION_MAP_NEXT;
    }

    long millisUntilLongAction(long nowMillis) {
        long remaining;
        if (activeKey < 0 || longActionSent) return 0L;
        remaining = LONG_PRESS_MILLIS - (nowMillis - pressedAtMillis);
        return remaining > 0L ? remaining : 1L;
    }
}
