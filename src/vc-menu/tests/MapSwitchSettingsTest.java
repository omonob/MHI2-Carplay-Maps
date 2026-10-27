import java.io.*;
import org.omonob.cpmap.ui.MapSwitchSettings;

public final class MapSwitchSettingsTest {
    static int checks;
    static void require(boolean condition, String description) {
        checks++;
        if (!condition) throw new AssertionError(description);
    }
    static void write(File f, String text) throws Exception {
        FileOutputStream out = new FileOutputStream(f);
        try { out.write(text.getBytes("US-ASCII")); } finally { out.close(); }
    }
    public static void main(String[] args) throws Exception {
        File root = new File(args[0]).getCanonicalFile();
        if (!root.mkdirs()) throw new IOException("Expected a fresh disposable test directory");
        MapSwitchSettings store = new MapSwitchSettings(new File(root, "normal"));
        require(store.read().enabled && store.read().generation == 0, "Preserve existing map-on default");
        require(!store.toggle().enabled, "First click turns preference off");
        require(!new MapSwitchSettings(new File(root, "normal")).read().enabled, "New instance reloads saved off");
        require(store.toggle().enabled, "Second click turns preference on");
        require(store.read().generation == 2, "One generation per click");
        for (int i = 0; i < 100; i++) store.toggle();
        require(store.read().enabled && store.read().generation == 102, "Repeated clicks stay consistent");
        write(new File(root, "normal/maps.pending"), "partial write");
        require(store.read().generation == 102, "Uncommitted partial file is ignored");
        write(new File(root, "normal/maps.state"), "format=1\nenabled=0\n");
        require(store.read().generation == 101 && !store.read().enabled, "Torn primary recovers last good backup");
        require(store.set(true).generation == 102, "Recovery can be saved again");
        require(store.read().enabled, "Recovered setting readback");
        File blocked = new File(root, "normal/maps.pending");
        require(blocked.delete() || !blocked.exists(), "Fixture clears only its own pending file");
        require(blocked.mkdir(), "Fixture blocks staging path");
        boolean refused = false;
        try { store.set(false); } catch (IOException expected) { refused = true; }
        require(refused && store.read().enabled && store.read().generation == 102, "Write failure never claims a changed state");
        File damaged = new File(root, "damaged");
        require(damaged.mkdir(), "Damaged fixture created");
        write(new File(damaged, "maps.state"), "corrupt");
        refused = false;
        try { new MapSwitchSettings(damaged).read(); } catch (IOException expected) { refused = true; }
        require(refused, "No silent guessed preference if all saved files are invalid");
        File notDirectory = new File(root, "file-not-dir");
        write(notDirectory, "fixture");
        refused = false;
        try { new MapSwitchSettings(notDirectory).set(false); } catch (IOException expected) { refused = true; }
        require(refused, "Not-a-directory destination rejected");
        System.out.println("MAP_SWITCH_SETTINGS_TEST_OK checks=" + checks);
    }
}
