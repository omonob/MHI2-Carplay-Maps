package org.omonob.cpmap.ui;
import java.io.*;
import java.util.zip.CRC32;
import org.dsi.ifc.carplay.AppState;
import org.dsi.ifc.carplay.DSICarplay;
/** Only a navigation ownership bit for map framing. No route text, music,
 * artwork, BAP output, radio action or persistent preference is read/written. */
public final class CarPlayNavigationOwnership {
 private static final File OWNER=new File("/ramdisk/carplay-dio-manager.pid");
 private static final File STATE=new File("/ramdisk/cpmap-navigation-owner.state");
 private static long generation,lastOwner;private static int lastActive=-1;
 private CarPlayNavigationOwnership(){}
 public static int navigationOwner(AppState[] states){
  int result=-1;
  if(states==null)return result;
  for(int i=0;i<states.length;i++){
   AppState s=states[i];
   if(s!=null&&s.appStateID==DSICarplay.APPLICATIONID_NAVIGATION){
    int next=s.owner==DSICarplay.APPLICATIONOWNER_DEVICE?1:0;
    if(result>=0&&result!=next)return 0;
    result=next;
   }
  }
  return result;
 }
 public static void observe(AppState[] states){
  try{if(MenuClusterProfile.is791())return;int active=navigationOwner(states);if(active>=0)publish(active);}catch(Throwable ignored){}
 }
 public static void deviceChanged(){
  try{if(MenuClusterProfile.is791())return;if(!new File("/dev/ipod0").exists())publish(0);}catch(Throwable ignored){}
 }
 private static long readOwner()throws IOException{
  if(!OWNER.isFile()||OWNER.length()<2||OWNER.length()>32)return 0;
  FileInputStream in=new FileInputStream(OWNER);byte[] b=new byte[32];int n;
  try{n=in.read(b);if(n<1||in.read()!=-1)return 0;}finally{in.close();}
  String text=new String(b,0,n,"US-ASCII").trim();
  for(int i=0;i<text.length();i++)if(text.charAt(i)<'0'||text.charAt(i)>'9')return 0;
  try{long pid=Long.parseLong(text);return pid>1&&pid<2147483648L?pid:0;}catch(NumberFormatException e){return 0;}
 }
 public static byte[] encode(long owner,int active,long gen)throws IOException{
  if(owner<2||owner>2147483647L||(active!=0&&active!=1)||gen<1)throw new IOException("Invalid ownership state");
  String body=new StringBuffer("format=1\nowner_pid=").append(owner).append("\nactive=").append(active).append("\ngeneration=").append(gen).append('\n').toString();
  byte[] b=body.getBytes("US-ASCII");CRC32 crc=new CRC32();crc.update(b,0,b.length);
  return new StringBuffer(body).append("crc32=").append(Long.toHexString(crc.getValue())).append('\n').toString().getBytes("US-ASCII");
 }
 private static synchronized void publish(int active)throws IOException{
  long owner=readOwner();if(owner==0)return;
  if(owner==lastOwner&&active==lastActive&&STATE.isFile())return;
  long gen=generation==Long.MAX_VALUE?1:generation+1;
  byte[] data=encode(owner,active,gen);File pending=new File(new StringBuffer(STATE.getPath()).append(".pending").toString());
  FileOutputStream out=new FileOutputStream(pending);try{out.write(data);out.flush();}finally{out.close();}
  if(!pending.renameTo(STATE))throw new IOException("RAM ownership commit failed");
  generation=gen;lastOwner=owner;lastActive=active;
 }
}
