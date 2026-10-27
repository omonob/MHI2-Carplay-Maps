package org.omonob.cpmap.ui;

/** The section heading only; never relabels the stock MirrorLink checkbox. */
public final class MenuSectionMessages {
    private static final String[] TEXT={
        "Virtual Cockpit settings",
        "虚拟座舱设置",
        "虛擬座艙設定",
        "仮想コックピット設定",
        "가상 콕핏 설정",
        "Einstellungen Virtual Cockpit",
        "Réglages du cockpit virtuel",
        "Ajustes del cockpit virtual",
        "Impostazioni cockpit virtuale",
        "Definições do cockpit virtual",
        "Configurações do cockpit virtual",
        "Virtual Cockpit-instellingen",
        "Настройки виртуальной кабины",
        "Nastavení virtuálního kokpitu",
        "Nastavenia virtuálneho kokpitu",
        "Ustawienia wirtualnego kokpitu",
        "Inställningar för Virtual Cockpit",
        "Indstillinger for Virtual Cockpit",
        "Innstillinger for Virtual Cockpit",
        "Virtuaaliohjaamon asetukset",
        "Virtuális műszerfal beállításai",
        "Setări cockpit virtual",
        "Sanal kokpit ayarları",
        "Ρυθμίσεις εικονικού κόκπιτ",
        "Налаштування віртуальної кабіни",
        "Postavke virtualnog kokpita",
        "Nastavitve virtualnega kokpita",
        "إعدادات المقصورة الافتراضية",
        "הגדרות תא נהג וירטואלי",
        "การตั้งค่าห้องขับเสมือน",
        "Pengaturan kokpit virtual"
    };
    private MenuSectionMessages(){ }
    public static int count(){return TEXT.length;}
    public static String get(String language){
        String code=UiMessages.normalize(language);
        String[] locales=UiMessages.supportedLocales();
        if(locales.length!=TEXT.length)throw new IllegalStateException("Section locale table mismatch");
        for(int i=0;i<locales.length;i++)if(locales[i].equals(code))return TEXT[i];
        return TEXT[0];
    }
}
