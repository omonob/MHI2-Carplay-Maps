package org.omonob.cpmap.ui;
/** Same locale order as UiMessages; sixth OEM selector, not a checkbox. */
public final class CardMenuMessages {
 private static final String[][] TEXT={
  {"Navigation card","Center","Left","Right","Off"},
  {"导航卡片","中间","靠左","靠右","关闭"},
  {"導航卡片","中間","靠左","靠右","關閉"},
  {"ナビカード","中央","左","右","オフ"},
  {"내비 카드","가운데","왼쪽","오른쪽","끄기"},
  {"Navigationskarte","Mitte","Links","Rechts","Aus"},
  {"Carte de guidage","Centre","Gauche","Droite","Désactivée"},
  {"Tarjeta de navegación","Centro","Izquierda","Derecha","Desactivada"},
  {"Scheda navigazione","Centro","Sinistra","Destra","Disattivata"},
  {"Cartão de navegação","Centro","Esquerda","Direita","Desligado"},
  {"Cartão de navegação","Centro","Esquerda","Direita","Desativado"},
  {"Navigatiekaart","Midden","Links","Rechts","Uit"},
  {"Карточка навигации","По центру","Слева","Справа","Выключена"},
  {"Navigační karta","Uprostřed","Vlevo","Vpravo","Vypnuto"},
  {"Navigačná karta","V strede","Vľavo","Vpravo","Vypnuté"},
  {"Karta nawigacji","Środek","Lewa","Prawa","Wyłączona"},
  {"Navigeringskort","Mitten","Vänster","Höger","Av"},
  {"Navigationskort","Midten","Venstre","Højre","Fra"},
  {"Navigasjonskort","Midten","Venstre","Høyre","Av"},
  {"Navigointikortti","Keskellä","Vasen","Oikea","Pois"},
  {"Navigációs kártya","Középen","Balra","Jobbra","Ki"},
  {"Card de navigație","Centru","Stânga","Dreapta","Oprit"},
  {"Navigasyon kartı","Orta","Sol","Sağ","Kapalı"},
  {"Κάρτα πλοήγησης","Κέντρο","Αριστερά","Δεξιά","Ανενεργή"},
  {"Картка навігації","По центру","Ліворуч","Праворуч","Вимкнено"},
  {"Navigacijska kartica","Sredina","Lijevo","Desno","Isključeno"},
  {"Navigacijska kartica","Sredina","Levo","Desno","Izklopljeno"},
  {"بطاقة الملاحة","الوسط","اليسار","اليمين","إيقاف"},
  {"כרטיס ניווט","מרכז","שמאל","ימין","כבוי"},
  {"การ์ดนำทาง","กลาง","ซ้าย","ขวา","ปิด"},
  {"Kartu navigasi","Tengah","Kiri","Kanan","Nonaktif"}
 };
 public static String get(String language,int key){
  if(key<0||key>4)throw new IllegalArgumentException("Unknown card label");
  String normalized=UiMessages.normalize(language);String[] locales=UiMessages.supportedLocales();
  if(TEXT.length!=locales.length)throw new IllegalStateException("Card locale table drift");
  for(int i=0;i<locales.length;i++)if(locales[i].equals(normalized))return TEXT[i][key];
  return TEXT[0][key];
 }
}
