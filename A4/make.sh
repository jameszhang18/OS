cp images/onefile.img onefile.img
ext2_mkdir onefile.img /abc
ext2_mkdir onefile.img /abc/level1
ext2_mkdir onefile.img /abc/level1/level2
ext2_mkdir onefile.img /abc/level1/level3
ext2_mkdir onefile.img /abc/level1/level2/level4

ext2_cp onefile.img self-tester/files/oneblock.txt /abc/level1/bfile
ext2_cp onefile.img self-tester/files/oneblock.txt /abc/level1/cfile
ext2_cp onefile.img self-tester/files/oneblock.txt /abc/level1/level2/cfile

readimage onefile.img