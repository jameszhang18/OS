# Link
cp ./A4-self-test/images/twolevel.img ./A4-self-test/runs/case6-ln-hard.img
cp ./A4-self-test/images/twolevel.img ./A4-self-test/runs/case7-ln-soft.img

# Link
echo "Link Test 6"
./ext2_ln ./A4-self-test/runs/case6-ln-hard.img /level1/level2/bfile /bfilelink
echo "Link test 7"
./ext2_ln ./A4-self-test/runs/case7-ln-soft.img -s /level1/level2/bfile /bfilesoftlink

# --- Now do the dumps ---
the_files="$(ls ./A4-self-test/runs)"
for the_file in $the_files
do
	g=$(basename $the_file)
	./ext2_dump ./A4-self-test/runs/$g > ./A4-self-test/results/$g.txt
done

echo "Link Test 6 - compare"
diff ./A4-self-test/results/case6-ln-hard.img.txt ./A4-self-test/solution-results/case6-ln-hard.img.txt

echo "Link test 7 - compare"
diff ./A4-self-test/results/case7-ln-soft.img.txt ./A4-self-test/solution-results/case7-ln-soft.img.txt