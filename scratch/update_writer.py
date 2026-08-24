import sys
import re

with open(r'E:\SpineSkeletonDataConverter-main\src\SkeletonData43BinaryWriter.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# First, insert getBezierCount right before writeTimeline
bezier_func = '''
int getBezierCount(const Timeline& timeline, int valueNum) {
    int count = 0;
    for (size_t i = 0; i < timeline.size() - 1; i++) {
        if (timeline[i].curveType == CurveType::CURVE_BEZIER) {
            count += timeline[i].bezier.size() > 0 ? timeline[i].bezier.size() : (valueNum * 4);
        }
    }
    return count;
}
'''
if 'getBezierCount' not in content:
    content = content.replace('void writeTimeline(', bezier_func + '\nvoid writeTimeline(')

def repl_writeTimeline(m):
    timeline_var = m.group(1)
    val_num = m.group(2)
    return f'writeVarint(binary, getBezierCount({timeline_var}, {val_num}), true);\n                    writeTimeline(binary, {timeline_var}, {val_num});'

content = re.sub(r'writeTimeline\(binary,\s+([A-Za-z0-9_>.-]+),\s+([0-9]+)\);', repl_writeTimeline, content)

def repl_deform(m):
    return '''case ATTACHMENT_DEFORM: {
                        writeVarint(binary, timeline.size(), true);
                        int bezierCount = 0;
                        for (size_t i = 0; i < timeline.size() - 1; i++) {
                            if (timeline[i].curveType == CurveType::CURVE_BEZIER) {
                                bezierCount += timeline[i].bezier.size() > 0 ? timeline[i].bezier.size() : 4;
                            }
                        }
                        writeVarint(binary, bezierCount, true);
                        writeFloat(binary, timeline[0].time);'''
content = re.sub(r'case ATTACHMENT_DEFORM: \{\s*writeVarint\(binary,\s*timeline\.size\(\),\s*true\);\s*writeFloat\(binary,\s*timeline\[0\]\.time\);', repl_deform, content)

start_draw = content.find('    writeVarint(binary, animation.drawOrder.size(), true);')
end_draw = content.find('}\n\nBinary writeBinaryData(')

draw_order_sliders = '''    writeVarint(binary, animation.sliders.size(), true); 
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
'''

content = content[:start_draw] + draw_order_sliders + content[end_draw:]

with open(r'E:\SpineSkeletonDataConverter-main\src\SkeletonData43BinaryWriter.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
