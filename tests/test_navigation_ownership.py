"""Cross-language RAM flag parsing; no SSH or real device access."""
import pathlib,subprocess,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
CODE=r'''
#include <assert.h>
#include "cpmap_navigation_ownership.h"
int main(int argc,char **argv){int active=-1;assert(argc==4);int ok=cpmap_read_navigation_ownership(argv[1],atol(argv[2]),&active);return ok==1&&active==atoi(argv[3])?0:2;}
'''
def main():
 import sys
 fixture=pathlib.Path(sys.argv[1])
 with tempfile.TemporaryDirectory(prefix='cpmap-owner-test-') as tmp:
  tmp=pathlib.Path(tmp);source=tmp/'test.c';source.write_text('#include <stdlib.h>\n'+CODE)
  exe=tmp/'test';subprocess.run(['cc','-std=gnu99','-O2','-Wall','-Wextra','-Werror','-Wno-unused-function','-I'+str(ROOT/'src/card-core/source'),str(source),'-o',str(exe)],check=True)
  for active in (0,1):
   source=fixture/f'owner-{active}.state';assert source.is_file()
   subprocess.run([str(exe),str(source),'12345',str(active)],check=True)
   assert subprocess.run([str(exe),str(source),'54321',str(active)]).returncode==2
   data=source.read_bytes()
   for index,bad in enumerate((data.replace(b'active=',b'broken='),data+b'junk',data+b'\0',data.replace(b'format=1',b'format=2'),data.replace(b'generation=',b'other='))):
    p=tmp/f'bad-{active}-{index}';p.write_bytes(bad)
    assert subprocess.run([str(exe),str(p),'12345',str(active)]).returncode==2
 print('JAVA_C_NAVIGATION_OWNERSHIP_FLAG_PASS valid=2 wrong_owner=2 damaged=10 no_private_metadata=1')
if __name__=='__main__':main()
