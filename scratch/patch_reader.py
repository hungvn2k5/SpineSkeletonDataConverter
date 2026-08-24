import sys
import shutil
import re

reader_path = r'E:\SpineSkeletonDataConverter-main\src\SkeletonData43BinaryReader.cpp'
shutil.copy(r'E:\SpineSkeletonDataConverter-main\src\SkeletonData42BinaryReader.cpp', reader_path)

with open(reader_path, 'r', encoding='utf-8') as f:
    c = f.read()

c = c.replace('namespace spine42', 'namespace spine43')

c = re.sub(r'data\.shearY = readFloat\(&input\);\s*data\.length = readFloat\(&input\);\s*data\.inherit = \(InheritMode\)readVarint\(&input, true\);\s*data\.skinRequired = readBoolean\(&input\);\s*if \(skeletonData\.nonessential\) \{\s*data\.color = readColor\(&input\);\s*data\.icon = readString\(&input\)(?:\.value_or\(\"[^\"]*\"\))?;\s*data\.visible = readBoolean\(&input\);\s*\}', 
'''        data.shearY = readFloat(&input);
        data.inherit = (InheritMode)readByte(&input);
        data.length = readFloat(&input);
        data.skinRequired = readBoolean(&input);
        if (skeletonData.nonessential) {
            data.color = readColor(&input);
            data.icon = readString(&input).value_or("");
            data.iconSize = readFloat(&input);
            data.iconRotation = readFloat(&input);
            data.visible = readBoolean(&input);
        }''', c)

# constraints
start_idx = c.find('    /* IK constraints */')
end_idx = c.find('    /* Default skin */')
c = c[:start_idx] + '''    /* Constraints */
    int constraintCount = readVarint(&input, true);
    for (int i = 0; i < constraintCount; i++) {
        std::string name = readString(&input).value_or("");
        int type = readByte(&input);
        throw std::runtime_error("Constraints parsing not fully implemented for 4.3!");
    }

''' + c[end_idx:]

# skin
c = re.sub(r'int n = readVarint\(&input, true\);\s*for \(int i = 0; i < n; i\+\+\) \{\s*int boneIndex = readVarint\(&input, true\);\s*if \(boneIndex < skeletonData\.bones\.size\(\)\) skin\.bones\.push_back\(skeletonData\.bones\[boneIndex\]\.name(?:\.value_or\(\"[^\"]*\"\))?\);\s*\}.*?int slotCount = readVarint\(&input, true\);',
'''int n = readVarint(&input, true);
        for (int i = 0; i < n; i++) {
            int boneIndex = readVarint(&input, true);
            if (boneIndex < skeletonData.bones.size()) skin.bones.push_back(skeletonData.bones[boneIndex].name.value_or(""));
        }

        int constraintCount = readVarint(&input, true);
        for (int i = 0; i < constraintCount; i++) {
            int constraintIndex = readVarint(&input, true);
        }

        int slotCount = readVarint(&input, true);''', c, flags=re.DOTALL)

# folder
c = c.replace('    /* Events timeline */\n    int eventsCount = readVarint(&input, true);',
'''    /* Draw order folder timelines */
    int folderCount = readVarint(&input, true);
    if (folderCount > 0) throw std::runtime_error("Draw order folder timelines not implemented!");

    /* Events timeline */
    int eventsCount = readVarint(&input, true);''')

with open(reader_path, 'w', encoding='utf-8') as f:
    f.write(c)

print('Reader patched.')
