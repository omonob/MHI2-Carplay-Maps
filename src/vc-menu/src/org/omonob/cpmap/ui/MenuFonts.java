package org.omonob.cpmap.ui;
import java.io.File;
import de.vw.mib.graphics.font.FontAttribs;
import de.vw.mib.widgets.models.TextAreaModel;
/** Only the new menu labels use these optional, named OEM font families. */
final class MenuFonts {
    private MenuFonts() { }
    static void apply(TextAreaModel label,FontAttribs original,String language) {
        String name=null;
        if(language.equals("ko"))name="CPMAP-Korean.ttf";
        if(language.equals("ar") || language.equals("he") || language.equals("th"))name="CPMAP-Universal.ttf";
        if(name!=null && new File(new StringBuffer("/mnt/app/eso/hmi/lsd/Resources/skin1/Fonts/").append(name).toString()).isFile()
            && new File(new StringBuffer("/mnt/app/eso/hmi/lsd/Resources/skin3/Fonts/").append(name).toString()).isFile())
            label.set_font(new FontAttribs(name,original.getSize(),original.getStyle()));
        else label.set_font(new FontAttribs(original));
    }
}
