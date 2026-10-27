import java.io.File;
import org.omonob.cpmap.ui.*;
public final class FeatureFlowTest{
 static int checks;
 static void check(boolean value){checks++;if(!value)throw new AssertionError("feature flow "+checks);}
 static void state(ProjectionSettings s,int map,int mirror,boolean nav,boolean music)throws Exception{
  ProjectionSettings.State p=s.read();ProjectionRuntimePolicy.refreshNow();
  check(p.mapMask==map);check(p.mirrorMask==mirror);
  check(ProjectionRuntimePolicy.navigationAllowed()==nav);check(ProjectionRuntimePolicy.musicAllowed()==music);
 }
 public static void main(String[] args)throws Exception{
  System.setProperty("cpmap.ui.stateDir",args[0]);ProjectionSettings s=new ProjectionSettings(new File(args[0]));
  s.select(2,ProjectionSettings.INFO_ALL);state(s,1,0,true,true); // 1 information on
  s.setNavigationInformation(false);state(s,1,0,false,true);    // 2 nav off, music unaffected
  s.setMusicInformation(true);state(s,1,0,false,true);          // 3 music on, nav remains off
  s.select(2,ProjectionSettings.INFO_ALL);s.setCarplayMap(false);state(s,0,0,true,true); // 4 map off only
  s.setCarplayMirror(true);state(s,0,1,true,true);              // 5 copy main screen
  check(ClusterCommandRouter.port()==7033);
  check("canvas-zoom-in".equals(ClusterCommandRouter.command("zoom-in")));
  check("canvas-zoom-out".equals(ClusterCommandRouter.command("zoom-out")));
  check("canvas-noop".equals(ClusterCommandRouter.command("map-next")));
  for(int nav=0;nav<2;nav++)for(int music=0;music<2;music++){
   s.setNavigationInformation(nav!=0);s.setMusicInformation(music!=0);state(s,0,1,nav!=0,music!=0);
  }
  s.setCarplayMirror(false);s.setCarplayMap(true);ProjectionRuntimePolicy.refreshNow();
  check(ClusterCommandRouter.port()==7032);check("zoom-in".equals(ClusterCommandRouter.command("zoom-in")));
  check("map-next".equals(ClusterCommandRouter.command("map-next")));
  boolean blocked=false;try{s.setCarplayMirror(true);}catch(IllegalArgumentException expected){blocked=true;}check(blocked);
  System.out.println("FEATURE_FLOW_PASS checks="+checks+" steps=5 independent_information=1 canvas_routing=1");
 }
}
