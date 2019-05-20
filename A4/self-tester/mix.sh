cp ./A4-self-test/images/manyfiles.img ./A4-self-test/runs/case18-cp-rm-dir.img

echo "Step1 - Copy 3 files to folder2"
./ext2_cp ./A4-self-test/runs/case18-cp-rm-dir.img ./A4-self-test/files/traces.txt /folder2
./ext2_cp ./A4-self-test/runs/case18-cp-rm-dir.img ./A4-self-test/files/oneblock.txt /folder2
./ext2_cp ./A4-self-test/runs/case18-cp-rm-dir.img ./A4-self-test/files/largefile.txt /folder2/g.txt

echo "Step2 - remove g.txt"
./ext2_rm ./A4-self-test/runs/case18-cp-rm-dir.img /folder2/g.txt
echo "Step3 - restore g.txt"
./ext2_restore ./A4-self-test/runs/case18-cp-rm-dir.img /folder2/g.txt
# echo "Step4 - remove 3 files"
# ./ext2_rm ./A4-self-test/runs/case18-cp-rm-dir.img /folder2/g.txt
# ./ext2_rm ./A4-self-test/runs/case18-cp-rm-dir.img /folder2/oneblock.txt
# ./ext2_rm ./A4-self-test/runs/case18-cp-rm-dir.img /folder2/traces.txt
# echo "Step5 - restore traces.txt"
# ./ext2_restore ./A4-self-test/runs/case18-cp-rm-dir.img /folder2/traces.txt

the_files="$(ls ./A4-self-test/runs)"
for the_file in $the_files
do
	g=$(basename $the_file)
	./ext2_dump ./A4-self-test/runs/$g > ./A4-self-test/results/$g.txt
done
