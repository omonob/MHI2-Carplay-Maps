import java.io.File;
import org.omonob.cpmap.ui.MapSwitchSettings;

public final class MapSwitchStateCli {
    public static void main(String[] args) throws Exception {
        MapSwitchSettings store = new MapSwitchSettings(new File(args[0]));
        MapSwitchSettings.State state;
        if ("click".equals(args[1])) state = store.toggle(); else state = store.read();
        System.out.println("enabled=" + (state.enabled ? 1 : 0) + " generation=" + state.generation + " source=" + state.source);
    }
}
