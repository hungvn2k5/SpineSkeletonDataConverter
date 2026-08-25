#include "SkeletonData.h"
#include <iostream>

namespace spine43 {

Sequence readSequence(DataInput* input) {
    Sequence sequence; 
    sequence.count = readVarint(input, true);
    sequence.start = readVarint(input, true);
    sequence.digits = readVarint(input, true);
    sequence.setupIndex = readVarint(input, true);
    return sequence;
}

void readFloatArray(DataInput* input, int n, std::vector<float>& array) {
    array.resize(n, 0);
    for (int i = 0; i < n; i++)
        array[i] = readFloat(input);
}

short readShort(DataInput* input) {
    int ch1 = readByte(input);
    int ch2 = readByte(input);
    return (short)((ch1 << 8) + (ch2 << 0));
}

void readShortArray(DataInput* input, int n, std::vector<unsigned short>& array) {
    array.resize(n, 0);
    for (int i = 0; i < n; i++)
        array[i] = (unsigned short) readVarint(input, true);
}

int readVertices(DataInput* input, std::vector<float>& vertices, bool weighted) {
    int vertexCount = readVarint(input, true); 
    if (!weighted) {
        readFloatArray(input, vertexCount << 1, vertices);
    } else {
        int n = readVarint(input, true); 
        for (int b = 0; b < n;) {
            int boneCount = readVarint(input, true); 
            vertices.push_back(boneCount);
            b++;
            for (int ii = 0; ii < boneCount; ii++) {
                vertices.push_back(readVarint(input, true));
                vertices.push_back(readFloat(input));
                vertices.push_back(readFloat(input));
                vertices.push_back(readFloat(input));
                b++;
            }
        }
    }
    return vertexCount; 
}

void readCurve(DataInput* input, TimelineFrame& frame, int timelineCount) {
    for (int i = 0; i < timelineCount * 4; i++) {
        frame.curve.push_back(readFloat(input));
    }
}

Timeline readTimeline(DataInput* input, int frameCount, int valueNum) {
    Timeline timeline;
    float time = readFloat(input);
    float value1 = readFloat(input);
    float value2 = valueNum > 1 ? readFloat(input) : 0.0f;
    float value3 = valueNum > 2 ? readFloat(input) : 0.0f;
    for (int frameIndex = 0; frameIndex < frameCount - 1; frameIndex++) {
        TimelineFrame frame; 
        frame.time = time; 
        frame.value1 = value1; 
        if (valueNum > 1) frame.value2 = value2; 
        if (valueNum > 2) frame.value3 = value3;
        time = readFloat(input);
        value1 = readFloat(input);
        if (valueNum > 1) value2 = readFloat(input);
        if (valueNum > 2) value3 = readFloat(input);
        switch (readSByte(input)) {
            case CURVE_STEPPED: {
                frame.curveType = CurveType::CURVE_STEPPED;
                break; 
            }
            case CURVE_BEZIER: {
                frame.curveType = CurveType::CURVE_BEZIER; 
                readCurve(input, frame, valueNum);
                break; 
            }
        }
        timeline.push_back(frame);
    }
    TimelineFrame frame;
    frame.time = time;
    frame.value1 = value1;
    if (valueNum > 1) frame.value2 = value2;
    if (valueNum > 2) frame.value3 = value3;
    timeline.push_back(frame);
    return timeline;
}

Skin readSkin(DataInput* input, bool defaultSkin, SkeletonData* skeletonData) {
    Skin skin; 
    int slotCount = 0; 
    if (defaultSkin) {
        slotCount = readVarint(input, true);
        if (slotCount == 0) return skin;
        skin.name = "default";
    } else {
        skin.name = readString(input).value_or("");
        if (skeletonData->nonessential) {
            Color color = readColor(input);
            if (color != Color{0xff, 0xff, 0xff, 0xff}) skin.color = color;
        }
        // Skin bones
        for (int i = 0, n = readVarint(input, true); i < n; i++) {
            int bIdx = readVarint(input, true);
            if (bIdx >= 0 && bIdx < (int)skeletonData->bones.size()) {
                skin.bones.push_back(skeletonData->bones[bIdx].name.value_or(""));
            }
        }
        // Skin constraints (unified list in 4.3!)
        for (int i = 0, n = readVarint(input, true); i < n; i++) {
            int cIdx = readVarint(input, true);
            if (cIdx >= 0 && cIdx < (int)skeletonData->constraintNames.size()) {
                skin.constraints.push_back(skeletonData->constraintNames[cIdx]);
            }
        }
        slotCount = readVarint(input, true);
    }
    for (int i = 0; i < slotCount; i++) {
        int slotIndex = readVarint(input, true);
        std::string slotName = skeletonData->slots[slotIndex].name.value_or("");
        int nn = readVarint(input, true);
        for (int ii = 0; ii < nn; ii++) {
            std::string attachmentName = readStringRef(input, skeletonData).value_or("");
            Attachment attachment;
            int flags = readByte(input);
            attachment.name = (flags & 8) != 0 ? readStringRef(input, skeletonData).value_or("") : attachmentName;
            attachment.type = static_cast<AttachmentType>(flags & 0x7);
            switch (attachment.type) {
                case AttachmentType_Region: {
                    RegionAttachment region; 
                    attachment.path = (flags & 16) != 0 ? readStringRef(input, skeletonData).value_or("") : attachment.name;
                    if ((flags & 32) != 0) region.color = readColor(input);
                    if ((flags & 64) != 0) region.sequence = readSequence(input);
                    if ((flags & 128) != 0) region.rotation = readFloat(input);
                    region.x = readFloat(input);
                    region.y = readFloat(input);
                    region.scaleX = readFloat(input);
                    region.scaleY = readFloat(input);
                    region.width = readFloat(input);
                    region.height = readFloat(input);
                    attachment.data = region;
                    break; 
                }
                case AttachmentType_Boundingbox: {
                    BoundingboxAttachment box;
                    attachment.path = attachment.name; 
                    int vertexCount = readVertices(input, box.vertices, (flags & 16) != 0);
                    box.vertexCount = vertexCount;
                    if (skeletonData->nonessential) {
                        Color color = readColor(input);
                        if (color != Color{0xff, 0xff, 0xff, 0xff}) box.color = color;
                    }
                    attachment.data = box;
                    break; 
                }
                case AttachmentType_Mesh: {
                    MeshAttachment mesh; 
                    attachment.path = (flags & 16) != 0 ? readStringRef(input, skeletonData).value_or("") : attachment.name;
                    if ((flags & 32) != 0) {
                        int colorInt = readInt(input);
                        mesh.color = Color{ (unsigned char)((colorInt >> 24) & 0xff), (unsigned char)((colorInt >> 16) & 0xff), (unsigned char)((colorInt >> 8) & 0xff), (unsigned char)(colorInt & 0xff) };
                    }
                    if ((flags & 64) != 0) mesh.sequence = readSequence(input);
                    mesh.hullLength = readVarint(input, true) * 2;
                    mesh.timelines = (flags & 128) != 0 ? 1 : 0;
                    int vertexCount = readVertices(input, mesh.vertices, (flags & 128) != 0);
                    int verticesLength = vertexCount * 2;
                    readFloatArray(input, verticesLength, mesh.uvs);
                    int trianglesCount = readVarint(input, true);
                    readShortArray(input, trianglesCount, mesh.triangles);
                    int timelineSlotsCount = readVarint(input, true);
                    for (int t = 0; t < timelineSlotsCount; t++) {
                        mesh.timelineSlots.push_back(readVarint(input, true));
                    }
                    if (skeletonData->nonessential) {
                        int edgesCount = readVarint(input, true);
                        for (int e = 0; e < edgesCount; e++) mesh.edges.push_back(readVarint(input, true));
                        mesh.width = readFloat(input);
                        mesh.height = readFloat(input);
                    }
                    attachment.data = mesh;
                    break; 
                }
                case AttachmentType_Linkedmesh: {
                    LinkedmeshAttachment linkedMesh; 
                    attachment.path = (flags & 16) != 0 ? readStringRef(input, skeletonData).value_or("") : attachment.name;
                    if ((flags & 32) != 0) {
                        int colorInt = readInt(input);
                        linkedMesh.color = Color{ (unsigned char)((colorInt >> 24) & 0xff), (unsigned char)((colorInt >> 16) & 0xff), (unsigned char)((colorInt >> 8) & 0xff), (unsigned char)(colorInt & 0xff) };
                    }
                    if ((flags & 64) != 0) linkedMesh.sequence = readSequence(input);
                    linkedMesh.timelines = (flags & 128) != 0 ? 1 : 0;
                    linkedMesh.sourceIndex = readVarint(input, true);
                    linkedMesh.skinIndex = readVarint(input, true);
                    linkedMesh.parentMesh = readStringRef(input, skeletonData).value_or("");
                    if (skeletonData->nonessential) {
                        linkedMesh.width = readFloat(input);
                        linkedMesh.height = readFloat(input);
                    }
                    attachment.data = linkedMesh;
                    break; 
                }
                case AttachmentType_Path: {
                    PathAttachment path; 
                    attachment.path = attachment.name; 
                    path.closed = (flags & 16) != 0;
                    path.constantSpeed = (flags & 32) != 0;
                    int vertexCount = readVertices(input, path.vertices, (flags & 64) != 0);
                    path.vertexCount = vertexCount;
                    readFloatArray(input, vertexCount / 3, path.lengths);
                    if (skeletonData->nonessential) {
                        int colorInt = readInt(input);
                        Color color{ (unsigned char)((colorInt >> 24) & 0xff), (unsigned char)((colorInt >> 16) & 0xff), (unsigned char)((colorInt >> 8) & 0xff), (unsigned char)(colorInt & 0xff) };
                        if (color != Color{0xff, 0xff, 0xff, 0xff}) path.color = color;
                    }
                    attachment.data = path;
                    break; 
                }
                case AttachmentType_Point: {
                    PointAttachment point; 
                    attachment.path = attachment.name; 
                    point.rotation = readFloat(input);
                    point.x = readFloat(input);
                    point.y = readFloat(input);
                    if (skeletonData->nonessential) {
                        int colorInt = readInt(input);
                        Color color{ (unsigned char)((colorInt >> 24) & 0xff), (unsigned char)((colorInt >> 16) & 0xff), (unsigned char)((colorInt >> 8) & 0xff), (unsigned char)(colorInt & 0xff) };
                        if (color != Color{0xff, 0xff, 0xff, 0xff}) point.color = color;
                    }
                    attachment.data = point;
                    break; 
                }
                case AttachmentType_Clipping: {
                    ClippingAttachment clipping; 
                    attachment.path = attachment.name; 
                    int endSlotIndex = readVarint(input, true);
                    if (endSlotIndex >= 0 && endSlotIndex < (int) skeletonData->slots.size()) {
                        clipping.endSlot = skeletonData->slots[endSlotIndex].name;
                    }
                    int vertexCount = readVertices(input, clipping.vertices, (flags & 16) != 0);
                    clipping.vertexCount = vertexCount;
                    if (skeletonData->nonessential) {
                        int colorInt = readInt(input);
                        Color color{ (unsigned char)((colorInt >> 24) & 0xff), (unsigned char)((colorInt >> 16) & 0xff), (unsigned char)((colorInt >> 8) & 0xff), (unsigned char)(colorInt & 0xff) };
                        if (color != Color{0xff, 0xff, 0xff, 0xff}) clipping.color = color;
                    }
                    attachment.data = clipping;
                    break; 
                }
            }
            skin.attachments[slotName][attachmentName] = attachment;
        }
    }
    return skin;
}

Animation readAnimation(DataInput* input, SkeletonData* skeletonData) {
    Animation animation; 
    animation.name = readString(input).value_or("");
    
    // Slot timelines
    for (int i = 0, n = readVarint(input, true); i < n; i++) {
        int slotIndex = readVarint(input, true);
        std::string slotName = skeletonData->slots[slotIndex].name.value_or("");
        MultiTimeline slotTimeline; 
        int nn = readVarint(input, true);
        for (int ii = 0; ii < nn; ii++) {
            SlotTimelineType timelineType = static_cast<SlotTimelineType>(readByte(input));
            int frameCount = readVarint(input, true);
            switch (timelineType) {
                case SlotTimelineType::SLOT_ATTACHMENT: {
                    Timeline timeline;
                    for (int frameIndex = 0; frameIndex < frameCount; frameIndex++) {
                        TimelineFrame frame; 
                        frame.time = readFloat(input);
                        frame.str1 = readStringRef(input, skeletonData);
                        timeline.push_back(frame);
                    }
                    slotTimeline["attachment"] = timeline;
                    break; 
                }
                case SlotTimelineType::SLOT_RGBA: {
                    Timeline timeline;
                    int bezierCount = readVarint(input, true);
                    float time = readFloat(input);
                    Color color = readColor(input);
                    for (int frameIndex = 0; frameIndex < frameCount - 1; frameIndex++) {
                        TimelineFrame frame; 
                        frame.time = time; 
                        frame.color1 = color; 
                        time = readFloat(input);
                        color = readColor(input);
                        switch (readSByte(input)) {
                            case CURVE_STEPPED: {
                                frame.curveType = CurveType::CURVE_STEPPED;
                                break; 
                            }
                            case CURVE_BEZIER: {
                                frame.curveType = CurveType::CURVE_BEZIER;
                                readCurve(input, frame, 4);
                                break; 
                            }
                        }
                        timeline.push_back(frame);
                    }
                    TimelineFrame frame;
                    frame.time = time;
                    frame.color1 = color;
                    timeline.push_back(frame);
                    slotTimeline["rgba"] = timeline;
                    break; 
                }
                case SlotTimelineType::SLOT_RGB: {
                    Timeline timeline;
                    int bezierCount = readVarint(input, true);
                    float time = readFloat(input);
                    Color color = readColor(input, false);
                    for (int frameIndex = 0; frameIndex < frameCount - 1; frameIndex++) {
                        TimelineFrame frame; 
                        frame.time = time; 
                        frame.color1 = color; 
                        time = readFloat(input);
                        color = readColor(input, false);
                        switch (readSByte(input)) {
                            case CURVE_STEPPED: {
                                frame.curveType = CurveType::CURVE_STEPPED;
                                break; 
                            }
                            case CURVE_BEZIER: {
                                frame.curveType = CurveType::CURVE_BEZIER;
                                readCurve(input, frame, 3);
                                break; 
                            }
                        }
                        timeline.push_back(frame);
                    }
                    TimelineFrame frame;
                    frame.time = time;
                    frame.color1 = color;
                    timeline.push_back(frame);
                    slotTimeline["rgb"] = timeline;
                    break; 
                }
                case SlotTimelineType::SLOT_RGBA2: {
                    Timeline timeline;
                    int bezierCount = readVarint(input, true);
                    float time = readFloat(input);
                    Color light = readColor(input);
                    Color dark = readColor(input, false);
                    for (int frameIndex = 0; frameIndex < frameCount - 1; frameIndex++) {
                        TimelineFrame frame; 
                        frame.time = time; 
                        frame.color1 = light; 
                        frame.color2 = dark; 
                        time = readFloat(input);
                        light = readColor(input);
                        dark = readColor(input, false);
                        switch (readSByte(input)) {
                            case CURVE_STEPPED: {
                                frame.curveType = CurveType::CURVE_STEPPED;
                                break; 
                            }
                            case CURVE_BEZIER: {
                                frame.curveType = CurveType::CURVE_BEZIER;
                                readCurve(input, frame, 7);
                                break; 
                            }
                        }
                        timeline.push_back(frame);
                    }
                    TimelineFrame frame;
                    frame.time = time;
                    frame.color1 = light;
                    frame.color2 = dark;
                    timeline.push_back(frame);
                    slotTimeline["rgba2"] = timeline;
                    break; 
                }
                case SlotTimelineType::SLOT_RGB2: {
                    Timeline timeline;
                    int bezierCount = readVarint(input, true);
                    float time = readFloat(input);
                    Color light = readColor(input, false);
                    Color dark = readColor(input, false);
                    for (int frameIndex = 0; frameIndex < frameCount - 1; frameIndex++) {
                        TimelineFrame frame; 
                        frame.time = time; 
                        frame.color1 = light; 
                        frame.color2 = dark; 
                        time = readFloat(input);
                        light = readColor(input, false);
                        dark = readColor(input, false);
                        switch (readSByte(input)) {
                            case CURVE_STEPPED: {
                                frame.curveType = CurveType::CURVE_STEPPED;
                                break; 
                            }
                            case CURVE_BEZIER: {
                                frame.curveType = CurveType::CURVE_BEZIER;
                                readCurve(input, frame, 6);
                                break; 
                            }
                        }
                        timeline.push_back(frame);
                    }
                    TimelineFrame frame;
                    frame.time = time;
                    frame.color1 = light;
                    frame.color2 = dark;
                    timeline.push_back(frame);
                    slotTimeline["rgb2"] = timeline;
                    break; 
                }
                case SlotTimelineType::SLOT_ALPHA: {
                    Timeline timeline;
                    int bezierCount = readVarint(input, true);
                    float time = readFloat(input);
                    float alpha = readByte(input) / 255.0f; 
                    for (int frameIndex = 0; ; frameIndex++) {
                        TimelineFrame frame; 
                        frame.time = time; 
                        frame.value1 = alpha; 
                        if (frameIndex == frameCount - 1) {
                            timeline.push_back(frame);
                            break;
                        }
                        time = readFloat(input);
                        alpha = readByte(input) / 255.0f; 
                        switch (readSByte(input)) {
                            case CURVE_STEPPED: {
                                frame.curveType = CurveType::CURVE_STEPPED;
                                break; 
                            }
                            case CURVE_BEZIER: {
                                frame.curveType = CurveType::CURVE_BEZIER;
                                readCurve(input, frame, 1);
                                break; 
                            }
                        }
                        timeline.push_back(frame);
                    }
                    slotTimeline["alpha"] = timeline;
                    break;
                }
            }
        }
        animation.slots[slotName] = slotTimeline;
    }

    // Bone timelines
    for (int i = 0, n = readVarint(input, true); i < n; i++) {
        std::string boneName = skeletonData->bones[readVarint(input, true)].name.value_or("");
        MultiTimeline boneTimeline;
        int nn = readVarint(input, true);
        for (int ii = 0; ii < nn; ii++) {
            BoneTimelineType timelineType = static_cast<BoneTimelineType>(readByte(input));
            int frameCount = readVarint(input, true);
            if (timelineType == BONE_INHERIT) {
                Timeline timeline; 
                for (int frameIndex = 0; frameIndex < frameCount; frameIndex++) {
                    TimelineFrame frame; 
                    frame.time = readFloat(input);
                    frame.inherit = (Inherit) readByte(input);
                    timeline.push_back(frame);
                }
                boneTimeline["inherit"] = timeline;
                continue; 
            }
            int bezierCount = readVarint(input, true);
            switch (timelineType) {
                case BONE_ROTATE: {
                    boneTimeline["rotate"] = readTimeline(input, frameCount, 1);
                    break;
                }
                case BONE_TRANSLATE: {
                    boneTimeline["translate"] = readTimeline(input, frameCount, 2);
                    break;
                }
                case BONE_TRANSLATEX: {
                    boneTimeline["translatex"] = readTimeline(input, frameCount, 1);
                    break;
                }
                case BONE_TRANSLATEY: {
                    boneTimeline["translatey"] = readTimeline(input, frameCount, 1);
                    break;
                }
                case BONE_SCALE: {
                    boneTimeline["scale"] = readTimeline(input, frameCount, 2);
                    break;
                }
                case BONE_SCALEX: {
                    boneTimeline["scalex"] = readTimeline(input, frameCount, 1);
                    break;
                }
                case BONE_SCALEY: {
                    boneTimeline["scaley"] = readTimeline(input, frameCount, 1);
                    break;
                }
                case BONE_SHEAR: {
                    boneTimeline["shear"] = readTimeline(input, frameCount, 2);
                    break;
                }
                case BONE_SHEARX: {
                    boneTimeline["shearx"] = readTimeline(input, frameCount, 1);
                    break;
                }
                case BONE_SHEARY: {
                    boneTimeline["sheary"] = readTimeline(input, frameCount, 1);
                    break;
                }
            }
        }
        animation.bones[boneName] = boneTimeline;
    }

    // IK constraint timelines
    for (int i = 0, n = readVarint(input, true); i < n; i++) {
        int ikIndex = readVarint(input, true);
        std::string ikName = ikIndex < (int)skeletonData->ikConstraints.size() ? skeletonData->ikConstraints[ikIndex].name.value_or("") : "";
        int frameCount = readVarint(input, true);
        int bezierCount = readVarint(input, true);
        Timeline timeline;
        int flags = readByte(input);
        float time = readFloat(input);
        float mix = (flags & 1) != 0 ? ((flags & 2) != 0 ? readFloat(input) : 1.0f) : 0.0f; 
        float softness = (flags & 4) != 0 ? readFloat(input) : 0.0f;
        bool bendPositive = (flags & 8) != 0;
        bool compress = (flags & 16) != 0;
        bool stretch = (flags & 32) != 0;
        for (int frameIndex = 0; frameIndex < frameCount - 1; frameIndex++) {
            TimelineFrame frame; 
            frame.time = time; 
            frame.value1 = mix; 
            frame.value2 = softness; 
            frame.bendPositive = bendPositive;
            frame.compress = compress;
            frame.stretch = stretch;
            flags = readByte(input);
            time = readFloat(input);
            mix = (flags & 1) != 0 ? ((flags & 2) != 0 ? readFloat(input) : 1.0f) : 0.0f;
            softness = (flags & 4) != 0 ? readFloat(input) : 0.0f;
            bendPositive = (flags & 8) != 0;
            compress = (flags & 16) != 0;
            stretch = (flags & 32) != 0;
            if ((flags & 64) != 0) {
                frame.curveType = CurveType::CURVE_STEPPED;
            } else if ((flags & 128) != 0) {
                frame.curveType = CurveType::CURVE_BEZIER;
                readCurve(input, frame, 2);
            }
            timeline.push_back(frame);
        }
        TimelineFrame frame;
        frame.time = time;
        frame.value1 = mix;
        frame.value2 = softness;
        frame.bendPositive = bendPositive;
        frame.compress = compress;
        frame.stretch = stretch;
        timeline.push_back(frame);
        animation.ik[ikName] = timeline;
    }

    // Transform constraint timelines
    for (int i = 0, n = readVarint(input, true); i < n; i++) {
        int tIndex = readVarint(input, true);
        std::string transformName = tIndex < (int)skeletonData->transformConstraints.size() ? skeletonData->transformConstraints[tIndex].name.value_or("") : "";
        int frameCount = readVarint(input, true);
        int bezierCount = readVarint(input, true);
        Timeline timeline;
        float time = readFloat(input);
        float mixRotate = readFloat(input);
        float mixX = readFloat(input);
        float mixY = readFloat(input);
        float mixScaleX = readFloat(input);
        float mixScaleY = readFloat(input);
        float mixShearY = readFloat(input);
        for (int frameIndex = 0; frameIndex < frameCount - 1; frameIndex++) {
            TimelineFrame frame; 
            frame.time = time; 
            frame.value1 = mixRotate; 
            frame.value2 = mixX; 
            frame.value3 = mixY; 
            frame.value4 = mixScaleX; 
            frame.value5 = mixScaleY; 
            frame.value6 = mixShearY; 
            time = readFloat(input);
            mixRotate = readFloat(input);
            mixX = readFloat(input);
            mixY = readFloat(input);
            mixScaleX = readFloat(input);
            mixScaleY = readFloat(input);
            mixShearY = readFloat(input);
            switch (readSByte(input)) {
                case CURVE_STEPPED: {
                    frame.curveType = CurveType::CURVE_STEPPED;
                    break; 
                }
                case CURVE_BEZIER: {
                    frame.curveType = CurveType::CURVE_BEZIER;
                    readCurve(input, frame, 6);
                    break; 
                }
            }
            timeline.push_back(frame);
        }
        TimelineFrame frame;
        frame.time = time;
        frame.value1 = mixRotate;
        frame.value2 = mixX;
        frame.value3 = mixY;
        frame.value4 = mixScaleX;
        frame.value5 = mixScaleY;
        frame.value6 = mixShearY;
        timeline.push_back(frame);
        animation.transform[transformName] = timeline;
    }

    // Path constraint timelines
    for (int i = 0, n = readVarint(input, true); i < n; i++) {
        int pIndex = readVarint(input, true);
        std::string pathName = pIndex < (int)skeletonData->pathConstraints.size() ? skeletonData->pathConstraints[pIndex].name.value_or("") : "";
        MultiTimeline pathTimeline; 
        int nn = readVarint(input, true);
        for (int ii = 0; ii < nn; ii++) {
            PathTimelineType timelineType = static_cast<PathTimelineType>(readByte(input));
            int frameCount = readVarint(input, true);
            int bezierCount = readVarint(input, true);
            switch (timelineType) {
                case PATH_POSITION: {
                    pathTimeline["position"] = readTimeline(input, frameCount, 1);
                    break;
                }
                case PATH_SPACING: {
                    pathTimeline["spacing"] = readTimeline(input, frameCount, 1);
                    break;
                }
                case PATH_MIX: {
                    pathTimeline["mix"] = readTimeline(input, frameCount, 3);
                    break; 
                }
            }
        }
        animation.path[pathName] = pathTimeline;
    }

    // Physics constraint timelines
    for (int i = 0, n = readVarint(input, true); i < n; i++) {
        int index = readVarint(input, true) - 1;
        std::string physicsName = (index >= 0 && index < (int)skeletonData->physicsConstraints.size()) ? skeletonData->physicsConstraints[index].name.value_or("") : "";
        MultiTimeline physicsTimeline;
        int nn = readVarint(input, true);
        for (int ii = 0; ii < nn; ii++) {
            PhysicsTimelineType timelineType = static_cast<PhysicsTimelineType>(readByte(input));
            int frameCount = readVarint(input, true);
            if (timelineType == PHYSICS_RESET) {
                Timeline timeline; 
                for (int frameIndex = 0; frameIndex < frameCount; frameIndex++) {
                    TimelineFrame frame; 
                    frame.time = readFloat(input);
                    timeline.push_back(frame);
                }
                physicsTimeline["reset"] = timeline;
                continue; 
            }
            int bezierCount = readVarint(input, true);
            switch (timelineType) {
                case PHYSICS_INERTIA: {
                    physicsTimeline["inertia"] = readTimeline(input, frameCount, 1);
                    break;
                }
                case PHYSICS_STRENGTH: {
                    physicsTimeline["strength"] = readTimeline(input, frameCount, 1);
                    break;
                }
                case PHYSICS_DAMPING: {
                    physicsTimeline["damping"] = readTimeline(input, frameCount, 1);
                    break;
                }
                case PHYSICS_MASS: {
                    physicsTimeline["mass"] = readTimeline(input, frameCount, 1);
                    break;
                }
                case PHYSICS_WIND: {
                    physicsTimeline["wind"] = readTimeline(input, frameCount, 1);
                    break;
                }
                case PHYSICS_GRAVITY: {
                    physicsTimeline["gravity"] = readTimeline(input, frameCount, 1);
                    break;
                }
                case PHYSICS_MIX: {
                    physicsTimeline["mix"] = readTimeline(input, frameCount, 1);
                    break;
                }
            }
        }
        animation.physics[physicsName] = physicsTimeline;
    }

    // Slider constraint timelines (4.3)
    for (int i = 0, n = readVarint(input, true); i < n; i++) {
        int sIndex = readVarint(input, true);
        std::string sliderName = sIndex < (int)skeletonData->sliderConstraints.size() ? skeletonData->sliderConstraints[sIndex].name.value_or("") : "";
        MultiTimeline sliderTimeline;
        int nn = readVarint(input, true);
        for (int ii = 0; ii < nn; ii++) {
            int timelineType = readByte(input);
            int frameCount = readVarint(input, true);
            int bezierCount = readVarint(input, true);
            if (timelineType == 0) {
                sliderTimeline["time"] = readTimeline(input, frameCount, 1);
            } else if (timelineType == 1) {
                sliderTimeline["mix"] = readTimeline(input, frameCount, 1);
            }
        }
        animation.sliders[sliderName] = sliderTimeline;
    }

    // Attachment timelines (skin / slot / attachment)
    for (int i = 0, n = readVarint(input, true); i < n; i++) {
        int skinIdx = readVarint(input, true);
        std::string skinName = skinIdx < (int)skeletonData->skins.size() ? skeletonData->skins[skinIdx].name : ""; 
        int nn = readVarint(input, true);
        for (int ii = 0; ii < nn; ii++) {
            int slotIndex = readVarint(input, true);
            std::string slotName = slotIndex < (int)skeletonData->slots.size() ? skeletonData->slots[slotIndex].name.value_or("") : "";
            for (int iii = 0, nnn = readVarint(input, true); iii < nnn; iii++) {
                std::string attachmentName = readStringRef(input, skeletonData).value_or("");
                MultiTimeline attachmentTimeline;
                AttachmentTimelineType timelineType = static_cast<AttachmentTimelineType>(readByte(input));
                int frameCount = readVarint(input, true);
                switch (timelineType) {
                    case ATTACHMENT_DEFORM: {
                        int bezierCount = readVarint(input, true);
                        float time = readFloat(input);
                        for (int frameIndex = 0; ; frameIndex++) {
                            TimelineFrame frame; 
                            frame.time = time; 
                            size_t end = (size_t) readVarint(input, true);
                            if (end != 0) {
                                size_t start = (size_t) readVarint(input, true);
                                frame.int1 = start;
                                end += start;
                                for (size_t v = start; v < end; v++)
                                    frame.vertices.push_back(readFloat(input));
                            }
                            if (frameIndex == frameCount - 1) {
                                attachmentTimeline["deform"].push_back(frame);
                                break; 
                            }
                            time = readFloat(input);
                            switch (readSByte(input)) {
                                case CURVE_STEPPED: {
                                    frame.curveType = CurveType::CURVE_STEPPED;
                                    break; 
                                }
                                case CURVE_BEZIER: {
                                    frame.curveType = CurveType::CURVE_BEZIER;
                                    readCurve(input, frame, 1);
                                    break;
                                }
                            }
                            attachmentTimeline["deform"].push_back(frame);
                        }
                        break; 
                    }
                    case ATTACHMENT_SEQUENCE: {
                        for (int frameIndex = 0; frameIndex < frameCount; frameIndex++) {
                            TimelineFrame frame; 
                            frame.time = readFloat(input);
                            int modeAndIndex = readInt(input);
                            frame.sequenceMode = (SequenceMode) (modeAndIndex & 0xf);
                            frame.int1 = modeAndIndex >> 4; 
                            frame.value1 = readFloat(input);
                            attachmentTimeline["sequence"].push_back(frame);
                        }
                        break; 
                    }
                }
                animation.attachments[skinName][slotName][attachmentName] = attachmentTimeline;
            }
        }
    }

    // DrawOrder timeline
    size_t drawOrderCount = (size_t) readVarint(input, true);
    for (size_t i = 0; i < drawOrderCount; i++) {
        TimelineFrame frame; 
        frame.time = readFloat(input);
        size_t offsetCount = (size_t) readVarint(input, true);
        for (size_t ii = 0; ii < offsetCount; ii++) {
            int slotIdx = readVarint(input, true);
            std::string slotName = slotIdx < (int)skeletonData->slots.size() ? skeletonData->slots[slotIdx].name.value_or("") : "";
            frame.offsets.push_back(std::make_pair(slotName, readVarint(input, true)));
        }
        animation.drawOrder.push_back(frame);
    }

    // DrawOrder Folder timelines (4.3)
    size_t folderCount = (size_t) readVarint(input, true);
    for (size_t i = 0; i < folderCount; i++) {
        size_t folderSlotCount = (size_t) readVarint(input, true);
        for (size_t ii = 0; ii < folderSlotCount; ii++) readVarint(input, true); // skip folder slots
        size_t keyCount = (size_t) readVarint(input, true);
        for (size_t ii = 0; ii < keyCount; ii++) {
            float time = readFloat(input);
            size_t changeCount = (size_t) readVarint(input, true);
            for (size_t c = 0; c < changeCount; c++) {
                readVarint(input, true);
                readVarint(input, true);
            }
        }
    }

    // Event timeline
    int eventCount = readVarint(input, true);
    for (int i = 0; i < eventCount; i++) {
        TimelineFrame frame; 
        frame.time = readFloat(input);
        int eventIndex = readVarint(input, true);
        const EventData& eventData = skeletonData->events[eventIndex];
        frame.str1 = eventData.name;
        frame.int1 = readVarint(input, false);
        frame.value1 = readFloat(input);
        OptStr str = readString(input);
        if (str.has_value()) frame.str2 = str;
        else frame.str2 = eventData.stringValue;
        if (eventData.audioPath) {
            frame.value2 = readFloat(input);
            frame.value3 = readFloat(input);
        }
        animation.events.push_back(frame);
    }
    return animation; 
}

SkeletonData readBinaryData(const Binary& binary) {
    SkeletonData skeletonData;
    DataInput input; 
    input.cursor = binary.data(); 
    input.end = binary.data() + binary.size();

    uint64_t lowHash = (uint64_t) readInt(&input); 
    uint64_t highHash = (uint64_t) readInt(&input);
    skeletonData.hash = highHash << 32 | (lowHash & 0xffffffff);
    if (skeletonData.hash != 0) skeletonData.hashString = std::to_string(skeletonData.hash);
    skeletonData.version = readString(&input);

    skeletonData.x = readFloat(&input); 
    skeletonData.y = readFloat(&input);
    skeletonData.width = readFloat(&input);
    skeletonData.height = readFloat(&input);
    skeletonData.referenceScale = readFloat(&input); 

    skeletonData.nonessential = readBoolean(&input); 

    if (skeletonData.nonessential) {
        skeletonData.fps = readFloat(&input); 
        skeletonData.imagesPath = readString(&input); 
        skeletonData.audioPath = readString(&input);
    }

    int numStrings = readVarint(&input, true);
    for (int i = 0; i < numStrings; i++)
        skeletonData.strings.push_back(readString(&input).value_or(""));
    
    /* Bones */
    int boneCount = readVarint(&input, true);
    for (int i = 0; i < boneCount; i++) {
        BoneData boneData;
        boneData.name = readString(&input).value_or("");
        int parentIndex = i == 0 ? 0 : readVarint(&input, true);
        if (i != 0) {
            boneData.parent = skeletonData.bones[parentIndex].name.value_or("");
        }
        boneData.rotation = readFloat(&input);
        boneData.x = readFloat(&input);
        boneData.y = readFloat(&input);
        boneData.scaleX = readFloat(&input);
        boneData.scaleY = readFloat(&input);
        boneData.shearX = readFloat(&input);
        boneData.shearY = readFloat(&input);
        // 4.3 inherit byte before length float
        boneData.inherit = static_cast<Inherit>(readByte(&input));
        boneData.length = readFloat(&input);
        boneData.skinRequired = readBoolean(&input);
        if (skeletonData.nonessential) {
            Color color = readColor(&input);
            if (color != Color{0x9b, 0x9b, 0x9b, 0xff}) boneData.color = color; 
            boneData.icon = readString(&input);
            boneData.iconSize = readFloat(&input);
            boneData.iconRotation = readFloat(&input);
            boneData.visible = readBoolean(&input);
        }
        skeletonData.bones.push_back(boneData);
    }

    /* Slots */
    int slotCount = readVarint(&input, true);
    for (int i = 0; i < slotCount; i++) {
        SlotData slotData; 
        slotData.name = readString(&input);
        slotData.bone = skeletonData.bones[readVarint(&input, true)].name;
        Color color = readColor(&input);
        if (color != Color{0xff, 0xff, 0xff, 0xff}) slotData.color = color;
        int darkColorInt = readInt(&input);
        if (darkColorInt != -1) {
            unsigned char r = (unsigned char)((darkColorInt >> 16) & 0xff);
            unsigned char g = (unsigned char)((darkColorInt >> 8) & 0xff);
            unsigned char b = (unsigned char)(darkColorInt & 0xff);
            slotData.darkColor = Color{ r, g, b, 255 }; 
        }
        slotData.attachmentName = readStringRef(&input, &skeletonData);
        slotData.blendMode = static_cast<BlendMode>(readVarint(&input, true));
        if (skeletonData.nonessential) slotData.visible = readBoolean(&input);
        skeletonData.slots.push_back(slotData);
    }

    /* Constraints (Unified 4.3) */
    int constraintCount = readVarint(&input, true);
    for (int i = 0; i < constraintCount; i++) {
        std::string name = readString(&input).value_or("");
        skeletonData.constraintNames.push_back(name);
        int type = readByte(&input);
        
        switch (type) {
            case 0: { // IK
                IKConstraintData data;
                data.name = name;
                data.order = i;
                int nn = readVarint(&input, true);
                for (int ii = 0; ii < nn; ii++) {
                    int boneIdx = readVarint(&input, true);
                    data.bones.push_back(skeletonData.bones[boneIdx].name.value_or(""));
                }
                int targetIdx = readVarint(&input, true);
                data.target = skeletonData.bones[targetIdx].name.value_or("");
                int flags = readByte(&input);
                data.skinRequired = (flags & 1) != 0;
                if ((flags & 2) != 0) data.scaleYMode = static_cast<ScaleYMode>(readByte(&input));
                data.bendPositive = (flags & 4) == 0;
                data.compress = (flags & 8) != 0;
                data.stretch = (flags & 16) != 0;
                if ((flags & 32) != 0) data.mix = (flags & 64) != 0 ? readFloat(&input) : 1.0f;
                if ((flags & 128) != 0) data.softness = readFloat(&input);
                skeletonData.ikConstraints.push_back(data);
                break;
            }
            case 2: { // TRANSFORM (CONSTRAINT_TRANSFORM=2 per Unity SkeletonBinary.cs)
                TransformConstraintData data;
                data.name = name;
                data.order = i;
                int nn = readVarint(&input, true);
                for (int ii = 0; ii < nn; ii++) data.bones.push_back(skeletonData.bones[readVarint(&input, true)].name.value_or(""));
                data.target = skeletonData.bones[readVarint(&input, true)].name.value_or("");
                int flags = readByte(&input);
                data.skinRequired = (flags & 1) != 0;
                data.localSource = (flags & 2) != 0;
                data.localTarget = (flags & 4) != 0;
                data.additive = (flags & 8) != 0;
                data.clamp = (flags & 16) != 0;
                int propCount = flags >> 5;
                for (int ii = 0; ii < propCount; ii++) {
                    FromProperty fromProp;
                    fromProp.type = readByte(&input);
                    fromProp.offset = readFloat(&input);
                    int toCount = readByte(&input);
                    for (int jj = 0; jj < toCount; jj++) {
                        ToProperty toProp;
                        toProp.type = readByte(&input);
                        toProp.offset = readFloat(&input);
                        toProp.max = readFloat(&input);
                        toProp.scale = readFloat(&input);
                        fromProp.to.push_back(toProp);
                    }
                    data.properties.push_back(fromProp);
                }
                int offsetFlags = readByte(&input);
                if ((offsetFlags & 1) != 0) data.offsetRotation = readFloat(&input);
                if ((offsetFlags & 2) != 0) data.offsetX = readFloat(&input);
                if ((offsetFlags & 4) != 0) data.offsetY = readFloat(&input);
                if ((offsetFlags & 8) != 0) data.offsetScaleX = readFloat(&input);
                if ((offsetFlags & 16) != 0) data.offsetScaleY = readFloat(&input);
                if ((offsetFlags & 32) != 0) data.offsetShearY = readFloat(&input);
                
                int mixFlags = readByte(&input);
                if ((mixFlags & 1) != 0) data.mixRotate = readFloat(&input);
                if ((mixFlags & 2) != 0) data.mixX = readFloat(&input);
                if ((mixFlags & 4) != 0) data.mixY = readFloat(&input);
                if ((mixFlags & 8) != 0) data.mixScaleX = readFloat(&input);
                if ((mixFlags & 16) != 0) data.mixScaleY = readFloat(&input);
                if ((mixFlags & 32) != 0) data.mixShearY = readFloat(&input);
                skeletonData.transformConstraints.push_back(data);
                break;
            }
            case 1: { // PATH (CONSTRAINT_PATH=1 per Unity SkeletonBinary.cs)
                PathConstraintData data;
                data.name = name;
                data.order = i;
                int nn = readVarint(&input, true);
                for (int ii = 0; ii < nn; ii++) {
                    int boneIdx = readVarint(&input, true);
                    data.bones.push_back(skeletonData.bones[boneIdx].name.value_or(""));
                }
                data.target = skeletonData.slots[readVarint(&input, true)].name.value_or("");
                int flags = readByte(&input);
                data.skinRequired = (flags & 1) != 0;
                data.positionMode = static_cast<PositionMode>((flags >> 1) & 1);
                data.spacingMode = static_cast<SpacingMode>((flags >> 2) & 3);
                data.rotateMode = static_cast<RotateMode>((flags >> 4) & 3);
                if ((flags & 128) != 0) data.offsetRotation = readFloat(&input);
                data.position = readFloat(&input);
                data.spacing = readFloat(&input);
                data.mixRotate = readFloat(&input);
                data.mixX = readFloat(&input);
                data.mixY = readFloat(&input);
                skeletonData.pathConstraints.push_back(data);
                break;
            }
            case 3: { // PHYSICS (type 3 in 4.3 runtime)
                PhysicsConstraintData data;
                data.name = name;
                data.order = i;
                data.bone = skeletonData.bones[readVarint(&input, true)].name.value_or("");
                int flags = readByte(&input);
                data.skinRequired = (flags & 1) != 0;
                if ((flags & 2) != 0) data.x = readFloat(&input);
                if ((flags & 4) != 0) data.y = readFloat(&input);
                if ((flags & 8) != 0) data.rotate = readFloat(&input);
                if ((flags & 16) != 0) {
                    float scaleX = readFloat(&input);
                    if (scaleX < -2) {
                        data.scaleYMode = ScaleYMode_Volume;
                        scaleX = -2 - scaleX;
                    } else if (scaleX < 0) {
                        data.scaleYMode = ScaleYMode_Uniform;
                        scaleX = -1 - scaleX;
                    }
                    data.scaleX = scaleX;
                }
                if ((flags & 32) != 0) data.shearX = readFloat(&input);
                data.limit = ((flags & 64) != 0 ? readFloat(&input) : 5000.0f);
                unsigned char stepByte = readByte(&input);
                data.step = stepByte != 0 ? (1.0f / stepByte) : 60.0f;
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
                break;
            }
            case 4: { // SLIDER (type 4 in 4.3 runtime)
                SliderConstraintData data;
                data.name = name;
                data.order = i;
                int flags = readByte(&input);
                data.skinRequired = (flags & 1) != 0;
                data.loop = (flags & 2) != 0;
                data.additive = (flags & 4) != 0;
                
                if ((flags & 8) != 0) {
                    float value = readFloat(&input);
                    if (skeletonData.nonessential && (flags & 64) != 0)
                        data.max = value;
                    else
                        data.time = value;
                }
                if ((flags & 16) != 0) data.mix = (flags & 32) != 0 ? readFloat(&input) : 1.0f;
                if ((flags & 64) != 0) {
                    data.local = (flags & 128) != 0;
                    data.bone = skeletonData.bones[readVarint(&input, true)].name.value_or("");
                    data.propertyOffset = readFloat(&input);
                    data.propertyType = readByte(&input);
                    data.offset = readFloat(&input);
                    data.scale = readFloat(&input);
                    data.hasProperty = true;
                }
                skeletonData.sliderConstraints.push_back(data);
                break;
            }
        }
    }

    /* Default skin & Skins */
    Skin defaultSkin = readSkin(&input, true, &skeletonData);
    if (!defaultSkin.name.empty()) {
        skeletonData.skins.push_back(defaultSkin);
    }
    int skinCount = readVarint(&input, true);
    for (int i = 0; i < skinCount; i++) {
        skeletonData.skins.push_back(readSkin(&input, false, &skeletonData));
    }

    /* Linked meshes resolve */
    for (auto& skin : skeletonData.skins) {
        for (auto& [slotName, slotMap] : skin.attachments) {
            for (auto& [attachmentName, attachmentObj] : slotMap) {
                if (attachmentObj.type == AttachmentType::AttachmentType_Linkedmesh) {
                    LinkedmeshAttachment& linkedMesh = (LinkedmeshAttachment&) attachmentObj.data;
                    if (linkedMesh.skinIndex >= 0 && linkedMesh.skinIndex < (int)skeletonData.skins.size())
                        linkedMesh.skin = skeletonData.skins[linkedMesh.skinIndex].name;
                }
            }
        }
    }

    /* Events */
    int eventCount = readVarint(&input, true);
    for (int i = 0; i < eventCount; i++) {
        EventData eventData; 
        eventData.name = readString(&input).value_or("");
        eventData.intValue = readVarint(&input, false);
        eventData.floatValue = readFloat(&input);
        eventData.stringValue = readString(&input);
        eventData.audioPath = readString(&input);
        if (eventData.audioPath && eventData.audioPath->length() > 0) {
            eventData.volume = readFloat(&input);
            eventData.balance = readFloat(&input);
        }
        skeletonData.events.push_back(eventData);
    }

    /* Animations */
    int animationCount = readVarint(&input, true);
    for (int i = 0; i < animationCount; i++) {
        Animation animation = readAnimation(&input, &skeletonData);
        skeletonData.animations.push_back(animation);
    }

    /* Slider constraint animation indices (read after animations array) */
    for (auto& slider : skeletonData.sliderConstraints) {
        int animIdx = readVarint(&input, true);
        if (animIdx >= 0 && animIdx < (int)skeletonData.animations.size()) {
            slider.animation = skeletonData.animations[animIdx].name;
        }
    }

    return skeletonData;
}

}
