all:
	gcc CompareFS\(K\=2\)Pthread.c -o ComparePthreadK2 -lpthread 
	gcc CompareFS\(K\)Pthread.c -o ComparePthreadK -lpthread
	gcc CompareFS\(K\=2\)OpenMP.c -o CompareOpenMPK2 -fopenmp
	gcc CompareFS\(K\)OpenMP.c -o CompareOpenMPK -fopenmp
