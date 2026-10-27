package org.omonob.cpmap.ui;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.zip.CRC32;

/** Schema 3: map-priority, paired atomic settings. Engines are not implied. */
public final class ProjectionSettings {
    public static final int VC_MAP=0, VC_MIRROR=1, CARPLAY_INFORMATION=2;
    public static final int CP=1, AA=2, CL=4;
    public static final int INFO_ALL=0, INFO_NAVIGATION_OFF=1, INFO_MUSIC_OFF=2, INFO_NONE=3;
    private static final int[] MAP_CHOICES={CP|AA,CP,AA,0};
    private static final int[] MIRROR_CHOICES={CP|AA|CL,CP|AA,AA|CL,CP|CL,CP,AA,CL,0};
    private static final Object LOCK=new Object();
    private final File directory;
    public static final class State {
        public final int mapMask,mirrorMask,information;
        public final long generation;
        public final String source;
        State(int map,int mirror,int info,long gen,String origin){mapMask=map;mirrorMask=mirror;information=info;generation=gen;source=origin;}
        public int value(int field){
            if(field==VC_MAP)return index(MAP_CHOICES,mapMask);
            if(field==VC_MIRROR)return index(MIRROR_CHOICES,mirrorMask);
            if(field==CARPLAY_INFORMATION)return information;
            throw new IllegalArgumentException("Unknown setting");
        }
        public boolean navigationInformationEnabled(){return information==INFO_ALL || information==INFO_MUSIC_OFF;}
        public boolean musicInformationEnabled(){return information==INFO_ALL || information==INFO_NAVIGATION_OFF;}
    }
    private static int index(int[] choices,int mask){
        for(int i=0;i<choices.length;i++)if(choices[i]==mask)return i;
        throw new IllegalArgumentException("Unknown source mask");
    }
    public static int choiceCount(int field){
        if(field==VC_MAP || field==CARPLAY_INFORMATION)return 4;
        if(field==VC_MIRROR)return 8;
        throw new IllegalArgumentException("Unknown setting");
    }
    public static int choiceMask(int field,int choice){
        if(choice<0 || choice>=choiceCount(field))throw new IllegalArgumentException("Invalid choice");
        if(field==VC_MAP)return MAP_CHOICES[choice];
        if(field==VC_MIRROR)return MIRROR_CHOICES[choice];
        return choice;
    }
    public static boolean validPair(int map,int mirror){return map>=0 && map<=3 && mirror>=0 && mirror<=7 && (map&mirror)==0;}
    public static boolean mirrorChoiceAllowed(State state,int choice){return validPair(state.mapMask,choiceMask(VC_MIRROR,choice));}
    public static int mirrorAfterMapChoice(State state,int choice){return state.mirrorMask & ~choiceMask(VC_MAP,choice);}
    public static int mirrorChoiceIndex(int mask){return index(MIRROR_CHOICES,mask);}
    public ProjectionSettings(File root)throws IOException{
        if(root==null)throw new IOException("Missing preference directory");directory=root.getCanonicalFile();
    }
    public static ProjectionSettings systemStore()throws IOException{
        String path=System.getProperty("cpmap.ui.stateDir");
        if(path==null || path.length()==0)path="/mnt/icab/persistent/cpmap-ui";
        return new ProjectionSettings(new File(path));
    }
    public State read()throws IOException{synchronized(LOCK){return readLocked();}}
    /** Independent switches share the existing four-choice information state. */
    public State setNavigationInformation(boolean enabled)throws IOException{
        synchronized(LOCK){State old=readLocked();return select(CARPLAY_INFORMATION,
            (enabled?0:1)|(old.musicInformationEnabled()?0:2));}
    }
    public State setMusicInformation(boolean enabled)throws IOException{
        synchronized(LOCK){State old=readLocked();return select(CARPLAY_INFORMATION,
            (old.navigationInformationEnabled()?0:1)|(enabled?0:2));}
    }
    public State setCarplayMap(boolean enabled)throws IOException{
        synchronized(LOCK){State old=readLocked();int mask=enabled?(old.mapMask|CP):(old.mapMask&~CP);
            return select(VC_MAP,index(MAP_CHOICES,mask));}
    }
    public State setCarplayMirror(boolean enabled)throws IOException{
        synchronized(LOCK){State old=readLocked();int mask=enabled?(old.mirrorMask|CP):(old.mirrorMask&~CP);
            return select(VC_MIRROR,index(MIRROR_CHOICES,mask));}
    }
    private State readLocked()throws IOException{
        File main=file("settings.state"),previous=file("settings.previous");
        State value=decode(main,"saved");if(value!=null)return value;
        value=decode(previous,"recovered");if(value!=null)return value;
        if(main.exists() || previous.exists())throw new IOException("Saved settings are damaged");
        MapSwitchSettings.State old=new MapSwitchSettings(directory).read();
        return new State(old.enabled?CP:0,0,INFO_ALL,old.generation,old.generation==0?"default":"legacy-map-state");
    }
    public State select(int field,int choice)throws IOException{
        int selected=choiceMask(field,choice);
        synchronized(LOCK){
            State old=readLocked();int map=old.mapMask,mirror=old.mirrorMask,info=old.information;
            if(field==VC_MAP){map=selected;mirror&=~map;}
            else if(field==VC_MIRROR){
                if(!validPair(map,selected))throw new IllegalArgumentException("Map source conflicts with mirror");
                mirror=selected;
            }else info=choice;
            if(old.generation==Long.MAX_VALUE)throw new IOException("Preference generation overflow");
            State next=new State(map,mirror,info,old.generation+1,"saved");
            if(!validPair(map,mirror))throw new IOException("Refusing conflicting settings");
            if(!directory.exists() && !directory.mkdirs())throw new IOException("Cannot create preference directory");
            if(!directory.isDirectory())throw new IOException("Invalid preference directory");
            File pending=file("settings.pending");FileOutputStream out=new FileOutputStream(pending);
            try{out.write(encoded(next).getBytes("US-ASCII"));out.flush();out.getFD().sync();}finally{out.close();}
            State stage=decode(pending,"pending");if(stage==null || !equal(next,stage))throw new IOException("Stage readback failed");
            File main=file("settings.state"),previous=file("settings.previous");
            if(decode(main,"saved")!=null){
                if(previous.exists() && !previous.delete())throw new IOException("Cannot rotate backup");
                if(!main.renameTo(previous))throw new IOException("Cannot preserve backup");
            }else if(main.exists() && !main.delete())throw new IOException("Cannot replace damaged primary");
            if(!pending.renameTo(main))throw new IOException("Commit failed; previous state preserved");
            State committed=readLocked();if(!equal(next,committed))throw new IOException("Commit readback failed");return committed;
        }
    }
    private static boolean equal(State a,State b){return a.mapMask==b.mapMask && a.mirrorMask==b.mirrorMask && a.information==b.information && a.generation==b.generation;}
    private File file(String name)throws IOException{
        File f=new File(directory,name);
        if(!directory.equals(f.getCanonicalFile().getParentFile()))throw new IOException("Settings path escaped directory");return f;
    }
    private static String body(State s){return new StringBuffer("format=3\nvc_map_mask=").append(s.mapMask)
        .append("\nvc_mirror_mask=").append(s.mirrorMask).append("\ncarplay_information=").append(s.information)
        .append("\ngeneration=").append(s.generation).append('\n').toString();}
    private static String checksum(String body)throws IOException{
        byte[] bytes=body.getBytes("US-ASCII");CRC32 crc=new CRC32();crc.update(bytes,0,bytes.length);
        return new StringBuffer(body).append("crc32=").append(Long.toHexString(crc.getValue())).append('\n').toString();
    }
    private static String encoded(State s)throws IOException{return checksum(body(s));}
    private static int digit(String text,String prefix,int max){
        if(!text.startsWith(prefix) || text.length()!=prefix.length()+1)return -1;
        int value=text.charAt(prefix.length())-'0';return value>=0 && value<=max?value:-1;
    }
    private static State decode(File file,String origin)throws IOException{
        if(!file.isFile() || file.length()<1 || file.length()>256)return null;
        byte[] bytes=new byte[(int)file.length()];FileInputStream in=new FileInputStream(file);int offset=0;
        try{while(offset<bytes.length){int n=in.read(bytes,offset,bytes.length-offset);if(n<0)return null;offset+=n;}if(in.read()!=-1)return null;}finally{in.close();}
        String text=new String(bytes,"US-ASCII");boolean old=text.startsWith("format=2\n");
        String[] lines=new String[old?7:6];int begin=0;
        for(int i=0;i<lines.length;i++){int end=text.indexOf('\n',begin);if(end<0)return null;lines[i]=text.substring(begin,end);begin=end+1;}
        if(begin!=text.length() || !(old?"format=2":"format=3").equals(lines[0]))return null;
        int map=digit(lines[1],old?"carplay_map=":"vc_map_mask=",old?1:3);
        int mirror=digit(lines[2],old?"carplay_mirror=":"vc_mirror_mask=",old?1:7);
        int info=digit(lines[3],"carplay_information=",old?2:3),carlife=old?digit(lines[4],"carlife_mirror=",1):0;
        int genLine=old?5:4;
        if(map<0 || mirror<0 || info<0 || carlife<0 || !lines[genLine].startsWith("generation="))return null;
        long generation;try{generation=Long.parseLong(lines[genLine].substring(11));}catch(NumberFormatException e){return null;}
        if(generation<1 || !lines[genLine].equals(new StringBuffer("generation=").append(generation).toString()))return null;
        if(!lines[lines.length-1].startsWith("crc32="))return null;
        String originalBody=text.substring(0,text.lastIndexOf("crc32="));if(!text.equals(checksum(originalBody)))return null;
        if(old){int requested=mirror | (carlife==1?CL:0);mirror=requested & ~map;
            return new State(map,mirror,info,generation,requested==mirror?"schema2-migrated":"schema2-map-priority");}
        if(!validPair(map,mirror))return null;
        State value=new State(map,mirror,info,generation,origin);return text.equals(encoded(value))?value:null;
    }
}
