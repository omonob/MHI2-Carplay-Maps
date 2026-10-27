"""Bounded v2.0.0 -> v2.0.1 archive delta; no device access."""
import io,json,sys,zipfile

def main():
 with zipfile.ZipFile(sys.argv[1]) as old,zipfile.ZipFile(sys.argv[2]) as new:
  assert set(old.namelist())==set(new.namelist()),'Unexpected archive inventory change'
  prefix='Custom/carplay_maps_vc/payload/'
  profiles=('MU1102/791','MU1102/790','MU1367/791','MU1367/790','MU1440/790')
  menus={prefix+'profiles/'+p+'/VCMenu.jar' for p in profiles}
  cores={prefix+'profiles/'+p+'/core.so' for p in profiles if p.endswith('/790')}
  expected=menus|cores|{prefix+'common/install-carplay-maps.sh','Toolbox/GEM/carplay-maps-vc.esd','CARPLAY_MAPS_VC_README.md'}
  changed={p for p in old.namelist() if old.read(p)!=new.read(p)}
  assert changed==expected,(changed-expected,expected-changed)
  for name in menus:
   with zipfile.ZipFile(io.BytesIO(old.read(name))) as a,zipfile.ZipFile(io.BytesIO(new.read(name))) as b:
    assert set(b.namelist())-set(a.namelist())=={'org/omonob/cpmap/ui/CarPlayNavigationOwnership.class','de/vw/mib/asl/internal/carplay/target/DSIHandler.class'}
    assert not set(a.namelist())-set(b.namelist())
    assert {p for p in a.namelist() if a.read(p)!=b.read(p)}=={'META-INF/OFFLINE_PADDING'},'Existing menu class changed'
  for profile in ('MU1102/791','MU1367/791'):
   name=prefix+'profiles/'+profile+'/core.so';assert old.read(name)==new.read(name)
  print(json.dumps({'result':'V2_0_1_BOUNDED_SCOPE_PASS','changed_files':len(changed),'unchanged_files':len(old.namelist())-len(changed),'new_jar_classes_per_profile':2,'existing_menu_classes_unchanged':True,'aid12_3_cores_unchanged':True,'bridge_supervisor_mirror_controls_unchanged':True},indent=2))
if __name__=='__main__':main()
