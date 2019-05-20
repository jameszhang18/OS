# Copy
cp ./self-tester/images/emptydisk.img ./self-tester/runs/case1-cp.img
cp ./self-tester/images/emptydisk.img ./self-tester/runs/case2-cp-large.img
cp ./self-tester/images/removed.img ./self-tester/runs/case3-cp-rm-dir.img

# Copy
echo "Copy Test 1"
./ext2_cp ./self-tester/runs/case1-cp.img ./self-tester/files/oneblock.txt /file.txt
echo "Copy Test 2"
./ext2_cp ./self-tester/runs/case2-cp-large.img ./self-tester/files/largefile.txt /big.txt
echo "Copy Test 3"
./ext2_cp ./self-tester/runs/case3-cp-rm-dir.img ./self-tester/files/oneblock.txt /level1/file

the_files="$(ls ./self-tester/runs)"
for the_file in $the_files
do
	g=$(basename $the_file)
	./ext2_dump ./self-tester/runs/$g > ./self-tester/results/$g.txt
done

echo "Copy Test 1 - compare"
diff ./self-tester/results/case1-cp.img.txt ./self-tester/solution-results/case1-cp.img.txt

echo "Copy Test 2 - compare"
diff ./self-tester/results/case2-cp-large.img.txt ./self-tester/solution-results/case2-cp-large.img.txt

echo "Copy Test 3 - compare"
diff ./self-tester/results/case3-cp-rm-dir.img.txt ./self-tester/solution-results/case3-cp-rm-dir.img.txt