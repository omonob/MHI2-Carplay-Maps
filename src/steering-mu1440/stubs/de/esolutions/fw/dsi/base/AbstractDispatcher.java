package de.esolutions.fw.dsi.base;
import java.util.Iterator;
import org.dsi.ifc.base.DSIListener;
public abstract class AbstractDispatcher {
    protected AbstractDispatcher(int id, String listenerName) {}
    protected Iterator getUnconfirmedNotificationListenerIterator(int id) { return null; }
    protected Iterator getNotificationListenerIterator(int id) { return null; }
    protected void confirmNotificationListener(int id, DSIListener listener) {}
    protected void traceException(Exception exception) {}
    protected Object[] getResponseListenerList() { return null; }
}
