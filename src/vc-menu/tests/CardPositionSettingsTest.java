import java.io.*;
import java.util.zip.CRC32;
import org.omonob.cpmap.ui.*;
public final class CardPositionSettingsTest {
 static int checks;
 static void check(boolean ok,String text){checks++;if(!ok)throw new AssertionError(text);}
 static byte[] read(File f)throws Exception{byte[] b=new byte[(int)f.length()];FileInputStream in=new FileInputStream(f);try{int n=0;while(n<b.length){int k=in.read(b,n,b.length-n);if(k<0)throw new IOException();n+=k;}}finally{in.close();}return b;}
 static void write(File f,byte[] b)throws Exception{FileOutputStream out=new FileOutputStream(f);try{out.write(b);}finally{out.close();}}
 static String crc(String body)throws Exception{CRC32 c=new CRC32();byte[] b=body.getBytes("US-ASCII");c.update(b,0,b.length);return body+"crc32="+Long.toHexString(c.getValue())+"\n";}
 public static void main(String[] a)throws Exception{
  File dir=new File(a[0]);CardPositionSettings store=new CardPositionSettings(dir);
  check(store.read().position==2&&store.read().generation==0,"preserve existing right default");
  check(!dir.exists(),"reading default does not write");
  ProjectionSettings projection=new ProjectionSettings(dir);projection.select(0,0);projection.select(2,2);
  new WirelessCarPlaySettings(dir).set(true);new AnimationSettings(dir,null).set(0);
  String[] protectedFiles={"settings.state","settings.previous","wireless-carplay.state","animation.state"};
  byte[][] before=new byte[protectedFiles.length][];for(int i=0;i<before.length;i++)before[i]=read(new File(dir,protectedFiles[i]));
  for(int p=0;p<4;p++){
   CardPositionSettings.State s=store.set(p);check(s.position==p&&s.generation==p+1,"all card positions");
   CardPositionSettings fresh=new CardPositionSettings(dir);check(fresh.read().position==p,"fresh reader remembers position");
   for(int i=0;i<before.length;i++)check(java.util.Arrays.equals(before[i],read(new File(dir,protectedFiles[i]))),"other preferences byte-identical");
  }
  File main=new File(dir,"navigation-card.state"),backup=new File(dir,"navigation-card.previous");
  byte[] valid=read(main),previous=read(backup);
  for(int i=0;i<valid.length;i++){
   byte[] bad=(byte[])valid.clone();bad[i]^=1;write(main,bad);
   CardPositionSettings.State recovered=store.read();
   check(recovered.position==2&&recovered.generation==3&&recovered.source.equals("recovered"),"CRC corruption fallback byte "+i);
   check(java.util.Arrays.equals(previous,read(backup)),"read fallback never rewrites backup");
  }
  write(main,new byte[]{0});write(backup,new byte[]{0});
  try{store.read();throw new AssertionError("guessed corrupt state");}catch(IOException expected){checks++;}
  write(backup,previous);CardPositionSettings.State fixed=store.set(1);check(fixed.position==1&&fixed.generation==4,"repair damaged primary with valid previous");
  write(main,crc("format=1\nposition=2\ngeneration=9223372036854775807\n").getBytes("US-ASCII"));
  try{store.set(0);throw new AssertionError("generation wrapped");}catch(IOException expected){checks++;}
  for(int invalid=-2;invalid<7;invalid++)if(invalid<0||invalid>3){
   try{store.set(invalid);throw new AssertionError("invalid position accepted");}catch(IllegalArgumentException expected){checks++;}
  }
  String[] locales=UiMessages.supportedLocales();
  for(int i=0;i<locales.length;i++)for(int k=0;k<5;k++)check(CardMenuMessages.get(locales[i],k).length()>0,"locale card label");
  check(CardMenuMessages.get("xx",1).equals("Center"),"unknown locale fallback");
  // Dedicated fixture: all readers must agree with Java's final saved RIGHT.
  File cross=new File(dir,"cross-language");new CardPositionSettings(cross).set(2);
  System.out.println("CARD_POSITION_SETTINGS_OK checks="+checks+" locales="+locales.length+" modes=4 other_preferences_unchanged=1");
 }
}
