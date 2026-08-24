import sys

wpath = r'E:\SpineSkeletonDataConverter-main\src\SkeletonData43BinaryWriter.cpp'
with open(wpath, 'r', encoding='utf-8') as f:
    writer = f.read()

missing_parts = '''
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
'''

if '/* Skins */' not in writer:
    writer += missing_parts

with open(wpath, 'w', encoding='utf-8') as f:
    f.write(writer)
