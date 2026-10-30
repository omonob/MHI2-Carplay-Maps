import java.nio.file.*;
import java.util.*;
import java.util.jar.*;
import jdk.internal.org.objectweb.asm.*;
import jdk.internal.org.objectweb.asm.tree.*;
/** Build from the exact original class bytes, never from a newer replacement. */
public final class BuildG11Navigation {
 static final String MODE="de/vw/mib/asl/internal/carplay/common/CarPlayModeHandling";
 static final String NAV="de/vw/mib/asl/internal/navigation/api/impl/ASLNavigationServices";
 static final String PROXY="de/esolutions/fw/comm/dsi/carplay/impl/DSICarplayProxy";
 static final String HELPER="org/omonob/cpmap/compat/G11NavigationRequestFilter";
 static final String MODE_METHOD="handleModeChanges";
 static ClassNode node(byte[] b){ClassNode n=new ClassNode();new ClassReader(b).accept(n,ClassReader.SKIP_DEBUG|ClassReader.SKIP_FRAMES);if(n.version!=46)throw new IllegalStateException("Expected CDC original");return n;}
 static byte[] bytes(ClassNode n){
  // JXE's final Object.getClass opcode converts to invokespecial even when
  // the receiver is not this. A JAR needs the valid equivalent invokevirtual;
  // getClass is final, so virtual dispatch cannot alter its behavior.
  for(MethodNode m:n.methods)for(AbstractInsnNode i:m.instructions)if(i instanceof MethodInsnNode){
   MethodInsnNode call=(MethodInsnNode)i;
   if(call.getOpcode()==Opcodes.INVOKESPECIAL&&call.owner.equals("java/lang/Object")&&call.name.equals("getClass")&&call.desc.equals("()Ljava/lang/Class;"))call.setOpcode(Opcodes.INVOKEVIRTUAL);
  }
  ClassWriter w=new ClassWriter(ClassWriter.COMPUTE_MAXS);n.accept(w);return w.toByteArray();
 }
 static AbstractInsnNode previous(AbstractInsnNode p){p=p.getPrevious();while(p!=null&&p.getOpcode()<0)p=p.getPrevious();return p;}
 static void removeBlock(MethodNode m,AbstractInsnNode last,int count){for(int i=0;i<count;i++){AbstractInsnNode p=previous(last);m.instructions.remove(last);last=p;}}
 static byte[] patchMode(byte[] b){
  ClassNode n=node(b);int active=0,property=0,methods=0;
  for(MethodNode m:n.methods)if(m.name.equals(MODE_METHOD)&&m.desc.equals("([Lorg/dsi/ifc/carplay/Resource;[Lorg/dsi/ifc/carplay/AppState;)V")){
   methods++;
   for(AbstractInsnNode i=m.instructions.getFirst();i!=null;){AbstractInsnNode next=i.getNext();
    if(i instanceof MethodInsnNode){MethodInsnNode call=(MethodInsnNode)i;AbstractInsnNode p=previous(i);
     if(call.name.equals("setActiveNavigation")&&call.owner.equals("de/vw/mib/asl/internal/carplay/common/CarPlayGlobalProperies")&&call.desc.equals("(I)V")&&p.getOpcode()==Opcodes.ICONST_2){
      AbstractInsnNode field=previous(p),self=previous(field);
      if(!(field instanceof FieldInsnNode)||field.getOpcode()!=Opcodes.GETFIELD||!(self instanceof VarInsnNode)||self.getOpcode()!=Opcodes.ALOAD||((VarInsnNode)self).var!=0)throw new IllegalStateException("Unknown active-navigation assignment");
      removeBlock(m,i,4);active++;
     }else if(call.name.equals("valueChangedBoolean")&&call.owner.equals("de/vw/mib/asl/ASLPropertyManager")&&call.desc.equals("(IZ)V")&&p.getOpcode()==Opcodes.ICONST_1){
      AbstractInsnNode id=previous(p),manager=previous(id);
      if(!(id instanceof LdcInsnNode)||!(((LdcInsnNode)id).cst instanceof Integer)||!(manager instanceof FieldInsnNode)||manager.getOpcode()!=Opcodes.GETSTATIC||!((FieldInsnNode)manager).name.equals("aslPropertyManager"))throw new IllegalStateException("Unknown navigation property assignment");
      removeBlock(m,i,4);property++;
     }
    }
    i=next;
   }
  }
  if(methods!=1||active!=1||property!=1)throw new IllegalStateException("Navigation-only assignment count mismatch "+methods+"/"+active+"/"+property);
  return bytes(n);
 }
 static byte[] guardNavigation(byte[] b){
  ClassNode n=node(b);int count=0;
  for(MethodNode m:n.methods)if(m.name.equals("stopGuidance")&&m.desc.equals("(I)I")){
   LabelNode proceed=new LabelNode();InsnList prefix=new InsnList();prefix.add(new VarInsnNode(Opcodes.ILOAD,1));prefix.add(new IntInsnNode(Opcodes.BIPUSH,6));prefix.add(new JumpInsnNode(Opcodes.IF_ICMPNE,proceed));prefix.add(new InsnNode(Opcodes.ICONST_0));prefix.add(new InsnNode(Opcodes.IRETURN));prefix.add(proceed);m.instructions.insert(prefix);count++;
  }
  if(count!=1)throw new IllegalStateException("Exact stopGuidance ABI missing");return bytes(n);
 }
 static byte[] guardProxy(byte[] b){
  ClassNode n=node(b);int count=0;
  for(MethodNode m:n.methods)if(m.name.equals("requestModeChange")&&m.desc.equals("([Lorg/dsi/ifc/carplay/ResourceRequest;[Lorg/dsi/ifc/carplay/AppStateRequest;Ljava/lang/String;)V")){
   LabelNode proceed=new LabelNode();InsnList prefix=new InsnList();prefix.add(new VarInsnNode(Opcodes.ALOAD,2));prefix.add(new MethodInsnNode(Opcodes.INVOKESTATIC,HELPER,"suppress","([Lorg/dsi/ifc/carplay/AppStateRequest;)Z",false));prefix.add(new JumpInsnNode(Opcodes.IFEQ,proceed));prefix.add(new InsnNode(Opcodes.RETURN));prefix.add(proceed);m.instructions.insert(prefix);count++;
  }
  if(count!=1)throw new IllegalStateException("Exact mode request ABI missing");return bytes(n);
 }
 static byte[] legacy(byte[] b){ClassNode n=new ClassNode();new ClassReader(b).accept(n,ClassReader.SKIP_FRAMES|ClassReader.SKIP_DEBUG);n.version=46;return bytes(n);}
 static void add(JarOutputStream out,String name,byte[] b)throws Exception{JarEntry e=new JarEntry(name+".class");e.setTime(0);out.putNextEntry(e);out.write(b);out.closeEntry();}
 public static void main(String[] a)throws Exception{
  try(JarFile stock=new JarFile(a[0]);JarOutputStream out=new JarOutputStream(Files.newOutputStream(Path.of(a[2])))){
   add(out,MODE,patchMode(stock.getInputStream(stock.getJarEntry(MODE+".class")).readAllBytes()));
   add(out,NAV,guardNavigation(stock.getInputStream(stock.getJarEntry(NAV+".class")).readAllBytes()));
   add(out,PROXY,guardProxy(stock.getInputStream(stock.getJarEntry(PROXY+".class")).readAllBytes()));
   add(out,HELPER,legacy(Files.readAllBytes(Path.of(a[1],HELPER+".class"))));
  }
  System.out.println("EXACT_G11_NAVIGATION_PATCH_BUILT classes=3 helper=1 original_ABI_retained=1");
 }
}
