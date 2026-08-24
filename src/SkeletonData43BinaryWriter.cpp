#include "SkeletonData.h"
#include <set>

namespace spine43 {

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
    for (int frameIndex = 1; frameIndex < timeline.size(); frameIndex++) {
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

void writeSkin(Binary& binary, const Skin& skin, const SkeletonData& skeletonData, bool defaultSkin) {
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
                if (skeletonData.bones[i].name && *skeletonData.bones[i].name == boneName) {
                    boneIndex = i;
                    break;
                }
            }
            writeVarint(binary, boneIndex, true);
        }
        writeVarint(binary, skin.ik.size(), true);
        for (const std::string& ikName : skin.ik) {
            int ikIndex = 0;
            for (size_t i = 0; i < skeletonData.ikConstraints.size(); i++) {
                if (skeletonData.ikConstraints[i].name && *skeletonData.ikConstraints[i].name == ikName) {
                    ikIndex = i;
                    break;
                }
            }
            writeVarint(binary, ikIndex, true);
        }
        writeVarint(binary, skin.transform.size(), true);
        for (const std::string& transformName : skin.transform) {
            int transformIndex = 0;
            for (size_t i = 0; i < skeletonData.transformConstraints.size(); i++) {
                if (skeletonData.transformConstraints[i].name && *skeletonData.transformConstraints[i].name == transformName) {
                    transformIndex = i;
                    break;
                }
            }
            writeVarint(binary, transformIndex, true);
        }
        writeVarint(binary, skin.path.size(), true);
        for (const std::string& pathName : skin.path) {
            int pathIndex = 0;
            for (size_t i = 0; i < skeletonData.pathConstraints.size(); i++) {
                if (skeletonData.pathConstraints[i].name && *skeletonData.pathConstraints[i].name == pathName) {
                    pathIndex = i;
                    break;
                }
            }
            writeVarint(binary, pathIndex, true);
        }
        writeVarint(binary, skin.physics.size(), true);
        for (const std::string& physicsName : skin.physics) {
            int physicsIndex = 0;
            for (size_t i = 0; i < skeletonData.physicsConstraints.size(); i++) {
                if (skeletonData.physicsConstraints[i].name && *skeletonData.physicsConstraints[i].name == physicsName) {
                    physicsIndex = i;
                    break;
                }
            }
            writeVarint(binary, physicsIndex, true);
        }
        writeVarint(binary, skin.attachments.size(), true);
    }
    for (const auto& [slotName, slotMap] : skin.attachments) {
        int slotIndex = 0;
        for (size_t i = 0; i < skeletonData.slots.size(); i++) {
            if (skeletonData.slots[i].name && *skeletonData.slots[i].name == slotName) {
                slotIndex = i;
                break;
            }
        }
        writeVarint(binary, slotIndex, true);
        writeVarint(binary, slotMap.size(), true);
        for (const auto& [attachmentName, attachment] : slotMap) {
            writeStringRef(binary, attachmentName, skeletonData);
            unsigned char flags = 0; 
            flags |= attachment.type & 0x7; 
            if (attachment.name != attachmentName) flags |= 8; 
            switch (attachment.type) {
                case AttachmentType_Region: {
                    const RegionAttachment& region = std::get<RegionAttachment>(attachment.data);
                    if (attachment.path != attachment.name) flags |= 16;
                    if (region.color) flags |= 32;
                    if (region.sequence) flags |= 64;
                    if (region.rotation != 0.0f) flags |= 128;
                    break; 
                }
                case AttachmentType_Boundingbox: {
                    const BoundingboxAttachment& box = std::get<BoundingboxAttachment>(attachment.data);
                    if (box.vertices.size() != box.vertexCount * 2) flags |= 16;
                    break;
                }
                case AttachmentType_Mesh: {
                    const MeshAttachment& mesh = std::get<MeshAttachment>(attachment.data);
                    if (attachment.path != attachment.name) flags |= 16;
                    if (mesh.color) flags |= 32;
                    if (mesh.sequence) flags |= 64;
                    if (mesh.vertices.size() != mesh.uvs.size()) flags |= 128;
                    break; 
                }
                case AttachmentType_Linkedmesh: {
                    const LinkedmeshAttachment& linkedMesh = std::get<LinkedmeshAttachment>(attachment.data);
                    if (attachment.path != attachment.name) flags |= 16;
                    if (linkedMesh.color) flags |= 32;
                    if (linkedMesh.sequence) flags |= 64;
                    if (linkedMesh.timelines != 0) flags |= 128;
                    break; 
                }
                case AttachmentType_Path: {
                    const PathAttachment& path = std::get<PathAttachment>(attachment.data);
                    if (path.closed) flags |= 16;
                    if (path.constantSpeed) flags |= 32;
                    if (path.vertices.size() != path.vertexCount * 2) flags |= 64;
                    break; 
                }
                case AttachmentType_Clipping: {
                    const ClippingAttachment& clipping = std::get<ClippingAttachment>(attachment.data);
                    if (clipping.vertices.size() != clipping.vertexCount * 2) flags |= 16;
                    break;
                }
            }
            writeByte(binary, flags);
            if ((flags & 8) != 0) writeStringRef(binary, attachment.name, skeletonData);
            switch (attachment.type) {
                case AttachmentType_Region: {
                    const RegionAttachment& region = std::get<RegionAttachment>(attachment.data);
                    if ((flags & 16) != 0) writeStringRef(binary, attachment.path, skeletonData);
                    if ((flags & 32) != 0) writeColor(binary, region.color.value());
                    if ((flags & 64) != 0) writeSequence(binary, region.sequence.value());
                    if ((flags & 128) != 0) writeFloat(binary, region.rotation);
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
                    writeVertices(binary, box.vertices, (flags & 16) != 0);
                    if (skeletonData.nonessential) {
                        if (box.color) writeColor(binary, box.color.value());
                        else writeColor(binary, Color{0xff, 0xff, 0xff, 0xff});
                    }
                    break;
                }
                case AttachmentType_Mesh: {
                    const MeshAttachment& mesh = std::get<MeshAttachment>(attachment.data);
                    if ((flags & 16) != 0) writeStringRef(binary, attachment.path, skeletonData);
                    if ((flags & 32) != 0) writeColor(binary, mesh.color.value());
                    if ((flags & 64) != 0) writeSequence(binary, mesh.sequence.value());

                    // 详见 https://github.com/wang606/SpineSkeletonDataConverter/issues/4 
                    // 按理来说 mesh.triangles.size() = (vertexCount * 2 - mesh.hullLength - 2) * 3
                    // 为了让 Spine 官方工具能正确读取，这里写入计算得到的 hullLength
                    // writeVarint(binary, mesh.hullLength, true);
                    int actualHullLength = mesh.uvs.size() - mesh.triangles.size() / 3 - 2; 
                    writeVarint(binary, actualHullLength, true);
                    if (actualHullLength != mesh.hullLength) {
                        std::cout << "Warning: Mismatch hullLength for mesh attachment '" << attachment.name << "'. Expected: " << mesh.hullLength << ", Actual: " << actualHullLength << std::endl;
                        std::cout << "Actual hullLength will be written to the binary file." << std::endl;
                    }
                    
                    writeVertices(binary, mesh.vertices, (flags & 128) != 0);
                    writeFloatArray(binary, mesh.uvs);
                    writeShortArray(binary, mesh.triangles);
                    if (skeletonData.nonessential) {
                        writeVarint(binary, mesh.edges.size(), true);
                        writeShortArray(binary, mesh.edges);
                        writeFloat(binary, mesh.width);
                        writeFloat(binary, mesh.height);
                    }
                    break; 
                }
                case AttachmentType_Linkedmesh: {
                    const LinkedmeshAttachment& linkedMesh = std::get<LinkedmeshAttachment>(attachment.data);
                    if ((flags & 16) != 0) writeStringRef(binary, attachment.path, skeletonData);
                    if ((flags & 32) != 0) writeColor(binary, linkedMesh.color.value());
                    if ((flags & 64) != 0) writeSequence(binary, linkedMesh.sequence.value());
                    int skinIndex = 0;
                    if (linkedMesh.skin) {
                        for (size_t i = 0; i < skeletonData.skins.size(); i++) {
                            if (skeletonData.skins[i].name == linkedMesh.skin.value()) {
                                skinIndex = i;
                                break;
                            }
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
                    writeVertices(binary, path.vertices, (flags & 64) != 0);
                    writeFloatArray(binary, path.lengths); 
                    if (skeletonData.nonessential) {
                        if (path.color) writeColor(binary, path.color.value());
                        else writeColor(binary, Color{0xff, 0xff, 0xff, 0xff});
                    }
                    break; 
                }
                case AttachmentType_Point: {
                    const PointAttachment& point = std::get<PointAttachment>(attachment.data);
                    writeFloat(binary, point.x);
                    writeFloat(binary, point.y);
                    writeFloat(binary, point.rotation);
                    if (skeletonData.nonessential) {
                        if (point.color) writeColor(binary, point.color.value());
                        else writeColor(binary, Color{0xff, 0xff, 0xff, 0xff});
                    }
                    break;
                }
                case AttachmentType_Clipping: {
                    const ClippingAttachment& clipping = std::get<ClippingAttachment>(attachment.data);
                    int slotIndex = -1; 
                    for (size_t i = 0; i < skeletonData.slots.size(); i++) {
                        if (skeletonData.slots[i].name == clipping.endSlot) {
                            slotIndex = i;
                            break;
                        }
                    }
                    writeVarint(binary, slotIndex, true);
                    writeVertices(binary, clipping.vertices, (flags & 16) != 0);
                    if (skeletonData.nonessential) {
                        if (clipping.color) writeColor(binary, clipping.color.value());
                        else writeColor(binary, Color{0xff, 0xff, 0xff, 0xff});
                    }
                    break;
                }
            }
        }
    }
}

void writeAnimation(Binary& binary, const Animation& animation, const SkeletonData& skeletonData) {
    writeString(binary, animation.name); 
    writeVarint(binary, 0, true); // numTimelines is no use.
    writeVarint(binary, animation.slots.size(), true);
    for (const auto& [slotName, multiTimeline] : animation.slots) {
        int slotIndex = 0; 
        for (size_t i = 0; i < skeletonData.slots.size(); i++) {
            if (skeletonData.slots[i].name && *skeletonData.slots[i].name == slotName) {
                slotIndex = i;
                break;
            }
        }
        writeVarint(binary, slotIndex, true);
        writeVarint(binary, multiTimeline.size(), true);
        for (const auto& [timelineName, timeline] : multiTimeline) {
            SlotTimelineType timelineType = slotTimelineTypeMap.at(timelineName);
            writeByte(binary, (unsigned char)timelineType);
            writeVarint(binary, timeline.size(), true);
            switch (timelineType) {
                case SlotTimelineType::SLOT_ATTACHMENT: {
                    for (const auto& frame : timeline) {
                        writeFloat(binary, frame.time);
                        writeStringRef(binary, frame.str1, skeletonData);
                    }
                    break;
                }
                case SlotTimelineType::SLOT_RGBA: {
                    writeVarint(binary, timeline.size() * 4, true); 
                    writeFloat(binary, timeline[0].time);
                    writeColor(binary, timeline[0].color1.value()); 
                    for (int frameIndex = 1; frameIndex < timeline.size(); frameIndex++) {
                        writeFloat(binary, timeline[frameIndex].time); 
                        writeColor(binary, timeline[frameIndex].color1.value()); 
                        CurveType curveType = timeline[frameIndex - 1].curveType;
                        writeSByte(binary, (signed char)curveType);
                        if (curveType == CurveType::CURVE_BEZIER) {
                            writeCurve(binary, timeline[frameIndex - 1]);
                        }
                    }
                    break;
                }
                case SlotTimelineType::SLOT_RGB: {
                    writeVarint(binary, timeline.size() * 3, true); 
                    writeFloat(binary, timeline[0].time);
                    writeColor(binary, timeline[0].color1.value(), false); 
                    for (int frameIndex = 1; frameIndex < timeline.size(); frameIndex++) {
                        writeFloat(binary, timeline[frameIndex].time); 
                        writeColor(binary, timeline[frameIndex].color1.value(), false); 
                        CurveType curveType = timeline[frameIndex - 1].curveType;
                        writeSByte(binary, (signed char)curveType);
                        if (curveType == CurveType::CURVE_BEZIER) {
                            writeCurve(binary, timeline[frameIndex - 1]);
                        }
                    }
                    break;
                }
                case SlotTimelineType::SLOT_RGBA2: {
                    writeVarint(binary, timeline.size() * 7, true); 
                    writeFloat(binary, timeline[0].time);
                    writeColor(binary, timeline[0].color1.value()); 
                    writeColor(binary, timeline[0].color2.value(), false); 
                    for (int frameIndex = 1; frameIndex < timeline.size(); frameIndex++) {
                        writeFloat(binary, timeline[frameIndex].time); 
                        writeColor(binary, timeline[frameIndex].color1.value()); 
                        writeColor(binary, timeline[frameIndex].color2.value(), false); 
                        CurveType curveType = timeline[frameIndex - 1].curveType;
                        writeSByte(binary, (signed char)curveType);
                        if (curveType == CurveType::CURVE_BEZIER) {
                            writeCurve(binary, timeline[frameIndex - 1]);
                        }
                    }
                    break;
                }
                case SlotTimelineType::SLOT_RGB2: {
                    writeVarint(binary, timeline.size() * 6, true); 
                    writeFloat(binary, timeline[0].time);
                    writeColor(binary, timeline[0].color1.value(), false); 
                    writeColor(binary, timeline[0].color2.value(), false); 
                    for (int frameIndex = 1; frameIndex < timeline.size(); frameIndex++) {
                        writeFloat(binary, timeline[frameIndex].time); 
                        writeColor(binary, timeline[frameIndex].color1.value(), false); 
                        writeColor(binary, timeline[frameIndex].color2.value(), false); 
                        CurveType curveType = timeline[frameIndex - 1].curveType;
                        writeSByte(binary, (signed char)curveType);
                        if (curveType == CurveType::CURVE_BEZIER) {
                            writeCurve(binary, timeline[frameIndex - 1]);
                        }
                    }
                    break;
                }
                case SlotTimelineType::SLOT_ALPHA: {
                    writeVarint(binary, timeline.size(), true);
                    writeFloat(binary, timeline[0].time);
                    writeByte(binary, (int)(timeline[0].value1 * 255.0f));
                    for (int frameIndex = 1; frameIndex < timeline.size(); frameIndex++) {
                        writeFloat(binary, timeline[frameIndex].time); 
                        writeByte(binary, (int)(timeline[frameIndex].value1 * 255.0f));
                        CurveType curveType = timeline[frameIndex - 1].curveType;
                        writeSByte(binary, (signed char)curveType);
                        if (curveType == CurveType::CURVE_BEZIER) {
                            writeCurve(binary, timeline[frameIndex - 1]);
                        }
                    }
                    break;
                }
            }
        }
    }
    writeVarint(binary, animation.bones.size(), true);
    for (const auto& [boneName, multiTimeline] : animation.bones) {
        int boneIndex = 0; 
        for (size_t i = 0; i < skeletonData.bones.size(); i++) {
            if (skeletonData.bones[i].name && *skeletonData.bones[i].name == boneName) {
                boneIndex = i;
                break;
            }
        }
        writeVarint(binary, boneIndex, true);
        writeVarint(binary, multiTimeline.size(), true);
        for (const auto& [timelineName, timeline] : multiTimeline) {
            BoneTimelineType timelineType = boneTimelineTypeMap.at(timelineName);
            writeByte(binary, (unsigned char)timelineType);
            writeVarint(binary, timeline.size(), true);
            if (timelineType == BONE_INHERIT) {
                for (const auto& frame : timeline) {
                    writeFloat(binary, frame.time);
                    writeByte(binary, (unsigned char)frame.inherit);
                }
                continue; 
            }
            switch (timelineType) {
                case BONE_ROTATE:
                case BONE_TRANSLATEX:
                case BONE_TRANSLATEY:
                case BONE_SCALEX:
                case BONE_SCALEY:
                case BONE_SHEARX:
                case BONE_SHEARY: {
                    writeVarint(binary, timeline.size(), true); 
                    writeVarint(binary, getBezierCount(timeline, 1), true);
                    writeTimeline(binary, timeline, 1);
                    break;
                }
                case BONE_TRANSLATE:
                case BONE_SCALE:
                case BONE_SHEAR: {
                    writeVarint(binary, timeline.size() * 2, true); 
                    writeVarint(binary, getBezierCount(timeline, 2), true);
                    writeTimeline(binary, timeline, 2);
                    break;
                }
            }
        }
    }
    writeVarint(binary, animation.ik.size(), true);
    for (const auto& [ikName, timeline] : animation.ik) {
        int ikIndex = 0; 
        for (size_t i = 0; i < skeletonData.ikConstraints.size(); i++) {
            if (skeletonData.ikConstraints[i].name && *skeletonData.ikConstraints[i].name == ikName) {
                ikIndex = i;
                break;
            }
        }
        writeVarint(binary, ikIndex, true);
        writeVarint(binary, timeline.size(), true);
        writeVarint(binary, timeline.size() * 2, true);
        unsigned char flags = 0; 
        if (timeline[0].value1 != 0.0f) {
            flags |= 1; 
            if (timeline[0].value1 != 1.0f) flags |= 2; 
        }
        if (timeline[0].value2 != 0.0f) flags |= 4; 
        if (timeline[0].bendPositive) flags |= 8;
        if (timeline[0].compress) flags |= 16;
        if (timeline[0].stretch) flags |= 32;
        writeByte(binary, flags);
        writeFloat(binary, timeline[0].time);
        if ((flags & 1) != 0 && (flags & 2) != 0) writeFloat(binary, timeline[0].value1);
        if ((flags & 4) != 0) writeFloat(binary, timeline[0].value2);
        for (int frameIndex = 1; frameIndex < timeline.size(); frameIndex++) {
            flags = 0; 
            if (timeline[frameIndex].value1 != 0.0f) {
                flags |= 1; 
                if (timeline[frameIndex].value1 != 1.0f) flags |= 2; 
            }
            if (timeline[frameIndex].value2 != 0.0f) flags |= 4; 
            if (timeline[frameIndex].bendPositive) flags |= 8;
            if (timeline[frameIndex].compress) flags |= 16;
            if (timeline[frameIndex].stretch) flags |= 32;
            if (timeline[frameIndex - 1].curveType == CurveType::CURVE_STEPPED) {
                flags |= 64; 
            } else if (timeline[frameIndex - 1].curveType == CurveType::CURVE_BEZIER) {
                flags |= 128; 
            }
            writeByte(binary, flags);
            writeFloat(binary, timeline[frameIndex].time);
            if ((flags & 1) != 0 && (flags & 2) != 0) writeFloat(binary, timeline[frameIndex].value1);
            if ((flags & 4) != 0) writeFloat(binary, timeline[frameIndex].value2);
            if ((flags & 128) != 0) writeCurve(binary, timeline[frameIndex - 1]);
        }
    }
    writeVarint(binary, animation.transform.size(), true); 
    for (const auto& [transformName, timeline] : animation.transform) {
        int transformIndex = 0; 
        for (size_t i = 0; i < skeletonData.transformConstraints.size(); i++) {
            if (skeletonData.transformConstraints[i].name && *skeletonData.transformConstraints[i].name == transformName) {
                transformIndex = i;
                break;
            }
        }
        writeVarint(binary, transformIndex, true);
        writeVarint(binary, timeline.size(), true);
        writeVarint(binary, timeline.size() * 6, true);
        writeFloat(binary, timeline[0].time);
        writeFloat(binary, timeline[0].value1);
        writeFloat(binary, timeline[0].value2);
        writeFloat(binary, timeline[0].value3);
        writeFloat(binary, timeline[0].value4);
        writeFloat(binary, timeline[0].value5);
        writeFloat(binary, timeline[0].value6);
        for (int frameIndex = 1; frameIndex < timeline.size(); frameIndex++) {
            writeFloat(binary, timeline[frameIndex].time);
            writeFloat(binary, timeline[frameIndex].value1);
            writeFloat(binary, timeline[frameIndex].value2);
            writeFloat(binary, timeline[frameIndex].value3);
            writeFloat(binary, timeline[frameIndex].value4);
            writeFloat(binary, timeline[frameIndex].value5);
            writeFloat(binary, timeline[frameIndex].value6);
            CurveType curveType = timeline[frameIndex - 1].curveType;
            writeSByte(binary, (signed char)curveType);
            if (curveType == CurveType::CURVE_BEZIER) {
                writeCurve(binary, timeline[frameIndex - 1]);
            }
        }
    }
    writeVarint(binary, animation.path.size(), true); 
    for (const auto& [pathName, multiTimeline] : animation.path) {
        int pathIndex = 0; 
        for (size_t i = 0; i < skeletonData.pathConstraints.size(); i++) {
            if (skeletonData.pathConstraints[i].name && *skeletonData.pathConstraints[i].name == pathName) {
                pathIndex = i;
                break;
            }
        }
        writeVarint(binary, pathIndex, true);
        writeVarint(binary, multiTimeline.size(), true);
        for (const auto& [timelineName, timeline] : multiTimeline) {
            PathTimelineType timelineType = pathTimelineTypeMap.at(timelineName);
            writeByte(binary, (unsigned char)timelineType);
            writeVarint(binary, timeline.size(), true);
            switch (timelineType) {
                case PATH_POSITION:
                case PATH_SPACING: {
                    writeVarint(binary, timeline.size(), true); 
                    writeVarint(binary, getBezierCount(timeline, 1), true);
                    writeTimeline(binary, timeline, 1);
                    break;
                }
                case PATH_MIX: {
                    writeVarint(binary, timeline.size() * 3, true); 
                    writeVarint(binary, getBezierCount(timeline, 3), true);
                    writeTimeline(binary, timeline, 3);
                    break;
                }
            }
        }
    }
    writeVarint(binary, animation.physics.size(), true); 
    for (const auto& [physicsName, multiTimeline] : animation.physics) {
        int physicsIndex = -1; 
        for (size_t i = 0; i < skeletonData.physicsConstraints.size(); i++) {
            if (skeletonData.physicsConstraints[i].name && *skeletonData.physicsConstraints[i].name == physicsName) {
                physicsIndex = i;
                break;
            }
        }
        writeVarint(binary, physicsIndex + 1, true);
        writeVarint(binary, multiTimeline.size(), true);
        for (const auto& [timelineName, timeline] : multiTimeline) {
            PhysicsTimelineType timelineType = physicsTimelineTypeMap.at(timelineName);
            writeByte(binary, (unsigned char)timelineType);
            writeVarint(binary, timeline.size(), true);
            if (timelineType == PHYSICS_RESET) {
                for (const auto& frame : timeline) {
                    writeFloat(binary, frame.time);
                }
                continue;
            }
            writeVarint(binary, timeline.size(), true); 
            writeVarint(binary, getBezierCount(timeline, 1), true);
                    writeTimeline(binary, timeline, 1); 
        }
    }
    writeVarint(binary, animation.attachments.size(), true); 
    for (const auto& [skinName, skinMap] : animation.attachments) {
        int skinIndex = 0;
        for (size_t i = 0; i < skeletonData.skins.size(); i++) {
            if (skeletonData.skins[i].name == skinName) {
                skinIndex = i;
                break;
            }
        }
        writeVarint(binary, skinIndex, true);
        writeVarint(binary, skinMap.size(), true);
        for (const auto& [slotName, slotMap] : skinMap) {
            int slotIndex = 0; 
            for (size_t i = 0; i < skeletonData.slots.size(); i++) {
                if (skeletonData.slots[i].name && *skeletonData.slots[i].name == slotName) {
                    slotIndex = i;
                    break;
                }
            }
            writeVarint(binary, slotIndex, true);
            writeVarint(binary, slotMap.size(), true);
            for (const auto& [attachmentName, multiTimeline] : slotMap) {
                writeStringRef(binary, attachmentName, skeletonData); 
                assert(multiTimeline.size() == 1); 
                AttachmentTimelineType timelineType = attachmentTimelineTypeMap.at(multiTimeline.begin()->first); 
                writeByte(binary, (unsigned char)timelineType);
                const auto& timeline = multiTimeline.begin()->second;
                writeVarint(binary, timeline.size(), true);
                switch (timelineType) {
                    case ATTACHMENT_DEFORM: {
                        writeVarint(binary, timeline.size(), true);
                        int bezierCount = 0;
                        for (size_t i = 0; i < timeline.size() - 1; i++) {
                            if (timeline[i].curveType == CurveType::CURVE_BEZIER) {
                                bezierCount += timeline[i].curve.size() > 0 ? timeline[i].curve.size() : 4;
                            }
                        }
                        writeVarint(binary, bezierCount, true);
                        writeFloat(binary, timeline[0].time);
                        for (int frameIndex = 0; ; frameIndex++) {
                            writeVarint(binary, timeline[frameIndex].vertices.size(), true); 
                            if (timeline[frameIndex].vertices.size() > 0) {
                                writeVarint(binary, timeline[frameIndex].int1, true); 
                                for (float v : timeline[frameIndex].vertices) {
                                    writeFloat(binary, v);
                                }
                            }
                            if (frameIndex == timeline.size() - 1) break;
                            writeFloat(binary, timeline[frameIndex + 1].time);
                            CurveType curveType = timeline[frameIndex].curveType;
                            writeSByte(binary, (signed char)curveType);
                            if (curveType == CurveType::CURVE_BEZIER) {
                                writeCurve(binary, timeline[frameIndex]);
                            }
                        }
                        break; 
                    }
                    case ATTACHMENT_SEQUENCE: {
                        for (const auto& frame : timeline) {
                            writeFloat(binary, frame.time);
                            writeInt(binary, (frame.int1 << 4) | (frame.sequenceMode & 0xf));
                            writeFloat(binary, frame.value1);
                        }
                        break;
                    }
                }
            }
        }
    }
    writeVarint(binary, animation.sliders.size(), true); 
    for (const auto& [sliderName, multiTimeline] : animation.sliders) {
        int sliderIndex = -1; 
        for (size_t i = 0; i < skeletonData.sliderConstraints.size(); i++) {
            if (skeletonData.sliderConstraints[i].name && *skeletonData.sliderConstraints[i].name == sliderName) {
                sliderIndex = i;
                break;
            }
        }
        writeVarint(binary, sliderIndex + 1, true);
        writeVarint(binary, multiTimeline.size(), true);
        for (const auto& [timelineName, timeline] : multiTimeline) {
            if (timelineName == "mix") {
                writeByte(binary, 0); // type mix
            } else if (timelineName == "time") {
                writeByte(binary, 1); // type time
            }
            writeVarint(binary, timeline.size(), true);
            writeVarint(binary, getBezierCount(timeline, 1), true);
            writeTimeline(binary, timeline, 1);
        }
    }
    
    if (animation.drawOrder.size() > 0) {
        writeVarint(binary, 1, true); // 1 draw order timeline
        writeByte(binary, 0); // type = draw order
        writeVarint(binary, animation.drawOrder.size(), true);
        for (const auto& frame : animation.drawOrder) {
            writeFloat(binary, frame.time);
            writeVarint(binary, frame.offsets.size(), true);
            for (const auto& [slotName, offset] : frame.offsets) {
                int slotIndex = 0; 
                for (size_t i = 0; i < skeletonData.slots.size(); i++) {
                    if (skeletonData.slots[i].name && *skeletonData.slots[i].name == slotName) {
                        slotIndex = i;
                        break;
                    }
                }
                writeVarint(binary, slotIndex, true);
                writeVarint(binary, offset, true);
            }
        }
    } else {
        writeVarint(binary, 0, true);
    }
    
    writeVarint(binary, animation.events.size(), true);
    for (const auto& frame : animation.events) {
        writeFloat(binary, frame.time);
        int eventIndex = 0;
        for (size_t i = 0; i < skeletonData.events.size(); i++) {
            if (skeletonData.events[i].name == frame.str1) {
                eventIndex = i;
                break;
            }
        }
        const EventData& eventData = skeletonData.events[eventIndex];
        writeVarint(binary, eventIndex, true);
        writeVarint(binary, frame.int1, false);
        writeFloat(binary, frame.value1);
        if (frame.str2 != eventData.stringValue) {
            writeString(binary, frame.str2);
        } else {
            writeString(binary, std::nullopt);
        }
        if (eventData.audioPath) {
            writeFloat(binary, frame.value2);
            writeFloat(binary, frame.value3);
        }
    }
}

Binary writeBinaryData(SkeletonData& skeletonData) {
    Binary binary;
    
    writeInt(binary, skeletonData.hash & 0xffffffff); 
    writeInt(binary, (skeletonData.hash >> 32) & 0xffffffff);
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

    std::set<std::string> strings; 
    for (const SlotData& slot : skeletonData.slots) {
        if (slot.attachmentName) strings.insert(slot.attachmentName.value()); 
    }
    for (const Skin& skin : skeletonData.skins) {
        for (const auto& [slotName, slotMap] : skin.attachments) {
            for (const auto& [attachmentName, attachment] : slotMap) {
                strings.insert(attachmentName);
                if (attachment.name != attachmentName) strings.insert(attachment.name);
                if (attachment.path != attachment.name) strings.insert(attachment.path);
            }
        }
    }
    // Collect attachment names referenced in animations
    for (const Animation& animation : skeletonData.animations) {
        // Collect attachment names from slot attachment timelines
        for (const auto& [slotName, multiTimeline] : animation.slots) {
            if (multiTimeline.contains("attachment")) {
                const auto& timeline = multiTimeline.at("attachment");
                for (const auto& frame : timeline) {
                    if (frame.str1) strings.insert(frame.str1.value());
                }
            }
        }
        // Collect attachment names from deform/sequence animations
        for (const auto& [skinName, skinMap] : animation.attachments) {
            for (const auto& [slotName, slotMap] : skinMap) {
                for (const auto& [attachmentName, multiTimeline] : slotMap) {
                    strings.insert(attachmentName);
                }
            }
        }
    }
    writeVarint(binary, strings.size(), true);
    skeletonData.strings.clear();
    for (const std::string& str : strings) {
        skeletonData.strings.push_back(str);
        writeString(binary, str);
    }

    /* Bones */
    writeVarint(binary, skeletonData.bones.size(), true);
    for (const BoneData& bone : skeletonData.bones) {
        writeString(binary, bone.name);
        if (bone.parent) {
            int parentIndex = 0; 
            for (size_t i = 0; i < skeletonData.bones.size(); i++) {
                if (skeletonData.bones[i].name && *skeletonData.bones[i].name == *bone.parent) {
                    parentIndex = i;
                    break;
                }
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
        writeFloat(binary, bone.length);
        writeVarint(binary, bone.inherit, true); 
        writeBoolean(binary, bone.skinRequired); 
        if (skeletonData.nonessential) {
            if (bone.color) writeColor(binary, bone.color.value());
            else writeColor(binary, Color{0x9b, 0x9b, 0x9b, 0xff});
            writeString(binary, bone.icon); 
            writeBoolean(binary, bone.visible); 
        }
    }

    /* Slots */
    writeVarint(binary, skeletonData.slots.size(), true);
    for (const SlotData& slot : skeletonData.slots) {
        writeString(binary, slot.name);
        int boneIndex = 0; 
        for (size_t i = 0; i < skeletonData.bones.size(); i++) {
            if (skeletonData.bones[i].name && *skeletonData.bones[i].name == slot.bone) {
                boneIndex = i;
                break;
            }
        }
        writeVarint(binary, boneIndex, true);
        if (slot.color) writeColor(binary, slot.color.value());
        else writeColor(binary, Color{0xff, 0xff, 0xff, 0xff});
        if (slot.darkColor) {
            writeByte(binary, slot.darkColor.value().a); 
            writeByte(binary, slot.darkColor.value().r);
            writeByte(binary, slot.darkColor.value().g);
            writeByte(binary, slot.darkColor.value().b);
        } else writeColor(binary, Color{0xff, 0xff, 0xff, 0xff});
        writeStringRef(binary, slot.attachmentName, skeletonData);
        writeVarint(binary, slot.blendMode, true); 
        if (skeletonData.nonessential) writeBoolean(binary, slot.visible);
    }

    /* Constraints */
    writeVarint(binary, 0, true); // Write 0 for unified constraintCount
    /* Skins */
    bool hasDefaultSkin = false;
    for (const Skin& skin : skeletonData.skins) {
        if (skin.name == "default") {
            hasDefaultSkin = true;
            writeSkin(binary, skin, skeletonData, true);
            break;
        }
    }
    if (!hasDefaultSkin) {
        writeVarint(binary, 0, true);
    }
    
    writeVarint(binary, hasDefaultSkin ? skeletonData.skins.size() - 1 : skeletonData.skins.size(), true);
    for (const Skin& skin : skeletonData.skins) {
        if (skin.name != "default") {
            writeSkin(binary, skin, skeletonData, false);
        }
    }

    /* Events */
    writeVarint(binary, skeletonData.events.size(), true);
    for (const EventData& event : skeletonData.events) {
        writeString(binary, event.name);
        writeVarint(binary, event.intValue, false);
        writeFloat(binary, event.floatValue);
        writeString(binary, event.stringValue);
        writeString(binary, event.audioPath);
        if (event.audioPath && event.audioPath.value().length() > 0) {
            writeFloat(binary, event.volume);
            writeFloat(binary, event.balance);
        }
    }

    /* Animations */
    writeVarint(binary, skeletonData.animations.size(), true);
    for (const Animation& animation : skeletonData.animations) {
        writeAnimation(binary, animation, skeletonData);
    }

    return binary;
}

}
