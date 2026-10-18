"""Restore fonts into a private local executable from your own verified disc."""
from pathlib import Path
import argparse,hashlib,json,struct

def restore(template_dir,disc,output):
    root=Path(template_dir);output=Path(output)
    if output.exists():raise ValueError('Output already exists; choose a new private output file')
    manifest=json.loads((root/'build-template.json').read_text())
    data=bytearray((root/'melee.exe.template').read_bytes())
    sha=lambda b:hashlib.sha256(b).hexdigest()
    if sha(data)!=manifest['template_sha256']:raise ValueError('Template hash mismatch')
    with Path(disc).open('rb') as f:
        f.seek(0x420);base=struct.unpack('>I',f.read(4))[0];f.seek(base);header=f.read(256)
        offsets=struct.unpack_from('>18I',header);addresses=struct.unpack_from('>18I',header,0x48);sizes=struct.unpack_from('>18I',header,0x90)
        length=max(o+s for o,s in zip(offsets,sizes) if s)
        if length>16*1024*1024:raise ValueError('Unexpected disc executable size')
        f.seek(base);dol=f.read(length)
    if hashlib.sha1(dol).hexdigest()!='08e0bf20134dfcb260699671004527b2d6bb1a45':raise ValueError('Expected a USA revision2 disc executable')
    locations={'debug_font':(0x804088B8,0x1C00),'sislib_font':(0x8040CD40,0x23E00)}
    for region in manifest['regions']:
        address,length=locations[region['name']]
        o,a,s=next((o,a,s) for o,a,s in zip(offsets,addresses,sizes) if a<=address and address+length<=a+s)
        font=dol[o+address-a:o+address-a+length];at=region['offset']
        if length!=region['length'] or at<0 or at+length>len(data) or any(data[at:at+length]) or sha(font)!=region['sha256']:raise ValueError('Font region validation failed')
        data[at:at+length]=font
    if sha(data)!=manifest['local_restored_executable_sha256']:raise ValueError('Restored executable hash mismatch')
    output.parent.mkdir(parents=True,exist_ok=True)
    with output.open('xb') as f:f.write(data)
    print('Prepared private executable. Do not upload this file: it contains your disc fonts.')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--template-dir',type=Path,default=Path(__file__).resolve().parent);p.add_argument('--disc',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    args=p.parse_args();restore(args.template_dir,args.disc,args.output)
