# Self-test
cp ./A4-self-test/images/twolevel.img ./A4-self-test/runs/sekftest.img
cp ./A4-self-test/images/largefile.img ./A4-self-test/runs/sekftest2.img

echo "Mkdir 1"
./ext2_mkdir ./A4-self-test/runs/sekftest.img /level1/aabb


# Copy a file
echo "Copy 1"
./ext2_cp ./A4-self-test/runs/sekftest.img ./A4-self-test/files/oneblock.txt /level1/file

# Link
echo "Hard Link 1" #error
./ext2_ln ./A4-self-test/runs/sekftest.img /level1 blink

echo "Hard Link 2"
./ext2_ln ./A4-self-test/runs/sekftest.img /level1/level2/bfile /aaaaay.txt

echo "Soft Link 1"
./ext2_ln ./A4-self-test/runs/sekftest.img -s /level1/level2/bfile /z.txtaaa

# Remove 3 times
echo "Remove 1"
./ext2_rm ./A4-self-test/runs/sekftest.img /afile
echo "Remove 2"
./ext2_rm ./A4-self-test/runs/sekftest.img /level1/level2/bfile	
echo "Remove 3"
./ext2_rm ./A4-self-test/runs/sekftest.img /level1/file
echo "Remove Soft Link"
./ext2_rm ./A4-self-test/runs/sekftest.img /z.txtaaa
echo "Remove Hard Link"
./ext2_rm ./A4-self-test/runs/sekftest.img /aaaaay.txt

echo "Remove 4" # error
./ext2_rm ./A4-self-test/runs/sekftest.img /level1/aabb

# Remove Corner Case
echo "Remove Corner" # error
./ext2_rm ./A4-self-test/runs/sekftest.img /level1

# Restore 
echo "Restore 1"
./ext2_restore ./A4-self-test/runs/sekftest.img /level1/level2/bfile
echo "Restore 2"
./ext2_restore ./A4-self-test/runs/sekftest.img /level1/file
echo "Restore 3"
./ext2_restore ./A4-self-test/runs/sekftest.img /afile
echo "Restore Soft"
./ext2_restore ./A4-self-test/runs/sekftest.img /z.txtaaa

# --- Now do the dumps ---
the_files="$(ls ./A4-self-test/runs)"
for the_file in $the_files
do
	g=$(basename $the_file)
	./ext2_dump ./A4-self-test/runs/$g > ./A4-self-test/results/$g.txt
done

echo "Final - compare"
diff ./A4-self-test/results/sekftest.img.txt ./A4-self-test/results/sample.img.txt