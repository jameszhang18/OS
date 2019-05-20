
#!/bin/sh
# Mkdir
cp ./A4-self-test/images/emptydisk.img ./A4-self-test/runs/case4-mkdir.img
cp ./A4-self-test/images/onedirectory.img ./A4-self-test/runs/case5-mkdir-2.img

#--- Now, do the test cases ---

# Mkdir
echo "Mkdir Test - level 1"
./ext2_mkdir ./A4-self-test/runs/case4-mkdir.img /level1/
echo "Mkdir Test - level 2"
./ext2_mkdir ./A4-self-test/runs/case5-mkdir-2.img /level1/level2
echo "Mkdir Test - level 3"
./ext2_mkdir ./A4-self-test/runs/case5-mkdir-2.img /level1/level2/level3
echo "Mkdir Test - Path not valid"
./ext2_mkdir ./A4-self-test/runs/case4-mkdir.img /
echo "Mkdir Test - ENOENT"
./ext2_mkdir ./A4-self-test/runs/case5-mkdir-2.img /level1/level2/level4/level5
echo "Mkdir Test - Need an absolute path for formatted disk" 
./ext2_mkdir ./A4-self-test/runs/case5-mkdir-2.img level1/level2/level4
echo "Mkdir Test - level 4"
./ext2_mkdir ./A4-self-test/runs/case5-mkdir-2.img /level1/level2/level3/level4
echo "Mkdir Test - level 5"
./ext2_mkdir ./A4-self-test/runs/case5-mkdir-2.img /level1/level2/level3/level4/level5
echo "Mkdir Test - level 6"
./ext2_mkdir ./A4-self-test/runs/case5-mkdir-2.img /level1/level2/level3/level4/level5/level6
echo "Mkdir Test - No enough blocks or inodes"
./ext2_mkdir ./A4-self-test/runs/case5-mkdir-2.img /level1/level2/level3/level4/level5/level7
for i in $(seq 14); 
do
	./ext2_mkdir ./A4-self-test/runs/case5-mkdir-2.img /level1/$i;
done

# --- Now do the dumps ---
the_files="$(ls ./A4-self-test/runs)"
for the_file in $the_files
do
	g=$(basename $the_file)
	./ext2_dump ./A4-self-test/runs/$g > ./A4-self-test/results/$g.txt
done

# echo "Mkdir Test 4 - compare"
# diff ./A4-self-test/results/case4-mkdir.img.txt ./A4-self-test/solution-results/case4-mkdir.img.txt

# echo "Mkdir Test 5 - compare"
# diff ./A4-self-test/results/case5-mkdir-2.img.txt ./A4-self-test/solution-results/case5-mkdir-2.img.txt