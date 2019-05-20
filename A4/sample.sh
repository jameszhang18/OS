# Self-test
cp ./A4-self-test/images/twolevel.img ./A4-self-test/runs/sample.img


# echo "Mkdir 2"
# ./ext2_mkdir ./A4-self-test/runs/sample.img /level1


echo "Mkdir 1"
./ext2_mkdir ./A4-self-test/runs/sample.img /level1/aabb


# Copy a file
echo "Copy 1"
./ext2_cp ./A4-self-test/runs/sample.img ./A4-self-test/files/oneblock.txt /level1/file


# echo "Hard Link 1"
# ./ext2_ln ./A4-self-test/runs/sample.img /level1 /blink

echo "Hard Link 2"
./ext2_ln ./A4-self-test/runs/sample.img /level1/level2/bfile /aaaaay.txt
echo "Remove 2"
./ext2_rm ./A4-self-test/runs/sample.img /level1/level2/bfile

# echo "Soft Link 1"
# ./ext2_ln ./A4-self-test/runs/sample.img -s /level1/level2/bfile /z.txtaaa

# echo "Remove Soft Link"
# ./ext2_rm ./A4-self-test/runs/sample.img /z.txt
echo "Remove Hard Link"
./ext2_rm ./A4-self-test/runs/sample.img /aaaaay.txt

echo "Restore Soft Link"
./ext2_restore ./A4-self-test/runs/sample.img /z.txt
echo "Restore bfile "
./ext2_restore ./A4-self-test/runs/sample.img /level1/level2/bfile

# --- Now do the dumps ---

the_files="$(ls ./A4-self-test/runs)"
for the_file in $the_files
do
	g=$(basename $the_file)
	./ext2_dump ./A4-self-test/runs/$g > ./A4-self-test/results/$g.txt
done