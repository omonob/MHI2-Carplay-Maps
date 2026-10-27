import java.io.File;
import org.omonob.cpmap.ui.ProjectionSettings;
public final class ProjectionStateCli {
 public static void main(String[] args)throws Exception{
  ProjectionSettings store=new ProjectionSettings(new File(args[0]));
  if(args[1].equals("select")){store.select(0,1);store.select(1,2);store.select(2,3);}
  ProjectionSettings.State state=store.read();
  System.out.println("PROJECTION_STATE "+state.mapMask+state.mirrorMask+state.information+" generation="+state.generation+" source="+state.source);
 }
}
