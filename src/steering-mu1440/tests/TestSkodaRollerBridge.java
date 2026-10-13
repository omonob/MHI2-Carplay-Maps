import de.vw.mib.carplay.clustercontrol.Mhi2ClusterControlBridge;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.SocketTimeoutException;

public final class TestSkodaRollerBridge {
    private static final int PORT = 7032;

    public static void main(String[] args) throws Exception {
        DatagramSocket socket = new DatagramSocket(PORT);
        socket.setSoTimeout(300);
        try {
            Mhi2ClusterControlBridge.onEncoder2(4, 39, 1, 0, 1);
            assertNoPacket(socket, "wrong encoder id");

            Mhi2ClusterControlBridge.onEncoder2(4, 40, 2, 0, 1);
            assertNoPacket(socket, "unverified step magnitude");

            Mhi2ClusterControlBridge.onEncoder2(4, 40, 1, 0, 1);
            assertPacket(socket, "zoom-in");

            Mhi2ClusterControlBridge.onEncoder2(4, 40, -1, 0, 1);
            assertPacket(socket, "zoom-out");

            Mhi2ClusterControlBridge.onEncoder2(4, 40, 1, 1, 1);
            assertNoPacket(socket, "unexpected fourth parameter");

            Mhi2ClusterControlBridge.onEncoder2(4, 40, 1, 0, 0);
            assertNoPacket(socket, "invalid event");
        }
        finally {
            socket.close();
        }
        System.out.println("SKODA_ROLLER_BRIDGE_TEST_OK");
    }

    private static void assertPacket(DatagramSocket socket, String expected)
        throws Exception {
        byte[] bytes = new byte[64];
        DatagramPacket packet = new DatagramPacket(bytes, bytes.length);
        socket.setSoTimeout(1200);
        socket.receive(packet);
        String actual = new String(packet.getData(), packet.getOffset(),
                                   packet.getLength(), "US-ASCII");
        if (!expected.equals(actual)) {
            throw new AssertionError("expected " + expected + ", got " + actual);
        }
    }

    private static void assertNoPacket(DatagramSocket socket, String label)
        throws Exception {
        byte[] bytes = new byte[64];
        DatagramPacket packet = new DatagramPacket(bytes, bytes.length);
        socket.setSoTimeout(300);
        try {
            socket.receive(packet);
            throw new AssertionError(label + " unexpectedly emitted a command");
        }
        catch (SocketTimeoutException expected) {
        }
    }
}
