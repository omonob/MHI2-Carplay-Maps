package org.omonob.cpmap.ui;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.zip.CRC32;

/** Mutable user preference, deliberately separate from keys and guarded payloads. */
public final class MapSwitchSettings {
    private static final Object LOCK = new Object();
    private final File directory;
    public static final boolean DEFAULT_ENABLED = true;

    public static final class State {
        public final boolean enabled;
        public final long generation;
        public final String source;
        State(boolean value, long revision, String origin) {
            enabled = value;
            generation = revision;
            source = origin;
        }
    }

    public MapSwitchSettings(File root) throws IOException {
        if (root == null) throw new IOException("Missing preference directory");
        directory = root.getCanonicalFile();
    }

    public static MapSwitchSettings systemStore() throws IOException {
        String path = System.getProperty("cpmap.ui.stateDir");
        if (path == null || path.length() == 0) path = "/mnt/icab/persistent/cpmap-ui";
        return new MapSwitchSettings(new File(path));
    }

    public State read() throws IOException {
        synchronized (LOCK) { return readLocked(); }
    }

    private State readLocked() throws IOException {
        File current = file("maps.state");
        File backup = file("maps.previous");
        State main = decode(current, "saved");
        State previous = decode(backup, "recovered");
        if (main != null) return main;
        if (previous != null) return previous;
        if (current.exists() || backup.exists()) throw new IOException("Preference files are damaged; refusing a guessed state");
        return new State(DEFAULT_ENABLED, 0, "default");
    }

    public State set(boolean enabled) throws IOException {
        synchronized (LOCK) {
            State previous = readLocked();
            if (previous.generation == Long.MAX_VALUE) throw new IOException("Preference generation overflow");
            if (!directory.exists() && !directory.mkdirs()) throw new IOException("Cannot create preference directory");
            if (!directory.isDirectory()) throw new IOException("Preference destination is not a directory");
            File pending = file("maps.pending");
            byte[] bytes = encode(enabled, previous.generation + 1);
            FileOutputStream out = new FileOutputStream(pending);
            try { out.write(bytes); out.flush(); out.getFD().sync(); }
            finally { out.close(); }
            State staged = decode(pending, "pending");
            if (staged == null || staged.enabled != enabled || staged.generation != previous.generation + 1)
                throw new IOException("Staged preference readback failed");
            File current = file("maps.state");
            File backup = file("maps.previous");
            // Never replace the last valid backup with a damaged primary.
            if (decode(current, "saved") != null) {
                if (backup.exists() && !backup.delete()) throw new IOException("Cannot rotate previous preference");
                if (!current.renameTo(backup)) throw new IOException("Cannot preserve previous preference");
            } else if (current.exists() && !current.delete()) {
                throw new IOException("Cannot replace damaged preference");
            }
            if (!pending.renameTo(current)) throw new IOException("Preference commit failed; previous state preserved");
            State committed = readLocked();
            if (committed.enabled != enabled || committed.generation != staged.generation)
                throw new IOException("Committed preference readback failed");
            return committed;
        }
    }

    public State toggle() throws IOException {
        synchronized (LOCK) { return set(!readLocked().enabled); }
    }

    private File file(String name) throws IOException {
        File result = new File(directory, name);
        if (!directory.equals(result.getCanonicalFile().getParentFile()))
            throw new IOException("Preference path escaped its own directory");
        return result;
    }

    private static String body(boolean enabled, long generation) {
        return new StringBuffer("format=1\nenabled=").append(enabled ? '1' : '0')
            .append("\ngeneration=").append(generation).append('\n').toString();
    }

    private static String checksum(String body) throws IOException {
        CRC32 crc = new CRC32();
        byte[] bytes = body.getBytes("US-ASCII");
        crc.update(bytes, 0, bytes.length);
        return Long.toHexString(crc.getValue());
    }

    private static byte[] encode(boolean enabled, long generation) throws IOException {
        String data = body(enabled, generation);
        return new StringBuffer(data).append("crc32=").append(checksum(data)).append('\n').toString().getBytes("US-ASCII");
    }

    private static State decode(File file, String source) throws IOException {
        if (!file.isFile() || file.length() > 192) return null;
        BufferedReader in = new BufferedReader(new InputStreamReader(new FileInputStream(file), "US-ASCII"));
        try {
            String format = in.readLine();
            String flag = in.readLine();
            String revision = in.readLine();
            String crc = in.readLine();
            if (!"format=1".equals(format) || !("enabled=0".equals(flag) || "enabled=1".equals(flag))
                || revision == null || !revision.startsWith("generation=") || crc == null || in.readLine() != null) return null;
            long generation;
            try { generation = Long.parseLong(revision.substring(11)); }
            catch (NumberFormatException e) { return null; }
            if (generation < 1) return null;
            boolean value = "enabled=1".equals(flag);
            if (!new StringBuffer("crc32=").append(checksum(body(value, generation))).toString().equals(crc)) return null;
            return new State(value, generation, source);
        } finally { in.close(); }
    }
}
