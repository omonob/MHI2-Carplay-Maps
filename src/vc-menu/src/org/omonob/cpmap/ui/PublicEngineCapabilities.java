package org.omonob.cpmap.ui;
/** Public2.0 ships the existing CP engines only, not AA/CarLife engines.
 * Keep the shared menu vocabulary, but never offer a nonfunctional action. */
public final class PublicEngineCapabilities{
 private PublicEngineCapabilities(){}
 public static boolean choiceAllowed(int field,int choice){
  if(field==ProjectionSettings.VC_MAP||field==ProjectionSettings.VC_MIRROR)
   return (ProjectionSettings.choiceMask(field,choice)&~ProjectionSettings.CP)==0;
  return true;
 }
}
