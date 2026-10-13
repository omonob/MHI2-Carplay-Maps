package de.vw.mib.carplay.clustercontrol;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;

/*
 * Receives stock DSIKeyPanel states without consuming them and forwards only
 * four compact commands to the V7 native preload on 127.0.0.1.  Network I/O
 * is isolated on one daemon thread so the DSI callback can return immediately.
 */
public final class Mhi2ClusterControlBridge {
    private static final int KEYBOARD_MFW = 4;
    private static final int SKODA_ROLLER = 40;
    private static final int ENCODER_IDLE_PARAMETER = 0;
    private static final int ENCODER_VALID = 1;
    private static final int LOOPBACK_PORT = 7032;
    private static final int QUEUE_SIZE = 16;
    private static final Object LOCK = new Object();
    private static final Mhi2ClusterKeyState STATE =
        new Mhi2ClusterKeyState();
    private static final int[] QUEUE = new int[QUEUE_SIZE];

    private static int queueHead;
    private static int queueTail;
    private static int queueCount;
    private static boolean workerStarted;

    private Mhi2ClusterControlBridge() {
    }

    public static void onKey2(int keyboard, int key, int state,
                              int param3, int valid) {
        int action;
        if (keyboard != KEYBOARD_MFW) return;
        if (key != Mhi2ClusterKeyState.KEY_UP &&
            key != Mhi2ClusterKeyState.KEY_DOWN) return;

        synchronized (LOCK) {
            startWorkerLocked();
            action = STATE.onKey(key, state, System.currentTimeMillis());
            if (action != Mhi2ClusterKeyState.ACTION_NONE) {
                enqueueLocked(action);
            }
            LOCK.notifyAll();
        }
    }

    /*
     * MU1102 live capture (2026-10-04): the Skoda steering-wheel roller is
     * DSI keyboard 4 / encoder 40.  One detent is +1 or -1; the fourth
     * parameter is 0 and the validity byte is 1.  Keep the match exact so
     * unrelated rotary controls cannot reach the CarPlay Maps command port.
     */
    public static void onEncoder2(int keyboard, int encoder, int steps,
                                  int param3, int valid) {
        int action;
        if (keyboard != KEYBOARD_MFW || encoder != SKODA_ROLLER) return;
        if (param3 != ENCODER_IDLE_PARAMETER) return;
        if ((valid & 0x7F) != ENCODER_VALID) return;
        if (steps == 1) action = Mhi2ClusterKeyState.ACTION_ZOOM_IN;
        else if (steps == -1) action = Mhi2ClusterKeyState.ACTION_ZOOM_OUT;
        else return;

        synchronized (LOCK) {
            startWorkerLocked();
            enqueueLocked(action);
            LOCK.notifyAll();
        }
    }

    private static void startWorkerLocked() {
        Thread thread;
        if (workerStarted) return;
        thread = new Thread(new Runnable() {
            public void run() {
                commandLoop();
            }
        }, "cpvc-cluster-control");
        thread.setDaemon(true);
        workerStarted = true;
        thread.start();
    }

    private static void enqueueLocked(int action) {
        if (queueCount == QUEUE_SIZE) {
            queueHead = (queueHead + 1) % QUEUE_SIZE;
            --queueCount;
        }
        QUEUE[queueTail] = action;
        queueTail = (queueTail + 1) % QUEUE_SIZE;
        ++queueCount;
    }

    private static int dequeueLocked() {
        int action = QUEUE[queueHead];
        queueHead = (queueHead + 1) % QUEUE_SIZE;
        --queueCount;
        return action;
    }

    private static void commandLoop() {
        DatagramSocket socket = null;
        InetAddress loopback = null;
        for (;;) {
            int action;
            synchronized (LOCK) {
                for (;;) {
                    action = STATE.onTimer(System.currentTimeMillis());
                    if (action != Mhi2ClusterKeyState.ACTION_NONE) {
                        enqueueLocked(action);
                    }
                    if (queueCount > 0) {
                        action = dequeueLocked();
                        break;
                    }
                    try {
                        long waitMillis = STATE.millisUntilLongAction(
                            System.currentTimeMillis());
                        if (waitMillis > 0L) LOCK.wait(waitMillis);
                        else LOCK.wait();
                    }
                    catch (InterruptedException ignored) {
                    }
                }
            }

            try {
                byte[] bytes = commandFor(action).getBytes("US-ASCII");
                if (socket == null) socket = new DatagramSocket();
                if (loopback == null) loopback = InetAddress.getByName(
                    "127.0.0.1");
                socket.send(new DatagramPacket(bytes, bytes.length,
                                               loopback, LOOPBACK_PORT));
            }
            catch (Throwable ignored) {
                if (socket != null) socket.close();
                socket = null;
                loopback = null;
            }
        }
    }

    private static String commandFor(int action) {
        if (action == Mhi2ClusterKeyState.ACTION_ZOOM_IN) return "zoom-in";
        if (action == Mhi2ClusterKeyState.ACTION_ZOOM_OUT) return "zoom-out";
        if (action == Mhi2ClusterKeyState.ACTION_MAP_PREVIOUS) {
            return "map-previous";
        }
        return "map-next";
    }
}
