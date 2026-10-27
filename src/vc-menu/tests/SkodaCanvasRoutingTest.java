import java.io.File;
import java.net.*;
import org.omonob.cpmap.ui.*;
import de.vw.mib.carplay.clustercontrol.Mhi2ClusterControlBridge;
public final class SkodaCanvasRoutingTest{
 static String packet(DatagramSocket s)throws Exception{byte[] b=new byte[64];DatagramPacket p=new DatagramPacket(b,b.length);s.receive(p);return new String(b,0,p.getLength(),"US-ASCII");}
 static void expect(DatagramSocket s,String wanted)throws Exception{if(!wanted.equals(packet(s)))throw new AssertionError("Skoda roller direction or port changed");}
 public static void main(String[] args)throws Exception{
  System.setProperty("cpmap.ui.stateDir",args[0]);ProjectionSettings store=new ProjectionSettings(new File(args[0]));
  DatagramSocket map=new DatagramSocket(7032,InetAddress.getByName("127.0.0.1")),canvas=new DatagramSocket(7033,InetAddress.getByName("127.0.0.1"));
  map.setSoTimeout(1500);canvas.setSoTimeout(1500);
  try{
   store.setCarplayMap(true);ProjectionRuntimePolicy.refreshNow();
   Mhi2ClusterControlBridge.onEncoder2(4,40,1,0,1);expect(map,"zoom-in");
   Mhi2ClusterControlBridge.onEncoder2(4,40,-1,0,1);expect(map,"zoom-out");
   store.setCarplayMap(false);store.setCarplayMirror(true);ProjectionRuntimePolicy.refreshNow();
   Mhi2ClusterControlBridge.onEncoder2(4,40,1,0,1);expect(canvas,"canvas-zoom-in");
   Mhi2ClusterControlBridge.onEncoder2(4,40,-1,0,1);expect(canvas,"canvas-zoom-out");
   map.setSoTimeout(200);try{packet(map);throw new AssertionError("Mirror leaked to phone map");}catch(SocketTimeoutException expected){}
  }finally{map.close();canvas.close();}
  System.out.println("MU1440_REAL_ROLLER_CANVAS_ROUTING_PASS up=in down=out map_leak=0");
 }
}
