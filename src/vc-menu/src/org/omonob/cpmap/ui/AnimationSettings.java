package org.omonob.cpmap.ui;
import java.io.*;
import java.util.zip.CRC32;
/** VC connection-intro policy. Never changes the head-unit boot animation. */
public final class AnimationSettings {
 public static final int EVERY_CONNECT=0,ONCE_PER_BOOT=1,OFF=2;
 private static final Object LOCK=new Object();
 private final File directory,legacy;
 public static final class State {
  public final int mode;public final long generation;public final String source;
  State(int m,long g,String s){mode=m;generation=g;source=s;}
 }
 public AnimationSettings(File dir,File old)throws IOException{if(dir==null)throw new IOException("Missing state directory");directory=dir.getCanonicalFile();legacy=old;}
 public static AnimationSettings systemStore()throws IOException{
  String root=System.getProperty("cpmap.ui.stateDir"),old=System.getProperty("cpmap.ui.legacyIntroPolicy");
  if(root==null||root.length()==0)root="/mnt/icab/persistent/cpmap-ui";
  if(old==null||old.length()==0)old="/mnt/app/carplaymaps/intro-policy";
  return new AnimationSettings(new File(root),new File(old));
 }
 public State read()throws IOException{synchronized(LOCK){return readLocked();}}
 private State readLocked()throws IOException{
  File main=file("animation.state"),backup=file("animation.previous");State s=decode(main,"saved");if(s!=null)return s;
  s=decode(backup,"recovered");if(s!=null)return s;
  if(main.exists()||backup.exists())throw new IOException("Damaged animation preference; refusing a guessed state");
  int mode=ONCE_PER_BOOT;String source="default";
  if(legacy!=null&&legacy.isFile()){
   source="legacy";if(legacy.length()<=63){FileInputStream in=new FileInputStream(legacy);byte[] b=new byte[(int)legacy.length()];int n=0;
    try{while(n<b.length){int k=in.read(b,n,b.length-n);if(k<0)break;n+=k;}}finally{in.close();}
    String text=new String(b,"US-ASCII");if(text.equals("every-connect\n")||text.equals("every-connect\r\n"))mode=EVERY_CONNECT;
    else if(text.equals("off\n")||text.equals("off\r\n"))mode=OFF;
   }
  }return new State(mode,0,source);
 }
 public State set(int mode)throws IOException{
  if(mode<0||mode>2)throw new IllegalArgumentException("Unknown animation mode");
  synchronized(LOCK){State old=readLocked();if(old.generation==Long.MAX_VALUE)throw new IOException("Generation overflow");
   if(!directory.isDirectory()&&!directory.mkdirs())throw new IOException("Cannot create state directory");
   State next=new State(mode,old.generation+1,"saved");File pending=file("animation.pending");FileOutputStream out=new FileOutputStream(pending);
   try{out.write(encode(next));out.flush();out.getFD().sync();}finally{out.close();}
   State check=decode(pending,"pending");if(check==null||check.mode!=mode||check.generation!=next.generation)throw new IOException("Stage readback failed");
   File main=file("animation.state"),backup=file("animation.previous");
   if(decode(main,"main")!=null){if(backup.exists()&&!backup.delete())throw new IOException("Backup rotation failed");if(!main.renameTo(backup))throw new IOException("Backup failed");}
   else if(main.exists()&&!main.delete())throw new IOException("Cannot replace damaged primary");
   if(!pending.renameTo(main))throw new IOException("Commit failed; previous preserved");
   check=readLocked();if(check.mode!=mode||check.generation!=next.generation)throw new IOException("Commit readback failed");return check;
  }
 }
 private File file(String name)throws IOException{File f=new File(directory,name);if(!directory.equals(f.getCanonicalFile().getParentFile()))throw new IOException("Escaped settings directory");return f;}
 private static byte[] encode(State s)throws IOException{
  String body=new StringBuffer("format=1\nmode=").append(s.mode).append("\ngeneration=").append(s.generation).append('\n').toString();
  byte[] b=body.getBytes("US-ASCII");CRC32 crc=new CRC32();crc.update(b,0,b.length);
  return new StringBuffer(body).append("crc32=").append(Long.toHexString(crc.getValue())).append('\n').toString().getBytes("US-ASCII");
 }
 private static State decode(File f,String source)throws IOException{
  if(!f.isFile()||f.length()<1||f.length()>192)return null;byte[] b=new byte[(int)f.length()];FileInputStream in=new FileInputStream(f);int n=0;
  try{while(n<b.length){int k=in.read(b,n,b.length-n);if(k<0)return null;n+=k;}if(in.read()!=-1)return null;}finally{in.close();}
  String text=new String(b,"US-ASCII");String prefix="format=1\nmode=";if(!text.startsWith(prefix)||text.length()<=prefix.length())return null;
  int mode=text.charAt(prefix.length())-'0';String genPrefix="\ngeneration=";
  if(mode<0||mode>2||!text.startsWith(genPrefix,prefix.length()+1))return null;
  int begin=prefix.length()+1+genPrefix.length(),end=text.indexOf('\n',begin);if(end<0)return null;
  long gen;try{gen=Long.parseLong(text.substring(begin,end));}catch(NumberFormatException e){return null;}
  if(gen<1)return null;State s=new State(mode,gen,source);return text.equals(new String(encode(s),"US-ASCII"))?s:null;
 }
}
