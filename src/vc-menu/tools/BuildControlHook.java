import java.nio.file.*;
import java.util.jar.*;
import java.util.*;
import jdk.internal.org.objectweb.asm.*;
/** Preserve the exact bench key-state/dispatcher code; route only final commands. */
public final class BuildControlHook{
 static int commands,ports;
 static byte[] patch(byte[] bytes){
  if(new String(bytes,java.nio.charset.StandardCharsets.ISO_8859_1).contains("org/omonob/cpmap/ui/ClusterCommandRouter"))throw new IllegalStateException("Refusing to route an already patched control class twice");
  ClassWriter writer=new ClassWriter(ClassWriter.COMPUTE_MAXS);
  new ClassReader(bytes).accept(new ClassVisitor(Opcodes.ASM8,writer){
   public MethodVisitor visitMethod(int a,String n,String d,String s,String[] e){
    MethodVisitor next=super.visitMethod(a,n,d,s,e);
    return new MethodVisitor(Opcodes.ASM8,next){
     public void visitIntInsn(int opcode,int value){
      if(n.equals("commandLoop")&&opcode==Opcodes.SIPUSH&&value==7032){
       ports++;super.visitMethodInsn(Opcodes.INVOKESTATIC,"org/omonob/cpmap/ui/ClusterCommandRouter","port","()I",false);
      }else super.visitIntInsn(opcode,value);
     }
     public void visitInsn(int opcode){
      if(n.equals("commandFor")&&d.equals("(I)Ljava/lang/String;")&&opcode==Opcodes.ARETURN){
       commands++;super.visitMethodInsn(Opcodes.INVOKESTATIC,"org/omonob/cpmap/ui/ClusterCommandRouter","command","(Ljava/lang/String;)Ljava/lang/String;",false);
      }
      super.visitInsn(opcode);
     }
    };
   }
  },ClassReader.SKIP_FRAMES);return writer.toByteArray();
 }
 public static void main(String[] args)throws Exception{
  Path output=Path.of(args[1]);Path stage=Files.createTempFile(output.getParent(),"controls-",".tmp");
  int changed=0;
  try(JarFile input=new JarFile(args[0]);JarOutputStream out=new JarOutputStream(Files.newOutputStream(stage))){
   var entries=input.entries();while(entries.hasMoreElements()){
    var entry=entries.nextElement();byte[] bytes=input.getInputStream(entry).readAllBytes();
    if(entry.getName().equals("de/vw/mib/carplay/clustercontrol/Mhi2ClusterControlBridge.class")){bytes=patch(bytes);changed++;}
    var copy=new JarEntry(entry.getName());copy.setTime(0);out.putNextEntry(copy);out.write(bytes);out.closeEntry();
   }
   // Keep a partially completed overlay bootable even if power is lost before
   // lsd.sh switches to the menu prepend: controls carry their own read-only
   // policy dependencies. The completed chain uses the identical menu copies.
   if(args.length==3)try(JarFile menu=new JarFile(args[2])){
    var list=menu.entries();while(list.hasMoreElements()){
     var e=list.nextElement();String n=e.getName();
     if(!(n.startsWith("org/omonob/cpmap/ui/ClusterCommandRouter") || n.startsWith("org/omonob/cpmap/ui/ProjectionRuntimePolicy") || n.startsWith("org/omonob/cpmap/ui/ProjectionSettings") || n.startsWith("org/omonob/cpmap/ui/MapSwitchSettings")))continue;
     var copy=new JarEntry(n);copy.setTime(0);out.putNextEntry(copy);out.write(menu.getInputStream(e).readAllBytes());out.closeEntry();
    }
   }
  }
  if(changed!=1||commands!=4||ports!=1)throw new IllegalStateException("Exact controls ABI mismatch "+changed+"/"+commands+"/"+ports);
  Files.move(stage,output,StandardCopyOption.ATOMIC_MOVE,StandardCopyOption.REPLACE_EXISTING);
  System.out.println("CONTROL_ROUTING_HOOK_PASS changed=1 key_state_untouched=1 dispatcher_untouched=1");
 }
}
