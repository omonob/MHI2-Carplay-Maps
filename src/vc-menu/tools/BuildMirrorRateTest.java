import java.nio.file.*;
import java.util.jar.*;
import jdk.internal.org.objectweb.asm.*;

/** Patch one constant-producing callsite in the exact original firmware class. */
public final class BuildMirrorRateTest {
    private static final String TARGET="de/vw/mib/asl/internal/mostkombi/streamsink/usecases/ChangeDataRateSequence";
    private static final String HELPER="org/omonob/cpmap/ui/MirrorOutputRateTest";
    private static int hooks;
    static byte[] patched(byte[] bytes){
        ClassWriter out=new ClassWriter(ClassWriter.COMPUTE_MAXS);
        new ClassReader(bytes).accept(new ClassVisitor(Opcodes.ASM8,out){
            public void visit(int v,int a,String n,String s,String p,String[] i){
                if(!TARGET.equals(n))throw new IllegalArgumentException("Wrong stock class");
                super.visit(46,a,n,s,p,i);
            }
            public MethodVisitor visitMethod(int a,String n,String d,String s,String[] e){
                MethodVisitor next=super.visitMethod(a,n,d,s,e);
                if(!n.equals("_stepChangeDataRate") || !d.equals("()V"))return next;
                return new MethodVisitor(Opcodes.ASM8,next){
                    public void visitIntInsn(int op,int value){
                        super.visitIntInsn(op,value);
                        if(op==Opcodes.BIPUSH && value==10){
                            super.visitMethodInsn(Opcodes.INVOKESTATIC,HELPER,"choose","(I)I",false);
                            hooks++;
                        }
                    }
                };
            }
        },ClassReader.SKIP_FRAMES);
        if(hooks!=1)throw new IllegalStateException("Expected one original 10-FPS callsite, got "+hooks);
        return out.toByteArray();
    }
    static byte[] legacy(byte[] bytes){
        ClassWriter out=new ClassWriter(0);
        new ClassReader(bytes).accept(new ClassVisitor(Opcodes.ASM8,out){
            public void visit(int v,int a,String n,String s,String p,String[] i){super.visit(46,a,n,null,p,i);}
        },ClassReader.SKIP_FRAMES);
        byte[] result=out.toByteArray();
        String raw=new String(result,java.nio.charset.StandardCharsets.ISO_8859_1);
        for(String token:new String[]{"java/lang/StringBuilder","java/nio/","java/util/concurrent","java/lang/invoke/"})
            if(raw.contains(token))throw new IllegalStateException("Post-CDC helper reference: "+token);
        return result;
    }
    static void entry(JarOutputStream jar,String name,byte[] bytes)throws Exception{
        JarEntry e=new JarEntry(name);e.setTime(0);jar.putNextEntry(e);jar.write(bytes);jar.closeEntry();
    }
    public static void main(String[] args)throws Exception{
        try(JarFile original=new JarFile(args[0]);JarOutputStream jar=new JarOutputStream(Files.newOutputStream(Path.of(args[2])))){
            byte[] stock=original.getInputStream(original.getJarEntry(TARGET+".class")).readAllBytes();
            Files.write(Path.of(args[3]),stock);
            entry(jar,TARGET+".class",patched(stock));
            entry(jar,HELPER+".class",legacy(Files.readAllBytes(Path.of(args[1],HELPER+".class"))));
        }
        if(args.length>4)try(JarOutputStream smoke=new JarOutputStream(Files.newOutputStream(Path.of(args[4])))){
            String main="org/omonob/cpmap/ui/MirrorOutputRateSmoke";
            entry(smoke,main+".class",legacy(Files.readAllBytes(Path.of(args[1],main+".class"))));
        }
        System.out.println("MIRROR_RATE_JAR_BUILT stock_classes_changed=1 hooks="+hooks+" helper_major=46 no_timers=1");
    }
}
