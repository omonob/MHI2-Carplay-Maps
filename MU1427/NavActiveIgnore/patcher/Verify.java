import org.objectweb.asm.*; import org.objectweb.asm.tree.*; import org.objectweb.asm.tree.analysis.*;
import java.net.*; import java.nio.file.*; import java.io.*;
public class Verify {
  public static void main(String[] a) throws Exception {
    URLClassLoader cl = new URLClassLoader(new URL[]{Paths.get(a[0]).toUri().toURL(), Paths.get(a[1]).toUri().toURL()}, null);
    int bad=0;
    for (int i=2;i<a.length;i++){
      ClassNode cn=new ClassNode(); new ClassReader(Files.readAllBytes(Paths.get(a[i]))).accept(cn,0);
      for (MethodNode m: cn.methods){ if (m.instructions.size()==0) continue;
        SimpleVerifier v=new SimpleVerifier(Type.getObjectType(cn.name), cn.superName==null?null:Type.getObjectType(cn.superName), cn.interfaces.stream().map(Type::getObjectType).toList(), (cn.access&Opcodes.ACC_INTERFACE)!=0);
        v.setClassLoader(cl);
        try { new Analyzer<>(v).analyze(cn.name,m); } catch(Exception e){ bad++; System.out.println("FAIL "+cn.name+"."+m.name+m.desc+": "+e.getMessage()); }
      }
      System.out.println("checked "+cn.name+" v"+cn.version);
    }
    System.exit(bad==0?0:1);
  }
}
