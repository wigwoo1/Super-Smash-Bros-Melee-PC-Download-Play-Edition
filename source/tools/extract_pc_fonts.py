"""Generate local font includes from a checksum-verified user disc. Do not distribute outputs."""
import argparse, hashlib, pathlib, struct

def extract(image, output):
    with pathlib.Path(image).open('rb') as stream:
        stream.seek(0x420)
        base = struct.unpack('>I', stream.read(4))[0]
        stream.seek(base)
        header = stream.read(256)
        offsets = struct.unpack_from('>18I', header, 0)
        addresses = struct.unpack_from('>18I', header, 0x48)
        sizes = struct.unpack_from('>18I', header, 0x90)
        length = max(o+s for o,s in zip(offsets,sizes) if s)
        if length > 16*1024*1024:
            raise ValueError('Unexpected DOL size')
        stream.seek(base)
        dol = stream.read(length)
    if hashlib.sha1(dol).hexdigest() != '08e0bf20134dfcb260699671004527b2d6bb1a45':
        raise ValueError('DOL does not match USA revision 2; nothing written')
    for name, address, length, stride in [('debug_font',0x804088B8,0x1C00,56), ('sislib_font',0x8040CD40,0x23E00,512)]:
        segment = next(((o,a,s) for o,a,s in zip(offsets,addresses,sizes) if a <= address and address+length <= a+s), None)
        if segment is None:
            raise ValueError('Font range outside DOL segments')
        offset = segment[0] + address - segment[1]
        data = dol[offset:offset+length]
        target = pathlib.Path(output)/'sysdolphin/baselib'/f'{name}.inc'
        target.parent.mkdir(parents=True, exist_ok=True)
        rows = ['/* Generated from a user-supplied verified disc; not distributable source. */']
        for start in range(0,length,stride):
            rows.append('{{' + ','.join(f'0x{b:02x}' for b in data[start:start+stride]) + '}},')
        target.write_text('\n'.join(rows)+'\n')
        print(f'{name}: {length} bytes, {length//stride} glyphs')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('image')
    parser.add_argument('output')
    args = parser.parse_args()
    extract(args.image,args.output)
