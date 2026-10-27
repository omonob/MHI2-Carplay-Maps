import java.io.File;
import org.omonob.cpmap.ui.WirelessCarPlaySettings;
public final class WirelessStateCli {
 public static void main(String[] args)throws Exception{
  WirelessCarPlaySettings store=new WirelessCarPlaySettings(new File(args[0]));
  WirelessCarPlaySettings.State s=args.length>1?store.set("1".equals(args[1])):store.read();
  System.out.println("enabled="+(s.enabled?1:0)+" generation="+s.generation+" source="+s.source);
 }
}
