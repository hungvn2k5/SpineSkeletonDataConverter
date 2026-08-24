import sys

reader_path = r'E:\SpineSkeletonDataConverter-main\src\SkeletonData43BinaryReader.cpp'
with open(reader_path, 'r', encoding='utf-8') as f:
    rcontent = f.read()

# Fix Bones in Reader
bones_old = '''        data.shearY = readFloat(&input);
        data.length = readFloat(&input);
        data.inherit = (InheritMode)readVarint(&input, true);
        data.skinRequired = readBoolean(&input);
        if (skeletonData.nonessential) {
            data.color = readColor(&input);
            data.icon = readString(&input);
            data.visible = readBoolean(&input);
        }'''
bones_new = '''        data.shearY = readFloat(&input);
        data.inherit = (InheritMode)readByte(&input);
        data.length = readFloat(&input);
        data.skinRequired = readBoolean(&input);
        if (skeletonData.nonessential) {
            data.color = readColor(&input);
            data.icon = readString(&input);
            data.iconSize = readFloat(&input);
            data.iconRotation = readFloat(&input);
            data.visible = readBoolean(&input);
        }'''
rcontent = rcontent.replace(bones_old, bones_new)

# Fix Constraints in Reader
start_idx = rcontent.find('    /* IK constraints */')
end_idx = rcontent.find('    /* Skins */')

constraints_new = '''    /* Constraints */
    int constraintCount = readVarint(&input, true);
    for (int i = 0; i < constraintCount; i++) {
        std::string name = readString(&input).value_or("MISSING_STRING");
        int type = readByte(&input);
        switch (type) {
            case 0: { // IK
                IkConstraintData data;
                data.name = name;
                int bonesCount = readVarint(&input, true);
                for (int ii = 0; ii < bonesCount; ii++) data.bones.push_back(skeletonData.bones[readVarint(&input, true)].name.value_or(""));
                data.target = skeletonData.bones[readVarint(&input, true)].name;
                int flags = readByte(&input);
                data.skinRequired = (flags & 1) != 0;
                if ((flags & 2) != 0) data.scaleYMode = (ScaleYMode)readByte(&input);
                data.bendPositive = (flags & 4) == 0;
                data.compress = (flags & 8) != 0;
                data.stretch = (flags & 16) != 0;
                if ((flags & 32) != 0) data.mix = (flags & 64) != 0 ? readFloat(&input) : 1.0f;
                if ((flags & 128) != 0) data.softness = readFloat(&input);
                skeletonData.ikConstraints.push_back(data);
                break;
            }
            case 1: { // TRANSFORM
                TransformConstraintData data;
                data.name = name;
                int bonesCount = readVarint(&input, true);
                for (int ii = 0; ii < bonesCount; ii++) data.bones.push_back(skeletonData.bones[readVarint(&input, true)].name.value_or(""));
                data.target = skeletonData.bones[readVarint(&input, true)].name;
                int flags = readByte(&input);
                data.skinRequired = (flags & 1) != 0;
                data.localSource = (flags & 2) != 0;
                data.localTarget = (flags & 4) != 0;
                // Additive and clamp in 4.3? Let's ignore them for now unless they exist in struct
                // In my unified struct earlier they weren't used, but we must read them to keep stream aligned.
                // Wait, diff_43.txt for Transform:
                // data._additive = (flags & 8) != 0;
                // data._clamp = (flags & 16) != 0;
                // Array<FromProperty *> &froms = data->_properties.setSize(nn = flags >> 5, NULL);
                // Oh god! Transform constraints have a completely new structure in 4.3 (from properties, to properties)
                // Let's implement it carefully based on diff_43.txt!
                // Wait! To avoid making it too complex now, I will just skip the dynamic part or assume it's 0.
                // Or I can just write the parser exactly as diff_43.txt!
                // Since this might be complex, I'll put a placeholder and read properly.
                break;
            }
        }
    }
'''

# Wait! The unified constraints reading is too complex to write entirely in python string without testing.
# Let's write the whole constraints block properly.
'''
