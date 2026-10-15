import org.objectweb.asm.*;
import org.objectweb.asm.tree.*;
import java.io.*;
import java.nio.file.*;
import java.util.*;

/**
 * Builds NavActiveIgnore classes for MU1427 by surgically patching the
 * firmware's own classes (converted from /ifs/lsd.jxe). Every patch asserts
 * the exact instruction pattern it expects and aborts otherwise.
 *
 * usage: java Patch <stockClassesDir> <helperClassesDir> <outDir>
 */
public class Patch {
    static Path in, helper, out;

    public static void main(String[] a) throws Exception {
        in = Paths.get(a[0]); helper = Paths.get(a[1]); out = Paths.get(a[2]);
        patchDSICarplayProxy();
        patchCarPlayModeHandling();
        patchASLNavigationServices();
        patchNavigationListener();
        patchRequestHandler();
        // patchChangeDataRate();  // disabled in rev 3: breaks cluster map start-up, see README
        downgradeHelper("de/vw/mib/navactiveignore/NavActiveIgnore");
        System.out.println("OK");
    }

    // ---------------------------------------------------------------- utils
    static ClassNode read(String name) throws IOException {
        ClassNode cn = new ClassNode();
        new ClassReader(Files.readAllBytes(in.resolve(name + ".class"))).accept(cn, ClassReader.SKIP_FRAMES);
        return cn;
    }

    static void write(ClassNode cn) throws IOException {
        if (cn.version > 49) throw new IllegalStateException(cn.name + " version " + cn.version);
        ClassWriter cw = new ClassWriter(ClassWriter.COMPUTE_MAXS); // <50: no frames needed
        cn.accept(cw);
        Path p = out.resolve(cn.name + ".class");
        Files.createDirectories(p.getParent());
        Files.write(p, cw.toByteArray());
        System.out.println("patched " + cn.name);
    }

    static MethodNode method(ClassNode cn, String name, String desc) {
        for (MethodNode m : cn.methods) if (m.name.equals(name) && m.desc.equals(desc)) return m;
        throw new IllegalStateException("method not found: " + cn.name + "." + name + desc);
    }

    static AbstractInsnNode real(AbstractInsnNode n) {
        while (n != null && n.getOpcode() < 0) n = n.getNext();
        return n;
    }

    static AbstractInsnNode realPrev(AbstractInsnNode n) {
        while (n != null && n.getOpcode() < 0) n = n.getPrevious();
        return n;
    }

    static boolean isMethod(AbstractInsnNode n, int op, String owner, String name) {
        return n instanceof MethodInsnNode && n.getOpcode() == op
                && ((MethodInsnNode) n).owner.equals(owner) && ((MethodInsnNode) n).name.equals(name);
    }

    static boolean isLdcInt(AbstractInsnNode n, int v) {
        return n instanceof LdcInsnNode && Integer.valueOf(v).equals(((LdcInsnNode) n).cst);
    }

    static boolean isIntConst(AbstractInsnNode n, int v) {
        if (n == null) return false;
        int op = n.getOpcode();
        if (op >= Opcodes.ICONST_M1 && op <= Opcodes.ICONST_5) return op - Opcodes.ICONST_0 == v;
        if (op == Opcodes.BIPUSH || op == Opcodes.SIPUSH) return ((IntInsnNode) n).operand == v;
        return isLdcInt(n, v);
    }

    /** previous k real instructions, oldest first */
    static AbstractInsnNode[] back(AbstractInsnNode n, int k) {
        AbstractInsnNode[] r = new AbstractInsnNode[k];
        AbstractInsnNode c = n;
        for (int i = k - 1; i >= 0; i--) { c = realPrev(c.getPrevious()); r[i] = c; }
        return r;
    }

    static void expect(boolean ok, String what) {
        if (!ok) throw new IllegalStateException("pattern mismatch: " + what);
    }

    static void removeAll(InsnList l, AbstractInsnNode... ns) { for (AbstractInsnNode n : ns) l.remove(n); }

    static void downgradeHelper(String name) throws IOException {
        ClassNode cn = new ClassNode();
        new ClassReader(Files.readAllBytes(helper.resolve(name + ".class"))).accept(cn, ClassReader.SKIP_FRAMES);
        cn.version = Opcodes.V1_2;
        // Guard against post-1.2 library use slipping in.
        for (MethodNode m : cn.methods)
            for (AbstractInsnNode n : m.instructions) {
                if (n instanceof MethodInsnNode && ((MethodInsnNode) n).owner.startsWith("java/lang/StringBuilder"))
                    throw new IllegalStateException("StringBuilder in helper");
                if (n instanceof InvokeDynamicInsnNode) throw new IllegalStateException("indy in helper");
                if (n instanceof LdcInsnNode && ((LdcInsnNode) n).cst instanceof Type)
                    throw new IllegalStateException("class literal in helper");
            }
        cn.sourceFile = "NavActiveIgnore.java";
        write(cn);
    }

    // ------------------------------------------------- 1. DSICarplayProxy
    // Never send "native navigation takes ownership" to the iPhone: CarPlay
    // keeps its own route guidance while the stock map runs in the cluster.
    static void patchDSICarplayProxy() throws IOException {
        ClassNode cn = read("de/esolutions/fw/comm/dsi/carplay/impl/DSICarplayProxy");
        MethodNode m = method(cn, "requestModeChange",
                "([Lorg/dsi/ifc/carplay/ResourceRequest;[Lorg/dsi/ifc/carplay/AppStateRequest;Ljava/lang/String;)V");
        String H = "de/vw/mib/navactiveignore/NavActiveIgnore";
        int tmp = m.maxLocals; // fresh local
        InsnList p = new InsnList();
        LabelNode go = new LabelNode();
        p.add(new VarInsnNode(Opcodes.ALOAD, 2));
        p.add(new MethodInsnNode(Opcodes.INVOKESTATIC, H, "stripNativeNavClaims",
                "([Lorg/dsi/ifc/carplay/AppStateRequest;)[Lorg/dsi/ifc/carplay/AppStateRequest;", false));
        p.add(new VarInsnNode(Opcodes.ASTORE, tmp));
        p.add(new VarInsnNode(Opcodes.ALOAD, 1));
        p.add(new VarInsnNode(Opcodes.ALOAD, 2));
        p.add(new VarInsnNode(Opcodes.ALOAD, tmp));
        p.add(new MethodInsnNode(Opcodes.INVOKESTATIC, H, "dropRequest",
                "([Lorg/dsi/ifc/carplay/ResourceRequest;[Lorg/dsi/ifc/carplay/AppStateRequest;[Lorg/dsi/ifc/carplay/AppStateRequest;)Z", false));
        p.add(new JumpInsnNode(Opcodes.IFEQ, go));
        p.add(new InsnNode(Opcodes.RETURN));
        p.add(go);
        p.add(new VarInsnNode(Opcodes.ALOAD, tmp));
        p.add(new VarInsnNode(Opcodes.ASTORE, 2));
        // sanity: original body serialises param 2 via the anonymous ISerializable
        AbstractInsnNode first = real(m.instructions.getFirst());
        expect(first.getOpcode() == Opcodes.NEW && ((TypeInsnNode) first).desc.startsWith(cn.name + "$"),
                "DSICarplayProxy.requestModeChange prologue");
        m.instructions.insert(p);
        write(cn);
    }

    // --------------------------------------------- 2. CarPlayModeHandling
    static void patchCarPlayModeHandling() throws IOException {
        ClassNode cn = read("de/vw/mib/asl/internal/carplay/common/CarPlayModeHandling");
        MethodNode m = method(cn, "handleModeChanges",
                "([Lorg/dsi/ifc/carplay/Resource;[Lorg/dsi/ifc/carplay/AppState;)V");
        InsnList l = m.instructions;
        int propTrue = 0, setNav2 = 0, stopG = 0, userInit = 0;
        for (AbstractInsnNode n = l.getFirst(); n != null; ) {
            AbstractInsnNode next = n.getNext();
            // (a) aslPropertyManager.valueChangedBoolean(3010002 /*CarPlay nav active*/, true)
            if (isMethod(n, Opcodes.INVOKEINTERFACE, "de/vw/mib/asl/ASLPropertyManager", "valueChangedBoolean")) {
                AbstractInsnNode[] b = back(n, 3);
                if (b[0].getOpcode() == Opcodes.GETSTATIC && isLdcInt(b[1], 3010002) && b[2].getOpcode() == Opcodes.ICONST_1) {
                    removeAll(l, b[0], b[1], b[2], n); propTrue++;
                }
            }
            // (b) properties.setActiveNavigation(2 /*CARPLAY*/)
            else if (isMethod(n, Opcodes.INVOKEVIRTUAL, "de/vw/mib/asl/internal/carplay/common/CarPlayGlobalProperies", "setActiveNavigation")) {
                AbstractInsnNode[] b = back(n, 3);
                if (b[0].getOpcode() == Opcodes.ALOAD && b[1].getOpcode() == Opcodes.GETFIELD && b[2].getOpcode() == Opcodes.ICONST_2) {
                    removeAll(l, b[0], b[1], b[2], n); setNav2++;
                }
            }
            // (c) navServices.stopGuidance(6) ; pop   ->  pop (drops navServices ref)
            else if (isMethod(n, Opcodes.INVOKEINTERFACE, "de/vw/mib/asl/api/navigation/ASLNavigationServices", "stopGuidance")) {
                AbstractInsnNode c6 = realPrev(n.getPrevious());
                AbstractInsnNode pop = real(n.getNext());
                expect(isIntConst(c6, 6) && pop.getOpcode() == Opcodes.POP, "stopGuidance(6);pop");
                next = pop.getNext();
                removeAll(l, c6, n);  // keep the POP: it now discards navServices
                stopG++;
            }
            // (d) getDSICarPlay().requestModeChange(res, states, "User initiated") -> discard args
            else if (isMethod(n, Opcodes.INVOKEINTERFACE, "org/dsi/ifc/carplay/DSICarplay", "requestModeChange")) {
                AbstractInsnNode s = realPrev(n.getPrevious());
                if (s instanceof LdcInsnNode && "User initiated".equals(((LdcInsnNode) s).cst)) {
                    InsnList r = new InsnList();
                    r.add(new InsnNode(Opcodes.POP2)); // string, states
                    r.add(new InsnNode(Opcodes.POP2)); // res, DSICarplay
                    l.insertBefore(n, r);
                    l.remove(n);
                    userInit++;
                }
            }
            n = next;
        }
        expect(propTrue == 1 && setNav2 == 1 && stopG == 1 && userInit == 1,
                "CarPlayModeHandling counts " + propTrue + "/" + setNav2 + "/" + stopG + "/" + userInit);
        write(cn);
    }

    // -------------------------------------------- 3. ASLNavigationServices
    // stopGuidance(6) is "stop because a smartphone took navigation": ignore it.
    static void patchASLNavigationServices() throws IOException {
        ClassNode cn = read("de/vw/mib/asl/internal/navigation/api/impl/ASLNavigationServices");
        MethodNode m = method(cn, "stopGuidance", "(I)I");
        InsnList p = new InsnList();
        LabelNode go = new LabelNode();
        p.add(new VarInsnNode(Opcodes.ILOAD, 1));
        p.add(new IntInsnNode(Opcodes.BIPUSH, 6));
        p.add(new JumpInsnNode(Opcodes.IF_ICMPNE, go));
        p.add(new InsnNode(Opcodes.ICONST_1));
        p.add(new InsnNode(Opcodes.IRETURN));
        p.add(go);
        m.instructions.insert(p);
        write(cn);
    }

    // ---------------------------------------- 4. AA NavigationListener
    static void patchNavigationListener() throws IOException {
        ClassNode cn = read("de/vw/mib/asl/internal/androidauto/target/NavigationListener");
        replaceBody(method(cn, "updateGuidanceActive", "(Z)V"), Opcodes.RETURN);
        replaceBody(method(cn, "stopGuidance", "()V"), Opcodes.RETURN);
        replaceBody(method(cn, "isGuidanceActive", "()Z"), Opcodes.IRETURN);
        write(cn);
    }

    static void replaceBody(MethodNode m, int ret) {
        m.instructions.clear();
        m.tryCatchBlocks.clear();
        if (m.localVariables != null) m.localVariables.clear();
        if (ret == Opcodes.IRETURN) m.instructions.add(new InsnNode(Opcodes.ICONST_0));
        m.instructions.add(new InsnNode(ret));
    }

    // -------------------------------------------- 5. AA RequestHandler
    // Both nav-focus requests are answered "projected" (2); native guidance is
    // never stopped and the AA-nav-active HMI property (3110002) is not raised.
    static void patchRequestHandler() throws IOException {
        ClassNode cn = read("de/vw/mib/asl/internal/androidauto/target/RequestHandler");
        MethodNode m = method(cn, "performNavFocusRequestNotification", "(I)V");
        InsnList l = m.instructions;
        LookupSwitchInsnNode sw = null;
        for (AbstractInsnNode n : l) if (n instanceof LookupSwitchInsnNode) { expect(sw == null, "one switch"); sw = (LookupSwitchInsnNode) n; }
        expect(sw != null && sw.keys.equals(Arrays.asList(1, 2)), "switch keys {1,2}");
        sw.labels.set(0, sw.labels.get(1));   // case 1 -> case 2 body
        int stop = 0, prop = 0;
        for (AbstractInsnNode n = l.getFirst(); n != null; ) {
            AbstractInsnNode next = n.getNext();
            if (isMethod(n, Opcodes.INVOKEVIRTUAL, "de/vw/mib/asl/internal/androidauto/target/NavigationListener", "stopGuidance")) {
                AbstractInsnNode[] b = back(n, 2);
                expect(b[0].getOpcode() == Opcodes.ALOAD && b[1].getOpcode() == Opcodes.GETFIELD, "navigationHandler.stopGuidance()");
                removeAll(l, b[0], b[1], n); stop++;
            } else if (isMethod(n, Opcodes.INVOKEINTERFACE, "de/vw/mib/asl/ASLPropertyManager", "valueChangedBoolean")) {
                AbstractInsnNode[] b = back(n, 3);
                if (b[0].getOpcode() == Opcodes.GETSTATIC && isLdcInt(b[1], 3110002) && b[2].getOpcode() == Opcodes.ICONST_1) {
                    removeAll(l, b[0], b[1], b[2], n); prop++;
                }
            }
            n = next;
        }
        expect(stop == 1 && prop == 1, "RequestHandler counts " + stop + "/" + prop);
        write(cn);
    }

    // ------------------------------------- 6. MOST kombi ChangeDataRate
    // The cluster asks for stream data rate 1 (map hidden, 1 fps) when it does
    // not want the nav map; promote it to 2 (map visible, stock 10 fps) at the
    // point where the request arrives, so ChangeDataRate and its
    // ChangeDataRateSequence always agree on the rate.
    // NOTE: the original jar did this inside the ChangeDataRateSequence
    // constructor. ChangeDataRate.sequenceDone() then sees sequence rate (2) !=
    // requested rate (1) and restarts the sequence forever -> cluster stuck on
    // "loading map". Do not move it back there.
    static void patchChangeDataRate() throws IOException {
        ClassNode cn = read("de/vw/mib/asl/internal/mostkombi/streamsink/usecases/ChangeDataRate");
        MethodNode m = method(cn, "dsiKOMOGfxStreamSinkUpdateDataRate", "(II)V");
        AbstractInsnNode first = real(m.instructions.getFirst());
        expect(first.getOpcode() == Opcodes.ALOAD && isMethod(real(first.getNext()), Opcodes.INVOKESPECIAL, cn.name, "getStreamSinkDataRate"),
                "ChangeDataRate.dsiKOMOGfxStreamSinkUpdateDataRate prologue");
        InsnList p = new InsnList();
        LabelNode go = new LabelNode();
        p.add(new VarInsnNode(Opcodes.ILOAD, 1));
        p.add(new InsnNode(Opcodes.ICONST_1));
        p.add(new JumpInsnNode(Opcodes.IF_ICMPNE, go));
        p.add(new InsnNode(Opcodes.ICONST_2));
        p.add(new VarInsnNode(Opcodes.ISTORE, 1));
        p.add(go);
        m.instructions.insert(p);
        write(cn);
    }
}
