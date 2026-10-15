import sys,struct,os
d=open(sys.argv[1],'rb').read()
b=d.find(b'imagefs')
img_size,hdr_dir_size,dir_off=struct.unpack_from('<III',d,b+8)
print(hex(b),img_size,hdr_dir_size,dir_off)
o=b+dir_off;end=b+hdr_dir_size
files={}
while o<end:
    sz,ext,ino,mode,gid,uid,mt=struct.unpack_from('<HHIIIII',d,o)
    if sz==0:break
    t=mode&0o170000
    if t==0o100000:
        off,fsz=struct.unpack_from('<II',d,o+24); name=d[o+32:o+sz].split(b'\0')[0].decode()
        files[name]=(b+off,fsz)
    o+=sz
for n,(off,sz) in sorted(files.items()):
    if len(sys.argv)>2 and n in sys.argv[2:]:
        open(os.path.basename(n),'wb').write(d[off:off+sz]); print('wrote',n,sz)
print(len(files)); [print(n,s) for n,(o_,s) in files.items() if 'jxe' in n or 'jre' in n[:4]][:20]
