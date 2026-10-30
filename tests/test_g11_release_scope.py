"""Negative compatibility and wrapper checks for the G11 prerelease."""
import tempfile
from pathlib import Path
from test_toolbox_install import PROFILES,package,fixture,digest,run,navigation_payload,REPO

def main():
    checks=0
    assert len(PROFILES)==7
    for mu,size in [('MU1433',52751492),('MU1447',55712574)]:
        p=package(mu,'790');payload=p/'payload'
        assert ('"$BASE/payload" '+mu+' 790') in (p/'run-install.sh').read_text();checks+=1
        wrapper=(payload/'common/dio_manager.wrapper.sh').read_text()
        assert mu+'/790' in wrapper
        assert wrapper==(REPO/'src/toolbox/dio_manager.wrapper.sh').read_text();checks+=1
        for rejected,cluster,jxe in [('MU0033','790',47375274),(mu,'791',size),(mu,'790',47375274),(mu,'790',size+1)]:
            with tempfile.TemporaryDirectory(prefix='g11-reject-') as td:
                root=Path(td);dio,lsd,jars=fixture(root,jxe,'stock',navigation_payload(payload,mu,'790'))
                before=(digest(dio),digest(lsd))
                run(p/'install.sh',root,payload,rejected,cluster,ok=False)
                assert before==(digest(dio),digest(lsd))
                assert not (root/'mnt/app/carplaymaps').exists();checks+=1
        for f in p.rglob('*'):
            if f.is_file() and f.suffix in ('.sh','.txt'):
                assert b'MU0033' not in f.read_bytes(),f
    for file in ('src/toolbox/cpmap_common.sh','src/toolbox/cpmap_install_790.sh'):
        assert b'MU0033' not in (REPO/file).read_bytes();checks+=1
    print('G11_RELEASE_SCOPE_AND_REJECT_PASS checks='+str(checks)+' wrappers=exact excluded_firmware_write=0')

if __name__=='__main__':main()
