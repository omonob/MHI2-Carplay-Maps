package org.omonob.cpmap.ui;
/** Read-only policy: preferences never start an unimplemented engine. */
public final class ProjectionRuntimePolicy {
    private static ProjectionSettings.State cached=new ProjectionSettings.State(1,0,0,0,"default");
    private static long nextRead;
    private static boolean readSucceeded;
    private ProjectionRuntimePolicy(){ }
    private static synchronized ProjectionSettings.State state(){
        long now=System.currentTimeMillis();
        if(now>=nextRead || nextRead-now>1000){nextRead=now+500;
            try{cached=ProjectionSettings.systemStore().read();readSucceeded=true;}catch(Exception ignored){readSucceeded=false;}}
        return cached;
    }
    public static synchronized void refreshNow(){nextRead=0;state();}
    /** Legacy CP-only caller. AA permission must not start the CP engine. */
    public static boolean mapAllowed(){return (state().mapMask&ProjectionSettings.CP)!=0;}
    public static boolean androidAutoMapRequested(){return (state().mapMask&ProjectionSettings.AA)!=0;}
    public static boolean navigationAllowed(){return state().navigationInformationEnabled();}
    public static boolean musicAllowed(){return state().musicInformationEnabled();}
    public static boolean callActiveOrMusicHidden(boolean active){return active || !musicAllowed();}
    /** Only the existing CarPlay artwork paths are filtered; OEM/radio/BT are untouched. */
    public static org.dsi.ifc.global.ResourceLocator filterCover(org.dsi.ifc.global.ResourceLocator cover){
        if(cover==null || musicAllowed() || cover.url==null)return cover;
        String url=cover.url;
        if(url.startsWith("/var/app/icab/tmp/cp_cover_") || url.equals("/var/app/icab/tmp/cp_cover.jpg")
           || url.startsWith("/mnt/icab/persistent/carplayvc/covers/") || url.startsWith("/tsd/var/carplayvc/cover"))return null;
        return cover;
    }
    public static boolean carplayMirrorRequested(){return (state().mirrorMask&ProjectionSettings.CP)!=0;}
    public static boolean androidAutoMirrorRequested(){return (state().mirrorMask&ProjectionSettings.AA)!=0;}
    public static boolean carlifeMirrorRequested(){return (state().mirrorMask&ProjectionSettings.CL)!=0;}
    public static boolean mirrorEngineAvailable(){return new java.io.File("/mnt/app/carplaymaps/cpvc-mirror-canvas").isFile() && new java.io.File("/mnt/app/carplaymaps/libcpvc-mirror-tap.so").isFile();}
    public static boolean androidAutoMapEngineAvailable(){return false;}
    public static boolean lastReadSucceeded(){state();return readSucceeded;}
}
