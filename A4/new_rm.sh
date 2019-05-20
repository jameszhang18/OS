# Remove
cp ./A4-self-test/images/manyfiles.img ./A4-self-test/runs/case8-rm.img
cp ./A4-self-test/images/manyfiles.img ./A4-self-test/runs/case9-rm-2.img
cp ./A4-self-test/images/manyfiles.img ./A4-self-test/runs/case10-rm-3.img
cp ./A4-self-test/images/largefile.img ./A4-self-test/runs/case11-rm-large.img

# Remove
echo "Remove Test 8"
./ext2_rm ./A4-self-test/runs/case8-rm.img /c.txt
echo "Remove Test 9"
./ext2_rm ./A4-self-test/runs/case9-rm-2.img /level1/d.txt
echo "Remove Test 10"
./ext2_rm ./A4-self-test/runs/case10-rm-3.img /level1/e.txt
echo "Remove Test 11"
./ext2_rm ./A4-self-test/runs/case11-rm-large.img /largefile.txt
echo "Remove - ENOENT"
./ext2_rm ./A4-self-test/runs/case11-rm-large.img /smallfile.txt
echo "Remove - EISDIR"
./ext2_rm ./A4-self-test/runs/case11-rm-large.img /lost+found

# --- Now do the dumps ---
the_files="$(ls ./A4-self-test/runs)"
for the_file in $the_files
do
	g=$(basename $the_file)
	./ext2_dump ./A4-self-test/runs/$g > ./A4-self-test/results/$g.txt
done

echo "Remove Test 8 - compare"
diff ./A4-self-test/results/case8-rm.img.txt ./A4-self-test/solution-results/case8-rm.img.txt

echo "Remove Test 9 - compare"
diff ./A4-self-test/results/case9-rm-2.img.txt ./A4-self-test/solution-results/case9-rm-2.img.txt

echo "Remove Test 10 - compare"
diff ./A4-self-test/results/case10-rm-3.img.txt ./A4-self-test/solution-results/case10-rm-3.img.txt

echo "Remove Test 11 - compare"
diff ./A4-self-test/results/case11-rm-large.img.txt ./A4-self-test/solution-results/case11-rm-large.img.txt