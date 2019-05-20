all: ext2_mkdir ext2_cp ext2_rm ext2_restore ext2_ln ext2_checker ext2_rm_bonus
 
ext2_mkdir: ext2_mkdir.c ext2_helper.c ext2_helper.h ext2.h
	gcc -Wall -g $^ -o $@

ext2_cp: ext2_cp.c ext2_helper.c ext2_helper.h ext2.h
	gcc -Wall -g $^ -o $@

ext2_rm: ext2_rm.c ext2_helper.c ext2_helper.h ext2.h
	gcc -Wall -g $^ -o $@

ext2_restore: ext2_restore.c ext2_helper.c ext2_helper.h ext2.h
	gcc -Wall -g $^ -o $@

ext2_ln: ext2_ln.c ext2_helper.c ext2_helper.h ext2.h
	gcc -Wall -g $^ -o $@

ext2_checker: ext2_checker.c ext2_helper.c ext2_helper.h ext2.h
	gcc -Wall -g $^ -o $@

ext2_rm_bonus: ext2_rm_bonus.c ext2_helper.c ext2_helper.h ext2.h
	gcc -Wall -g $^ -o $@


clean:
	rm -f ext2_mkdir *.o
	rm -f ext2_cp *.o
	rm -f ext2_rm *.o
	rm -f ext2_restore *.o
	rm -f ext2_ln *.o
	rm -f ext2_checker *.o
	rm -f ext2_rm_bonus *.o
	rm -f *img
	rm -f *txt