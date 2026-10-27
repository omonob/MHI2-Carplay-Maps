package org.omonob.cpmap.ui;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.InputStreamReader;
import de.vw.mib.configuration.ConfigurationManager;
import de.vw.mib.asl.framework.internal.framework.ServiceManager;

/** Native HMI labels; locale overrides are limited to this offline test module. */
public final class UiMessages {
    public static final int MAP=0, MIRROR=1, INFORMATION=2, CARLIFE=3, OFF=4, ON=5;
    public static final int ALL=6, NAVIGATION_OFF=7, MUSIC_OFF=8, PENDING=9;
    public static final int SAVE_FAILED=10, CURRENT=11, OFFLINE_TEST=12, COUNT=13;
    private static final String[] LOCALES={"en","zh_CN","zh_TW","ja","ko","de","fr","es","it","pt","pt_BR","nl","ru","cs","sk","pl","sv","da","nb","fi","hu","ro","tr","el","uk","hr","sl","ar","he","th","id"};
    private static final String[][] TEXT={
      {"CarPlay maps","CarPlay mirroring","CarPlay information","Baidu CarLife mirroring","Off","On","All enabled","Navigation off","Music off","Mirroring not implemented","Save failed","Current","Offline test"},
      {"CarPlay 地图","CarPlay 镜像","CarPlay 信息","百度 CarLife 镜像","关闭","开启","完全打开","关闭导航","关闭音乐","镜像引擎尚未接入","保存失败","当前","离线测试"},
      {"CarPlay 地圖","CarPlay 鏡像","CarPlay 資訊","百度 CarLife 鏡像","關閉","開啟","完全開啟","關閉導覽","關閉音樂","鏡像引擎尚未接入","儲存失敗","目前","離線測試"},
      {"CarPlay 地図","CarPlay ミラーリング","CarPlay 情報","Baidu CarLife ミラーリング","オフ","オン","すべて有効","ナビをオフ","音楽をオフ","ミラー機能は未実装","保存できません","現在","オフラインテスト"},
      {"CarPlay 지도","CarPlay 미러링","CarPlay 정보","Baidu CarLife 미러링","끄기","켜기","모두 켜기","내비게이션 끄기","음악 끄기","미러링 엔진 미구현","저장 실패","현재","오프라인 테스트"},
      {"CarPlay-Karte","CarPlay-Spiegelung","CarPlay-Informationen","Baidu-CarLife-Spiegelung","Aus","Ein","Alles aktiviert","Navigation aus","Musik aus","Spiegelung nicht implementiert","Speichern fehlgeschlagen","Aktuell","Offline-Test"},
      {"Carte CarPlay","Recopie CarPlay","Informations CarPlay","Recopie Baidu CarLife","Désactivé","Activé","Tout activer","Navigation désactivée","Musique désactivée","Recopie non implémentée","Échec de l’enregistrement","Actuel","Test hors ligne"},
      {"Mapa de CarPlay","Duplicación de CarPlay","Información de CarPlay","Duplicación de Baidu CarLife","Desactivado","Activado","Todo activado","Navegación desactivada","Música desactivada","Duplicación no implementada","Error al guardar","Actual","Prueba sin conexión"},
      {"Mappa CarPlay","Duplicazione CarPlay","Informazioni CarPlay","Duplicazione Baidu CarLife","Disattivato","Attivato","Tutto attivato","Navigazione disattivata","Musica disattivata","Duplicazione non implementata","Salvataggio non riuscito","Attuale","Test offline"},
      {"Mapa CarPlay","Espelhamento CarPlay","Informações CarPlay","Espelhamento Baidu CarLife","Desligado","Ligado","Tudo ativado","Navegação desligada","Música desligada","Espelhamento não implementado","Falha ao guardar","Atual","Teste offline"},
      {"Mapa do CarPlay","Espelhamento do CarPlay","Informações do CarPlay","Espelhamento Baidu CarLife","Desativado","Ativado","Tudo ativado","Navegação desativada","Música desativada","Espelhamento não implementado","Falha ao salvar","Atual","Teste offline"},
      {"CarPlay-kaart","CarPlay-schermkopie","CarPlay-informatie","Baidu CarLife-schermkopie","Uit","Aan","Alles ingeschakeld","Navigatie uit","Muziek uit","Schermkopie nog niet beschikbaar","Opslaan mislukt","Huidig","Offlinetest"},
      {"Карты CarPlay","Зеркало CarPlay","Данные CarPlay","Зеркало Baidu CarLife","Выключено","Включено","Всё включено","Без навигации","Без музыки","Зеркало не реализовано","Ошибка сохранения","Сейчас","Офлайн-тест"},
      {"Mapa CarPlay","Zrcadlení CarPlay","Informace CarPlay","Zrcadlení Baidu CarLife","Vypnuto","Zapnuto","Vše zapnuto","Navigace vypnuta","Hudba vypnuta","Zrcadlení není implementováno","Uložení selhalo","Aktuální","Offline test"},
      {"Mapa CarPlay","Zrkadlenie CarPlay","Informácie CarPlay","Zrkadlenie Baidu CarLife","Vypnuté","Zapnuté","Všetko zapnuté","Navigácia vypnutá","Hudba vypnutá","Zrkadlenie nie je implementované","Uloženie zlyhalo","Aktuálne","Offline test"},
      {"Mapa CarPlay","Kopia ekranu CarPlay","Informacje CarPlay","Kopia ekranu Baidu CarLife","Wyłączone","Włączone","Wszystko włączone","Nawigacja wyłączona","Muzyka wyłączona","Kopia ekranu niedostępna","Błąd zapisu","Bieżące","Test offline"},
      {"CarPlay-karta","CarPlay-spegling","CarPlay-information","Baidu CarLife-spegling","Av","På","Allt aktiverat","Navigering av","Musik av","Spegling ej implementerad","Kunde inte spara","Aktuell","Offlinetest"},
      {"CarPlay-kort","CarPlay-spejling","CarPlay-information","Baidu CarLife-spejling","Fra","Til","Alt aktiveret","Navigation fra","Musik fra","Spejling er ikke implementeret","Kunne ikke gemme","Aktuel","Offlinetest"},
      {"CarPlay-kart","CarPlay-speiling","CarPlay-informasjon","Baidu CarLife-speiling","Av","På","Alt aktivert","Navigasjon av","Musikk av","Speiling er ikke implementert","Kunne ikke lagre","Gjeldende","Frakoblet test"},
      {"CarPlay-kartta","CarPlay-peilaus","CarPlay-tiedot","Baidu CarLife -peilaus","Pois","Päällä","Kaikki käytössä","Navigointi pois","Musiikki pois","Peilausta ei ole toteutettu","Tallennus epäonnistui","Nykyinen","Offline-testi"},
      {"CarPlay-térkép","CarPlay-tükrözés","CarPlay-információk","Baidu CarLife-tükrözés","Ki","Be","Minden bekapcsolva","Navigáció kikapcsolva","Zene kikapcsolva","A tükrözés nincs megvalósítva","Sikertelen mentés","Jelenlegi","Offline teszt"},
      {"Hartă CarPlay","Oglindire CarPlay","Informații CarPlay","Oglindire Baidu CarLife","Oprit","Pornit","Totul activat","Navigație oprită","Muzică oprită","Oglindirea nu este implementată","Salvare eșuată","Curent","Test offline"},
      {"CarPlay haritası","CarPlay yansıtma","CarPlay bilgileri","Baidu CarLife yansıtma","Kapalı","Açık","Tümü açık","Navigasyon kapalı","Müzik kapalı","Yansıtma henüz uygulanmadı","Kaydedilemedi","Geçerli","Çevrimdışı test"},
      {"Χάρτης CarPlay","Κατοπτρισμός CarPlay","Πληροφορίες CarPlay","Κατοπτρισμός Baidu CarLife","Ανενεργό","Ενεργό","Όλα ενεργά","Πλοήγηση ανενεργή","Μουσική ανενεργή","Ο κατοπτρισμός δεν υλοποιήθηκε","Αποτυχία αποθήκευσης","Τρέχον","Δοκιμή εκτός σύνδεσης"},
      {"Карти CarPlay","Дзеркало CarPlay","Інформація CarPlay","Дзеркало Baidu CarLife","Вимкнено","Увімкнено","Усе ввімкнено","Навігацію вимкнено","Музику вимкнено","Дзеркало ще не реалізовано","Помилка збереження","Поточне","Офлайн-тест"},
      {"CarPlay karta","CarPlay zrcaljenje","CarPlay informacije","Baidu CarLife zrcaljenje","Isključeno","Uključeno","Sve uključeno","Navigacija isključena","Glazba isključena","Zrcaljenje nije implementirano","Spremanje nije uspjelo","Trenutno","Izvanmrežni test"},
      {"Zemljevid CarPlay","Zrcaljenje CarPlay","Podatki CarPlay","Zrcaljenje Baidu CarLife","Izklopljeno","Vklopljeno","Vse vključeno","Navigacija izklopljena","Glasba izklopljena","Zrcaljenje še ni izvedeno","Shranjevanje ni uspelo","Trenutno","Preizkus brez povezave"},
      {"خرائط CarPlay","عكس شاشة CarPlay","معلومات CarPlay","عكس شاشة Baidu CarLife","إيقاف","تشغيل","تشغيل الكل","إيقاف الملاحة","إيقاف الموسيقى","عكس الشاشة غير مطوّر بعد","فشل الحفظ","الحالي","اختبار دون اتصال"},
      {"מפות CarPlay","שיקוף CarPlay","מידע CarPlay","שיקוף Baidu CarLife","כבוי","פעיל","הכול פעיל","ניווט כבוי","מוזיקה כבויה","השיקוף עדיין לא ממומש","השמירה נכשלה","נוכחי","בדיקה לא מקוונת"},
      {"แผนที่ CarPlay","การสะท้อน CarPlay","ข้อมูล CarPlay","การสะท้อน Baidu CarLife","ปิด","เปิด","เปิดทั้งหมด","ปิดการนำทาง","ปิดเพลง","ยังไม่ได้พัฒนาระบบสะท้อน","บันทึกไม่สำเร็จ","ปัจจุบัน","ทดสอบออฟไลน์"},
      {"Peta CarPlay","Pencerminan CarPlay","Informasi CarPlay","Pencerminan Baidu CarLife","Nonaktif","Aktif","Semua aktif","Navigasi nonaktif","Musik nonaktif","Pencerminan belum diterapkan","Gagal menyimpan","Saat ini","Uji offline"}
    };

    private UiMessages() { }
    public static String[] supportedLocales() {
        String[] result=new String[LOCALES.length];
        System.arraycopy(LOCALES,0,result,0,result.length); return result;
    }
    public static String normalize(String language) {
        if(language==null) return "en";
        String code=language.replace('-','_');
        if(code.startsWith("zh")) return code.indexOf("TW")>=0 || code.indexOf("HK")>=0 || code.indexOf("Hant")>=0?"zh_TW":"zh_CN";
        if(code.equals("pt_BR")) return "pt_BR";
        int dash=code.indexOf('_'); if(dash>=0) code=code.substring(0,dash);
        if(code.equals("no")) code="nb";
        if(code.equals("iw")) code="he";
        if(code.equals("in")) code="id";
        for(int i=0;i<LOCALES.length;i++) if(LOCALES[i].equals(code)) return code;
        return "en";
    }
    public static String get(String language,int key) {
        if(key<0 || key>=COUNT) throw new IllegalArgumentException("Unknown label");
        String code=normalize(language);
        for(int i=0;i<LOCALES.length;i++) if(LOCALES[i].equals(code)) return TEXT[i][key];
        return TEXT[0][key];
    }
    public static boolean isRightToLeft(String language) {
        String code=normalize(language); return code.equals("ar") || code.equals("he");
    }
    public static String currentLanguage() {
        String forced=System.getProperty("cpmap.ui.language");
        if(forced!=null && forced.length()>0) return normalize(forced);
        // Offline capture override, not a production system-language mutation.
        try {
            String root=System.getProperty("cpmap.ui.stateDir");
            if(root==null || root.length()==0) root="/mnt/icab/persistent/cpmap-ui";
            File file=new File(root,"test-language");
            if(file.isFile() && file.length()<=16) {
                BufferedReader reader=new BufferedReader(new InputStreamReader(new FileInputStream(file),"US-ASCII"));
                try { String value=reader.readLine(); if(value!=null && value.length()>0) return normalize(value); }
                finally { reader.close(); }
            }
        } catch(Throwable ignored) { }
        try {
            Object manager=ServiceManager.configManagerDiag;
            if(manager instanceof ConfigurationManager)
                return normalize(((ConfigurationManager)manager).getCurrentGuiLanguage());
        } catch(Throwable ignored) { }
        return "en";
    }
}
