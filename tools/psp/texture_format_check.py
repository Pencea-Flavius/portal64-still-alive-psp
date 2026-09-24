#!/usr/bin/env python3
# Checks skeletool64's PSP texture output against a copy made before a change
# to it: every texture must decode to the same texels at level 0, whatever
# format it is written in now. A texture still in its old format must be
# written byte for byte as before.
#
#     tools/psp/texture_format_check.py <old codegen dir> <new codegen dir>
#
# Both directories are copies of build/psp/codegen (only the .c files with a
# struct PspTexture in them matter).

import os
import re
import sys

ARRAY = re.compile(r'(unsigned (?:char|short|int)) __attribute__\(\(aligned\(16\)\)\) (\w+)\[\] = \{(.*?)\};', re.S)
LEVELS = re.compile(r'const void\* (\w+)\[\] = \{(.*?)\};', re.S)
TEXTURE = re.compile(r'struct PspTexture (\w+) = \{(.*?)\};', re.S)
UNIT = {'unsigned char': 1, 'unsigned short': 2, 'unsigned int': 4}
BITS = {'GU_PSM_T4': 4, 'GU_PSM_T8': 8, 'GU_PSM_5551': 16, 'GU_PSM_8888': 32}


def parse(path):
    text = open(path).read()
    arrays = {}

    for match in ARRAY.finditer(text):
        unit = UNIT[match.group(1)]
        data = bytearray()

        for value in re.findall(r'0x[0-9a-f]+', match.group(3)):
            data += int(value, 16).to_bytes(unit, 'little')

        arrays[match.group(2)] = (match.group(3), bytes(data))

    levels = {m.group(1): [v.strip() for v in m.group(2).split(',') if v.strip()] for m in LEVELS.finditer(text)}
    textures = {}

    for match in TEXTURE.finditer(text):
        fields = [v.strip() for v in match.group(2).split(',') if v.strip()]
        textures[match.group(1)] = {
            'levels': levels[fields[0]],
            'width': int(fields[2]),
            'height': int(fields[3]),
            'format': fields[4],
            'swizzled': fields[5] == '1',
            'clut': fields[6] if len(fields) > 6 and fields[6] != '0' else None,
        }

    return arrays, textures


def min_buffer_width(bits):
    return max(8, 128 // bits)


def decode_level0(texture, arrays):
    bits = BITS[texture['format']]
    width, height = texture['width'], texture['height']
    buffer_width = max(min_buffer_width(bits), width)
    row_bytes = buffer_width * bits // 8
    data = arrays[texture['levels'][0]][1]

    if texture['swizzled']:
        linear = bytearray(len(data))
        offset = 0

        for block_y in range(height // 8):
            for block_x in range(row_bytes // 16):
                for row in range(8):
                    start = (block_y * 8 + row) * row_bytes + block_x * 16
                    linear[start:start + 16] = data[offset:offset + 16]
                    offset += 16

        data = bytes(linear)

    clut = None

    if texture['clut']:
        raw = arrays[texture['clut']][1]
        clut = [int.from_bytes(raw[i:i + 4], 'little') for i in range(0, len(raw), 4)]

    texels = []

    for y in range(height):
        row = data[y * row_bytes:(y + 1) * row_bytes]

        for x in range(width):
            if bits == 4:
                value = (row[x // 2] >> ((x & 1) * 4)) & 0xF
            else:
                value = int.from_bytes(row[x * bits // 8:(x + 1) * bits // 8], 'little')

            texels.append(clut[value] if clut else value)

    return bits, texels


def to_5551(abgr):
    r, g, b, a = abgr & 0xFF, (abgr >> 8) & 0xFF, (abgr >> 16) & 0xFF, abgr >> 24
    return (0x8000 if a >= 0x80 else 0) | ((b >> 3) << 10) | ((g >> 3) << 5) | (r >> 3)


def main(old_dir, new_dir):
    failures = 0
    counts = {}
    sizes = {'old': 0, 'new': 0}

    for root, _, files in os.walk(new_dir):
        for name in files:
            if not name.endswith('.c'):
                continue

            new_path = os.path.join(root, name)
            old_path = os.path.join(old_dir, os.path.relpath(new_path, new_dir))

            if not os.path.exists(old_path):
                continue

            old_arrays, old_textures = parse(old_path)
            new_arrays, new_textures = parse(new_path)

            for texture_name, new in new_textures.items():
                old = old_textures.get(texture_name)

                if not old:
                    continue

                counts[old['format'] + ' -> ' + new['format']] = counts.get(old['format'] + ' -> ' + new['format'], 0) + 1
                sizes['old'] += sum(len(old_arrays[level][1]) for level in old['levels'])
                sizes['new'] += sum(len(new_arrays[level][1]) for level in new['levels'])
                sizes['new'] += len(new_arrays[new['clut']][1]) if new['clut'] else 0

                if new['format'] == old['format']:
                    for old_level, new_level in zip(old['levels'], new['levels']):
                        if old_arrays[old_level][0] != new_arrays[new_level][0] or len(old['levels']) != len(new['levels']):
                            print('changed while keeping its format:', texture_name)
                            failures += 1
                            break
                    continue

                old_bits, old_texels = decode_level0(old, old_arrays)
                _, new_texels = decode_level0(new, new_arrays)

                if old_bits == 16:
                    new_texels = [to_5551(texel) for texel in new_texels]

                if old_texels != new_texels:
                    bad = sum(1 for a, b in zip(old_texels, new_texels) if a != b)
                    print('level 0 differs:', texture_name, old['format'], '->', new['format'], bad, 'texels')
                    failures += 1

    for change, count in sorted(counts.items()):
        print(f'{count:4d}  {change}')

    print(f"texture bytes: {sizes['old']} -> {sizes['new']}")
    print('FAIL' if failures else 'OK')
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1], sys.argv[2]))
