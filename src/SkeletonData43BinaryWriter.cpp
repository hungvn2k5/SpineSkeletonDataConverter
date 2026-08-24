#include "SkeletonData.h"
#include <set>

namespace spine43 {

using ::writeByte;
using ::writeSByte;
using ::writeBoolean;
using ::writeInt;
using ::writeColor;
using ::writeVarint;
using ::writeFloat;
using ::writeString;
using ::writeStringRef;

void writeSequence(Binary& binary, const Sequence& sequence) {
    writeVarint(binary, sequence.count, true);
    writeVarint(binary, sequence.start, true);
    writeVarint(binary, sequence.digits, true);
    writeVarint(binary, sequence.setupIndex, true);
}

void writeFloatArray(Binary& binary, const std::vector<float>& array) {
    for (float value : array) {
        writeFloat(binary, value);
    }
}

void writeShortArray(Binary& binary, const std::vector<unsigned short>& array) {
    for (unsigned short value : array) {
        writeVarint(binary, value, true);
    }
}

void writeVertices(Binary& binary, const std::vector<float>& vertices, bool weighted) {
    if (!weighted) {
        int verticesLength = vertices.size(); 
        int vertexCount = verticesLength >> 1;
        writeVarint(binary, vertexCount, true);
        writeFloatArray(binary, vertices);
    } else {
        int vertexCount = 0; 
        int verticesIdx = 0; 
        while (verticesIdx < vertices.size()) {
            int boneCount = (int)vertices[verticesIdx++];
            vertexCount++; 
            verticesIdx += boneCount * 4;
        }
        writeVarint(binary, vertexCount, true);
        verticesIdx = 0; 
        for (int i = 0; i < vertexCount; i++) {
            int boneCount = (int)vertices[verticesIdx++]; 
            writeVarint(binary, boneCount, true);
            for (int ii = 0; ii < boneCount; ii++) {
                writeVarint(binary, (int)vertices[verticesIdx++], true);
                writeFloat(binary, vertices[verticesIdx++]);
                writeFloat(binary, vertices[verticesIdx++]);
                writeFloat(binary, vertices[verticesIdx++]);
            }
        }
    }
}

void writeCurve(Binary& binary, const TimelineFrame& frame) {
    for (int i = 0; i < frame.curve.size(); i++) {
        writeFloat(binary, frame.curve[i]);
    }
}

int getBezierCount(const Timeline& timeline, int valueNum) {
    int count = 0;
    for (size_t i = 0; i < timeline.size() - 1; i++) {
        if (timeline[i].curveType == CurveType::CURVE_BEZIER) {
            count += timeline[i].curve.size() > 0 ? timeline[i].curve.size() : (valueNum * 4);
        }
    }
    return count;
}

void writeTimeline(Binary& binary, const Timeline& timeline, int valueNum) {
    writeFloat(binary, timeline[0].time); 
    writeFloat(binary, timeline[0].value1);
    if (valueNum > 1) writeFloat(binary, timeline[0].value2);
    if (valueNum > 2) writeFloat(binary, timeline[0].value3);
    for (size_t frameIndex = 1; frameIndex < timeline.size(); frameIndex++) {
        writeFloat(binary, timeline[frameIndex].time); 
        writeFloat(binary, timeline[frameIndex].value1);
        if (valueNum > 1) writeFloat(binary, timeline[frameIndex].value2);
        if (valueNum > 2) writeFloat(binary, timeline[frameIndex].value3);
        CurveType curveType = timeline[frameIndex - 1].curveType;
        writeSByte(binary, (signed char)curveType);
        if (curveType == CurveType::CURVE_BEZIER) {
            writeCurve(binary, timeline[frameIndex - 1]);
        }
    }
}

void writeSkin(Binary& binary, const Skin& skin, SkeletonData& skeletonData, bool defaultSkin) {
    if (defaultSkin) {
        writeVarint(binary, skin.attachments.size(), true); 
    } else {
        writeString(binary, skin.name); 
        if (skeletonData.nonessential) {
            if (skin.color) writeColor(binary, skin.color.value()); 
            else writeColor(binary, Color{0xff, 0xff, 0xff, 0xff});
        }
        writeVarint(binary, skin.bones.size(), true);
        for (const std::string& boneName : skin.bones) {
            int boneIndex = 0;
            for (size_t i = 0; i < skeletonData.bones.size(); i++) {
                if (skeletonData.bones[i].name == boneName) { boneIndex = i; break; }
            }
            writeVarint(binary, boneIndex, true);
        }
        // Unified constraints in skin (4.3)
        writeVarint(binary, skin.constraints.size(), true);
        for (const std::string& constraintName : skin.constraints) {
            int cIndex = 0;
            for (size_t i = 0; i < skeletonData.constraintNames.size(); i++) {
                if (skeletonData.constraintNames[i] == constraintName) { cIndex = i; break; }
            }
            writeVarint(binary, cIndex, true);
        }
        writeVarint(binary, skin.attachments.size(), true);
    }
    for (const auto& [slotName, attachments] : skin.attachments) {
        int slotIndex = 0; 
        for (size_t i = 0; i < skeletonData.slots.size(); i++) {
            if (skeletonData.slots[i].name == slotName) { slotIndex = i; break; }
        }
        writeVarint(binary, slotIndex, true);
        writeVarint(binary, attachments.size(), true);
        for (const auto& [attachmentName, attachment] : attachments) {
            writeStringRef(binary, attachmentName, skeletonData);
            unsigned char flags = 0; 
            flags |= attachment.type & 0x7; 
            if (attachment.name != attachmentName) flags |= 8; 
            switch (attachment.type) {
                case AttachmentType_Region: {
                    const RegionAttachment& region = std::get<RegionAttachment>(attachment.data);
                    if (attachment.path != attachment.name) flags |= 16;
                    if (region.color.has_value()) flags |= 32;
                    if (region.sequence.has_value()) flags |= 64;
                    if (region.rotation != 0.0f) flags |= 128;
                    break; 
                }
                case AttachmentType_Boundingbox: {
                    const BoundingboxAttachment& box = std::get<BoundingboxAttachment>(attachment.data);
                    int verticesLength = box.vertices.size();
                    if (verticesLength > box.vertexCount * 2) flags |= 16;
                    break; 
                }
                case AttachmentType_Mesh: {
                    const MeshAttachment& mesh = std::get<MeshAttachment>(attachment.data);
                    int verticesLength = mesh.vertices.size();
                    int vertexCount = mesh.hullLength > 0 ? mesh.hullLength / 2 : (verticesLength >> 1);
                    if (attachment.path != attachment.name) flags |= 16;
                    if (mesh.color.has_value()) flags |= 32;
                    if (mesh.sequence.has_value()) flags |= 64;
                    if (verticesLength > vertexCount * 2) flags |= 128;
                    break; 
                }
                case AttachmentType_Linkedmesh: {
                    const LinkedmeshAttachment& linkedMesh = std::get<LinkedmeshAttachment>(attachment.data);
                    if (attachment.path != attachment.name) flags |= 16;
                    if (linkedMesh.color.has_value()) flags |= 32;
                    if (linkedMesh.sequence.has_value()) flags |= 64;
                    if (linkedMesh.timelines != 0) flags |= 128;
                    break; 
                }
                case AttachmentType_Path: {
                    const PathAttachment& path = std::get<PathAttachment>(attachment.data);
                    int verticesLength = path.vertices.size();
                    if (path.closed) flags |= 16;
                    if (path.constantSpeed) flags |= 32;
                    if (verticesLength > path.vertexCount * 2) flags |= 64;
                    break; 
                }
                case AttachmentType_Point: {
                    break; 
                }
                case AttachmentType_Clipping: {
                    const ClippingAttachment& clipping = std::get<ClippingAttachment>(attachment.data);
                    int verticesLength = clipping.vertices.size();
                    if (verticesLength > clipping.vertexCount * 2) flags |= 16;
                    break; 
                }
            }
            writeByte(binary, flags);
            if ((flags & 8) != 0) writeStringRef(binary, attachment.name, skeletonData);
            switch (attachment.type) {
                case AttachmentType_Region: {
                    const RegionAttachment& region = std::get<RegionAttachment>(attachment.data);
                    if ((flags & 16) != 0) writeStringRef(binary, attachment.path, skeletonData);
                    if (region.color.has_value()) writeColor(binary, region.color.value());
                    if (region.sequence.has_value()) writeSequence(binary, region.sequence.value());
                    if (region.rotation != 0.0f) writeFloat(binary, region.rotation);
                    writeFloat(binary, region.x);
                    writeFloat(binary, region.y);
                    writeFloat(binary, region.scaleX);
                    writeFloat(binary, region.scaleY);
                    writeFloat(binary, region.width);
                    writeFloat(binary, region.height);
                    break; 
                }
                case AttachmentType_Boundingbox: {
                    const BoundingboxAttachment& box = std::get<BoundingboxAttachment>(attachment.data);
                    bool weighted = (flags & 16) != 0;
                    writeVertices(binary, box.vertices, weighted);
                    if (skeletonData.nonessential) {
                        if (box.color.has_value()) writeColor(binary, box.color.value());
                        else writeColor(binary, Color{0xff, 0xff, 0xff, 0xff});
                    }
                    break; 
                }
                case AttachmentType_Mesh: {
                    const MeshAttachment& mesh = std::get<MeshAttachment>(attachment.data);
                    bool weighted = (flags & 128) != 0;
                    if ((flags & 16) != 0) writeStringRef(binary, attachment.path, skeletonData);
                    if (mesh.color.has_value()) writeColor(binary, mesh.color.value());
                    if (mesh.sequence.has_value()) writeSequence(binary, mesh.sequence.value());
                    writeVarint(binary, mesh.hullLength / 2, true);
                    writeVertices(binary, mesh.vertices, weighted);
                    writeFloatArray(binary, mesh.uvs);
                    writeShortArray(binary, mesh.triangles);
                    writeVarint(binary, mesh.timelineSlots.size(), true);
                    for (int slotIdx : mesh.timelineSlots) writeVarint(binary, slotIdx, true);
                    if (skeletonData.nonessential) {
                        writeVarint(binary, mesh.edges.size(), true);
                        for (unsigned short edge : mesh.edges) writeVarint(binary, edge, true);
                        writeFloat(binary, mesh.width);
                        writeFloat(binary, mesh.height);
                    }
                    break; 
                }
                case AttachmentType_Linkedmesh: {
                    const LinkedmeshAttachment& linkedMesh = std::get<LinkedmeshAttachment>(attachment.data);
                    if ((flags & 16) != 0) writeStringRef(binary, attachment.path, skeletonData);
                    if (linkedMesh.color.has_value()) writeColor(binary, linkedMesh.color.value());
                    if (linkedMesh.sequence.has_value()) writeSequence(binary, linkedMesh.sequence.value());
                    
                    int sourceIdx = linkedMesh.sourceIndex >= 0 ? linkedMesh.sourceIndex : 0;
                    writeVarint(binary, sourceIdx, true);

                    int skinIndex = 0;
                    if (linkedMesh.skin.has_value()) {
                        for (size_t i = 0; i < skeletonData.skins.size(); i++) {
                            if (skeletonData.skins[i].name == linkedMesh.skin.value()) { skinIndex = i; break; }
                        }
                    }
                    writeVarint(binary, skinIndex, true);
                    writeStringRef(binary, linkedMesh.parentMesh, skeletonData);
                    if (skeletonData.nonessential) {
                        writeFloat(binary, linkedMesh.width);
                        writeFloat(binary, linkedMesh.height);
                    }
                    break; 
                }
                case AttachmentType_Path: {
                    const PathAttachment& path = std::get<PathAttachment>(attachment.data);
                    bool weighted = (flags & 64) != 0;
                    writeVertices(binary, path.vertices, weighted);
                    writeFloatArray(binary, path.lengths);
                    if (skeletonData.nonessential) {
                        if (path.color.has_value()) writeColor(binary, path.color.value());
                        else writeColor(binary, Color{0xff, 0xff, 0xff, 0xff});
                    }
                    break; 
                }
                case AttachmentType_Point: {
                    const PointAttachment& point = std::get<PointAttachment>(attachment.data);
                    writeFloat(binary, point.rotation);
                    writeFloat(binary, point.x);
                    writeFloat(binary, point.y);
                    if (skeletonData.nonessential) {
                        if (point.color.has_value()) writeColor(binary, point.color.value());
                        else writeColor(binary, Color{0xff, 0xff, 0xff, 0xff});
                    }
                    break; 
                }
                case AttachmentType_Clipping: {
                    const ClippingAttachment& clipping = std::get<ClippingAttachment>(attachment.data);
                    bool weighted = (flags & 16) != 0;
                    int endSlotIndex = 0;
                    if (clipping.endSlot.has_value()) {
                        for (size_t i = 0; i < skeletonData.slots.size(); i++) {
                            if (skeletonData.slots[i].name == clipping.endSlot.value()) { endSlotIndex = i; break; }
                        }
                    }
                    writeVarint(binary, endSlotIndex, true);
                    writeVertices(binary, clipping.vertices, weighted);
                    if (skeletonData.nonessential) {
                        if (clipping.color.has_value()) writeColor(binary, clipping.color.value());
                        else writeColor(binary, Color{0xff, 0xff, 0xff, 0xff});
                    }
                    break; 
                }
            }
        }
    }
}

Binary writeBinaryData(SkeletonData& skeletonData) {
    Binary binary; 
    uint64_t hash = skeletonData.hash;
    writeInt(binary, (int)(hash & 0xffffffff));
    writeInt(binary, (int)(hash >> 32));

    writeString(binary, skeletonData.version);

    writeFloat(binary, skeletonData.x);
    writeFloat(binary, skeletonData.y);
    writeFloat(binary, skeletonData.width);
    writeFloat(binary, skeletonData.height);
    writeFloat(binary, skeletonData.referenceScale);

    writeBoolean(binary, skeletonData.nonessential);

    if (skeletonData.nonessential) {
        writeFloat(binary, skeletonData.fps);
        writeString(binary, skeletonData.imagesPath);
        writeString(binary, skeletonData.audioPath);
    }

    if (skeletonData.strings.empty()) {
        std::set<std::string> stringSet;
        for (const auto& skin : skeletonData.skins) {
            for (const auto& [slotName, slotMap] : skin.attachments) {
                for (const auto& [attachmentName, attachment] : slotMap) {
                    stringSet.insert(attachment.name);
                    if (attachment.path != attachment.name) stringSet.insert(attachment.path);
                }
            }
        }
        for (const auto& animation : skeletonData.animations) {
            for (const auto& [skinName, skinMap] : animation.attachments) {
                for (const auto& [slotName, slotMap] : skinMap) {
                    for (const auto& [attachmentName, multiTimeline] : slotMap) {
                        stringSet.insert(attachmentName);
                    }
                }
            }
        }
        skeletonData.strings.clear();
        for (const std::string& str : stringSet) {
            skeletonData.strings.push_back(str);
        }
    }

    writeVarint(binary, skeletonData.strings.size(), true);
    for (const std::string& str : skeletonData.strings) {
        writeString(binary, str);
    }

    /* Bones */
    writeVarint(binary, skeletonData.bones.size(), true);
    for (size_t i = 0; i < skeletonData.bones.size(); i++) {
        const auto& bone = skeletonData.bones[i];
        writeString(binary, bone.name);
        if (i > 0) {
            int parentIndex = 0;
            for (size_t j = 0; j < skeletonData.bones.size(); j++) {
                if (skeletonData.bones[j].name == bone.parent) { parentIndex = j; break; }
            }
            writeVarint(binary, parentIndex, true);
        }
        writeFloat(binary, bone.rotation);
        writeFloat(binary, bone.x);
        writeFloat(binary, bone.y);
        writeFloat(binary, bone.scaleX);
        writeFloat(binary, bone.scaleY);
        writeFloat(binary, bone.shearX);
        writeFloat(binary, bone.shearY);
        // 4.3 inherit byte before length float
        writeByte(binary, (int)bone.inherit);
        writeFloat(binary, bone.length);
        writeBoolean(binary, bone.skinRequired);
        if (skeletonData.nonessential) {
            writeColor(binary, bone.color.value_or(Color{0x9b, 0x9b, 0x9b, 0xff}));
            writeString(binary, bone.icon);
            writeFloat(binary, bone.iconSize);
            writeFloat(binary, bone.iconRotation);
            writeBoolean(binary, bone.visible);
        }
    }

    /* Slots */
    writeVarint(binary, skeletonData.slots.size(), true);
    for (const auto& slot : skeletonData.slots) {
        writeString(binary, slot.name);
        int boneIndex = 0;
        for (size_t i = 0; i < skeletonData.bones.size(); i++) {
            if (skeletonData.bones[i].name == slot.bone) { boneIndex = i; break; }
        }
        writeVarint(binary, boneIndex, true);
        writeColor(binary, slot.color.value_or(Color{0xff, 0xff, 0xff, 0xff}));
        if (slot.darkColor.has_value()) {
            Color d = slot.darkColor.value();
            int darkInt = (d.r << 16) | (d.g << 8) | d.b;
            writeInt(binary, darkInt);
        } else {
            writeInt(binary, -1);
        }
        writeStringRef(binary, slot.attachmentName, skeletonData);
        writeVarint(binary, (int)slot.blendMode, true);
        if (skeletonData.nonessential) writeBoolean(binary, slot.visible);
    }

    /* Constraints (Unified 4.3) */
    size_t totalConstraints = skeletonData.ikConstraints.size() + skeletonData.transformConstraints.size() +
                              skeletonData.pathConstraints.size() + skeletonData.physicsConstraints.size() +
                              skeletonData.sliderConstraints.size();
    writeVarint(binary, totalConstraints, true);

    // Write IK Constraints (type 0)
    for (const auto& data : skeletonData.ikConstraints) {
        writeString(binary, data.name);
        writeByte(binary, 0); // IK
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
        if (data.scaleYMode != ScaleYMode_None) flags |= 2;
        if (!data.bendPositive) flags |= 4;
        if (data.compress) flags |= 8;
        if (data.stretch) flags |= 16;
        if (data.mix != 1.0f) { flags |= 32; flags |= 64; }
        if (data.softness != 0.0f) flags |= 128;
        writeByte(binary, flags);
        if (data.scaleYMode != ScaleYMode_None) writeByte(binary, (int)data.scaleYMode);
        if (data.mix != 1.0f) writeFloat(binary, data.mix);
        if (data.softness != 0.0f) writeFloat(binary, data.softness);
    }

    // Write Transform Constraints (type 1)
    for (const auto& data : skeletonData.transformConstraints) {
        writeString(binary, data.name);
        writeByte(binary, 1); // Transform
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
        if (data.localTarget) flags |= 4;
        if (data.additive) flags |= 8;
        if (data.clamp) flags |= 16;
        flags |= (data.properties.size() << 5);
        writeByte(binary, flags);

        for (const auto& fromProp : data.properties) {
            writeByte(binary, fromProp.type);
            writeFloat(binary, fromProp.offset);
            writeByte(binary, fromProp.to.size());
            for (const auto& toProp : fromProp.to) {
                writeByte(binary, toProp.type);
                writeFloat(binary, toProp.offset);
                writeFloat(binary, toProp.max);
                writeFloat(binary, toProp.scale);
            }
        }
        
        unsigned char offsetFlags = 0;
        if (data.offsetRotation != 0.0f) offsetFlags |= 1;
        if (data.offsetX != 0.0f) offsetFlags |= 2;
        if (data.offsetY != 0.0f) offsetFlags |= 4;
        if (data.offsetScaleX != 0.0f) offsetFlags |= 8;
        if (data.offsetScaleY != 0.0f) offsetFlags |= 16;
        if (data.offsetShearY != 0.0f) offsetFlags |= 32;
        writeByte(binary, offsetFlags);
        if (data.offsetRotation != 0.0f) writeFloat(binary, data.offsetRotation);
        if (data.offsetX != 0.0f) writeFloat(binary, data.offsetX);
        if (data.offsetY != 0.0f) writeFloat(binary, data.offsetY);
        if (data.offsetScaleX != 0.0f) writeFloat(binary, data.offsetScaleX);
        if (data.offsetScaleY != 0.0f) writeFloat(binary, data.offsetScaleY);
        if (data.offsetShearY != 0.0f) writeFloat(binary, data.offsetShearY);
        
        unsigned char mixFlags = 0;
        if (data.mixRotate != 1.0f) mixFlags |= 1;
        if (data.mixX != 1.0f) mixFlags |= 2;
        if (data.mixY != 1.0f) mixFlags |= 4;
        if (data.mixScaleX != 1.0f) mixFlags |= 8;
        if (data.mixScaleY != 1.0f) mixFlags |= 16;
        if (data.mixShearY != 1.0f) mixFlags |= 32;
        writeByte(binary, mixFlags);
        if (data.mixRotate != 1.0f) writeFloat(binary, data.mixRotate);
        if (data.mixX != 1.0f) writeFloat(binary, data.mixX);
        if (data.mixY != 1.0f) writeFloat(binary, data.mixY);
        if (data.mixScaleX != 1.0f) writeFloat(binary, data.mixScaleX);
        if (data.mixScaleY != 1.0f) writeFloat(binary, data.mixScaleY);
        if (data.mixShearY != 1.0f) writeFloat(binary, data.mixShearY);
    }

    // Write Path Constraints (type 2)
    for (const auto& data : skeletonData.pathConstraints) {
        writeString(binary, data.name);
        writeByte(binary, 2); // Path
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
        if (data.skinRequired) flags |= 1;
        if (data.offsetRotation != 0.0f) flags |= 128;
        writeByte(binary, flags);
        if (data.offsetRotation != 0.0f) writeFloat(binary, data.offsetRotation);
        writeFloat(binary, data.position);
        writeFloat(binary, data.spacing);
        writeFloat(binary, data.mixRotate);
        writeFloat(binary, data.mixX);
        writeFloat(binary, data.mixY);
    }

    // Write Physics Constraints (type 3)
    for (const auto& data : skeletonData.physicsConstraints) {
        writeString(binary, data.name);
        writeByte(binary, 3); // Physics
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
        writeByte(binary, (int)stepFloat);
        
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

    // Write Slider Constraints (type 4)
    for (const auto& data : skeletonData.sliderConstraints) {
        writeString(binary, data.name);
        writeByte(binary, 4); // Slider
        unsigned char flags = 0;
        if (data.skinRequired) flags |= 1;
        if (data.loop) flags |= 2;
        if (data.additive) flags |= 4;
        if (data.time != 0.0f || data.max != 0.0f) flags |= 8;
        if (data.mix != 1.0f) { flags |= 16; flags |= 32; }
        if (data.hasProperty) flags |= 64;
        if (data.local) flags |= 128;
        writeByte(binary, flags);
        
        if (data.time != 0.0f || data.max != 0.0f) {
            writeFloat(binary, skeletonData.nonessential && (flags & 64) != 0 ? data.max : data.time);
        }
        if (data.mix != 1.0f) writeFloat(binary, data.mix);
        if (data.hasProperty) {
            int boneIndex = 0;
            for (size_t i = 0; i < skeletonData.bones.size(); i++) {
                if (skeletonData.bones[i].name == data.bone) { boneIndex = i; break; }
            }
            writeVarint(binary, boneIndex, true);
            writeFloat(binary, data.propertyOffset);
            writeByte(binary, data.propertyType);
            writeFloat(binary, data.offset);
            writeFloat(binary, data.scale);
        }
    }

    /* Skins */
    int defaultSkinIndex = -1;
    for (size_t i = 0; i < skeletonData.skins.size(); i++) {
        if (skeletonData.skins[i].name == "default") { defaultSkinIndex = i; break; }
    }
    if (defaultSkinIndex != -1) {
        writeSkin(binary, skeletonData.skins[defaultSkinIndex], skeletonData, true);
    } else {
        writeVarint(binary, 0, true);
    }

    int nonDefaultSkinCount = skeletonData.skins.size() - (defaultSkinIndex != -1 ? 1 : 0);
    writeVarint(binary, nonDefaultSkinCount, true);
    for (size_t i = 0; i < skeletonData.skins.size(); i++) {
        if ((int)i == defaultSkinIndex) continue;
        writeSkin(binary, skeletonData.skins[i], skeletonData, false);
    }

    /* Events */
    writeVarint(binary, skeletonData.events.size(), true);
    for (const auto& eventData : skeletonData.events) {
        writeString(binary, eventData.name);
        writeVarint(binary, eventData.intValue, false);
        writeFloat(binary, eventData.floatValue);
        writeString(binary, eventData.stringValue);
        writeString(binary, eventData.audioPath);
        if (eventData.audioPath.has_value() && eventData.audioPath.value().length() > 0) {
            writeFloat(binary, eventData.volume);
            writeFloat(binary, eventData.balance);
        }
    }

    /* Animations */
    writeVarint(binary, skeletonData.animations.size(), true);
    for (const auto& animation : skeletonData.animations) {
        writeString(binary, animation.name);
        
        int slotTimelinesCount = animation.slots.size();
        int boneTimelinesCount = animation.bones.size();
        int ikTimelinesCount = animation.ik.size();
        int transformTimelinesCount = animation.transform.size();
        int pathTimelinesCount = animation.path.size();
        int physicsTimelinesCount = animation.physics.size();
        int sliderTimelinesCount = animation.sliders.size();
        int attachmentTimelinesCount = animation.attachments.size();
        int drawOrderTimelinesCount = animation.drawOrder.size() > 0 ? 1 : 0;
        int eventTimelinesCount = animation.events.size() > 0 ? 1 : 0;
        
        int totalTimelines = slotTimelinesCount + boneTimelinesCount + ikTimelinesCount + transformTimelinesCount +
                            pathTimelinesCount + physicsTimelinesCount + sliderTimelinesCount + attachmentTimelinesCount +
                            drawOrderTimelinesCount + eventTimelinesCount;
        writeVarint(binary, totalTimelines, true);

        // Slot timelines
        writeVarint(binary, animation.slots.size(), true);
        for (const auto& [slotName, slotTimelines] : animation.slots) {
            int slotIndex = 0;
            for (size_t i = 0; i < skeletonData.slots.size(); i++) {
                if (skeletonData.slots[i].name == slotName) { slotIndex = i; break; }
            }
            writeVarint(binary, slotIndex, true);
            writeVarint(binary, slotTimelines.size(), true);
            for (const auto& [timelineTypeStr, timeline] : slotTimelines) {
                SlotTimelineType type = slotTimelineTypeMap.at(timelineTypeStr);
                writeByte(binary, static_cast<int>(type));
                int frameCount = timeline.size();
                writeVarint(binary, frameCount, true);
                if (type == SlotTimelineType::SLOT_ATTACHMENT) {
                    for (const auto& frame : timeline) {
                        writeFloat(binary, frame.time);
                        writeStringRef(binary, frame.str1, skeletonData);
                    }
                } else if (type == SlotTimelineType::SLOT_RGBA) {
                    int bezierCount = getBezierCount(timeline, 4);
                    writeVarint(binary, bezierCount, true);
                    writeTimeline(binary, timeline, 4);
                } else if (type == SlotTimelineType::SLOT_RGB) {
                    int bezierCount = getBezierCount(timeline, 3);
                    writeVarint(binary, bezierCount, true);
                    writeTimeline(binary, timeline, 3);
                } else if (type == SlotTimelineType::SLOT_RGBA2) {
                    int bezierCount = getBezierCount(timeline, 7);
                    writeVarint(binary, bezierCount, true);
                    writeTimeline(binary, timeline, 7);
                } else if (type == SlotTimelineType::SLOT_RGB2) {
                    int bezierCount = getBezierCount(timeline, 6);
                    writeVarint(binary, bezierCount, true);
                    writeTimeline(binary, timeline, 6);
                } else if (type == SlotTimelineType::SLOT_ALPHA) {
                    int bezierCount = getBezierCount(timeline, 1);
                    writeVarint(binary, bezierCount, true);
                    writeTimeline(binary, timeline, 1);
                }
            }
        }

        // Bone timelines
        writeVarint(binary, animation.bones.size(), true);
        for (const auto& [boneName, boneTimelines] : animation.bones) {
            int boneIndex = 0;
            for (size_t i = 0; i < skeletonData.bones.size(); i++) {
                if (skeletonData.bones[i].name == boneName) { boneIndex = i; break; }
            }
            writeVarint(binary, boneIndex, true);
            writeVarint(binary, boneTimelines.size(), true);
            for (const auto& [timelineTypeStr, timeline] : boneTimelines) {
                BoneTimelineType type = boneTimelineTypeMap.at(timelineTypeStr);
                writeByte(binary, static_cast<int>(type));
                int frameCount = timeline.size();
                writeVarint(binary, frameCount, true);
                if (type == BoneTimelineType::BONE_INHERIT) {
                    for (const auto& frame : timeline) {
                        writeFloat(binary, frame.time);
                        writeByte(binary, (int)frame.inherit);
                    }
                } else {
                    int valNum = (type == BONE_TRANSLATE || type == BONE_SCALE || type == BONE_SHEAR) ? 2 : 1;
                    int bezierCount = getBezierCount(timeline, valNum);
                    writeVarint(binary, bezierCount, true);
                    writeTimeline(binary, timeline, valNum);
                }
            }
        }

        // IK timelines
        writeVarint(binary, animation.ik.size(), true);
        for (const auto& [ikName, timeline] : animation.ik) {
            int ikIndex = 0;
            for (size_t i = 0; i < skeletonData.ikConstraints.size(); i++) {
                if (skeletonData.ikConstraints[i].name == ikName) { ikIndex = i; break; }
            }
            writeVarint(binary, ikIndex, true);
            writeVarint(binary, timeline.size(), true);
            int bezierCount = getBezierCount(timeline, 2);
            writeVarint(binary, bezierCount, true);
            for (size_t frameIndex = 0; frameIndex < timeline.size(); frameIndex++) {
                const auto& frame = timeline[frameIndex];
                unsigned char flags = 0;
                if (frame.value1 != 0.0f) flags |= 1;
                if (frame.value1 != 1.0f) flags |= 2;
                if (frame.value2 != 0.0f) flags |= 4;
                if (frame.bendPositive) flags |= 8;
                if (frame.compress) flags |= 16;
                if (frame.stretch) flags |= 32;
                if (frameIndex < timeline.size() - 1) {
                    if (frame.curveType == CurveType::CURVE_STEPPED) flags |= 64;
                    else if (frame.curveType == CurveType::CURVE_BEZIER) flags |= 128;
                }
                writeByte(binary, flags);
                writeFloat(binary, frame.time);
                if (frame.value1 != 0.0f && frame.value1 != 1.0f) writeFloat(binary, frame.value1);
                if (frame.value2 != 0.0f) writeFloat(binary, frame.value2);
                if (frameIndex < timeline.size() - 1 && frame.curveType == CurveType::CURVE_BEZIER) {
                    writeCurve(binary, frame);
                }
            }
        }

        // Transform timelines
        writeVarint(binary, animation.transform.size(), true);
        for (const auto& [transformName, timeline] : animation.transform) {
            int transformIndex = 0;
            for (size_t i = 0; i < skeletonData.transformConstraints.size(); i++) {
                if (skeletonData.transformConstraints[i].name == transformName) { transformIndex = i; break; }
            }
            writeVarint(binary, transformIndex, true);
            writeVarint(binary, timeline.size(), true);
            int bezierCount = getBezierCount(timeline, 6);
            writeVarint(binary, bezierCount, true);
            writeTimeline(binary, timeline, 6);
        }

        // Path timelines
        writeVarint(binary, animation.path.size(), true);
        for (const auto& [pathName, pathTimelines] : animation.path) {
            int pathIndex = 0;
            for (size_t i = 0; i < skeletonData.pathConstraints.size(); i++) {
                if (skeletonData.pathConstraints[i].name == pathName) { pathIndex = i; break; }
            }
            writeVarint(binary, pathIndex, true);
            writeVarint(binary, pathTimelines.size(), true);
            for (const auto& [timelineTypeStr, timeline] : pathTimelines) {
                PathTimelineType type = pathTimelineTypeMap.at(timelineTypeStr);
                writeByte(binary, static_cast<int>(type));
                writeVarint(binary, timeline.size(), true);
                int valNum = (type == PATH_MIX) ? 3 : 1;
                int bezierCount = getBezierCount(timeline, valNum);
                writeVarint(binary, bezierCount, true);
                writeTimeline(binary, timeline, valNum);
            }
        }

        // Physics timelines
        writeVarint(binary, animation.physics.size(), true);
        for (const auto& [physicsName, physicsTimelines] : animation.physics) {
            int physicsIndex = 0;
            for (size_t i = 0; i < skeletonData.physicsConstraints.size(); i++) {
                if (skeletonData.physicsConstraints[i].name == physicsName) { physicsIndex = i + 1; break; }
            }
            writeVarint(binary, physicsIndex, true);
            writeVarint(binary, physicsTimelines.size(), true);
            for (const auto& [timelineTypeStr, timeline] : physicsTimelines) {
                PhysicsTimelineType type = physicsTimelineTypeMap.at(timelineTypeStr);
                writeByte(binary, static_cast<int>(type));
                writeVarint(binary, timeline.size(), true);
                if (type == PHYSICS_RESET) {
                    for (const auto& frame : timeline) writeFloat(binary, frame.time);
                } else {
                    int bezierCount = getBezierCount(timeline, 1);
                    writeVarint(binary, bezierCount, true);
                    writeTimeline(binary, timeline, 1);
                }
            }
        }

        // Slider timelines (4.3)
        writeVarint(binary, animation.sliders.size(), true);
        for (const auto& [sliderName, sliderTimelines] : animation.sliders) {
            int sliderIndex = 0;
            for (size_t i = 0; i < skeletonData.sliderConstraints.size(); i++) {
                if (skeletonData.sliderConstraints[i].name == sliderName) { sliderIndex = i; break; }
            }
            writeVarint(binary, sliderIndex, true);
            writeVarint(binary, sliderTimelines.size(), true);
            for (const auto& [timelineTypeStr, timeline] : sliderTimelines) {
                int type = (timelineTypeStr == "time") ? 0 : 1;
                writeByte(binary, type);
                writeVarint(binary, timeline.size(), true);
                int bezierCount = getBezierCount(timeline, 1);
                writeVarint(binary, bezierCount, true);
                writeTimeline(binary, timeline, 1);
            }
        }

        // Attachment timelines
        writeVarint(binary, animation.attachments.size(), true);
        for (const auto& [skinName, skinMap] : animation.attachments) {
            int skinIndex = 0;
            for (size_t i = 0; i < skeletonData.skins.size(); i++) {
                if (skeletonData.skins[i].name == skinName) { skinIndex = i; break; }
            }
            writeVarint(binary, skinIndex, true);
            writeVarint(binary, skinMap.size(), true);
            for (const auto& [slotName, slotMap] : skinMap) {
                int slotIndex = 0;
                for (size_t i = 0; i < skeletonData.slots.size(); i++) {
                    if (skeletonData.slots[i].name == slotName) { slotIndex = i; break; }
                }
                writeVarint(binary, slotIndex, true);
                writeVarint(binary, slotMap.size(), true);
                for (const auto& [attachmentName, attachmentTimelines] : slotMap) {
                    writeStringRef(binary, attachmentName, skeletonData);
                    for (const auto& [timelineTypeStr, timeline] : attachmentTimelines) {
                        AttachmentTimelineType type = attachmentTimelineTypeMap.at(timelineTypeStr);
                        writeByte(binary, static_cast<int>(type));
                        writeVarint(binary, timeline.size(), true);
                        if (type == ATTACHMENT_DEFORM) {
                            int bezierCount = getBezierCount(timeline, 1);
                            writeVarint(binary, bezierCount, true);
                            writeFloat(binary, timeline[0].time);
                            for (size_t frameIndex = 0; frameIndex < timeline.size(); frameIndex++) {
                                const auto& frame = timeline[frameIndex];
                                if (frame.vertices.size() == 0) {
                                    writeVarint(binary, 0, true);
                                } else {
                                    writeVarint(binary, frame.vertices.size(), true);
                                    writeVarint(binary, frame.int1, true);
                                    for (float v : frame.vertices) writeFloat(binary, v);
                                }
                                if (frameIndex < timeline.size() - 1) {
                                    writeFloat(binary, timeline[frameIndex + 1].time);
                                    writeSByte(binary, (signed char)frame.curveType);
                                    if (frame.curveType == CurveType::CURVE_BEZIER) writeCurve(binary, frame);
                                }
                            }
                        } else if (type == ATTACHMENT_SEQUENCE) {
                            for (const auto& frame : timeline) {
                                writeFloat(binary, frame.time);
                                int modeAndIndex = ((int)frame.sequenceMode & 0xf) | (frame.int1 << 4);
                                writeInt(binary, modeAndIndex);
                                writeFloat(binary, frame.value1);
                            }
                        }
                    }
                }
            }
        }

        // DrawOrder timeline
        writeVarint(binary, animation.drawOrder.size(), true);
        for (const auto& frame : animation.drawOrder) {
            writeFloat(binary, frame.time);
            writeVarint(binary, frame.offsets.size(), true);
            for (const auto& offset : frame.offsets) {
                int slotIndex = 0;
                for (size_t i = 0; i < skeletonData.slots.size(); i++) {
                    if (skeletonData.slots[i].name == offset.first) { slotIndex = i; break; }
                }
                writeVarint(binary, slotIndex, true);
                writeVarint(binary, offset.second, true);
            }
        }

        // DrawOrder Folders timeline count (4.3)
        writeVarint(binary, 0, true);

        // Event timeline
        writeVarint(binary, animation.events.size(), true);
        for (const auto& frame : animation.events) {
            writeFloat(binary, frame.time);
            int eventIndex = 0;
            for (size_t i = 0; i < skeletonData.events.size(); i++) {
                if (skeletonData.events[i].name == frame.str1) { eventIndex = i; break; }
            }
            writeVarint(binary, eventIndex, true);
            writeVarint(binary, frame.int1, false);
            writeFloat(binary, frame.value1);
            writeString(binary, frame.str2);
            if (skeletonData.events[eventIndex].audioPath.has_value()) {
                writeFloat(binary, frame.value2);
                writeFloat(binary, frame.value3);
            }
        }
    }

    // Write slider constraint animation indices
    for (const auto& slider : skeletonData.sliderConstraints) {
        int animIdx = 0;
        for (size_t i = 0; i < skeletonData.animations.size(); i++) {
            if (skeletonData.animations[i].name == slider.animation) { animIdx = i; break; }
        }
        writeVarint(binary, animIdx, true);
    }

    return binary; 
}

}
