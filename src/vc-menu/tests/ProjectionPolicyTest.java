import java.io.*;
import org.omonob.cpmap.ui.*;
public final class ProjectionPolicyTest {
 static int checks;static void check(boolean b){checks++;if(!b)throw new AssertionError("policy "+checks);}
 public static void main(String[] args)throws Exception{
  File root=new File(args[0]);System.setProperty("cpmap.ui.stateDir",root.getAbsolutePath());ProjectionSettings store=new ProjectionSettings(root);
  ProjectionRuntimePolicy.refreshNow();check(ProjectionRuntimePolicy.mapAllowed());check(!ProjectionRuntimePolicy.androidAutoMapRequested());
  for(int info=0;info<4;info++){
   store.select(2,info);ProjectionRuntimePolicy.refreshNow();
   check(ProjectionRuntimePolicy.navigationAllowed()==(info==0 || info==2));check(ProjectionRuntimePolicy.musicAllowed()==(info==0 || info==1));
   check(ProjectionRuntimePolicy.callActiveOrMusicHidden(false)==(info==2 || info==3));check(ProjectionRuntimePolicy.callActiveOrMusicHidden(true));
  }
  store.select(0,2);store.select(1,3);ProjectionRuntimePolicy.refreshNow();
  check(!ProjectionRuntimePolicy.mapAllowed());check(ProjectionRuntimePolicy.androidAutoMapRequested());
  check(ProjectionRuntimePolicy.carplayMirrorRequested());check(!ProjectionRuntimePolicy.androidAutoMirrorRequested());check(ProjectionRuntimePolicy.carlifeMirrorRequested());
  store.select(0,0);ProjectionRuntimePolicy.refreshNow();check(ProjectionRuntimePolicy.mapAllowed());check(ProjectionRuntimePolicy.androidAutoMapRequested());
  check(!ProjectionRuntimePolicy.carplayMirrorRequested());check(!ProjectionRuntimePolicy.androidAutoMirrorRequested());check(ProjectionRuntimePolicy.carlifeMirrorRequested());
  check(!ProjectionRuntimePolicy.mirrorEngineAvailable());check(!ProjectionRuntimePolicy.androidAutoMapEngineAvailable());
  FileOutputStream out=new FileOutputStream(new File(root,"settings.state"));out.write(33);out.close();
  out=new FileOutputStream(new File(root,"settings.previous"));out.write(33);out.close();
  ProjectionRuntimePolicy.refreshNow();check(!ProjectionRuntimePolicy.lastReadSucceeded());check(ProjectionRuntimePolicy.mapAllowed());check(ProjectionRuntimePolicy.carlifeMirrorRequested());
  System.out.println("LINKED_POLICY_TEST_OK checks="+checks);
 }
}
