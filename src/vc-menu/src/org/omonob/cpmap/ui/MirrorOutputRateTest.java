package org.omonob.cpmap.ui;

import java.io.FileInputStream;

/** Event-driven mirror-only cadence; never changes CarPlay map stream FPS. */
public final class MirrorOutputRateTest {
    private MirrorOutputRateTest() { }

    static int chooseForState(int requested, boolean profile790, boolean valid,
                              boolean cpMap, boolean cpMirror) {
        return requested == 10 && profile790 && valid && !cpMap && cpMirror
            ? 30 : requested;
    }

    private static boolean profile790() {
        try {
            return MenuClusterProfile.cluster()==790;
        } catch (Throwable ignored) {
            return false;
        }
    }

    public static int choose(int requested) {
        if (requested != 10) return requested;
        try {
            ProjectionRuntimePolicy.refreshNow();
            return chooseForState(requested, profile790(),
                ProjectionRuntimePolicy.lastReadSucceeded(),
                ProjectionRuntimePolicy.mapAllowed(),
                ProjectionRuntimePolicy.carplayMirrorRequested());
        } catch (Throwable ignored) {
            return requested;
        }
    }
}
