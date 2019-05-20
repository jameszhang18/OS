for run in $(seq 2000 $END);

do
	student_tester 5 schedule4.txt
	cmp target.txt results.txt
done