package org.omonob.cpmap.ui;
/** Same 31 locales and order as the accepted menu; concise native selectors. */
public final class AnimationMenuMessages {
 private static final String[][] TEXT={
  {"VC animation","Every connection","Once per boot","Off"},
  {"虚拟座舱动画","每次启动","仅首次启动","关闭"},
  {"虛擬座艙動畫","每次啟動","僅首次啟動","關閉"},
  {"VC アニメーション","接続のたび","起動後の初回のみ","オフ"},
  {"가상 콕핏 애니메이션","연결할 때마다","부팅 후 한 번","끄기"},
  {"VC-Startanimation","Bei jeder Verbindung","Einmal pro Start","Aus"},
  {"Animation du VC","À chaque connexion","Une fois au démarrage","Désactivée"},
  {"Animación del VC","En cada conexión","Una vez al arrancar","Desactivada"},
  {"Animazione VC","A ogni connessione","Una volta all’avvio","Disattivata"},
  {"Animação do VC","Em cada ligação","Uma vez ao iniciar","Desligada"},
  {"Animação do VC","A cada conexão","Uma vez ao iniciar","Desativada"},
  {"VC-animatie","Bij elke verbinding","Eenmaal per start","Uit"},
  {"Анимация VC","При подключении","Один раз за запуск","Выключена"},
  {"Animace VC","Při každém připojení","Jednou po zapnutí","Vypnuto"},
  {"Animácia VC","Pri každom pripojení","Raz po zapnutí","Vypnuté"},
  {"Animacja VC","Przy każdym połączeniu","Raz po uruchomieniu","Wyłączona"},
  {"VC-animering","Vid varje anslutning","En gång per start","Av"},
  {"VC-animation","Ved hver forbindelse","Én gang pr. opstart","Fra"},
  {"VC-animasjon","Ved hver tilkobling","Én gang per oppstart","Av"},
  {"VC-animaatio","Joka yhteydellä","Kerran käynnistyksessä","Pois"},
  {"VC-animáció","Minden csatlakozáskor","Indításonként egyszer","Ki"},
  {"Animație VC","La fiecare conectare","O dată la pornire","Oprită"},
  {"VC animasyonu","Her bağlantıda","Açılışta bir kez","Kapalı"},
  {"Κίνηση VC","Σε κάθε σύνδεση","Μία φορά στην εκκίνηση","Ανενεργή"},
  {"Анімація VC","За підключення","Один раз за запуск","Вимкнена"},
  {"VC animacija","Pri svakom povezivanju","Jednom pri pokretanju","Isključena"},
  {"Animacija VC","Ob vsaki povezavi","Enkrat ob zagonu","Izklopljena"},
  {"رسوم VC المتحركة","عند كل اتصال","مرة عند التشغيل","إيقاف"},
  {"הנפשת VC","בכל חיבור","פעם אחת באתחול","כבוי"},
  {"ภาพเคลื่อนไหว VC","ทุกครั้งที่เชื่อมต่อ","ครั้งแรกหลังเปิดเครื่อง","ปิด"},
  {"Animasi VC","Setiap koneksi","Sekali saat mulai","Nonaktif"}
 };
 private AnimationMenuMessages(){}
 public static String get(String language,int key){
  if(key<0||key>3)throw new IllegalArgumentException("Animation label key");
  String[] locales=UiMessages.supportedLocales();if(locales.length!=TEXT.length)throw new IllegalStateException("Animation locale count");
  String code=UiMessages.normalize(language);for(int i=0;i<locales.length;i++)if(locales[i].equals(code))return TEXT[i][key];return TEXT[0][key];
 }
}
