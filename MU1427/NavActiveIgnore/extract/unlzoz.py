import sys,struct,ctypes
lz=ctypes.CDLL('liblzo2.so.2')
lz.__lzo_init_v2.restype=ctypes.c_int
d=open(sys.argv[1],'rb').read()
print(d[:4], hex(struct.unpack_from('<I',d,4)[0]))
cs=struct.unpack_from('<I',d,4)[0]
sizes=[];o=8
while True:
    v=struct.unpack_from('<I',d,o)[0];o+=4
    if v==0:break
    sizes.append(v)
print(len(sizes))
pos=0x800; out=bytearray()
for s in sizes:
    src=d[pos:pos+s]
    dst=ctypes.create_string_buffer(cs); dl=ctypes.c_ulong(cs)
    r=lz.lzo1z_decompress_safe(src,ctypes.c_ulong(s),dst,ctypes.byref(dl),None)
    if r!=0: print('err',r,pos,s); 
    out+=dst.raw[:dl.value]
    pos+=s
    # padding 0xFF?
    while pos<len(d) and d[pos]==0xFF : pos+=1
open(sys.argv[2],'wb').write(out); print(len(out))
