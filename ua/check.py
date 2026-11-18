import sys, re, glob, collections, os
W = os.path.dirname(os.path.abspath(__file__))
ph = re.compile(r'\{[^}]*\}|%[-+0-9.]*[a-zA-Z%]\d?|\\n|\\"|\\t')
bad = 0; total = 0; chars = collections.Counter(); uk_all = []
for n in range(1, 7):
    ru = [l.rstrip('\r\n') for l in open(f'{W}/ru_parts/part{n}.txt', encoding='utf-8') if l.strip()]
    uk = [l.rstrip('\r\n') for l in open(f'{W}/uk_parts/part{n}.txt', encoding='utf-8-sig') if l.strip()]
    if len(ru) != len(uk):
        print('COUNT MISMATCH part', n, len(ru), len(uk)); bad += 1; continue
    for a, b in zip(ru, uk):
        ia, ka, va = a.split('\t', 2); p = b.split('\t', 2)
        if len(p) != 3 or p[0] != ia or p[1] != ka:
            print('KEY', n, a[:60], '|', b[:60]); bad += 1; continue
        vb = p[2]
        if sorted(ph.findall(va)) != sorted(ph.findall(vb)):
            print('PH', n, ia, va[:50], '|', vb[:50]); bad += 1
        if re.search(r'(?<!\\)"', vb):
            print('QUOTE', n, ia, vb[:60]); bad += 1
        if re.search('[А-Яа-я]', va) and vb == va:
            print('UNCHANGED', n, ia, vb[:60])
        chars.update(vb); uk_all.append(vb); total += 1
print('checked', total, 'problems', bad)
ok = set("абвгдеёжзийклмнопрстуфхцчшщъыьэюяАБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ") | set(map(chr, range(32, 127))) | set('іїєґІЇЄҐ’ʼ')
print('unusual chars:', {c: k for c, k in chars.items() if c not in ok})
txt = '\n'.join(uk_all)
for pat in sys.argv[1:]:
    print(pat, len(re.findall(pat, txt)))
