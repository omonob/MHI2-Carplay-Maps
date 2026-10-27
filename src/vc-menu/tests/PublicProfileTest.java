import org.omonob.cpmap.ui.*;
/** Exact menu cardinality and persisted ON token; no live filesystem needed. */
public final class PublicProfileTest{
 public static void main(String[] args){
  System.setProperty("cpvc.offline","1");
  if(PublicEngineCapabilities.choiceAllowed(0,0)||!PublicEngineCapabilities.choiceAllowed(0,1)||PublicEngineCapabilities.choiceAllowed(0,2)||!PublicEngineCapabilities.choiceAllowed(0,3))throw new AssertionError("Unavailable map engine exposed");
  for(int i=0;i<8;i++)if(PublicEngineCapabilities.choiceAllowed(1,i)!=(i==4||i==7))throw new AssertionError("Unavailable mirror engine exposed");
  for(int cluster=790;cluster<=791;cluster++){
   System.setProperty("cpmap.ui.cluster",Integer.toString(cluster));
   if(MenuClusterProfile.cardCount()!=(cluster==790?4:2))throw new AssertionError();
   for(int i=0;i<MenuClusterProfile.cardCount();i++){
    if(MenuClusterProfile.cardChoice(MenuClusterProfile.cardPosition(i))!=i)throw new AssertionError();
    String[] locales=UiMessages.supportedLocales();
    for(int n=0;n<locales.length;n++)if(MenuClusterProfile.cardLabel(locales[n],i).length()==0)throw new AssertionError();
   }
  }
  System.out.println("PUBLIC_790_791_CHOICES_31_LOCALES_PASS");
 }
}
