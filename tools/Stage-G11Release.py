"""Stage audited build outputs and create a full MQB G11 prerelease.

No network, vehicle access, stock firmware copying, or standalone installer.
Only generated payloads/manifests are updated in the source profile folders.
"""
import argparse, hashlib, json, shutil, zipfile
from pathlib import Path, PurePosixPath

REPO = Path(__file__).resolve().parents[1]
VERSION = '2.1.0-g11-beta.1'
BASE_SHA = '86aada968b035710696f7e4a7cc7a2e13bb83fb4e71ad3c4374e40ecb193cbae'
PROFILES = [('MU1102','791','MU1102'),('MU1102','790','MU1102'),
            ('MU1367','791','MU1367'),('MU1367','790','MU1367'),
            ('MU1440','790','MU1440_SKODA'),('MU1433','790','MU1433_SKODA'),
            ('MU1447','790','MU1447_SEAT')]

def sha(data): return hashlib.sha256(data).hexdigest()
def pkg(mu, cluster, brand):
    return REPO / brand / ('790_AID10.5' if cluster == '790' else '791_AID12.3')

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('base-archive','native-build','native-repro','menu-build','navigation-build','output'):
        p.add_argument('--'+name,required=True,type=Path)
    a=p.parse_args()
    assert sha(a.base_archive.read_bytes()) == BASE_SHA, 'Exact public 2.0.1 base required'
    assert not a.output.exists(), 'Use a fresh output folder'
    for mu,cluster,brand in PROFILES:
        package=pkg(mu,cluster,brand); common=package/'payload/common'
        if mu in ('MU1433','MU1447'):
            native=a.native_build/mu/cluster/'core.so'
            assert native.read_bytes()==(a.native_repro/mu/cluster/'core.so').read_bytes()
            target=package/'payload/profiles'/mu/cluster
            shutil.copyfile(native,target/'core.so')
            for name in ('VCMenu.jar','VCMirrorRate.jar','CarPlayClusterControls.jar'):
                shutil.copyfile(a.menu_build/mu/name,target/name)
            if mu=='MU1433':
                shutil.copyfile(a.navigation_build/mu/'CarPlayMapsG11Navigation.jar',target/'CarPlayMapsG11Navigation.jar')
        shutil.copyfile(REPO/'src/toolbox/install-carplay-maps.sh',package/'install.sh')
        shutil.copyfile(REPO/'src/toolbox/install-carplay-maps.sh',common/'install-carplay-maps.sh')
        shutil.copyfile(REPO/'src/toolbox/restore-carplay-maps.sh',package/'restore-carplay-maps.sh')
        for script in ('restore-carplay-maps.sh','dio_manager.wrapper.sh'):
            shutil.copyfile(REPO/'src/toolbox'/script,common/script)
        shutil.copyfile(a.native_build/'common/cpmap-nav-check',common/'cpmap-nav-check')
        # Newly generated file, not a source logic rewrite.
        files=sorted(f for f in package.rglob('*') if f.is_file() and f.name!='SHA256SUMS.txt')
        (package/'SHA256SUMS.txt').write_bytes(''.join(sha(f.read_bytes())+'  '+f.relative_to(package).as_posix()+'\n' for f in files).encode('ascii'))
    assets=a.output/'release-assets';assets.mkdir(parents=True)
    name='VW_SKODA_SEAT_CarPlay_Maps_VC_FULL_MQB_Toolbox_V4.2A_v'+VERSION+'.zip'
    output=assets/name
    replacements={}
    common=pkg(*PROFILES[0])/'payload/common'
    for f in common.iterdir():
        if f.is_file(): replacements['Custom/carplay_maps_vc/payload/common/'+f.name]=f
    for mu,cluster,brand in PROFILES:
        for f in (pkg(mu,cluster,brand)/'payload/profiles'/mu/cluster).iterdir():
            if f.is_file(): replacements['Custom/carplay_maps_vc/payload/profiles/'+mu+'/'+cluster+'/'+f.name]=f
    for name_in,name_out in [('cpmap_common.sh','cpmap_common.sh'),('cpmap_install_790.sh','cpmap_install_790.sh'),('restore-carplay-maps.sh','cpmap_restore_core.sh')]:
        replacements['Toolbox/scripts/'+name_out]=REPO/'src/toolbox'/name_in
    notes=REPO/'releases'/('toolbox-plaintext-v'+VERSION+'.md')
    replacements['CARPLAY_MAPS_VC_README.md']=notes
    changes=[]
    with zipfile.ZipFile(a.base_archive) as old, zipfile.ZipFile(output,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        assert len(old.namelist())==len(set(old.namelist()))
        for item in old.infolist():
            path=PurePosixPath(item.filename)
            assert not path.is_absolute() and '..' not in path.parts
            assert (item.external_attr>>16)&0o170000!=0o120000
            f=replacements.pop(item.filename,None)
            before=old.read(item); after=f.read_bytes() if f else before
            if before!=after:
                assert item.filename.startswith('Custom/carplay_maps_vc/') or item.filename in ('Toolbox/scripts/cpmap_common.sh','Toolbox/scripts/cpmap_install_790.sh','Toolbox/scripts/cpmap_restore_core.sh','CARPLAY_MAPS_VC_README.md')
                changes.append(item.filename)
            z.writestr(item,after)
        for entry,f in sorted(replacements.items()):
            assert entry.startswith('Custom/carplay_maps_vc/payload/profiles/MU1433/') or entry.startswith('Custom/carplay_maps_vc/payload/profiles/MU1447/'), entry
            z.writestr(entry,f.read_bytes())
        # All previously published per-profile binaries remain byte-identical.
    with zipfile.ZipFile(a.base_archive) as old, zipfile.ZipFile(output) as z:
        for entry in old.namelist():
            if entry.startswith('Custom/carplay_maps_vc/payload/profiles/'):
                assert old.read(entry)==z.read(entry), 'Existing profile changed: '+entry
        assert not any('MU0033' in n or 'Standalone' in n for n in z.namelist())
        assert z.testzip() is None
    h=sha(output.read_bytes())
    (assets/'FULL_TOOLBOX_SHA256.txt').write_bytes((h+'  '+output.name+'\n').encode('ascii'))
    shutil.copyfile(notes,assets/'GITHUB_RELEASE_NOTES.md')
    report={'version':VERSION,'prerelease':True,'base_sha256':BASE_SHA,'archive_sha256':h,
            'profiles':len(PROFILES),'new_firmware':['MU1433','MU1447'],
            'existing_five_profile_payloads':'byte-identical to public 2.0.1',
            'unrelated_mqb_files':'byte-identical','standalone_included':False,
            'physical_g11_acceptance':False,'modified_owned_files':changes}
    (a.output/'package-audit.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print('G11_FULL_MQB_STAGED profiles=7 existing_five_unchanged=1 standalone=0 '+str(output))

if __name__=='__main__': main()
