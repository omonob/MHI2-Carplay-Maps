package org.omonob.cpmap.ui;
import java.io.FileInputStream;
/** Exact installer profile; never infer a cluster from the phone canvas. */
public final class MenuClusterProfile {
 private MenuClusterProfile(){}
 public static int cluster(){
  String test=System.getProperty("cpmap.ui.cluster");
  if(test!=null && "1".equals(System.getProperty("cpvc.offline"))){
   if("790".equals(test))return 790;if("791".equals(test))return 791;
  }
  FileInputStream in=null;
  try{
   in=new FileInputStream("/mnt/app/carplaymaps/PROFILE");
   byte[] bytes=new byte[64];int n=in.read(bytes);
   if(n<=0)throw new IllegalStateException("Empty cluster profile");
   String text=new String(bytes,0,n,"US-ASCII").trim();
   if(text.equals("MU1102/790")||text.equals("MU1367/790")||text.equals("MU1440/790"))return 790;
   if(text.equals("MU1102/791")||text.equals("MU1367/791"))return 791;
   throw new IllegalStateException("Unknown cluster profile");
  }catch(Exception error){throw new IllegalStateException(error.toString());}
  finally{if(in!=null)try{in.close();}catch(Exception ignored){}}
 }
 public static boolean is791(){return cluster()==791;}
 public static int cardChoice(int position){return is791()?Card791Options.choiceForPosition(position):position;}
 public static int cardPosition(int choice){return is791()?Card791Options.positionForChoice(choice):choice;}
 public static int cardCount(){return is791()?2:4;}
 public static String cardLabel(String language,int choice){return is791()?Card791Options.label(language,choice):CardMenuMessages.get(language,choice+1);}
}
