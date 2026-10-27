package org.omonob.cpmap.ui;
public final class WirelessMenuMessages {
 private static final String[] TEXT={
  "Wireless CarPlay","无线 CarPlay","無線 CarPlay","ワイヤレス CarPlay","무선 CarPlay",
  "Drahtloses CarPlay","CarPlay sans fil","CarPlay inalámbrico","CarPlay wireless",
  "CarPlay sem fios","CarPlay sem fio","Draadloze CarPlay","Беспроводной CarPlay",
  "Bezdrátové CarPlay","Bezdrôtové CarPlay","Bezprzewodowe CarPlay","Trådlös CarPlay",
  "Trådløs CarPlay","Trådløs CarPlay","Langaton CarPlay","Vezeték nélküli CarPlay",
  "CarPlay wireless","Kablosuz CarPlay","Ασύρματο CarPlay","Бездротовий CarPlay",
  "Bežični CarPlay","Brezžični CarPlay","CarPlay اللاسلكي","CarPlay אלחוטי","CarPlay ไร้สาย","CarPlay nirkabel"
 };
 private WirelessMenuMessages(){ }
 public static int count(){return TEXT.length;}
 public static String get(String language){
  String code=UiMessages.normalize(language);String[] locales=UiMessages.supportedLocales();
  if(TEXT.length!=locales.length)throw new IllegalStateException("Wireless locale count mismatch");
  for(int i=0;i<locales.length;i++)if(locales[i].equals(code))return TEXT[i];
  return TEXT[0];
 }
}
