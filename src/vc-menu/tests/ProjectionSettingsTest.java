import java.io.*;
import java.util.zip.CRC32;
import org.omonob.cpmap.ui.*;
public final class ProjectionSettingsTest {
    static int checks;
    static void check(boolean b,String s){checks++;if(!b)throw new AssertionError(s);}
    static void write(File f,String s)throws Exception{FileOutputStream o=new FileOutputStream(f);try{o.write(s.getBytes("US-ASCII"));}finally{o.close();}}
    static String checksum(String body)throws Exception{CRC32 crc=new CRC32();crc.update(body.getBytes("US-ASCII"));return body+"crc32="+Long.toHexString(crc.getValue())+"\n";}
    public static void main(String[] args)throws Exception{
        File root=new File(args[0]);check(root.mkdirs(),"fresh fixture");
        File normal=new File(root,"normal");ProjectionSettings store=new ProjectionSettings(normal);
        ProjectionSettings.State state=store.read();
        check(state.mapMask==1 && state.mirrorMask==0 && state.information==0,"preserved defaults");
        check(state.value(0)==1 && state.value(1)==7,"choice mapping");
        int legal=0,blocked=0;
        for(int map=0;map<4;map++)for(int mirror=0;mirror<8;mirror++)for(int info=0;info<4;info++){
            store.select(0,3);store.select(1,mirror);
            long before=store.read().generation;
            state=store.select(0,map);
            int mm=ProjectionSettings.choiceMask(0,map),vm=ProjectionSettings.choiceMask(1,mirror);
            check(state.mapMask==mm && state.mirrorMask==(vm&~mm),"map priority retains nonconflicting bits");
            check(state.generation==before+1,"paired change commits once");
            long generation=state.generation;
            if((mm&vm)==0){
                store.select(1,mirror);store.select(2,info);state=new ProjectionSettings(normal).read();legal++;
                check(state.mapMask==mm && state.mirrorMask==vm && state.information==info,"all legal combinations reload");
                check(state.navigationInformationEnabled()==(info==0 || info==2),"navigation policy");
                check(state.musicInformationEnabled()==(info==0 || info==1),"music policy");
            }else{
                boolean refused=false;try{store.select(1,mirror);}catch(IllegalArgumentException expected){refused=true;}
                check(refused && store.read().generation==generation,"blocked mirror cannot save");blocked++;
            }
            check(ProjectionSettings.validPair(store.read().mapMask,store.read().mirrorMask),"no invalid stored pair");
        }
        check(legal==72 && blocked==56,"all 128 joint states covered");
        for(int field=-1;field<4;field++)for(int choice=-1;choice<10;choice++){
            if(field>=0 && field<3 && choice>=0 && choice<ProjectionSettings.choiceCount(field))continue;
            long generation=store.read().generation;boolean refused=false;
            try{store.select(field,choice);}catch(IllegalArgumentException expected){refused=true;}
            check(refused && generation==store.read().generation,"invalid choice not committed");
        }
        for(int map=0;map<2;map++)for(int mirror=0;mirror<2;mirror++)for(int info=0;info<3;info++)for(int cl=0;cl<2;cl++){
            File old=new File(root,"old-"+map+mirror+info+cl);check(old.mkdir(),"schema2 fixture");
            write(new File(old,"settings.state"),checksum("format=2\ncarplay_map="+map+"\ncarplay_mirror="+mirror+"\ncarplay_information="+info+"\ncarlife_mirror="+cl+"\ngeneration=12\n"));
            ProjectionSettings migrated=new ProjectionSettings(old);state=migrated.read();
            check(state.mapMask==map && state.mirrorMask==((mirror|(cl<<2))&~map) && state.information==info,"schema2 migration preserves intent; maps win");
            state=migrated.select(2,3);
            check(state.generation==13 && !state.navigationInformationEnabled() && !state.musicInformationEnabled(),"schema3 migration commits both masks with info none");
        }
        File old=new File(root,"schema1");new MapSwitchSettings(old).set(false);
        check(new ProjectionSettings(old).read().mapMask==0,"schema1 map off remains OEM");
        long generation=store.read().generation;
        write(new File(normal,"settings.pending"),"partial");check(store.read().generation==generation,"pending ignored");
        write(new File(normal,"settings.state"),"partial");check(store.read().source.equals("recovered"),"previous state recovered");
        state=store.select(0,1);generation=state.generation;
        check(new File(normal,"settings.pending").mkdir(),"blocked stage fixture");
        boolean refused=false;try{store.select(2,3);}catch(IOException expected){refused=true;}
        check(refused && store.read().generation==generation,"failed stage preserves settings");
        File invalid=new File(root,"conflict");check(invalid.mkdir(),"invalid fixture");
        write(new File(invalid,"settings.state"),checksum("format=3\nvc_map_mask=1\nvc_mirror_mask=5\ncarplay_information=0\ngeneration=1\n"));
        refused=false;try{new ProjectionSettings(invalid).read();}catch(IOException expected){refused=true;}
        check(refused,"even correctly checksummed conflicting schema3 is rejected");
        write(new File(invalid,"settings.state"),"format=3\nvc_map_mask=1\nvc_mirror_mask=0\ncarplay_information=0\ngeneration=1\nmissing=x\n");
        refused=false;try{new ProjectionSettings(invalid).read();}catch(IOException expected){refused=true;}
        check(refused,"missing checksum becomes controlled damaged-store failure");
        String[] locales=UiMessages.supportedLocales();check(locales.length==31,"31 languages retained");
        check(MenuSectionMessages.count()==locales.length,"section heading locale table complete");
        for(int i=0;i<locales.length;i++){
            check(MenuSectionMessages.get(locales[i]).length()>0,"section heading translation retained");
            for(int key=0;key<UiMessages.COUNT;key++)check(UiMessages.get(locales[i],key).length()>0,"original translations retained");
            for(int key=0;key<LinkedMessages.COUNT;key++)check(LinkedMessages.get(locales[i],key).length()>0,"complete linked translations");
        }
        check(UiMessages.normalize("ja-JP").equals("ja"),"Japanese normalization");
        check(MenuSectionMessages.get("zh_CN").equals("虚拟座舱设置"),"section heading Chinese");
        check(MenuSectionMessages.get("en_GB").equals("Virtual Cockpit settings"),"section heading English");
        check(MenuSectionMessages.get("ja_JP").equals("仮想コックピット設定"),"section heading Japanese");
        check(MenuSectionMessages.get("unknown").equals("Virtual Cockpit settings"),"section heading safe fallback");
        check(UiMessages.isRightToLeft("ar") && UiMessages.isRightToLeft("he"),"RTL preserved");
        System.out.println("LINKED_SETTINGS_TEST_OK checks="+checks+" combinations=128 legal=72 blocked=56 locales=31");
    }
}
