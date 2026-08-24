import sys

rpath = r'E:\SpineSkeletonDataConverter-main\src\SkeletonData43BinaryReader.cpp'
with open(rpath, 'r', encoding='utf-8') as f: lines = f.readlines()

new_lines = []
for i, line in enumerate(lines):
    new_lines.append(line)
    if line.strip() in ['/* Bones */', '/* Slots */', '/* IK constraints */', '/* Transform constraints */', '/* Path constraints */', '/* Physics constraints */', '/* Skins */', '/* Events */', '/* Animations */']:
        new_lines.append(f'std::cout << "Reached " << "{line.strip()}" << std::endl;\n')

with open(rpath, 'w', encoding='utf-8') as f:
    f.writelines(new_lines)
