import java.io.File;
import java.net.*;
import org.omonob.cpmap.ui.*;
import de.vw.mib.carplay.clustercontrol.Mhi2ClusterControlBridge;
public final class ActualControlHookTest{
 static int checks;
 static String receive(DatagramSocket socket)throws Exception{byte[] data=new byte[64];DatagramPacket p=new DatagramPacket(data,data.length);socket.receive(p);return new String(p.getData(),0,p.getLength(),"US-ASCII");}
 static void press(int key)throws Exception{Mhi2ClusterControlBridge.onKey2(4,key,1,0,1);Thread.sleep(40);Mhi2ClusterControlBridge.onKey2(4,key,0,0,1);}
 static void check(boolean b){checks++;if(!b)throw new AssertionError("actual steering routing "+checks);}
 public static void main(String[] args)throws Exception{
  System.setProperty("cpmap.ui.stateDir",args[0]);ProjectionSettings s=new ProjectionSettings(new File(args[0]));
  DatagramSocket map=new DatagramSocket(7032,InetAddress.getByName("127.0.0.1"));
  DatagramSocket canvas=new DatagramSocket(7033,InetAddress.getByName("127.0.0.1"));
  map.setSoTimeout(1500);canvas.setSoTimeout(1500);
  try{
   s.setCarplayMap(true);ProjectionRuntimePolicy.refreshNow();press(38);check("zoom-in".equals(receive(map)));
   s.setCarplayMap(false);s.setCarplayMirror(true);ProjectionRuntimePolicy.refreshNow();
   press(38);check("canvas-zoom-in".equals(receive(canvas)));
   press(39);check("canvas-zoom-out".equals(receive(canvas)));
   map.setSoTimeout(200);boolean leaked=false;try{receive(map);leaked=true;}catch(SocketTimeoutException expected){}check(!leaked);
   s.setCarplayMirror(false);s.setCarplayMap(true);ProjectionRuntimePolicy.refreshNow();press(39);check("zoom-out".equals(receive(map)));
  }finally{map.close();canvas.close();}
  System.out.println("ACTUAL_GOLDEN_KEYSTATE_ROUTING_PASS checks="+checks+" real_patched_class=1 mirror_phone_map_leak=0");
 }
}
