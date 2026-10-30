"""Cross-profile ABI rejection and preservation of foreign navigation JARs."""
from pathlib import Path
import subprocess,tempfile
from test_toolbox_install import package,navigation_payload,navigation_name,host_checker,fixture,digest,run

def main():
 checks=0
 for mu,size,other in [('MU1433',52751492,'MU1447')]:
  pkg=package(mu,'790');payload=pkg/'payload';nav=navigation_payload(payload,mu,'790')
  othernav=navigation_payload(package(other,'790')/'payload',other,'790')
  for testjar in (nav,othernav,payload/'common/CarPlayNavigationPatch.jar'):
   p=subprocess.run([host_checker(),'--g11-payload',mu,str(testjar)])
   assert (p.returncode==0)==(testjar==nav);checks+=1
  with tempfile.TemporaryDirectory(prefix='g11-negative-') as td:
   bad=Path(td)/'bad.jar';original=nav.read_bytes()
   for content in (original[:-1],bytes([original[0]^1])+original[1:],original+b'junk'):
    bad.write_bytes(content);assert subprocess.run([host_checker(),'--g11-payload',mu,str(bad)]).returncode!=0;checks+=1
  with tempfile.TemporaryDirectory(prefix='g11-foreign-nav-') as td:
   root=Path(td);dio,lsd,jars=fixture(root,size,'stock',nav)
   foreign=jars/'CarPlayNavigationPatch.jar';foreign.write_bytes((payload/'common/CarPlayNavigationPatch.jar').read_bytes())
   text=lsd.read_text().replace('$J9 $BOOTCLASSPATH Main','BOOTCLASSPATH="$BOOTCLASSPATH -Xbootclasspath/p:$BASE_DIR/lsd/jars/CarPlayNavigationPatch.jar"\n$J9 $BOOTCLASSPATH Main')
   lsd.write_text(text);before=(digest(dio),digest(lsd),digest(foreign))
   run(pkg/'install.sh',root,payload,mu,'790')
   assert digest(foreign)==before[2] and digest(jars/navigation_name(mu))==digest(nav);checks+=1
   assert lsd.read_text().index(navigation_name(mu))<lsd.read_text().index('CarPlayNavigationPatch.jar');checks+=1
   run(pkg/'restore-carplay-maps.sh',root)
   assert before==(digest(dio),digest(lsd),digest(foreign));checks+=1
   assert not (jars/navigation_name(mu)).exists();checks+=1
  with tempfile.TemporaryDirectory(prefix='g11-metadata-') as td:
   root=Path(td);dio,lsd,jars=fixture(root,size,'stock',nav)
   run(pkg/'install.sh',root,payload,mu,'790');before=(digest(dio),digest(lsd))
   (root/'mnt/app/carplaymaps/rollback/NAV_JAR_NAME').write_text('../../foreign')
   run(pkg/'install.sh',root,payload,mu,'790',ok=False)
   run(pkg/'restore-carplay-maps.sh',root,ok=False)
   assert before==(digest(dio),digest(lsd));checks+=1
 print('G11_NAV_ABI_FOREIGN_PRESERVATION_PASS checks='+str(checks))

if __name__=='__main__':main()
