"""Upgrade the published v1.2.2 fixture to2.0 and preserve first backups."""
import sys,tempfile,zipfile
from pathlib import Path
from test_toolbox_install import PROFILES,package,fixture,digest,run

def main():
 with zipfile.ZipFile(sys.argv[1]) as old:
  # G11 was not supported by the published v1.2.2 archive.
  for mu,cluster,size in [p for p in PROFILES if p[0] not in ('MU1433','MU1447')]:
   with tempfile.TemporaryDirectory(prefix='cpmap-v2-upgrade-') as tmp:
    base=Path(tmp);payload=base/'v1-payload';prefix='Custom/carplay_maps_vc/payload/'
    for name in old.namelist():
     if not name.startswith(prefix) or name.endswith('/'):continue
     relative=Path(name[len(prefix):]);assert not relative.is_absolute() and '..' not in relative.parts
     dest=payload/relative;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(old.read(name))
    root=base/'unit';nav=payload/'common/CarPlayNavigationPatch.jar'
    dio,lsd,jars=fixture(root,size,'stock',nav);original=(digest(dio),digest(lsd))
    run(payload/'common/install-carplay-maps.sh',root,payload,mu,cluster)
    legacy=(digest(dio),digest(lsd));assert legacy!=original
    pkg=package(mu,cluster)
    output=run(pkg/'install.sh',root,pkg/'payload',mu,cluster);assert 'CPMAP_INSTALL_OK' in output
    runtime=root/'mnt/app/carplaymaps'
    assert digest(runtime/'rollback/lsd.sh')==original[1]
    assert digest(runtime/'stock_first/dio_manager.real')==original[0]
    assert lsd.read_text().count('VCMenu.jar')==1 and lsd.read_text().count('VCMirrorRate.jar')==1
    run(pkg/'restore-carplay-maps.sh',root)
    assert (digest(dio),digest(lsd))==original
    assert not (jars/'VCMenu.jar').exists() and not (jars/'VCMirrorRate.jar').exists()
   print('PUBLISHED_V1_TO_V2_UPGRADE_AND_ORIGINAL_RESTORE_PASS',mu,cluster)
if __name__=='__main__':main()
