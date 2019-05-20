
# Mkdir
cp ./A4-self-test/images/emptydisk.img ./A4-self-test/runs/case4-mkdir.img
cp ./A4-self-test/images/onedirectory.img ./A4-self-test/runs/case5-mkdir-2.img

#--- Now, do the test cases ---

# Mkdir
echo "Mkdir Test 4"
./ext2_mkdir ./A4-self-test/runs/case4-mkdir.img /level1
echo "Mkdir Test 5"
./ext2_mkdir ./A4-self-test/runs/case5-mkdir-2.img /level1/level2

# --- Now do the dumps ---
the_files="$(ls ./A4-self-test/runs)"
for the_file in $the_files
do
	g=$(basename $the_file)
	./ext2_dump ./A4-self-test/runs/$g > ./A4-self-test/results/$g.txt
done

echo "Mkdir Test 4 - compare"
diff ./A4-self-test/results/case4-mkdir.img.txt ./A4-self-test/solution-results/case4-mkdir.img.txt

echo "Mkdir Test 5 - compare"
diff ./A4-self-test/results/case5-mkdir-2.img.txt ./A4-self-test/solution-results/case5-mkdir-2.img.txt