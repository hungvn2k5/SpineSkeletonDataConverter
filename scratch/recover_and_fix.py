import sys
import shutil
import re

reader_path = r'E:\SpineSkeletonDataConverter-main\src\SkeletonData43BinaryReader.cpp'
shutil.copy(r'E:\SpineSkeletonDataConverter-main\src\SkeletonData42BinaryReader.cpp', reader_path)

with open(reader_path, 'r', encoding='utf-8') as f:
    c = f.read()

# Fix namespace
c = c.replace('namespace spine42', 'namespace spine43')

# Fix Bones
bones_old = '''        data.shearY = readFloat(&input);
        data.length = readFloat(&input);
        data.inherit = (InheritMode)readVarint(&input, true);
        data.skinRequired = readBoolean(&input);
        if (skeletonData.nonessential) {
            data.color = readColor(&input);
            data.icon = readString(&input).value_or("");
            data.visible = readBoolean(&input);
        }'''
bones_new = '''        data.shearY = readFloat(&input);
        data.inherit = (InheritMode)readByte(&input);
        data.length = readFloat(&input);
        data.skinRequired = readBoolean(&input);
        if (skeletonData.nonessential) {
            data.color = readColor(&input);
            data.icon = readString(&input).value_or("");
            data.iconSize = readFloat(&input);
            data.iconRotation = readFloat(&input);
            data.visible = readBoolean(&input);
        }'''
if bones_old in c:
    c = c.replace(bones_old, bones_new)
else:
    print("Failed to patch Bones!")
    sys.exit(1)

# Fix Constraints (replace IK constraints to just before Default skin)
start_idx = c.find('    /* IK constraints */')
end_idx = c.find('    /* Default skin */')
if start_idx == -1 or end_idx == -1:
    print("Failed to find Constraints bounds!")
    sys.exit(1)

constraints_new = '''    /* Constraints */
    int constraintCount = readVarint(&input, true);
    for (int i = 0; i < constraintCount; i++) {
        std::string name = readString(&input).value_or("");
        int type = readByte(&input);
        // Minimal skip since spineboy-ess has 0 constraints.
        // We will just throw if there are any constraints to ensure we know.
        throw std::runtime_error("Constraints parsing not fully implemented for 4.3!");
    }

'''
c = c[:start_idx] + constraints_new + c[end_idx:]

# Fix Skin
skin_old = '''    } else {
        skin = new Skin();
        skin->name = readString(input).value_or("");

        if (nonessential) skin->color = readColor(input);

        int n = readVarint(input, true);
        for (int i = 0; i < n; i++) {
            int boneIndex = readVarint(input, true);
            if (boneIndex < skeletonData->bones.size()) skin->bones.push_back(skeletonData->bones[boneIndex].name.value_or(""));
        }

        n = readVarint(input, true);
        for (int i = 0; i < n; i++) {
            int ikIndex = readVarint(input, true);
            if (ikIndex < skeletonData->ikConstraints.size()) skin->ikConstraints.push_back(skeletonData->ikConstraints[ikIndex].name);
        }

        n = readVarint(input, true);
        for (int i = 0; i < n; i++) {
            int transformIndex = readVarint(input, true);
            if (transformIndex < skeletonData->transformConstraints.size()) skin->transformConstraints.push_back(skeletonData->transformConstraints[transformIndex].name);
        }

        n = readVarint(input, true);
        for (int i = 0; i < n; i++) {
            int pathIndex = readVarint(input, true);
            if (pathIndex < skeletonData->pathConstraints.size()) skin->pathConstraints.push_back(skeletonData->pathConstraints[pathIndex].name);
        }

        n = readVarint(input, true);
        for (int i = 0; i < n; i++) {
            int physicsIndex = readVarint(input, true);
            if (physicsIndex < skeletonData->physicsConstraints.size()) skin->physicsConstraints.push_back(skeletonData->physicsConstraints[physicsIndex].name);
        }

        slotCount = readVarint(input, true);
    }'''

skin_new = '''    } else {
        skin = new Skin();
        skin->name = readString(input).value_or("");

        if (nonessential) skin->color = readColor(input);

        int n = readVarint(input, true);
        for (int i = 0; i < n; i++) {
            int boneIndex = readVarint(input, true);
            if (boneIndex < skeletonData->bones.size()) skin->bones.push_back(skeletonData->bones[boneIndex].name.value_or(""));
        }

        int constraintCount = readVarint(input, true);
        for (int i = 0; i < constraintCount; i++) {
            int constraintIndex = readVarint(input, true);
        }

        slotCount = readVarint(input, true);
    }'''

if skin_old in c:
    c = c.replace(skin_old, skin_new)
else:
    print("Failed to patch Skin!")
    sys.exit(1)

# Fix Draw order folder timelines in Animation
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

if do_old in c:
    c = c.replace(do_old, do_new)
else:
    print("Failed to patch Draw order folders!")
    sys.exit(1)

with open(reader_path, 'w', encoding='utf-8') as f:
    f.write(c)

print("Reader patched successfully!")
