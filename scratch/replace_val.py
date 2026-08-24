rpath = r'E:\SpineSkeletonDataConverter-main\src\SkeletonData43BinaryReader.cpp'
with open(rpath, 'r', encoding='utf-8') as f: writer = f.read()
writer = writer.replace('.value()', '.value_or("MISSING_STRING")')
with open(rpath, 'w', encoding='utf-8') as f: f.write(writer)
