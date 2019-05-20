

rm -rf ./runs/*
rm -rf ./results/*


# Restore
cp ./A4-self-test/images/removed.img ./A4-self-test/runs/case12-rs.img
cp ./A4-self-test/images/removed.img ./A4-self-test/runs/case13-rs-2.img
cp ./A4-self-test/images/removed-largefile.img ./A4-self-test/runs/case14-rs-large.img
cp ./A4-self-test/images/deletedfile.img ./A4-self-test/runs/case16-rs-deletedfile.img
cp ./A4-self-test/images/onefile.img ./A4-self-test/runs/case17-rs-onefile.img

#Restore
echo "Restore Test 1"
./ext2_restore ./A4-self-test/runs/case12-rs.img /c.txt
echo "Restore Test 2"
./ext2_restore ./A4-self-test/runs/case13-rs-2.img /level1/e.txt
echo "Restore Test 3"
./ext2_restore ./A4-self-test/runs/case14-rs-large.img /largefile.txt
echo "Restore Test 4 - rm and restore"
./ext2_rm ./A4-self-test/runs/case12-rs.img /b.txt
./ext2_restore ./A4-self-test/runs/case12-rs.img /b.txt
echo "Restore Test 5 - ENOENT (already used)"
./ext2_rm ./A4-self-test/runs/case12-rs.img /b.txt
./ext2_mkdir ./A4-self-test/runs/case12-rs.img /d
./ext2_mkdir ./A4-self-test/runs/case12-rs.img /e
./ext2_restore ./A4-self-test/runs/case12-rs.img /b.txt
echo "Restore Test 6 - EISDIR"
./ext2_restore ./A4-self-test/runs/case12-rs.img /e
echo "Restore Test 7 - deletedfile"
./ext2_restore ./A4-self-test/runs/case16-rs-deletedfile.img /afile

# --- Now do the dumps ---
the_files="$(ls ./A4-self-test/runs)"
for the_file in $the_files
do
  g=$(basename $the_file)
  ./ext2_dump ./A4-self-test/runs/$g > ./A4-self-test/results/$g.txt
done

# echo "Restore Test 1 - compare"
# diff ./A4-self-test/results/case12-rs.img.txt ./A4-self-test/solution-results/case12-rs.img.txt

# echo "Restore Test 2 - compare"
# diff ./A4-self-test/results/case13-rs-2.img.txt ./A4-self-test/solution-results/case13-rs-2.img.txt

# echo "Restore Test 3 - compare"
# diff ./A4-self-test/results/case14-rs-large.img.txt ./A4-self-test/solution-results/case14-rs-large.img.txt