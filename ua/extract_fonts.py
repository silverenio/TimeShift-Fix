"""Extract the original Cyrillic font textures from the game's out.s3darc into this folder.
Usage: python extract_fonts.py "<Steam>/steamapps/common/TimeShift/preload/paks/out.s3darc" """
import sys, os, zlib
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import s3d

a = s3d.read(sys.argv[1])
for name in ('font_main_cyr', 'font_regular_cyr'):
    e = [x for x in a['ents'] if x[0] == name and x[1] == 6][0]
    raw = zlib.decompress(a['d'][e[2]:e[2] + e[5]])
    open(os.path.join(os.path.dirname(os.path.abspath(__file__)), name + '.tex'), 'wb').write(raw)
    print(name, len(raw))
