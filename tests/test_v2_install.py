"""All five public profiles: clean install, repair, repeat, restore, rollback.
No SSH or real unit access. Temporary fixture filesystem only.
"""
import os, tempfile
from pathlib import Path
from test_toolbox_install import PROFILES, package, fixture, digest, run

def main():
 for mu,cluster,size in PROFILES:
  pkg=package(mu,cluster);payload=pkg/'payload';nav=payload/'common/CarPlayNavigationPatch.jar'
  for mode in ('stock','existing','merged','missing-file','partial','bad-startup'):
   with tempfile.TemporaryDirectory(prefix='cpmap-v2-fixture-') as tmp:
    root=Path(tmp);dio,lsd,jars=fixture(root,size,mode,nav);before=(digest(dio),digest(lsd))
    prefs=root/'mnt/icab/persistent/cpmap-ui';prefs.mkdir(parents=True)
    # This belongs to another installed feature. Install/restore must not reset it.
    (prefs/'settings.state').write_bytes(b'fixture-preserve-user-state')
    if mode=='bad-startup':
     run(pkg/'install.sh',root,payload,mu,cluster,ok=False);assert before==(digest(dio),digest(lsd));continue
    out=run(pkg/'install.sh',root,payload,mu,cluster);assert 'CPMAP_INSTALL_OK' in out
    for name in ('VCMenu.jar','VCMirrorRate.jar','CarPlayClusterControls.jar'):
     expected=payload/'profiles'/mu/cluster/name
     assert digest(jars/name)==digest(expected),name
     assert lsd.read_text().count(name)==1
    assert lsd.read_text().count('-Xbootclasspath/p:')==1
    runtime=root/'mnt/app/carplaymaps'
    assert digest(runtime/'cpvc-mirror-canvas')==digest(payload/'common'/f'cpvc-mirror-canvas-{cluster}')
    first=digest(lsd);run(pkg/'install.sh',root,payload,mu,cluster);assert digest(lsd)==first
    run(pkg/'restore-carplay-maps.sh',root);assert before==(digest(dio),digest(lsd))
    assert not (jars/'VCMenu.jar').exists() and not (jars/'VCMirrorRate.jar').exists()
    assert (prefs/'settings.state').read_bytes()==b'fixture-preserve-user-state'
   print('V2_INSTALL_REPEAT_RESTORE_PASS',mu,cluster,mode)
  for target in ('VCMenu.jar','VCMirrorRate.jar','CarPlayNavigationPatch.jar'):
   with tempfile.TemporaryDirectory(prefix='cpmap-v2-rollback-') as tmp:
    root=Path(tmp);dio,lsd,jars=fixture(root,size,'stock',nav);before=(digest(dio),digest(lsd))
    commands=root/'commands';commands.mkdir();mv=commands/'mv'
    mv.write_text('#!/bin/sh\ncase "$1" in */'+target+'.new) exit 99;; esac\nexec /bin/mv "$@"\n');mv.chmod(0o755)
    run(pkg/'install.sh',root,payload,mu,cluster,ok=False,extra_env={'PATH':str(commands)+':'+os.environ['PATH']})
    assert before==(digest(dio),digest(lsd))
    assert not (jars/'VCMenu.jar').exists() and not (jars/'VCMirrorRate.jar').exists()
   print('V2_PARTIAL_COMMIT_ROLLBACK_PASS',mu,cluster,target)
 print('V2_ALL_FIVE_INSTALL_PROFILES_PASS')
if __name__=='__main__':main()
