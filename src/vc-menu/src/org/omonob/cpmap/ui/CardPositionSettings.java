package org.omonob.cpmap.ui;
import java.io.*;
import java.util.zip.CRC32;
/** Independent navigation-card preference. Does not edit map/info/mirror state. */
public final class CardPositionSettings {
 public static final int CENTER=0,LEFT=1,RIGHT=2,OFF=3;
 private static final Object LOCK=new Object();
 private final File directory;
 public static final class State {
  public final int position;public final long generation;public final String source;
  State(int p,long g,String s){position=p;generation=g;source=s;}
 }
 public CardPositionSettings(File dir)throws IOException{
  if(dir==null)throw new IOException("Missing card preference directory");directory=dir.getCanonicalFile();
 }
 public static CardPositionSettings systemStore()throws IOException{
  String root=System.getProperty("cpmap.ui.stateDir");
  return new CardPositionSettings(new File(root==null||root.length()==0?"/mnt/icab/persistent/cpmap-ui":root));
 }
 public State read()throws IOException{synchronized(LOCK){return readLocked();}}
 private State readLocked()throws IOException{
  File main=file("navigation-card.state"),backup=file("navigation-card.previous");
  State s=decode(main,"saved");if(s!=null)return s;s=decode(backup,"recovered");if(s!=null)return s;
  if(main.exists()||backup.exists())throw new IOException("Damaged card preference; refusing guessed replacement");
  // First installation preserves this bench's accepted N8 right-hand layout.
  return new State(RIGHT,0,"default-existing-right");
 }
 public State set(int position)throws IOException{
  if(position<CENTER||position>OFF)throw new IllegalArgumentException("Unknown card position");
  synchronized(LOCK){State old=readLocked();
   if(old.generation==Long.MAX_VALUE)throw new IOException("Card generation overflow");
   if(!directory.isDirectory()&&!directory.mkdirs())throw new IOException("Cannot create preference directory");
   State next=new State(position,old.generation+1,"saved");File pending=file("navigation-card.pending");
   FileOutputStream out=new FileOutputStream(pending);
   try{out.write(encode(next));out.flush();out.getFD().sync();}finally{out.close();}
   State check=decode(pending,"pending");if(!same(next,check))throw new IOException("Card stage readback failed");
   File main=file("navigation-card.state"),backup=file("navigation-card.previous");
   if(decode(main,"main")!=null){
    if(backup.exists()&&!backup.delete())throw new IOException("Card backup rotation failed");
    if(!main.renameTo(backup))throw new IOException("Cannot preserve card backup");
   }else if(main.exists()&&!main.delete())throw new IOException("Cannot replace damaged card primary");
   if(!pending.renameTo(main))throw new IOException("Card commit failed; previous preserved");
   check=readLocked();if(!same(next,check))throw new IOException("Card commit readback failed");return check;
  }
 }
 private static boolean same(State a,State b){return b!=null&&a.position==b.position&&a.generation==b.generation;}
 private File file(String name)throws IOException{
  File f=new File(directory,name);if(!directory.equals(f.getCanonicalFile().getParentFile()))throw new IOException("Card path escaped directory");return f;
 }
 private static byte[] encode(State s)throws IOException{
  String body=new StringBuffer("format=1\nposition=").append(s.position).append("\ngeneration=").append(s.generation).append('\n').toString();
  byte[] b=body.getBytes("US-ASCII");CRC32 crc=new CRC32();crc.update(b,0,b.length);
  return new StringBuffer(body).append("crc32=").append(Long.toHexString(crc.getValue())).append('\n').toString().getBytes("US-ASCII");
 }
 private static State decode(File f,String origin)throws IOException{
  if(!f.isFile()||f.length()<1||f.length()>192)return null;
  byte[] b=new byte[(int)f.length()];FileInputStream in=new FileInputStream(f);int n=0;
  try{while(n<b.length){int k=in.read(b,n,b.length-n);if(k<0)return null;n+=k;}if(in.read()!=-1)return null;}finally{in.close();}
  String text=new String(b,"US-ASCII"),prefix="format=1\nposition=";
  if(!text.startsWith(prefix)||text.length()<=prefix.length())return null;
  int pos=text.charAt(prefix.length())-'0';String genPrefix="\ngeneration=";
  if(pos<0||pos>3||!text.startsWith(genPrefix,prefix.length()+1))return null;
  int begin=prefix.length()+1+genPrefix.length(),end=text.indexOf('\n',begin);if(end<0)return null;
  long gen;try{gen=Long.parseLong(text.substring(begin,end));}catch(NumberFormatException e){return null;}
  if(gen<1)return null;State s=new State(pos,gen,origin);
  return text.equals(new String(encode(s),"US-ASCII"))?s:null;
 }
}
