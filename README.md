# Parallel-FileSystem-Comparison
A set of 4 code pieces to compare a number(k) of file systems in parallel using Pthreads and OpenMP.

Contents -
-------------------
1)	A make file

2) 	4 .c files - 

	i) CompareFS(K)Pthread.c [C code of parallel program for comparing k filesystems using the Pthread library]
	
	ii)  CompareFS(K)OpenMP.c [C code of parallel program for comparing k filesystems using the OpenMP library]
	
	iii) CompareFS(K=2)Pthread.c [C code of parallel program for comparing 2 filesystems using the Pthread library]
	
	iv) CompareFS(K=2)OpenMP.c [C code of parallel program for comparing 2 filesystems using the OpenMP library]

To run the program -
--------------------
1) $make on the prompt.

2) Example runs for the three code files - (All input is taken using command line)
	
	i) ./ComparePthreadK k <full path of fileSys 1> <full path of fileSys 2> <..> ... <full path of fileSys k>

	ii) ./CompareOpenMPK k <full path of fileSys 1> <full path of fileSys 2> <..> ... <full path of fileSys k>

	iii) ./ComparePthreadK2 <full path of fileSys 1> <full path of fileSys 2>
	
	iv) ./CompareOpenMPK2 <full path of fileSys 1> <full path of fileSys 2> 

Output format -
---------------

1) For the programs taking as input k file systems, 

FILE <relative name of file> in FSs <list of filesystems number wise>// If a majority of the file systems have a file with the same contents.
FOLDER IN MAJORITY : <relative name of directory> in FSs <list of filesystems number wise>// If a majority of the file systems have the same directory content-wise & name-wise.

At the end,

IDENTICAL // If all file systems are completely identical content-wise and name-wise.
DIFFERENT // If there is a difference.

If the is no folder/file with a majority, the filesystems are divergent. 

2) For the programs specific to 2 file systems, 

Both FSs DIR  : <relative name of directory>   //If both file systems have a directory with same name in same level
Both FSs FILE : <relative name of file>		   //If both file systems have a file with same name and contents in same level
Both FSs HAVE FILE :  <relative name of file> BUT WITH DIFFERENT CONTENT 
ONLY FS 1 DIR  : <relative name of directory>
ONLY FS 2 DIR  : <relative name of file>
ONLY FS 1 FILE  : <relative name of file>
ONLY FS 2 DIR  : <relative name of file>

At the end, 

IDENTICAL FILESYSTEMS // If both file systems are completely identical content-wise and name-wise.
DIFFERENT FILESYSTEMS // If there is any difference


NOTE :
------
1) While running the program, give paths of root directories without an ending '/'. For ii) and iii) Give the number of filesystems (k) too as command line arguments.

2) Directories are expected as input. File (Non-directory) names are not to be given.

3) The 2 C codes specifically for 2 file systems was written earlier. The general case of k filesystems was designed later, The general solution for k filesystems can be used for 2 filesystems, by simply assigning k = 2, but there is performance benefit in the 2 programs written specifically for k = 2. This performance benefit is not present in the general solution for k file systems.

ISSUES AND WORK TO BE DONE:
---------------------------

	1) The program is not scalable and some modifications(replacing the empty while loops with conditional locks) will make it scalable
	2) There is one bug in the 2 programs for k filesystems. They don't give the exact output when k = 2. This will be corrected in future.
	3) Will shortly upload a document explaining the design of the solution and add more comments to the code.
