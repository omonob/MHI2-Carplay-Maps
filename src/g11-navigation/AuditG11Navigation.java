import java.nio.file.*;
import java.util.*;
import java.util.jar.*;
import jdk.internal.org.objectweb.asm.*;
import jdk.internal.org.objectweb.asm.tree.*;

/** Independent scope audit: compare every original member and instruction. */
public final class AuditG11Navigation {
 static ClassNode read(byte[] b) { ClassNode n=new ClassNode(); new ClassReader(b).accept(n,ClassReader.SKIP_DEBUG|ClassReader.SKIP_FRAMES); return n; }
 static void require(boolean b,String message) { if(!b)throw new AssertionError(message); }
 static List<AbstractInsnNode> ops(MethodNode m) { List<AbstractInsnNode> r=new ArrayList<>();for(AbstractInsnNode i:m.instructions)if(i.getOpcode()>=0)r.add(i);return r; }
 static String label(Map<LabelNode,Integer> ids,LabelNode n) { if(!ids.containsKey(n))ids.put(n,ids.size());return "L"+ids.get(n); }
 static List<String> fingerprint(MethodNode m) {
  Map<LabelNode,Integer> ids=new IdentityHashMap<>();List<String> r=new ArrayList<>();
  for(AbstractInsnNode i:ops(m)) {
   int op=i.getOpcode();String s="";
   if(i instanceof MethodInsnNode) { MethodInsnNode c=(MethodInsnNode)i;
    if(op==Opcodes.INVOKESPECIAL&&c.owner.equals("java/lang/Object")&&c.name.equals("getClass")&&c.desc.equals("()Ljava/lang/Class;"))op=Opcodes.INVOKEVIRTUAL;
    s=c.owner+"."+c.name+c.desc+"/"+c.itf;
   } else if(i instanceof FieldInsnNode){FieldInsnNode f=(FieldInsnNode)i;s=f.owner+"."+f.name+f.desc;}
   else if(i instanceof VarInsnNode)s=""+((VarInsnNode)i).var;
   else if(i instanceof IntInsnNode)s=""+((IntInsnNode)i).operand;
   else if(i instanceof TypeInsnNode)s=((TypeInsnNode)i).desc;
   else if(i instanceof LdcInsnNode)s=String.valueOf(((LdcInsnNode)i).cst);
   else if(i instanceof JumpInsnNode)s=label(ids,((JumpInsnNode)i).label);
   else if(i instanceof IincInsnNode){IincInsnNode x=(IincInsnNode)i;s=x.var+":"+x.incr;}
   else if(i instanceof TableSwitchInsnNode){TableSwitchInsnNode x=(TableSwitchInsnNode)i;s=x.min+":"+x.max+":"+label(ids,x.dflt);for(LabelNode l:x.labels)s+=":"+label(ids,l);}
   else if(i instanceof LookupSwitchInsnNode){LookupSwitchInsnNode x=(LookupSwitchInsnNode)i;s=x.keys+":"+label(ids,x.dflt);for(LabelNode l:x.labels)s+=":"+label(ids,l);}
   else if(i instanceof MultiANewArrayInsnNode){MultiANewArrayInsnNode x=(MultiANewArrayInsnNode)i;s=x.desc+":"+x.dims;}
   r.add(op+":"+s);
  }
  for(TryCatchBlockNode t:m.tryCatchBlocks)r.add("catch:"+label(ids,t.start)+":"+label(ids,t.end)+":"+label(ids,t.handler)+":"+t.type);
  int metadataStart=r.size();
  // Include target locations, rather than just jump order.
  for(Map.Entry<LabelNode,Integer> e:ids.entrySet()) {int index=0;for(AbstractInsnNode i:m.instructions){if(i==e.getKey())break;if(i.getOpcode()>=0)index++;}r.add("label:"+e.getValue()+":"+index);}
  Collections.sort(r.subList(metadataStart,r.size()));
  return r;
 }
 static void remove(MethodNode m,int start,int count) {List<AbstractInsnNode> o=ops(m);for(int i=start;i<start+count;i++)m.instructions.remove(o.get(i));}
 static int stripModeAssignments(MethodNode m) {
  int removed=0;
  for(;;){List<AbstractInsnNode> o=ops(m);boolean found=false;
   for(int i=3;i<o.size();i++)if(o.get(i) instanceof MethodInsnNode){MethodInsnNode c=(MethodInsnNode)o.get(i);
    boolean active=c.owner.equals("de/vw/mib/asl/internal/carplay/common/CarPlayGlobalProperies")&&c.name.equals("setActiveNavigation")&&c.desc.equals("(I)V")&&o.get(i-1).getOpcode()==Opcodes.ICONST_2;
    boolean property=c.owner.equals("de/vw/mib/asl/ASLPropertyManager")&&c.name.equals("valueChangedBoolean")&&c.desc.equals("(IZ)V")&&o.get(i-1).getOpcode()==Opcodes.ICONST_1;
    if(active||property){remove(m,i-3,4);removed++;found=true;break;}
   }
   if(!found)return removed;
  }
 }
 static void stripPrefix(MethodNode m,boolean navigation) {
  List<AbstractInsnNode> o=ops(m);int count=navigation?5:4;
  if(navigation){
   require(o.get(0).getOpcode()==Opcodes.ILOAD&&((VarInsnNode)o.get(0)).var==1,"nav argument");
   require(o.get(1).getOpcode()==Opcodes.BIPUSH&&((IntInsnNode)o.get(1)).operand==6,"nav cause");
   require(o.get(2).getOpcode()==Opcodes.IF_ICMPNE&&o.get(3).getOpcode()==Opcodes.ICONST_0&&o.get(4).getOpcode()==Opcodes.IRETURN,"nav guard");
  } else {
   require(o.get(0).getOpcode()==Opcodes.ALOAD&&((VarInsnNode)o.get(0)).var==2,"proxy argument");
   MethodInsnNode c=(MethodInsnNode)o.get(1);
   require(c.getOpcode()==Opcodes.INVOKESTATIC&&c.owner.equals("org/omonob/cpmap/compat/G11NavigationRequestFilter")&&c.name.equals("suppress")&&c.desc.equals("([Lorg/dsi/ifc/carplay/AppStateRequest;)Z"),"exact helper");
   require(o.get(2).getOpcode()==Opcodes.IFEQ&&o.get(3).getOpcode()==Opcodes.RETURN,"proxy guard");
  }
  AbstractInsnNode destination=((JumpInsnNode)o.get(2)).label.getNext();while(destination!=null&&destination.getOpcode()<0)destination=destination.getNext();
  require(destination==o.get(count),"guard must resume at exact stock body");remove(m,0,count);
 }
 static String signature(MethodNode m){return m.name+m.desc+"/"+m.access+"/"+m.exceptions;}
 static List<String> fields(ClassNode n){List<String> r=new ArrayList<>();for(FieldNode f:n.fields)r.add(f.name+f.desc+"/"+f.access+"/"+f.value);return r;}
 public static void main(String[] a)throws Exception {
  int unchanged=0,modified=0;
  try(JarFile stock=new JarFile(a[0]);JarFile patch=new JarFile(a[1])) {
   List<String> names=Collections.list(patch.entries()).stream().map(JarEntry::getName).filter(x->x.endsWith(".class")).toList();require(names.size()==4,"only three original classes and one helper");
   for(String name:names){if(name.startsWith("org/omonob/"))continue;
    ClassNode before=read(stock.getInputStream(stock.getJarEntry(name)).readAllBytes()),after=read(patch.getInputStream(patch.getJarEntry(name)).readAllBytes());
    require(before.name.equals(after.name)&&before.superName.equals(after.superName)&&before.access==after.access&&before.interfaces.equals(after.interfaces)&&before.version==after.version,"class metadata "+name);
    require(fields(before).equals(fields(after)),"all fields retained "+name);require(before.methods.size()==after.methods.size(),"all methods retained "+name);
    for(int i=0;i<before.methods.size();i++){MethodNode b=before.methods.get(i),p=after.methods.get(i);require(signature(b).equals(signature(p)),"exact member ABI "+name);
     boolean changed=false;
     if(name.endsWith("/CarPlayModeHandling.class")&&b.name.equals("handleModeChanges")){require(stripModeAssignments(b)==2,"two navigation-only assignments");changed=true;}
     if(name.endsWith("/ASLNavigationServices.class")&&b.name.equals("stopGuidance")){stripPrefix(p,true);changed=true;}
     if(name.endsWith("/DSICarplayProxy.class")&&b.name.equals("requestModeChange")){stripPrefix(p,false);changed=true;}
     require(fingerprint(b).equals(fingerprint(p)),"unexpected instruction change "+name+" "+b.name+b.desc);
     if(changed)modified++;else unchanged++;
    }
   }
  }
  require(modified==3,"exactly three navigation methods");
  System.out.println("G11_NAV_SCOPE_AUDIT_PASS modified_navigation_methods="+modified+" unchanged_methods="+unchanged+" original_fields_and_ABI=retained getClass_final_opcode=normalized");
 }
}
