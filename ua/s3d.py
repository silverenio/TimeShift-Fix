import struct,zlib
def chunk(d,o):
    u,c=struct.unpack_from('<II',d,o); return zlib.decompress(d[o+8:o+8+c]), o+8+c
def read(path):
    d=open(path,'rb').read()
    ver,o=chunk(d,0); base,o=chunk(d,o)
    idxoff=struct.unpack_from('<I',d,o)[0]; dataoff=o+4
    idx,_=chunk(d,idxoff)
    n=struct.unpack_from('<I',idx,0)[0]; p=4; ents=[]
    while p<len(idx):
        L=struct.unpack_from('<I',idx,p)[0]; name=idx[p+4:p+4+L].decode('latin-1'); p+=4+L
        t,off,h,us,cs=struct.unpack_from('<5I',idx,p); p+=20
        ents.append([name,t,off,h,us,cs])
    return dict(d=d,ver=ver,base=base,idxoff=idxoff,dataoff=dataoff,count=n,ents=ents,hdr=d[:o])

def write_index(a, path):
    """Rewrite the index at a['idxoff'] (keeps data/header); index chunk = [usize][csize][zlib]."""
    out=bytearray(struct.pack('<I',a['count']))
    for name,t,off,h,us,cs in a['ents']:
        nb=name.encode('latin-1'); out+=struct.pack('<I',len(nb))+nb+struct.pack('<5I',t,off,h,us,cs)
    z=zlib.compress(bytes(out),9)
    data=a['d'][:a['idxoff']]+struct.pack('<II',len(out),len(z))+z
    open(path,'wb').write(data)

def build_override(orig_path, out_path, overrides):
    """overrides: {entry_name: raw_bytes} -> appended to data, index updated. type 14 (strings) only."""
    a=read(orig_path); d=a['d']; idxoff=a['idxoff']
    data=bytearray(d[:idxoff])
    for key,raw in overrides.items():
        name,typ=(key if isinstance(key,tuple) else (key,14))
        z=zlib.compress(raw,9); off=len(data); data+=z
        e=[x for x in a['ents'] if x[0]==name and x[1]==typ][0]
        e[2:6]=[off, zlib.crc32(raw)&0xffffffff, len(raw), len(z)]
    newidx=len(data)
    hdrpos=a['dataoff']-4
    data[hdrpos:hdrpos+4]=struct.pack('<I',newidx)
    a['d']=bytes(data); a['idxoff']=newidx
    write_index(a,out_path)
