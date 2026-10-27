"""Validate the final full MQB archive against every standalone profile."""
import hashlib, json, re, sys, zipfile
from pathlib import Path
from test_toolbox_install import PROFILES, REPO, package

def sha(data):return hashlib.sha256(data).hexdigest()
def main():
 archive=Path(sys.argv[1]);records=[]
 with zipfile.ZipFile(archive) as z:
  assert z.testzip() is None
  assert len(z.namelist())==len(set(z.namelist()))
  assert not any(n.startswith('CarPlayMapsVC/') or 'MIB_Toolbox' in n for n in z.namelist())
  prefix='Custom/carplay_maps_vc/payload/'
  for mu,cluster,_ in PROFILES:
   p=package(mu,cluster)/'payload'
   for file in (p/'profiles'/mu/cluster).iterdir():
    assert z.read(prefix+'profiles/'+mu+'/'+cluster+'/'+file.name)==file.read_bytes(),file
   for file in (p/'common').iterdir():
    if file.name=='CarPlayClusterControls.jar':continue # selected firmware JAR is under profiles
    assert z.read(prefix+'common/'+file.name)==file.read_bytes(),file
   core=(p/'profiles'/mu/cluster/'core.so').read_bytes()
   assert core[:7]==b'\x7fELF\x01\x01\x01' and int.from_bytes(core[18:20],'little')==40
   if cluster=='790':assert sha(core)=='97ccb829f996b74305099ca54e6f4a693a17381b391d170c1227f44e49ff0769'
   if cluster=='791':assert sha(core)=='b3d28142eac16d220e639bd0d8056e595e4d4bbf90a9d63d2e3c402c62b870b0'
   for name in ('VCMenu.jar','VCMirrorRate.jar'):
    with zipfile.ZipFile(p/'profiles'/mu/cluster/name) as jar:
     assert jar.testzip() is None
     for entry in jar.namelist():
      if entry.endswith('.class'):
       assert int.from_bytes(jar.read(entry)[6:8],'big')==46,(name,entry)
   records.append(dict(firmware=mu,cluster=cluster,core_sha256=sha(core),pass_profile=True))
  assert z.read('Toolbox/scripts/cpmap_restore_core.sh')==(REPO/'src/toolbox/restore-carplay-maps.sh').read_bytes()
  assert z.read('Toolbox/GEM/carplay-maps-vc.esd')==(REPO/'src/toolbox/carplay-maps-vc.esd').read_bytes()
  assert z.read('CARPLAY_MAPS_VC_README.md')==(REPO/'releases/toolbox-plaintext-v2.0.1.md').read_bytes()
  for name in z.namelist():
   if not name.startswith(prefix):continue
   data=z.read(name)
   assert not name.endswith(('.eft','.jxe','.ifs','.bin','.raw','.ttf','.otf')),'Unexpected private/OEM payload: '+name
   assert not re.search(rb'AW[0-9]-MON[0-9]{2}\.[0-9]{2}\.[0-9]{2}[0-9A-Za-z]{10}',data),'Device identity in '+name
   assert not re.search(rb'-----BEGIN [A-Z ]*PRIVATE KEY-----[\r\n]+[A-Za-z0-9+/=]{48}',data),'Private key in '+name
  installer=z.read(prefix+'common/install-carplay-maps.sh')
  assert b'TOOLBOX-2.0.1' in installer and b'\r' not in installer
  assert b'ENCRYPTED_INSTALLATION_REQUIRES_ITS_OWN_MIGRATION_TOOL' in installer
  assert b'DISCONNECT_CARPLAY_BEFORE_INSTALL' in installer
 print(json.dumps(dict(result='V2_FULL_MQB_ARCHIVE_PASS',profiles=records,archive_sha256=sha(archive.read_bytes())),indent=2))
if __name__=='__main__':main()
