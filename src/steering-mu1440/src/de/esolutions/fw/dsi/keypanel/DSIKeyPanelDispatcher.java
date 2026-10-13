/*
 * Decompiled with CFR 0.152.
 */
package de.esolutions.fw.dsi.keypanel;

import de.esolutions.fw.comm.core.IReplyService;
import de.esolutions.fw.comm.dsi.keypanel.DSIKeyPanelReply;
import de.esolutions.fw.comm.dsi.keypanel.impl.DSIKeyPanelReplyService;
import de.esolutions.fw.dsi.base.AbstractDispatcher;
import java.lang.reflect.Method;
import java.util.Iterator;
import org.dsi.ifc.keypanel.DSIKeyPanelListener;

public class DSIKeyPanelDispatcher
extends AbstractDispatcher
implements DSIKeyPanelReply {
    private DSIKeyPanelReplyService service = new DSIKeyPanelReplyService(this);
    static Class class$org$dsi$ifc$keypanel$DSIKeyPanelListener;
    static Class class$java$lang$String;

    public DSIKeyPanelDispatcher(int n) {
        super(n, (class$org$dsi$ifc$keypanel$DSIKeyPanelListener == null ? (class$org$dsi$ifc$keypanel$DSIKeyPanelListener = DSIKeyPanelDispatcher.class$("org.dsi.ifc.keypanel.DSIKeyPanelListener")) : class$org$dsi$ifc$keypanel$DSIKeyPanelListener).getName());
    }

    public IReplyService getService() {
        return this.service;
    }

    public void updateKey2(int n, int n2, int n3, int n4, int n5) {
        if ((n5 & 0x80) == 128) {
            n5 ^= 0x80;
            Iterator iterator = this.getUnconfirmedNotificationListenerIterator(25);
            while (iterator.hasNext()) {
                try {
                    DSIKeyPanelListener dSIKeyPanelListener = (DSIKeyPanelListener)iterator.next();
                    this.confirmNotificationListener(25, dSIKeyPanelListener);
                    dSIKeyPanelListener.updateKey2(n, n2, n3, n4, n5);
                }
                catch (Exception exception) {
                    this.traceException(exception);
                }
            }
        } else {
            Iterator iterator = this.getNotificationListenerIterator(25);
            while (iterator.hasNext()) {
                try {
                    DSIKeyPanelListener dSIKeyPanelListener = (DSIKeyPanelListener)iterator.next();
                    dSIKeyPanelListener.updateKey2(n, n2, n3, n4, n5);
                }
                catch (Exception exception) {
                    this.traceException(exception);
                }
            }
        }
    }

    public void updateEncoder2(int n, int n2, int n3, int n4, int n5) {
        try {
            de.vw.mib.carplay.clustercontrol.Mhi2ClusterControlBridge.onEncoder2(
                n, n2, n3, n4, n5);
        }
        catch (Throwable ignored) {
            /* The production DSI dispatch path must never depend on the add-on. */
        }
        if ((n5 & 0x80) == 128) {
            n5 ^= 0x80;
            Iterator iterator = this.getUnconfirmedNotificationListenerIterator(23);
            while (iterator.hasNext()) {
                try {
                    DSIKeyPanelListener dSIKeyPanelListener = (DSIKeyPanelListener)iterator.next();
                    this.confirmNotificationListener(23, dSIKeyPanelListener);
                    dSIKeyPanelListener.updateEncoder2(n, n2, n3, n4, n5);
                }
                catch (Exception exception) {
                    this.traceException(exception);
                }
            }
        } else {
            Iterator iterator = this.getNotificationListenerIterator(23);
            while (iterator.hasNext()) {
                try {
                    DSIKeyPanelListener dSIKeyPanelListener = (DSIKeyPanelListener)iterator.next();
                    dSIKeyPanelListener.updateEncoder2(n, n2, n3, n4, n5);
                }
                catch (Exception exception) {
                    this.traceException(exception);
                }
            }
        }
    }

    public void updateDisplayTurnMechStatus(int n, int n2) {
        if ((n2 & 0x80) == 128) {
            n2 ^= 0x80;
            Iterator iterator = this.getUnconfirmedNotificationListenerIterator(7);
            while (iterator.hasNext()) {
                try {
                    DSIKeyPanelListener dSIKeyPanelListener = (DSIKeyPanelListener)iterator.next();
                    this.confirmNotificationListener(7, dSIKeyPanelListener);
                    dSIKeyPanelListener.updateDisplayTurnMechStatus(n, n2);
                }
                catch (Exception exception) {
                    this.traceException(exception);
                }
            }
        } else {
            Iterator iterator = this.getNotificationListenerIterator(7);
            while (iterator.hasNext()) {
                try {
                    DSIKeyPanelListener dSIKeyPanelListener = (DSIKeyPanelListener)iterator.next();
                    dSIKeyPanelListener.updateDisplayTurnMechStatus(n, n2);
                }
                catch (Exception exception) {
                    this.traceException(exception);
                }
            }
        }
    }

    public void updateRecognizerLanguage2(int n, String string, int n2, int n3) {
        if ((n3 & 0x80) == 128) {
            n3 ^= 0x80;
            Iterator iterator = this.getUnconfirmedNotificationListenerIterator(22);
            while (iterator.hasNext()) {
                try {
                    DSIKeyPanelListener dSIKeyPanelListener = (DSIKeyPanelListener)iterator.next();
                    this.confirmNotificationListener(22, dSIKeyPanelListener);
                    dSIKeyPanelListener.updateRecognizerLanguage2(n, string, n2, n3);
                }
                catch (Exception exception) {
                    this.traceException(exception);
                }
            }
        } else {
            Iterator iterator = this.getNotificationListenerIterator(22);
            while (iterator.hasNext()) {
                try {
                    DSIKeyPanelListener dSIKeyPanelListener = (DSIKeyPanelListener)iterator.next();
                    dSIKeyPanelListener.updateRecognizerLanguage2(n, string, n2, n3);
                }
                catch (Exception exception) {
                    this.traceException(exception);
                }
            }
        }
    }

    public void updateRecognizerMode(int n, int n2, int n3) {
        if ((n3 & 0x80) == 128) {
            n3 ^= 0x80;
            Iterator iterator = this.getUnconfirmedNotificationListenerIterator(9);
            while (iterator.hasNext()) {
                try {
                    DSIKeyPanelListener dSIKeyPanelListener = (DSIKeyPanelListener)iterator.next();
                    this.confirmNotificationListener(9, dSIKeyPanelListener);
                    dSIKeyPanelListener.updateRecognizerMode(n, n2, n3);
                }
                catch (Exception exception) {
                    this.traceException(exception);
                }
            }
        } else {
            Iterator iterator = this.getNotificationListenerIterator(9);
            while (iterator.hasNext()) {
                try {
                    DSIKeyPanelListener dSIKeyPanelListener = (DSIKeyPanelListener)iterator.next();
                    dSIKeyPanelListener.updateRecognizerMode(n, n2, n3);
                }
                catch (Exception exception) {
                    this.traceException(exception);
                }
            }
        }
    }

    public void updateCharacterEvent2(int n, String[] stringArray, int[] nArray, int n2) {
        if ((n2 & 0x80) == 128) {
            n2 ^= 0x80;
            Iterator iterator = this.getUnconfirmedNotificationListenerIterator(21);
            while (iterator.hasNext()) {
                try {
                    DSIKeyPanelListener dSIKeyPanelListener = (DSIKeyPanelListener)iterator.next();
                    this.confirmNotificationListener(21, dSIKeyPanelListener);
                    dSIKeyPanelListener.updateCharacterEvent2(n, stringArray, nArray, n2);
                }
                catch (Exception exception) {
                    this.traceException(exception);
                }
            }
        } else {
            Iterator iterator = this.getNotificationListenerIterator(21);
            while (iterator.hasNext()) {
                try {
                    DSIKeyPanelListener dSIKeyPanelListener = (DSIKeyPanelListener)iterator.next();
                    dSIKeyPanelListener.updateCharacterEvent2(n, stringArray, nArray, n2);
                }
                catch (Exception exception) {
                    this.traceException(exception);
                }
            }
        }
    }

    public void updateGesture2(int n, int n2, int n3, boolean bl, int n4, int n5, int n6, int n7, int n8, int n9) {
        if ((n9 & 0x80) == 128) {
            n9 ^= 0x80;
            Iterator iterator = this.getUnconfirmedNotificationListenerIterator(20);
            while (iterator.hasNext()) {
                try {
                    DSIKeyPanelListener dSIKeyPanelListener = (DSIKeyPanelListener)iterator.next();
                    this.confirmNotificationListener(20, dSIKeyPanelListener);
                    dSIKeyPanelListener.updateGesture2(n, n2, n3, bl, n4, n5, n6, n7, n8, n9);
                }
                catch (Exception exception) {
                    this.traceException(exception);
                }
            }
        } else {
            Iterator iterator = this.getNotificationListenerIterator(20);
            while (iterator.hasNext()) {
                try {
                    DSIKeyPanelListener dSIKeyPanelListener = (DSIKeyPanelListener)iterator.next();
                    dSIKeyPanelListener.updateGesture2(n, n2, n3, bl, n4, n5, n6, n7, n8, n9);
                }
                catch (Exception exception) {
                    this.traceException(exception);
                }
            }
        }
    }

    public void genericSettingResponse(int n, int n2, int n3) {
        Object[] objectArray = this.getResponseListenerList();
        if (objectArray != null) {
            for (int i2 = 0; i2 < objectArray.length; ++i2) {
                try {
                    DSIKeyPanelListener dSIKeyPanelListener = (DSIKeyPanelListener)objectArray[i2];
                    dSIKeyPanelListener.genericSettingResponse(n, n2, n3);
                    continue;
                }
                catch (Exception exception) {
                    this.traceException(exception);
                }
            }
        }
    }

    public void updateProximity(int n, int n2, int n3) {
        if ((n3 & 0x80) == 128) {
            n3 ^= 0x80;
            Iterator iterator = this.getUnconfirmedNotificationListenerIterator(18);
            while (iterator.hasNext()) {
                try {
                    DSIKeyPanelListener dSIKeyPanelListener = (DSIKeyPanelListener)iterator.next();
                    this.confirmNotificationListener(18, dSIKeyPanelListener);
                    dSIKeyPanelListener.updateProximity(n, n2, n3);
                }
                catch (Exception exception) {
                    this.traceException(exception);
                }
            }
        } else {
            Iterator iterator = this.getNotificationListenerIterator(18);
            while (iterator.hasNext()) {
                try {
                    DSIKeyPanelListener dSIKeyPanelListener = (DSIKeyPanelListener)iterator.next();
                    dSIKeyPanelListener.updateProximity(n, n2, n3);
                }
                catch (Exception exception) {
                    this.traceException(exception);
                }
            }
        }
    }

    public void updateAdvancedProximity(int n, int n2, int n3, int n4, int n5, int n6, int n7, int n8, int n9, int n10) {
        if ((n10 & 0x80) == 128) {
            n10 ^= 0x80;
            Iterator iterator = this.getUnconfirmedNotificationListenerIterator(27);
            while (iterator.hasNext()) {
                try {
                    DSIKeyPanelListener dSIKeyPanelListener = (DSIKeyPanelListener)iterator.next();
                    this.confirmNotificationListener(27, dSIKeyPanelListener);
                    dSIKeyPanelListener.updateAdvancedProximity(n, n2, n3, n4, n5, n6, n7, n8, n9, n10);
                }
                catch (Exception exception) {
                    this.traceException(exception);
                }
            }
        } else {
            Iterator iterator = this.getNotificationListenerIterator(27);
            while (iterator.hasNext()) {
                try {
                    DSIKeyPanelListener dSIKeyPanelListener = (DSIKeyPanelListener)iterator.next();
                    dSIKeyPanelListener.updateAdvancedProximity(n, n2, n3, n4, n5, n6, n7, n8, n9, n10);
                }
                catch (Exception exception) {
                    this.traceException(exception);
                }
            }
        }
    }

    public void lastKey(int n, int n2, int n3) {
        Object[] objectArray = this.getResponseListenerList();
        if (objectArray != null) {
            for (int i2 = 0; i2 < objectArray.length; ++i2) {
                try {
                    DSIKeyPanelListener dSIKeyPanelListener = (DSIKeyPanelListener)objectArray[i2];
                    dSIKeyPanelListener.lastKey(n, n2, n3);
                    continue;
                }
                catch (Exception exception) {
                    this.traceException(exception);
                }
            }
        }
    }

    public void updateKeyboardType(int n, int n2) {
        if ((n2 & 0x80) == 128) {
            n2 ^= 0x80;
            Iterator iterator = this.getUnconfirmedNotificationListenerIterator(19);
            while (iterator.hasNext()) {
                try {
                    DSIKeyPanelListener dSIKeyPanelListener = (DSIKeyPanelListener)iterator.next();
                    this.confirmNotificationListener(19, dSIKeyPanelListener);
                    dSIKeyPanelListener.updateKeyboardType(n, n2);
                }
                catch (Exception exception) {
                    this.traceException(exception);
                }
            }
        } else {
            Iterator iterator = this.getNotificationListenerIterator(19);
            while (iterator.hasNext()) {
                try {
                    DSIKeyPanelListener dSIKeyPanelListener = (DSIKeyPanelListener)iterator.next();
                    dSIKeyPanelListener.updateKeyboardType(n, n2);
                }
                catch (Exception exception) {
                    this.traceException(exception);
                }
            }
        }
    }

    public void updateTouchSensitiveArea(int n, int n2, int n3, int n4, int n5, int n6) {
        if ((n6 & 0x80) == 128) {
            n6 ^= 0x80;
            Iterator iterator = this.getUnconfirmedNotificationListenerIterator(24);
            while (iterator.hasNext()) {
                try {
                    DSIKeyPanelListener dSIKeyPanelListener = (DSIKeyPanelListener)iterator.next();
                    this.confirmNotificationListener(24, dSIKeyPanelListener);
                    dSIKeyPanelListener.updateTouchSensitiveArea(n, n2, n3, n4, n5, n6);
                }
                catch (Exception exception) {
                    this.traceException(exception);
                }
            }
        } else {
            Iterator iterator = this.getNotificationListenerIterator(24);
            while (iterator.hasNext()) {
                try {
                    DSIKeyPanelListener dSIKeyPanelListener = (DSIKeyPanelListener)iterator.next();
                    dSIKeyPanelListener.updateTouchSensitiveArea(n, n2, n3, n4, n5, n6);
                }
                catch (Exception exception) {
                    this.traceException(exception);
                }
            }
        }
    }

    public void getVersionInfo(int n, int n2, String string) {
        Object[] objectArray = this.getResponseListenerList();
        if (objectArray != null) {
            for (int i2 = 0; i2 < objectArray.length; ++i2) {
                try {
                    DSIKeyPanelListener dSIKeyPanelListener = (DSIKeyPanelListener)objectArray[i2];
                    dSIKeyPanelListener.getVersionInfo(n, n2, string);
                    continue;
                }
                catch (Exception exception) {
                    this.traceException(exception);
                }
            }
        }
    }

    public void updateInputPanelReady(int n, int n2, int n3) {
        if ((n3 & 0x80) == 128) {
            n3 ^= 0x80;
            Iterator iterator = this.getUnconfirmedNotificationListenerIterator(26);
            while (iterator.hasNext()) {
                try {
                    DSIKeyPanelListener dSIKeyPanelListener = (DSIKeyPanelListener)iterator.next();
                    this.confirmNotificationListener(26, dSIKeyPanelListener);
                    dSIKeyPanelListener.updateInputPanelReady(n, n2, n3);
                }
                catch (Exception exception) {
                    this.traceException(exception);
                }
            }
        } else {
            Iterator iterator = this.getNotificationListenerIterator(26);
            while (iterator.hasNext()) {
                try {
                    DSIKeyPanelListener dSIKeyPanelListener = (DSIKeyPanelListener)iterator.next();
                    dSIKeyPanelListener.updateInputPanelReady(n, n2, n3);
                }
                catch (Exception exception) {
                    this.traceException(exception);
                }
            }
        }
    }

    public void getProperty(int n, int n2, int n3, int n4, byte[] byArray) {
        Object[] objectArray = this.getResponseListenerList();
        if (objectArray != null) {
            for (int i2 = 0; i2 < objectArray.length; ++i2) {
                try {
                    DSIKeyPanelListener dSIKeyPanelListener = (DSIKeyPanelListener)objectArray[i2];
                    dSIKeyPanelListener.getProperty(n, n2, n3, n4, byArray);
                    continue;
                }
                catch (Exception exception) {
                    this.traceException(exception);
                }
            }
        }
    }

    public void asyncException(int n, String string, int n2) {
        Object[] objectArray = this.getResponseListenerList();
        if (objectArray != null) {
            for (int i2 = 0; i2 < objectArray.length; ++i2) {
                try {
                    DSIKeyPanelListener dSIKeyPanelListener = (DSIKeyPanelListener)objectArray[i2];
                    dSIKeyPanelListener.asyncException(n, string, n2);
                    continue;
                }
                catch (Exception exception) {
                    this.traceException(exception);
                }
            }
        }
    }

    public void yyIndication(String string, String string2) {
        Object[] objectArray = this.getResponseListenerList();
        if (objectArray != null) {
            for (int i2 = 0; i2 < objectArray.length; ++i2) {
                try {
                    DSIKeyPanelListener dSIKeyPanelListener = (DSIKeyPanelListener)objectArray[i2];
                    Method method = super.getClass().getMethod(
                        "yyIndication",
                        new Class[]{
                            class$java$lang$String == null ?
                                DSIKeyPanelDispatcher.class$("java.lang.String") :
                                class$java$lang$String,
                            class$java$lang$String == null ?
                                DSIKeyPanelDispatcher.class$("java.lang.String") :
                                class$java$lang$String
                        });
                    method.invoke(dSIKeyPanelListener,
                                  new Object[]{string, string2});
                    continue;
                }
                catch (Exception exception) {
                    this.traceException(exception);
                }
            }
        }
    }

    static /* synthetic */ Class class$(String string) {
        try {
            return Class.forName(string);
        }
        catch (ClassNotFoundException classNotFoundException) {
            throw new NoClassDefFoundError(classNotFoundException.getMessage());
        }
    }
}
