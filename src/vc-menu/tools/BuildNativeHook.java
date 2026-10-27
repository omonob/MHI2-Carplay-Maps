import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Enumeration;
import java.util.jar.JarEntry;
import java.util.jar.JarFile;
import java.util.jar.JarOutputStream;
import jdk.internal.org.objectweb.asm.*;

/** Local experiment only. Original firmware and skin files are never replaced. */
public final class BuildNativeHook {
    static final String TARGET = "de/vw/mib/view/internal/AbstractViewHandler";
    static final String BRIDGE = "org/omonob/cpmap/ui/IndependentMenuViewBridge";
    static int hooks;

    static byte[] patch(byte[] stock) {
        ClassReader reader = new ClassReader(stock);
        ClassWriter writer = new ClassWriter(ClassWriter.COMPUTE_MAXS);
        reader.accept(new ClassVisitor(Opcodes.ASM8, writer) {
            public void visit(int version, int access, String name, String signature, String parent, String[] interfaces) {
                if (!TARGET.equals(name)) throw new IllegalArgumentException("Unexpected stock hook class");
                super.visit(46, access, name, signature, parent, interfaces);
            }
            public MethodVisitor visitMethod(int access, String name, String descriptor, String signature, String[] exceptions) {
                MethodVisitor next = super.visitMethod(access, name, descriptor, signature, exceptions);
                if ("fatalError".equals(name) && "(Ljava/lang/RuntimeException;)V".equals(descriptor)) {
                    return new MethodVisitor(Opcodes.ASM8, next) {
                        public void visitCode() {
                            super.visitCode();
                            super.visitVarInsn(Opcodes.ALOAD, 0);
                            super.visitVarInsn(Opcodes.ALOAD, 1);
                            super.visitMethodInsn(Opcodes.INVOKESTATIC, BRIDGE, "failure", "(L"+TARGET+";Ljava/lang/Throwable;)V", false);
                        }
                    };
                }
                final String call = "setup".equals(name) && "()V".equals(descriptor) ? "attach"
                    : "activate".equals(name) && "()V".equals(descriptor) ? "refresh"
                    : "deinit".equals(name) && "()V".equals(descriptor) ? "detach"
                    : "destroy".equals(name) && "()V".equals(descriptor) ? "detach" : null;
                if (call == null) return next;
                hooks++;
                return new MethodVisitor(Opcodes.ASM8, next) {
                    public void visitInsn(int opcode) {
                        if (opcode == Opcodes.RETURN) {
                            super.visitVarInsn(Opcodes.ALOAD, 0);
                            super.visitMethodInsn(Opcodes.INVOKESTATIC, BRIDGE, call, "(L" + TARGET + ";)V", false);
                        }
                        super.visitInsn(opcode);
                    }
                };
            }
        }, ClassReader.SKIP_FRAMES);
        if (hooks != 4) throw new IllegalStateException("Exact lifecycle ABI not found");
        return writer.toByteArray();
    }

    static byte[] legacy(byte[] input) {
        ClassReader reader = new ClassReader(input);
        ClassWriter writer = new ClassWriter(0);
        reader.accept(new ClassVisitor(Opcodes.ASM8, writer) {
            public void visit(int version, int access, String name, String signature, String parent, String[] interfaces) {
                super.visit(46, access, name, null, parent, interfaces);
            }
        }, ClassReader.SKIP_FRAMES);
        byte[] bytes = writer.toByteArray();
        // Runtime sources deliberately avoid language/library features after Java 1.4.
        final String[] forbidden = {"java/lang/StringBuilder", "java/nio/", "java/util/concurrent", "java/lang/invoke/"};
        String raw = new String(bytes, java.nio.charset.StandardCharsets.ISO_8859_1);
        for (String token : forbidden) if (raw.contains(token)) throw new IllegalStateException("Post-CDC reference: " + token);
        return bytes;
    }

    public static void main(String[] args) throws Exception {
        Path baseline = Path.of(args[0]), classes = Path.of(args[1]), output = Path.of(args[2]);
        Path staged = Files.createTempFile(output.getParent(), "menu-build-", ".jar.tmp");
        try (JarFile source = new JarFile(baseline.toFile()); JarOutputStream jar = new JarOutputStream(Files.newOutputStream(staged))) {
            byte[] stock = source.getInputStream(source.getJarEntry(TARGET + ".class")).readAllBytes();
            add(jar, TARGET + ".class", patch(stock));
            try (var files = Files.walk(classes)) {
                for (Path file : files.filter(p -> p.toString().endsWith(".class")).sorted().toList()) {
                    String name = classes.relativize(file).toString().replace('\\', '/');
                    // Offline inspectors/extractors never belong in an installable runtime.
                    if (name.contains("OfflineRuntimeAssets") || name.contains("OemMenuInspector")) continue;
                    add(jar, name, legacy(Files.readAllBytes(file)));
                }
            }
        }
        padForIsolatedSlot(staged);
        Files.move(staged, output, java.nio.file.StandardCopyOption.ATOMIC_MOVE,
                   java.nio.file.StandardCopyOption.REPLACE_EXISTING);
        System.out.println("NATIVE_HOOK_BUILT hooks=" + hooks + " class_major=46");
    }
    static void add(JarOutputStream jar, String name, byte[] bytes) throws Exception {
        JarEntry entry = new JarEntry(name); entry.setTime(0); jar.putNextEntry(entry); jar.write(bytes); jar.closeEntry();
    }
    static byte[] padded(java.util.Map<String,byte[]> entries,int padding) throws Exception {
        java.io.ByteArrayOutputStream bytes=new java.io.ByteArrayOutputStream();
        try(JarOutputStream out=new JarOutputStream(bytes)) {
            for(var entry:entries.entrySet())add(out,entry.getKey(),entry.getValue());
            byte[] zeros=new byte[padding];java.util.zip.CRC32 crc=new java.util.zip.CRC32();crc.update(zeros);
            JarEntry pad=new JarEntry("META-INF/OFFLINE_PADDING");pad.setTime(0);pad.setMethod(JarEntry.STORED);
            pad.setSize(zeros.length);pad.setCompressedSize(zeros.length);pad.setCrc(crc.getValue());
            out.putNextEntry(pad);out.write(zeros);out.closeEntry();
        }
        return bytes.toByteArray();
    }
    static void padForIsolatedSlot(Path path) throws Exception {
        java.util.Map<String,byte[]> entries=new java.util.LinkedHashMap<>();
        try(JarFile jar=new JarFile(path.toFile())) {
            var iterator=jar.entries();while(iterator.hasMoreElements()){
                JarEntry entry=iterator.nextElement();entries.put(entry.getName(),jar.getInputStream(entry).readAllBytes());
            }
        }
        int remaining=131072-padded(entries,0).length;
        if(remaining<0)throw new IllegalStateException("Native test module exceeds fixed isolated slot");
        byte[] result=padded(entries,remaining);
        if(result.length!=131072)throw new IllegalStateException("Slot padding size mismatch");
        Files.write(path,result);
    }
}
