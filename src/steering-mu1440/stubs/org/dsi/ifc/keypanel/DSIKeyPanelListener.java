package org.dsi.ifc.keypanel;
import org.dsi.ifc.base.DSIListener;
public interface DSIKeyPanelListener extends DSIListener {
    void updateKey2(int a, int b, int c, int d, int e);
    void updateEncoder2(int a, int b, int c, int d, int e);
    void updateDisplayTurnMechStatus(int a, int b);
    void updateRecognizerLanguage2(int a, String b, int c, int d);
    void updateRecognizerMode(int a, int b, int c);
    void updateCharacterEvent2(int a, String[] b, int[] c, int d);
    void updateGesture2(int a, int b, int c, boolean d, int e, int f, int g, int h, int i, int j);
    void genericSettingResponse(int a, int b, int c);
    void updateProximity(int a, int b, int c);
    void updateAdvancedProximity(int a, int b, int c, int d, int e, int f, int g, int h, int i, int j);
    void lastKey(int a, int b, int c);
    void updateKeyboardType(int a, int b);
    void updateTouchSensitiveArea(int a, int b, int c, int d, int e, int f);
    void getVersionInfo(int a, int b, String c);
    void updateInputPanelReady(int a, int b, int c);
    void getProperty(int a, int b, int c, int d, byte[] e);
    void asyncException(int a, String b, int c);
}
