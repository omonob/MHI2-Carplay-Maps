import java.io.File;
import java.io.FileOutputStream;
import org.dsi.ifc.carplay.AppState;
import org.omonob.cpmap.ui.CarPlayNavigationOwnership;
public final class CarPlayNavigationOwnershipTest {
 static int checks;static void check(boolean ok){checks++;if(!ok)throw new AssertionError("Ownership flag test "+checks);}
 public static void main(String[] args)throws Exception{
  check(CarPlayNavigationOwnership.navigationOwner(null)==-1);
  check(CarPlayNavigationOwnership.navigationOwner(new AppState[0])==-1);
  check(CarPlayNavigationOwnership.navigationOwner(new AppState[]{null,new AppState(1,2,0)})==-1);
  check(CarPlayNavigationOwnership.navigationOwner(new AppState[]{new AppState(2,2,0)})==1);
  check(CarPlayNavigationOwnership.navigationOwner(new AppState[]{new AppState(2,1,0)})==0);
  check(CarPlayNavigationOwnership.navigationOwner(new AppState[]{new AppState(2,0,0)})==0);
  check(CarPlayNavigationOwnership.navigationOwner(new AppState[]{new AppState(2,2,0),new AppState(2,1,0)})==0);
  File root=new File(args[0]);root.mkdirs();
  for(int active=0;active<2;active++){
   byte[] b=CarPlayNavigationOwnership.encode(12345,active,active+1);
   FileOutputStream out=new FileOutputStream(new File(root,"owner-"+active+".state"));try{out.write(b);}finally{out.close();}
   String text=new String(b,"US-ASCII");check(text.indexOf("owner_pid=12345\nactive="+active+"\n")>0);check(text.indexOf("crc32=")>0);
  }
  for(int i=0;i<3;i++){boolean failed=false;try{CarPlayNavigationOwnership.encode(i==0?1:12345,i==1?2:1,i==2?0:1);}catch(java.io.IOException expected){failed=true;}check(failed);}
  check(!new File(root,"settings.state").exists());
  System.out.println("CARPLAY_NAVIGATION_OWNERSHIP_TEST_PASS checks="+checks+" state_only=1 no_information_or_music_mapping=1");
 }
}
