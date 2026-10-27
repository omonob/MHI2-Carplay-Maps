import java.io.PrintWriter;
import java.io.StringWriter;
import java.nio.file.Path;
import java.util.jar.JarFile;
import jdk.internal.org.objectweb.asm.*;
import jdk.internal.org.objectweb.asm.tree.*;
import jdk.internal.org.objectweb.asm.util.TraceClassVisitor;

/** Offline proof: remove only the two observer insertions and every original
 * class member and instruction must match the exact supplied firmware. */
public final class TestNavigationOwnershipHook {
 static final String NAME="de/vw/mib/asl/internal/carplay/target/DSIHandler.class";
 static final String HELPER="org/omonob/cpmap/ui/CarPlayNavigationOwnership";
 static ClassNode read(String jarPath)throws Exception{
  try(JarFile jar=new JarFile(Path.of(jarPath).toFile())){
   ClassNode node=new ClassNode();
   new ClassReader(jar.getInputStream(jar.getJarEntry(NAME)).readAllBytes()).accept(node,ClassReader.SKIP_DEBUG|ClassReader.SKIP_FRAMES);
   return node;
  }
 }
 static String trace(ClassNode node){
  StringWriter text=new StringWriter();node.accept(new TraceClassVisitor(new PrintWriter(text)));return text.toString();
 }
 public static void main(String[] args)throws Exception{
  ClassNode stock=read(args[0]),patched=read(args[1]);int mode=0,device=0;
  if(stock.version!=46||patched.version!=stock.version||patched.methods.size()!=stock.methods.size())throw new AssertionError("Class ABI changed");
  for(int i=0;i<patched.methods.size();i++){
   MethodNode method=patched.methods.get(i),original=stock.methods.get(i);boolean changed=false;
   for(AbstractInsnNode insn=method.instructions.getFirst();insn!=null;){
    AbstractInsnNode next=insn.getNext();
    if(insn instanceof MethodInsnNode){
     MethodInsnNode call=(MethodInsnNode)insn;
     if(HELPER.equals(call.owner)){
      if(call.getOpcode()!=Opcodes.INVOKESTATIC||next==null||next.getOpcode()!=Opcodes.RETURN)throw new AssertionError("Observer not at normal return");
      if("observe".equals(call.name)&&"([Lorg/dsi/ifc/carplay/AppState;)V".equals(call.desc)&&"handledsiCarPlayupdateMode".equals(method.name)){
       AbstractInsnNode previous=call.getPrevious();
       if(!(previous instanceof VarInsnNode)||previous.getOpcode()!=Opcodes.ALOAD||((VarInsnNode)previous).var!=2)throw new AssertionError("Wrong event argument");
       method.instructions.remove(previous);mode++;
      }else if("deviceChanged".equals(call.name)&&"()V".equals(call.desc)&&"handledsiCarPlayUpdateDeviceInfo".equals(method.name)){device++;}
      else throw new AssertionError("Unexpected observer call");
      method.instructions.remove(call);changed=true;
     }
    }
    insn=next;
   }
   if(changed){method.maxStack=original.maxStack;method.maxLocals=original.maxLocals;}
  }
  if(mode<1||device<1||!trace(stock).equals(trace(patched)))throw new AssertionError("Original CarPlay behavior changed");
  System.out.println("NAVIGATION_OWNERSHIP_HOOK_SCOPE_PASS original_members_and_instructions_unchanged=1 mode_returns="+mode+" device_returns="+device+" class_major=46");
 }
}
