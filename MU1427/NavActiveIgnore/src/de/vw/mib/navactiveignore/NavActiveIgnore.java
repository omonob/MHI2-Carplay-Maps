package de.vw.mib.navactiveignore;

import org.dsi.ifc.carplay.AppStateRequest;
import org.dsi.ifc.carplay.ResourceRequest;

/**
 * Helpers for the MU1427 NavActiveIgnore patch. Must stay Java-1.2 clean
 * (no StringBuilder, no autoboxing, no class literals): it is downgraded
 * to classfile version 46 for the J9 foun11 VM.
 */
public final class NavActiveIgnore {
    /** CarPlay appStateID for navigation. */
    public static final int APP_STATE_NAVIGATION = 2;

    private NavActiveIgnore() {
    }

    /** true for "head unit takes navigation ownership" (state=true, speechMode=0). */
    public static boolean isNativeNavClaim(AppStateRequest r) {
        return r != null && r.appStateID == APP_STATE_NAVIGATION && r.state && r.speechMode == 0;
    }

    /** Returns the array itself if it holds no native-nav claim, else a copy without the claims. */
    public static AppStateRequest[] stripNativeNavClaims(AppStateRequest[] in) {
        if (in == null) {
            return null;
        }
        int keep = 0;
        for (int i = 0; i < in.length; i++) {
            if (!isNativeNavClaim(in[i])) {
                keep++;
            }
        }
        if (keep == in.length) {
            return in;
        }
        AppStateRequest[] out = new AppStateRequest[keep];
        int j = 0;
        for (int i = 0; i < in.length; i++) {
            if (!isNativeNavClaim(in[i])) {
                out[j++] = in[i];
            }
        }
        return out;
    }

    /** Drop the DSI call only if stripping removed everything it was going to send. */
    public static boolean dropRequest(ResourceRequest[] resources, AppStateRequest[] original, AppStateRequest[] stripped) {
        if (original == stripped) {
            return false;
        }
        boolean noResources = resources == null || resources.length == 0;
        boolean noStates = stripped == null || stripped.length == 0;
        return noResources && noStates;
    }
}
