package org.omonob.cpmap.ui;
/** Routing only: original map commands remain unchanged outside CP mirroring. */
public final class ClusterCommandRouter {
    private ClusterCommandRouter(){}
    public static int port(){return ProjectionRuntimePolicy.carplayMirrorRequested()?7033:7032;}
    public static String command(String original){
        if(!ProjectionRuntimePolicy.carplayMirrorRequested())return original;
        if("zoom-in".equals(original))return "canvas-zoom-in";
        if("zoom-out".equals(original))return "canvas-zoom-out";
        // Map-app cycling must not leak to the phone's map while mirroring.
        // These arrive on the canvas-only port and are explicitly inert.
        return "canvas-noop";
    }
}
