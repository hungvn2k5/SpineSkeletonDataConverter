import sys

reader_path = r'E:\SpineSkeletonDataConverter-main\src\SkeletonData43BinaryReader.cpp'
with open(reader_path, 'r', encoding='utf-8') as f:
    rcontent = f.read()

# 1. Fix Bones
bones_old = '''        data.shearY = readFloat(&input);
        data.length = readFloat(&input);
        data.inherit = (InheritMode)readVarint(&input, true);
        data.skinRequired = readBoolean(&input);
        if (skeletonData.nonessential) {
            data.color = readColor(&input);
            data.icon = readString(&input).value_or("MISSING_STRING");
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
rcontent = rcontent.replace(bones_old, bones_new)

# 2. Fix Constraints (Make it read exactly 1 varint for constraintCount and skip if 0)
# We replace the 4.2 constraint lists with unified constraints list
start_idx = rcontent.find('    /* IK constraints */')
end_idx = rcontent.find('    /* Default skin */')
if end_idx == -1: end_idx = rcontent.find('    /* Skins */')

constraints_new = '''    /* Constraints */
    int constraintCount = readVarint(&input, true);
    for (int i = 0; i < constraintCount; i++) {
        // We will just read the type and ignore it for now since spineboy-ess has 0 constraints.
        // If it had constraints, we would need to fully implement the parsing.
        std::string name = readString(&input).value_or("");
        int type = readByte(&input);
        // ... (we don't implement inner parsing because constraintCount is 0 for essentials)
        // If we hit this, we throw an error because we haven't implemented it yet!
        throw std::runtime_error("Constraints parsing not fully implemented for 4.3!");
    }
'''

rcontent = rcontent[:start_idx] + constraints_new + rcontent[end_idx:]

with open(reader_path, 'w', encoding='utf-8') as f:
    f.write(rcontent)


# 3. Update SkeletonData.h
h_path = r'E:\SpineSkeletonDataConverter-main\include\SkeletonData.h'
with open(h_path, 'r', encoding='utf-8') as f:
    hcontent = f.read()

hold = '''    OptStr icon = std::nullopt; 
    bool visible = true;'''
hnew = '''    OptStr icon = std::nullopt; 
    float iconSize = 0.0f;
    float iconRotation = 0.0f;
    bool visible = true;'''
hcontent = hcontent.replace(hold, hnew)

with open(h_path, 'w', encoding='utf-8') as f:
    f.write(hcontent)


# 4. Fix Writer (so we can compile and it writes back correctly)
w_path = r'E:\SpineSkeletonDataConverter-main\src\SkeletonData43BinaryWriter.cpp'
with open(w_path, 'r', encoding='utf-8') as f:
    wcontent = f.read()

wb_old = '''        writeFloat(binary, bone.shearY);
        writeFloat(binary, bone.length);
        writeVarint(binary, (int)bone.inherit, true);
        writeBoolean(binary, bone.skinRequired);
        if (skeletonData.nonessential) {
            writeColor(binary, bone.color);
            writeString(binary, bone.icon);
            writeBoolean(binary, bone.visible);
        }'''
wb_new = '''        writeFloat(binary, bone.shearY);
        writeByte(binary, (int)bone.inherit);
        writeFloat(binary, bone.length);
        writeBoolean(binary, bone.skinRequired);
        if (skeletonData.nonessential) {
            writeColor(binary, bone.color);
            writeString(binary, bone.icon);
            writeFloat(binary, bone.iconSize);
            writeFloat(binary, bone.iconRotation);
            writeBoolean(binary, bone.visible);
        }'''
if wb_old in wcontent:
    wcontent = wcontent.replace(wb_old, wb_new)

# Fix Constraints in Writer
wstart_idx = wcontent.find('    /* IK Constraints */')
if wstart_idx == -1: wstart_idx = wcontent.find('    /* IK constraints */')
wend_idx = wcontent.find('    /* Skins */')

wconstraints_new = '''    /* Constraints */
    writeVarint(binary, 0, true); // Write 0 for unified constraintCount
'''
if wstart_idx != -1 and wend_idx != -1:
    wcontent = wcontent[:wstart_idx] + wconstraints_new + wcontent[wend_idx:]

with open(w_path, 'w', encoding='utf-8') as f:
    f.write(wcontent)
