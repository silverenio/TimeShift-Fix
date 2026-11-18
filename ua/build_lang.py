"""Build preload/paks/patch.s3darc with English strings replaced by Russian (ru) or Ukrainian (uk).
Usage: python build_lang.py ru|uk|en <path to original patch.s3darc> <output patch.s3darc>"""
import sys, os, re, json, glob
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import s3d, zlib
HERE = os.path.dirname(os.path.abspath(__file__))
# Letters missing from the game's 8-bit font table -> closest available glyphs
UK_MAP = str.maketrans({'і':'i','І':'I','ї':'ï','Ї':'ï','є':'е','Є':'Е','ґ':'г','Ґ':'Г','’':"'",'ʼ':"'"})

def uk_text():
    lines = json.load(open(os.path.join(HERE, 'ru_lines.json'), encoding='utf-8'))['lines']
    rx = re.compile(r'^(\S+|"[^"]*")(\s+)"(.*)"\s*$')
    n = 0
    for f in sorted(glob.glob(os.path.join(HERE, 'uk_parts', 'part*.txt'))):
        for ln in open(f, encoding='utf-8'):
            ln = ln.rstrip('\r\n')
            if not ln: continue
            i, key, val = ln.split('\t', 2)
            i = int(i); m = rx.match(lines[i])
            assert m and m.group(1) == key, (i, key)
            lines[i] = f'{m.group(1)}{m.group(2)}"{val.translate(UK_MAP)}"'
            n += 1
    print('ukrainian entries applied:', n)
    return '\r\n'.join(lines)

def main(mode, orig, out):
    a = s3d.read(orig)
    if mode == 'en':
        open(out, 'wb').write(a['d']); return
    if mode == 'ru':
        E = {(e[0], e[1]): e for e in a['ents']}
        E[('strings_english', 14)][2:6] = E[('strings_russian', 14)][2:6]
        s3d.write_index(a, out); return
    raw = b'\xff\xfe' + uk_text().encode('utf-16-le')
    s3d.build_override(orig, out, {'strings_english': raw})

if __name__ == '__main__':
    main(*sys.argv[1:4])
