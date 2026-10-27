import java.io.*;
import org.omonob.cpmap.ui.*;
public final class WirelessCarPlaySettingsTest {
 static int checks;
 static void check(boolean b,String m){checks++;if(!b)throw new AssertionError(m);}
 static void write(File file,String value)throws Exception{FileOutputStream out=new FileOutputStream(file);try{out.write(value.getBytes("US-ASCII"));}finally{out.close();}}
 public static void main(String[] args)throws Exception{
  File root=new File(args[0]);check(root.mkdirs(),"fresh wireless fixture");
  WirelessCarPlaySettings store=new WirelessCarPlaySettings(root);
  check(!store.read().enabled && store.read().generation==0,"default disabled without file");
  check(!new File(root,"wireless-carplay.state").exists(),"read does not write");
  WirelessCarPlaySettings.State state=store.set(true);
  check(state.enabled && state.generation==1,"enabled save");
  state=new WirelessCarPlaySettings(root).read();check(state.enabled && state.generation==1,"reload enabled");
  state=store.set(false);check(!state.enabled && state.generation==2,"disable save");
  check(new File(root,"wireless-carplay.previous").isFile(),"previous preserved");
  write(new File(root,"wireless-carplay.pending"),"partial");check(!store.read().enabled && store.read().generation==2,"pending ignored");
  write(new File(root,"wireless-carplay.state"),"damaged");state=store.read();check(state.enabled && state.generation==1 && state.source.equals("recovered"),"recover previous");
  state=store.set(false);check(!state.enabled && state.generation==2,"replace damaged primary");
  File pending=new File(root,"wireless-carplay.pending");check(pending.mkdir(),"blocked write fixture");
  boolean refused=false;try{store.set(true);}catch(IOException expected){refused=true;}
  check(refused && !store.read().enabled && store.read().generation==2,"failed write preserves state");
  check(!new File(root,"maps.state").exists() && !new File(root,"settings.state").exists(),"not a map/info preference mutation");
  String[] locales=UiMessages.supportedLocales();check(WirelessMenuMessages.count()==locales.length,"wireless locales complete");
  for(int i=0;i<locales.length;i++)check(WirelessMenuMessages.get(locales[i]).length()>0,"wireless label present");
  check(WirelessMenuMessages.get("zh_CN").equals("无线 CarPlay"),"Chinese wireless label");
  check(WirelessMenuMessages.get("ja_JP").equals("ワイヤレス CarPlay"),"Japanese wireless label");
  check(WirelessMenuMessages.get("en_GB").equals("Wireless CarPlay"),"English wireless label");
  System.out.println("WIRELESS_INTERFACE_TEST_OK checks="+checks+" default=off service_actions=none");
 }
}
