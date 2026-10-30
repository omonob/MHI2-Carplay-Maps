package org.omonob.cpmap.compat;
import org.dsi.ifc.carplay.AppStateRequest;
import org.dsi.ifc.carplay.DSICarplay;
/** Same narrow navigation-ownership suppression as the existing public patch.
 * No audio, speech, USB, UI, artwork or media fields are changed. */
public final class G11NavigationRequestFilter {
 private G11NavigationRequestFilter(){}
 public static boolean suppress(AppStateRequest[] requests){
  if(requests==null)return false;
  for(int i=0;i<requests.length;i++){
   AppStateRequest r=requests[i];
   if(r!=null&&r.appStateID==DSICarplay.APPLICATIONID_NAVIGATION&&r.state&&r.speechMode==0)return true;
  }
  return false;
 }
}
