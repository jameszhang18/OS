# Copy
cp ./A4-self-test/images/emptydisk.img ./A4-self-test/runs/case1-cp.img
cp ./A4-self-test/images/emptydisk.img ./A4-self-test/runs/case2-cp-large.img
cp ./A4-self-test/images/removed.img ./A4-self-test/runs/case3-cp-rm-dir.img

#Copy
echo "Copy Test 1"
./ext2_cp ./A4-self-test/runs/case1-cp.img ./A4-self-test/files/oneblock.txt /file.txt
echo "Copy Test 2"
./ext2_cp ./A4-self-test/runs/case2-cp-large.img ./A4-self-test/files/largefile.txt /big.txt
echo "Copy Test 3"
./ext2_cp ./A4-self-test/runs/case3-cp-rm-dir.img ./A4-self-test/files/oneblock.txt /level1/file
echo "Copy Test 4"
./ext2_mkdir ./A4-self-test/runs/case3-cp-rm-dir.img /level2
./ext2_mkdir ./A4-self-test/runs/case3-cp-rm-dir.img /level2/level3
./ext2_cp ./A4-self-test/runs/case3-cp-rm-dir.img ./A4-self-test/files/oneblock.txt /level2/oneblock_cpy
echo "Copy Test 5"
./ext2_cp ./A4-self-test/runs/case3-cp-rm-dir.img ./A4-self-test/files/oneblock.txt /level2/level3/
echo "Copy Test - ENOENT: Need an absolute path for formatted disk"
./ext2_cp ./A4-self-test/runs/case3-cp-rm-dir.img ./A4-self-test/files/oneblock.txt level2/level3/
echo "Copy Test - ENOENT: Ext2 formatted disk path not valid"
./ext2_cp ./A4-self-test/runs/case3-cp-rm-dir.img ./A4-self-test/files/oneblock.txt /
echo "Copy Test - ENOENT: Cannot open the source file"
./ext2_cp ./A4-self-test/runs/case3-cp-rm-dir.img ./A4-self-test/files/file /level1
echo "Copy Test - EEXIST: File already exist" 
./ext2_cp ./A4-self-test/runs/case3-cp-rm-dir.img ./A4-self-test/files/oneblock.txt /b.txt
echo "Copy Test - ENOENT: Parent directory not found 1"
./ext2_cp ./A4-self-test/runs/case3-cp-rm-dir.img ./A4-self-test/files/oneblock.txt /level2/level4/oneblock.txt
echo "Copy Test - ENOENT: Parent directory not found 2"
./ext2_cp ./A4-self-test/runs/case3-cp-rm-dir.img ./A4-self-test/files/oneblock.txt /level2/level4/oneblock.txt/


the_files="$(ls ./A4-self-test/runs)"
for the_file in $the_files
do
	g=$(basename $the_file)
	./ext2_dump ./A4-self-test/runs/$g > ./A4-self-test/results/$g.txt
done

# echo "Copy Test 1 - compare"
# diff ./A4-self-test/results/case1-cp.img.txt ./A4-self-test/solution-results/case1-cp.img.txt

# echo "Copy Test 2 - compare"
# diff ./A4-self-test/results/case2-cp-large.img.txt ./A4-self-test/solution-results/case2-cp-large.img.txt

# echo "Copy Test 3 - compare"
# diff ./A4-self-test/results/case3-cp-rm-dir.img.txt ./A4-self-test/solution-results/case3-cp-rm-dir.img.txt