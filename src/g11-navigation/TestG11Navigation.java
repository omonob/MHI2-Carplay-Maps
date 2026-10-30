import org.dsi.ifc.carplay.*;
import org.omonob.cpmap.compat.G11NavigationRequestFilter;
public final class TestG11Navigation {
 public static void main(String[] a)throws Exception{
  int checks=0;
  for(int id=0;id<7;id++)for(int state=0;state<2;state++)for(int speech=0;speech<5;speech++){
   AppStateRequest r=new AppStateRequest();r.appStateID=id;r.state=state==1;r.speechMode=speech;
   if(G11NavigationRequestFilter.suppress(new AppStateRequest[]{r})!=(id==2&&state==1&&speech==0))throw new AssertionError("Unexpected request suppression");checks++;
  }
  if(G11NavigationRequestFilter.suppress(null)||G11NavigationRequestFilter.suppress(new AppStateRequest[]{null}))throw new AssertionError("Null request");checks+=2;
  Class nav=Class.forName("de.vw.mib.asl.internal.navigation.api.impl.ASLNavigationServices");
  java.lang.reflect.Constructor ctor=nav.getConstructors()[0];Object object=ctor.newInstance(new Object[ctor.getParameterTypes().length]);
  Object result=nav.getMethod("stopGuidance",new Class[]{Integer.TYPE}).invoke(object,new Object[]{Integer.valueOf(6)});
  if(((Integer)result).intValue()!=0)throw new AssertionError("CarPlay stop not suppressed");checks++;
  Class mode=Class.forName("de.vw.mib.asl.internal.carplay.common.CarPlayModeHandling",false,TestG11Navigation.class.getClassLoader());
  if(mode.getConstructors()[0].getParameterTypes().length!=5)throw new AssertionError("Original five-argument constructor lost");checks++;
  Class.forName("de.esolutions.fw.comm.dsi.carplay.impl.DSICarplayProxy",false,TestG11Navigation.class.getClassLoader());checks++;
  System.out.println("G11_EXACT_NAVIGATION_BEHAVIOR_PASS checks="+checks+" audio_and_speech_requests_unchanged=1");
 }
}
