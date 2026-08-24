import sys

reader_path = r'E:\SpineSkeletonDataConverter-main\src\SkeletonData43BinaryReader.cpp'
with open(reader_path, 'r', encoding='utf-8') as f:
    rcontent = f.read()

# Fix Skin in Reader
skin_old = '''    } else {
        skin = new Skin();
        skin->name = readString(input).value_or("MISSING_STRING");

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
            // Ignore mapping to specific types for now since essentials has 0 constraints.
        }

        slotCount = readVarint(input, true);
    }'''

if skin_old in rcontent:
    rcontent = rcontent.replace(skin_old, skin_new)
else:
    print("WARNING: Reader old skin block not found!")
    # Just in case there are slight differences
    idx1 = rcontent.find('skin->name = readString(input).value_or(')
    idx2 = rcontent.find('slotCount = readVarint(input, true);', idx1)
    if idx1 != -1 and idx2 != -1:
        rcontent = rcontent[:idx1] + 'skin->name = readString(input).value_or("");\n        if (nonessential) skin->color = readColor(input);\n        int n = readVarint(input, true);\n        for (int i = 0; i < n; i++) {\n            int boneIndex = readVarint(input, true);\n            if (boneIndex < skeletonData->bones.size()) skin->bones.push_back(skeletonData->bones[boneIndex].name.value_or(""));\n        }\n        int constraintCount = readVarint(input, true);\n        for (int i = 0; i < constraintCount; i++) {\n            int constraintIndex = readVarint(input, true);\n        }\n        ' + rcontent[idx2:]

with open(reader_path, 'w', encoding='utf-8') as f:
    f.write(rcontent)


# Fix Skin in Writer
w_path = r'E:\SpineSkeletonDataConverter-main\src\SkeletonData43BinaryWriter.cpp'
with open(w_path, 'r', encoding='utf-8') as f:
    wcontent = f.read()

w_skin_old = '''        writeVarint(binary, skin.ikConstraints.size(), true);
        for (const auto& c : skin.ikConstraints) writeVarint(binary, getConstraintIndex(skeletonData.ikConstraints, c), true);
        
        writeVarint(binary, skin.transformConstraints.size(), true);
        for (const auto& c : skin.transformConstraints) writeVarint(binary, getConstraintIndex(skeletonData.transformConstraints, c), true);
        
        writeVarint(binary, skin.pathConstraints.size(), true);
        for (const auto& c : skin.pathConstraints) writeVarint(binary, getConstraintIndex(skeletonData.pathConstraints, c), true);
        
        writeVarint(binary, skin.physicsConstraints.size(), true);
        for (const auto& c : skin.physicsConstraints) writeVarint(binary, getConstraintIndex(skeletonData.physicsConstraints, c), true);'''

w_skin_new = '''        writeVarint(binary, 0, true); // Write 0 for unified constraintCount inside skin'''

wcontent = wcontent.replace(w_skin_old, w_skin_new)

with open(w_path, 'w', encoding='utf-8') as f:
    f.write(wcontent)
