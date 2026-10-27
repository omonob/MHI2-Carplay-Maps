import java.io.File;
import org.omonob.cpmap.ui.CardPositionSettings;
public final class CardStateCli {
 public static void main(String[] a)throws Exception{
  CardPositionSettings store=new CardPositionSettings(new File(a[0]));
  CardPositionSettings.State s=a.length==2?store.set(Integer.parseInt(a[1])):store.read();
  System.out.println("position="+s.position+" generation="+s.generation+" source="+s.source);
 }
}
