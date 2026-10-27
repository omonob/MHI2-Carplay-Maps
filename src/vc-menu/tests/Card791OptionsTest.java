import java.io.File;
import org.omonob.cpmap.ui.*;
public final class Card791OptionsTest {
 static int checks;static void check(boolean value){checks++;if(!value)throw new AssertionError("791 option check "+checks);}
 public static void main(String[] args)throws Exception{
  check(Card791Options.count()==2);
  for(int pos=0;pos<4;pos++)check(Card791Options.choiceForPosition(pos)==(pos==3?1:0));
  check(Card791Options.positionForChoice(0)==2);check(Card791Options.positionForChoice(1)==3);
  for(int bad=-1;bad<4;bad++){if(bad==0||bad==1)continue;boolean rejected=false;try{Card791Options.positionForChoice(bad);}catch(IllegalArgumentException e){rejected=true;}check(rejected);}
  for(int i=0;i<UiMessages.supportedLocales().length;i++){
   String lang=UiMessages.supportedLocales()[i];check(Card791Options.label(lang,0).equals(UiMessages.get(lang,UiMessages.ON)));check(Card791Options.label(lang,1).equals(UiMessages.get(lang,UiMessages.OFF)));
  }
  CardPositionSettings store=new CardPositionSettings(new File(args[0]));
  for(int choice=0;choice<2;choice++){store.set(Card791Options.positionForChoice(choice));check(Card791Options.choiceForPosition(new CardPositionSettings(new File(args[0])).read().position)==choice);}
  System.out.println("VW791_TWO_CHOICE_CARD_PASS checks="+checks+" locales=31 values=on,off persisted=1");
 }
}
