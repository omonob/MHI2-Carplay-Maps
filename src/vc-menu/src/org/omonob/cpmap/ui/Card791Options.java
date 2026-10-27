package org.omonob.cpmap.ui;
/**791 exposes only ON/OFF. Reuse the exact CRC wire protocol without exposing
 * any790-only positions. Store RIGHT as the neutral enabled token, not geometry. */
public final class Card791Options {
 public static int count(){return 2;}
 public static int choiceForPosition(int position){
  if(position<0||position>3)throw new IllegalArgumentException("Invalid stored card state");
  return position==CardPositionSettings.OFF?1:0;
 }
 public static int positionForChoice(int choice){
  if(choice<0||choice>=2)throw new IllegalArgumentException("791 only supports on/off");
  return choice==0?CardPositionSettings.RIGHT:CardPositionSettings.OFF;
 }
 public static String label(String language,int choice){
  positionForChoice(choice);
  return UiMessages.get(language,choice==0?UiMessages.ON:UiMessages.OFF);
 }
}
