import sys
import re

reader_path = r'E:\SpineSkeletonDataConverter-main\src\SkeletonData43BinaryReader.cpp'
with open(reader_path, 'r', encoding='utf-8') as f:
    reader = f.read()

# Replace unified constraints parsing with separate loops in reader
start_idx = reader.find('    /* Constraints */')
end_idx = reader.find('    /* Skins */')

new_reader = '''    /* IK constraints */
    int ikCount = readVarint(&input, true);
    for (int i = 0; i < ikCount; i++) {
        IkConstraintData ikData;
        ikData.name = readString(&input).value();
        ikData.order = readVarint(&input, true);
        int bonesCount = readVarint(&input, true);
        for (int ii = 0; ii < bonesCount; ii++) {
            ikData.bones.push_back(skeletonData.bones[readVarint(&input, true)].name.value());
        }
        ikData.target = skeletonData.bones[readVarint(&input, true)].name;
        int flags = readByte(&input);
        ikData.skinRequired = (flags & 1) != 0;
        ikData.bendDirection = (flags & 2) != 0 ? 1 : -1;
        ikData.compress = (flags & 4) != 0;
        ikData.stretch = (flags & 8) != 0;
        ikData.uniform = (flags & 16) != 0;
        ikData.mix = (flags & 32) != 0 ? ((flags & 64) != 0 ? readFloat(&input) : 1.0f) : 1.0f;
        if ((flags & 128) != 0) ikData.softness = readFloat(&input);
        skeletonData.ikConstraints.push_back(ikData);
    }

    /* Transform constraints */
    int transformCount = readVarint(&input, true);
    for (int i = 0; i < transformCount; i++) {
        TransformConstraintData data;
        data.name = readString(&input).value();
        data.order = readVarint(&input, true);
        int bonesCount = readVarint(&input, true);
        for (int ii = 0; ii < bonesCount; ii++) {
            data.bones.push_back(skeletonData.bones[readVarint(&input, true)].name.value());
        }
        data.target = skeletonData.bones[readVarint(&input, true)].name;
        int flags = readByte(&input);
        data.skinRequired = (flags & 1) != 0;
        data.localSource = (flags & 2) != 0;
        data.localTarget = (flags & 4) != 0;
        if ((flags & 8) != 0) data.offsetRotation = readFloat(&input);
        if ((flags & 16) != 0) data.offsetX = readFloat(&input);
        if ((flags & 32) != 0) data.offsetY = readFloat(&input);
        if ((flags & 64) != 0) data.offsetScaleX = readFloat(&input);
        if ((flags & 128) != 0) data.offsetScaleY = readFloat(&input);
        flags = readByte(&input);
        if ((flags & 1) != 0) data.offsetShearY = readFloat(&input);
        if ((flags & 2) != 0) data.mixRotate = readFloat(&input);
        if ((flags & 4) != 0) data.mixX = readFloat(&input);
        if ((flags & 8) != 0) data.mixY = readFloat(&input);
        if ((flags & 16) != 0) data.mixScaleX = readFloat(&input);
        if ((flags & 32) != 0) data.mixScaleY = readFloat(&input);
        if ((flags & 64) != 0) data.mixShearY = readFloat(&input);
        skeletonData.transformConstraints.push_back(data);
    }

    /* Path constraints */
    int pathCount = readVarint(&input, true);
    for (int i = 0; i < pathCount; i++) {
        PathConstraintData pathData;
        pathData.name = readString(&input).value();
        pathData.order = readVarint(&input, true);
        pathData.skinRequired = readBoolean(&input);
        int bonesCount = readVarint(&input, true);
        for (int ii = 0; ii < bonesCount; ii++) {
            pathData.bones.push_back(skeletonData.bones[readVarint(&input, true)].name.value());
        }
        pathData.target = skeletonData.slots[readVarint(&input, true)].name;
        int flags = readByte(&input);
        pathData.positionMode = (PositionMode)(flags & 1);
        pathData.spacingMode = (SpacingMode)((flags >> 1) & 3);
        pathData.rotateMode = (RotateMode)((flags >> 3) & 3);
        if ((flags & 128) != 0) pathData.offsetRotation = readFloat(&input);
        pathData.position = readFloat(&input);
        pathData.spacing = readFloat(&input);
        pathData.mixRotate = readFloat(&input);
        pathData.mixX = readFloat(&input);
        pathData.mixY = readFloat(&input);
        skeletonData.pathConstraints.push_back(pathData);
    }

    /* Physics constraints */
    int physicsCount = readVarint(&input, true);
    for (int i = 0; i < physicsCount; i++) {
        PhysicsConstraintData data;
        data.name = readString(&input).value();
        data.order = readVarint(&input, true);
        data.bone = skeletonData.bones[readVarint(&input, true)].name.value();
        int flags = readByte(&input);
        data.skinRequired = (flags & 1) != 0;
        if ((flags & 2) != 0) data.x = readFloat(&input);
        if ((flags & 4) != 0) data.y = readFloat(&input);
        if ((flags & 8) != 0) data.rotate = readFloat(&input);
        if ((flags & 16) != 0) data.scaleX = readFloat(&input);
        if ((flags & 32) != 0) data.shearX = readFloat(&input);
        data.limit = ((flags & 64) != 0) ? readFloat(&input) : 5000.0f;
        data.step = 1.0f / readByte(&input);
        data.inertia = readFloat(&input);
        data.strength = readFloat(&input);
        data.damping = readFloat(&input);
        data.massInverse = (flags & 128) != 0 ? readFloat(&input) : 1.0f;
        data.wind = readFloat(&input);
        data.gravity = readFloat(&input);
        flags = readByte(&input);
        data.inertiaGlobal = (flags & 1) != 0;
        data.strengthGlobal = (flags & 2) != 0;
        data.dampingGlobal = (flags & 4) != 0;
        data.massGlobal = (flags & 8) != 0;
        data.windGlobal = (flags & 16) != 0;
        data.gravityGlobal = (flags & 32) != 0;
        data.mixGlobal = (flags & 64) != 0;
        data.mix = (flags & 128) != 0 ? readFloat(&input) : 1.0f;
        skeletonData.physicsConstraints.push_back(data);
    }

'''

reader = reader[:start_idx] + new_reader + reader[end_idx:]

with open(reader_path, 'w', encoding='utf-8') as f:
    f.write(reader)


writer_path = r'E:\SpineSkeletonDataConverter-main\src\SkeletonData43BinaryWriter.cpp'
with open(writer_path, 'r', encoding='utf-8') as f:
    writer = f.read()

start_idx = writer.find('    /* Constraints (Unified) */')
if start_idx == -1:
    start_idx = writer.find('    /* Constraints */')
end_idx = writer.find('    /* Default Skin */')

new_writer = '''    /* IK Constraints */
    writeVarint(binary, skeletonData.ikConstraints.size(), true);
    for (const auto& data : skeletonData.ikConstraints) {
        writeString(binary, data.name);
        writeVarint(binary, data.order, true);
        writeVarint(binary, data.bones.size(), true);
        for (const auto& boneName : data.bones) {
            int boneIndex = 0;
            for (size_t i = 0; i < skeletonData.bones.size(); i++) {
                if (skeletonData.bones[i].name == boneName) { boneIndex = i; break; }
            }
            writeVarint(binary, boneIndex, true);
        }
        int targetIndex = 0;
        for (size_t i = 0; i < skeletonData.bones.size(); i++) {
            if (skeletonData.bones[i].name == data.target) { targetIndex = i; break; }
        }
        writeVarint(binary, targetIndex, true);
        
        unsigned char flags = 0;
        if (data.skinRequired) flags |= 1;
        if (data.bendDirection == 1) flags |= 2;
        if (data.compress) flags |= 4;
        if (data.stretch) flags |= 8;
        if (data.uniform) flags |= 16;
        if (data.mix != 1.0f) flags |= 32;
        if (data.mix != 1.0f) flags |= 64; // we just save it as mixed
        if (data.softness != 0.0f) flags |= 128;
        writeByte(binary, flags);
        if (data.mix != 1.0f) writeFloat(binary, data.mix);
        if (data.softness != 0.0f) writeFloat(binary, data.softness);
    }

    /* Transform Constraints */
    writeVarint(binary, skeletonData.transformConstraints.size(), true);
    for (const auto& data : skeletonData.transformConstraints) {
        writeString(binary, data.name);
        writeVarint(binary, data.order, true);
        writeVarint(binary, data.bones.size(), true);
        for (const auto& boneName : data.bones) {
            int boneIndex = 0;
            for (size_t i = 0; i < skeletonData.bones.size(); i++) {
                if (skeletonData.bones[i].name == boneName) { boneIndex = i; break; }
            }
            writeVarint(binary, boneIndex, true);
        }
        int targetIndex = 0;
        for (size_t i = 0; i < skeletonData.bones.size(); i++) {
            if (skeletonData.bones[i].name == data.target) { targetIndex = i; break; }
        }
        writeVarint(binary, targetIndex, true);
        
        unsigned char flags = 0;
        if (data.skinRequired) flags |= 1;
        if (data.localSource) flags |= 2;
        if (data.localTarget) flags |= 4; // wait, Spine 4.3 uses `local` (2) and `relative` (4) for binary format
        // In my struct I have `localSource` and `localTarget`. I will map localSource to bit 2 and localTarget to bit 4
        if (data.offsetRotation != 0.0f) flags |= 8;
        if (data.offsetX != 0.0f) flags |= 16;
        if (data.offsetY != 0.0f) flags |= 32;
        if (data.offsetScaleX != 0.0f) flags |= 64;
        if (data.offsetScaleY != 0.0f) flags |= 128;
        writeByte(binary, flags);
        if (data.offsetRotation != 0.0f) writeFloat(binary, data.offsetRotation);
        if (data.offsetX != 0.0f) writeFloat(binary, data.offsetX);
        if (data.offsetY != 0.0f) writeFloat(binary, data.offsetY);
        if (data.offsetScaleX != 0.0f) writeFloat(binary, data.offsetScaleX);
        if (data.offsetScaleY != 0.0f) writeFloat(binary, data.offsetScaleY);
        
        flags = 0;
        if (data.offsetShearY != 0.0f) flags |= 1;
        if (data.mixRotate != 1.0f) flags |= 2;
        if (data.mixX != 1.0f) flags |= 4;
        if (data.mixY != 1.0f) flags |= 8;
        if (data.mixScaleX != 1.0f) flags |= 16;
        if (data.mixScaleY != 1.0f) flags |= 32;
        if (data.mixShearY != 1.0f) flags |= 64;
        writeByte(binary, flags);
        if (data.offsetShearY != 0.0f) writeFloat(binary, data.offsetShearY);
        if (data.mixRotate != 1.0f) writeFloat(binary, data.mixRotate);
        if (data.mixX != 1.0f) writeFloat(binary, data.mixX);
        if (data.mixY != 1.0f) writeFloat(binary, data.mixY);
        if (data.mixScaleX != 1.0f) writeFloat(binary, data.mixScaleX);
        if (data.mixScaleY != 1.0f) writeFloat(binary, data.mixScaleY);
        if (data.mixShearY != 1.0f) writeFloat(binary, data.mixShearY);
    }

    /* Path Constraints */
    writeVarint(binary, skeletonData.pathConstraints.size(), true);
    for (const auto& data : skeletonData.pathConstraints) {
        writeString(binary, data.name);
        writeVarint(binary, data.order, true);
        writeByte(binary, data.skinRequired ? 1 : 0);
        writeVarint(binary, data.bones.size(), true);
        for (const auto& boneName : data.bones) {
            int boneIndex = 0;
            for (size_t i = 0; i < skeletonData.bones.size(); i++) {
                if (skeletonData.bones[i].name == boneName) { boneIndex = i; break; }
            }
            writeVarint(binary, boneIndex, true);
        }
        int targetIndex = 0;
        for (size_t i = 0; i < skeletonData.slots.size(); i++) {
            if (skeletonData.slots[i].name == data.target) { targetIndex = i; break; }
        }
        writeVarint(binary, targetIndex, true);
        
        unsigned char flags = (int)data.positionMode | ((int)data.spacingMode << 1) | ((int)data.rotateMode << 3);
        if (data.offsetRotation != 0.0f) flags |= 128;
        writeByte(binary, flags);
        if (data.offsetRotation != 0.0f) writeFloat(binary, data.offsetRotation);
        writeFloat(binary, data.position);
        writeFloat(binary, data.spacing);
        writeFloat(binary, data.mixRotate);
        writeFloat(binary, data.mixX);
        writeFloat(binary, data.mixY);
    }

    /* Physics Constraints */
    writeVarint(binary, skeletonData.physicsConstraints.size(), true);
    for (const auto& data : skeletonData.physicsConstraints) {
        writeString(binary, data.name);
        writeVarint(binary, data.order, true);
        int boneIndex = 0;
        for (size_t i = 0; i < skeletonData.bones.size(); i++) {
            if (skeletonData.bones[i].name == data.bone) { boneIndex = i; break; }
        }
        writeVarint(binary, boneIndex, true);
        
        unsigned char flags = 0;
        if (data.skinRequired) flags |= 1;
        if (data.x != 0.0f) flags |= 2;
        if (data.y != 0.0f) flags |= 4;
        if (data.rotate != 0.0f) flags |= 8;
        if (data.scaleX != 0.0f) flags |= 16;
        if (data.shearX != 0.0f) flags |= 32;
        if (data.limit != 5000.0f) flags |= 64;
        if (data.massInverse != 1.0f) flags |= 128;
        writeByte(binary, flags);
        
        if (data.x != 0.0f) writeFloat(binary, data.x);
        if (data.y != 0.0f) writeFloat(binary, data.y);
        if (data.rotate != 0.0f) writeFloat(binary, data.rotate);
        if (data.scaleX != 0.0f) writeFloat(binary, data.scaleX);
        if (data.shearX != 0.0f) writeFloat(binary, data.shearX);
        if (data.limit != 5000.0f) writeFloat(binary, data.limit);
        
        float stepFloat = data.step != 0.0f ? (1.0f / data.step) : 60.0f;
        writeByte(binary, (int)stepFloat); // 1.f / readByte in reader
        
        writeFloat(binary, data.inertia);
        writeFloat(binary, data.strength);
        writeFloat(binary, data.damping);
        if (data.massInverse != 1.0f) writeFloat(binary, data.massInverse);
        writeFloat(binary, data.wind);
        writeFloat(binary, data.gravity);
        
        flags = 0;
        if (data.inertiaGlobal) flags |= 1;
        if (data.strengthGlobal) flags |= 2;
        if (data.dampingGlobal) flags |= 4;
        if (data.massGlobal) flags |= 8;
        if (data.windGlobal) flags |= 16;
        if (data.gravityGlobal) flags |= 32;
        if (data.mixGlobal) flags |= 64;
        if (data.mix != 1.0f) flags |= 128;
        writeByte(binary, flags);
        if (data.mix != 1.0f) writeFloat(binary, data.mix);
    }

'''

writer = writer[:start_idx] + new_writer + writer[end_idx:]
with open(writer_path, 'w', encoding='utf-8') as f:
    f.write(writer)
