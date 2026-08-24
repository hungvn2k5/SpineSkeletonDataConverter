import sys

reader_path = r'E:\SpineSkeletonDataConverter-main\src\SkeletonData43BinaryReader.cpp'
with open(reader_path, 'r', encoding='utf-8') as f:
    rcontent = f.read()

do_old = '''    }

    /* Events timeline */
    int eventsCount = readVarint(&input, true);'''

do_new = '''    }

    /* Draw order folder timelines */
    int folderCount = readVarint(&input, true);
    if (folderCount > 0) {
        throw std::runtime_error("Draw order folder timelines not implemented!");
    }

    /* Events timeline */
    int eventsCount = readVarint(&input, true);'''

if do_old in rcontent:
    rcontent = rcontent.replace(do_old, do_new)
else:
    print('Draw order old block not found! Trying alternative...')
    # Let's search for "/* Events timeline */" or "/* Events */" inside readAnimation
    do_old2 = '''    }

    /* Events */
    int eventsCount = readVarint(&input, true);'''
    if do_old2 in rcontent:
        rcontent = rcontent.replace(do_old2, do_new.replace('Events timeline', 'Events'))
    else:
        print('Alternative also not found.')

with open(reader_path, 'w', encoding='utf-8') as f:
    f.write(rcontent)

# Update writer
writer_path = r'E:\SpineSkeletonDataConverter-main\src\SkeletonData43BinaryWriter.cpp'
with open(writer_path, 'r', encoding='utf-8') as f:
    wcontent = f.read()

wdo_old = '''    }

    /* Events */
    writeVarint(binary, animation.events.size(), true);'''

wdo_new = '''    }

    /* Draw order folder timelines */
    writeVarint(binary, 0, true);

    /* Events */
    writeVarint(binary, animation.events.size(), true);'''

if wdo_old in wcontent:
    wcontent = wcontent.replace(wdo_old, wdo_new)
else:
    print('Writer Draw order old block not found!')

with open(writer_path, 'w', encoding='utf-8') as f:
    f.write(wcontent)
