rpath = r'E:\SpineSkeletonDataConverter-main\src\SkeletonData43BinaryReader.cpp'
with open(rpath, 'r', encoding='utf-8') as f: lines = f.readlines()
new_lines = [l for l in lines if 'std::cout << "Reached "' not in l]
with open(rpath, 'w', encoding='utf-8') as f: f.writelines(new_lines)
