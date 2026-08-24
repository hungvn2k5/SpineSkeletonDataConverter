import sys
import re

writer_path = r'E:\SpineSkeletonDataConverter-main\src\SkeletonData43BinaryWriter.cpp'
with open(writer_path, 'r', encoding='utf-8') as f:
    w = f.read()

# Fix Bones
w = re.sub(r'writeFloat\(binary, bone\.shearY\);\s*writeFloat\(binary, bone\.length\);\s*writeVarint\(binary, \(int\)bone\.inherit, true\);\s*writeBoolean\(binary, bone\.skinRequired\);\s*if \(skeletonData\.nonessential\) \{\s*writeColor\(binary, bone\.color\);\s*writeString\(binary, bone\.icon\);\s*writeBoolean\(binary, bone\.visible\);\s*\}',
'''        writeFloat(binary, bone.shearY);
        writeByte(binary, (int)bone.inherit);
        writeFloat(binary, bone.length);
        writeBoolean(binary, bone.skinRequired);
        if (skeletonData.nonessential) {
            writeColor(binary, bone.color);
            writeString(binary, bone.icon);
            writeFloat(binary, bone.iconSize);
            writeFloat(binary, bone.iconRotation);
            writeBoolean(binary, bone.visible);
        }''', w)

# Fix Constraints
start_idx = w.find('    /* IK Constraints */')
if start_idx == -1: start_idx = w.find('    /* IK constraints */')
end_idx = w.find('    /* Skins */')
w = w[:start_idx] + '''    /* Constraints */
    writeVarint(binary, 0, true);

''' + w[end_idx:]

# Fix Skin
w = re.sub(r'writeVarint\(binary, skin\.ikConstraints\.size\(\), true\).*?writeVarint\(binary, skin\.physicsConstraints\.size\(\), true\);\s*for \(const auto& c : skin\.physicsConstraints\) writeVarint\(binary, getConstraintIndex\(skeletonData\.physicsConstraints, c\), true\);',
'''writeVarint(binary, 0, true);''', w, flags=re.DOTALL)

# Fix Draw order folder timelines
w = w.replace('    /* Events */\n    writeVarint(binary, animation.events.size(), true);',
'''    /* Draw order folder timelines */
    writeVarint(binary, 0, true);

    /* Events */
    writeVarint(binary, animation.events.size(), true);''')

with open(writer_path, 'w', encoding='utf-8') as f:
    f.write(w)

print('Writer patched.')
