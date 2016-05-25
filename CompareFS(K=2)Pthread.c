/*
	Written by Piyush Jain, 2013A7PS415P
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>	
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>
#include "Md5.c"


// All shared variables are global variables.
char nodeName[255]; //shared variable for name of file/dir to be compared
char fileHash[16]; //shared variable for file hash to be compared
pthread_t ids[2];
pthread_mutex_t mutex_lock_object = PTHREAD_MUTEX_INITIALIZER;

int fileSize = 0; //to communicate size of file to be shared
int thread_state = 0; //Used for signalling different states between threads.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      
int rootFS1Len, rootFS2Len;
/*
	An object is any directory other file. The mutex_lock_object can be used to sync -
	
	1) Entering/Leaving the directory in both filesystems.
	2) Start checking a regular file for same content in the file.
	3) And many other signals needed to be passed.
	
	For the different states required in this syncronization, we use an integer - thread_state having values -
	
	1) 0  -  Thread 2 waits for Thread 1 to modify this integer.
	2) 1  -  Thread 1 has sent the name of a directory. Thread 2 is to check for it in its node.
	3) 11 -  Thread 2 found the directory with the name specified. Both threads go ahead and recurse in that directory.
	4) 12 -  Thread 2 couldn't find the directory in its current node. Thread 1 to move ahead.
	5) 2  -  Thread 1 has sent the name of a regular file/ size of file/ hash of file. Thread 2 is to check for it.
	6) 21 -  Thread 2 found the file with the name specified/ matched the size/ hash. Both threads go ahead and check if the files are same.
	7) 22 -  Thread 2 couldn't find the file in its current node/ size mismatch/ hash mismatch. Thread 1 to move ahead.
	8) 3  -  Search in current node is done. Move back up a level in the traversal i.e., backtrack.
	9) 33 -  Thread 2 done checking in node.
*/

int checkFileCopy(char *fileName){
	
	int flag = 1;
	int i;
	char md5Hash[16] = {0}; int mySize = 0;
	FILE *inFile = fopen (fileName, "rb");
    MD5_CTX mdContext;
    int bytes;
    unsigned char data[1024];

  	if (inFile == NULL) {
    	printf ("%s can't be opened.\n", fileName);
    	return;
  	}

  	MD5Init (&mdContext);
  	while ((bytes = fread (data, 1, 1024, inFile)) != 0){
  		MD5Update (&mdContext, data, bytes);
  		mySize += bytes;	
  	}
    
  	MD5Final (&mdContext);
  	for (i = 0; i < 16; i++)
    sprintf (md5Hash+2*i, "%02x", mdContext.digest[i]);
  	fclose (inFile);
  	// Both threads have MD5 hash.
  	//printf("PATH %s HASH VALUE %s", fileName, md5Hash);
  	
	//If it is not a directory, assumed that it is a regular file and not a - fifo , pipe or any other special file.
	//Send name of file -> If matched, send size of file -> If matched, send a hash code
  	if(pthread_equal(pthread_self(), ids[0])>0){
  		//MASTER THREAD
  		//printf("FULLPATH %s\n", fileName);
  		
  		fileSize = mySize;
  		pthread_mutex_lock(&mutex_lock_object);
  			thread_state = 2;
  		pthread_mutex_unlock(&mutex_lock_object);
  		while(thread_state==2){

  		}
  		//printf("GOT RESPONSE 1\n");
  		if(thread_state==21){
  			for (i = 0; i < 16; i++){
  				fileHash[i] = md5Hash[i];
  			}
  			pthread_mutex_lock(&mutex_lock_object);
  				thread_state = 2;
  			pthread_mutex_unlock(&mutex_lock_object);
  			while(thread_state==2){

  			}
  			//printf("GOT RESPONSE 2\n");
  			if(thread_state==21){

  			}
  			else{
  				flag = 0;
  				//printf("HASH UNEQUAL\n");
  			}
  		}
  		else{
  			flag = 0;
  			//printf("SIZE UNEQUAL\n");
  		}
  		//printf("THREAD 1 EXITS\n");
  	}
  	else{
  		//SLAVE THREAD
  		//printf("FULLPATH %s\n", fileName);
  		
  		while(thread_state!=2){

  		}
  		if(mySize == fileSize){
  			pthread_mutex_lock(&mutex_lock_object);
  				thread_state = 21;
  			pthread_mutex_unlock(&mutex_lock_object);
  			while(thread_state!=2){

  			}
  			if(strncmp(md5Hash, fileHash, 16)==0){
  			pthread_mutex_lock(&mutex_lock_object);
  				thread_state = 21;
  			pthread_mutex_unlock(&mutex_lock_object);
  			}
  			else{
  			pthread_mutex_lock(&mutex_lock_object);
  				thread_state = 22;
  			pthread_mutex_unlock(&mutex_lock_object);
  			}
  		}
  		else{
  			pthread_mutex_lock(&mutex_lock_object);
  				thread_state = 22;
  			pthread_mutex_unlock(&mutex_lock_object);
  		}
  		//printf("THREAD 2 EXITS\n");
  	}
	return flag;
}
int checkDirectory(char *dirName){

	int flag = 1, i = 0, n, j = 0;
	struct dirent **result;
	if ((n = scandir(dirName, &result, NULL, alphasort)) < 0)
	{
		perror("scandir");
	}
	if(pthread_equal(pthread_self(), ids[0])>0){
		/*
		Check if name of sub-file is same in both filesystems. 
		If so call this function recursively for that sub-file in both directory.
		*/
				
		/* This is the master thread.*/
		while(i<n){
			//Iterate through files in node.
			char fullPath[1000] = {0};
			strcpy(fullPath, dirName);
			strcat(fullPath, "/");
			strcat(fullPath, result[i]->d_name);

			bzero(nodeName, 255);
			strcpy(nodeName, result[i]->d_name);

			while(thread_state!=0){

			}
			if(result[i]->d_type == DT_DIR){
				if((strcmp(result[i]->d_name, ".")==0)  || (strcmp(result[i]->d_name, "..")==0)){
					i++;
					continue;
				}
				//Signal for a directory and send name of directory.
				while(thread_state!=0){
				}
				pthread_mutex_lock(&mutex_lock_object);
							if(thread_state==33){
							//Print remaining files.
							//printf("Thread 1 got signal moving out\n");
							thread_state = 0;
							pthread_mutex_unlock(&mutex_lock_object);
							break;
						}
					//printf("Thread 1 sent signal for diretory %s\n", result[i]->d_name);
					thread_state = 1;
				pthread_mutex_unlock(&mutex_lock_object);
				
				while(thread_state==1){
				}
				
				pthread_mutex_lock(&mutex_lock_object);
				if(thread_state==11){
					//printf("Thread 1 got +ve signal for file %s\n", result[i]->d_name);
					printf("Both FSs DIR  : %s/%s\n", dirName+rootFS1Len, result[i]->d_name);
					thread_state = 0;
					pthread_mutex_unlock(&mutex_lock_object);
					//printf("Thread 1 Directory :- %s\n", result[i]->d_name);
					if(checkDirectory(fullPath)==0){
						flag = 0;
					}
				}
				else if(thread_state==12){
					//printf("Thread 1 got -ve signal for file %s\n", result[i]->d_name);
					printf("ONLY FS 1 DIR  : %s/%s\n", dirName+rootFS1Len, result[i]->d_name);
					thread_state = 0;
					pthread_mutex_unlock(&mutex_lock_object);
					//Add directory to list of objects only in filesystem 1.
					flag = 0;
					i++;
					continue;	
				}
				else{
					thread_state = 0;
					pthread_mutex_unlock(&mutex_lock_object);
					//printf("Thread 1 got signal moving out\n");
					break;
				}
			}
			else{
				//Signal for a file and check for same copy of file.
				while(thread_state!=0){
				}
				//Thread 1 has sent the name of a file. Thread 2 is to check for it in its node.
				pthread_mutex_lock(&mutex_lock_object);
				if(thread_state==33){
						//Print remaining files.
						//printf("Thread 1 got signal moving out\n");
						thread_state = 0;
						pthread_mutex_unlock(&mutex_lock_object);
						break;
				}
				//printf("Thread 1 sent signal for file %s\n", result[i]->d_name);
					thread_state = 2;
				pthread_mutex_unlock(&mutex_lock_object);
				
				while(thread_state==2){
				}
				
				if(thread_state==21){
					//printf("Thread 1 got +ve for file %s\n", result[i]->d_name);
					pthread_mutex_lock(&mutex_lock_object);
					thread_state = 0;
					pthread_mutex_unlock(&mutex_lock_object);
					if(checkFileCopy(fullPath)==1){
						printf("Both FSs FILE : %s/%s\n", dirName+rootFS1Len, result[i]->d_name);
					}
					else{
						printf("Both FSs HAVE FILE : %s/%s BUT WITH DIFFERENT CONTENT\n", dirName+rootFS1Len, result[i]->d_name);	
						flag = 0;
					}
					pthread_mutex_lock(&mutex_lock_object);
					thread_state = 0;
					pthread_mutex_unlock(&mutex_lock_object);
					i++;
					continue;
				}
				else if(thread_state==22){
					//printf("Thread 1 got -ve for file %s\n", result[i]->d_name);
					printf("ONLY FS 1 FILE : %s/%s\n", dirName+rootFS1Len, result[i]->d_name);
					thread_state = 0;
					pthread_mutex_unlock(&mutex_lock_object);
					//TODO Add file to list of objects only in filesystem 1.
					flag = 0;
					i++;
					continue;
				}
				else{
					thread_state = 0;
					pthread_mutex_unlock(&mutex_lock_object);
					//printf("Thread 1 got signal moving out\n");
					break;
				}

			}
			i++;
		}
		while(i<n){
			if(result[i]->d_type==DT_DIR){
				printf("ONLY FS 1 DIR  : %s/%s\n", dirName+rootFS1Len, result[i]->d_name);
			}
			else{
				printf("ONLY FS 1 FILE : %s/%s\n", dirName+rootFS1Len, result[i]->d_name);
			}
			i++;
			flag = 0;
		}
		free(result);
		//Signal to end node check.
		while(thread_state!=0){

		}
		pthread_mutex_lock(&mutex_lock_object);
			//printf("Hard break by Thread 1. Value of thread_state %d\n", thread_state);
			thread_state = 3;
		pthread_mutex_unlock(&mutex_lock_object);
		//printf("THREAD 1 DONE\n");
	}
	else{
		/*
		Check if name of sub-file is same in both filesystems. 
		If so call this function recursively for that sub-file in both directory.
		*/
		
		/* This is the slave thread. Will use the read end of the pipe*/
		while(1){
			//Iterate through files in node.
			char fullPath[1000] = {0};
			if(i<n){
				if((strcmp(result[i]->d_name, ".")==0)  || (strcmp(result[i]->d_name, "..")==0)){
					i++;
					continue;
				}
				strcpy(fullPath, dirName);
				strcat(fullPath, "/");
				strcat(fullPath, result[i]->d_name);	
			}
				//Check for . and ..
				while((thread_state==33)||(thread_state==22)||(thread_state==12)||(thread_state==21)||(thread_state==11)||(thread_state==0)){
				}
				pthread_mutex_lock(&mutex_lock_object);
				if(thread_state==1){
					int printTillIndex = i;
					while(printTillIndex<n&&(strcmp(nodeName, result[printTillIndex]->d_name)>0)){
						flag = 0;
						if(result[printTillIndex]->d_type==DT_DIR){
							printf("ONLY FS 2 DIR  : %s/%s\n", dirName+rootFS2Len, result[printTillIndex]->d_name);
						}
						else{
							printf("ONLY FS 2 FILE : %s/%s\n", dirName+rootFS2Len, result[printTillIndex]->d_name);
						}
						//printf("HERE1\n");
						printTillIndex++;
					}
					i = printTillIndex;
					if(i>=n){
						thread_state=33;
						pthread_mutex_unlock(&mutex_lock_object);
						//printf("Gave 33\n");
						/*while(thread_state!=3){

						}*/
						//thread_state = 0;
					}
					else if(strcmp(result[i]->d_name, nodeName)==0){
						thread_state=11;
						pthread_mutex_unlock(&mutex_lock_object);
						//printf("Thread 2 also has Directory :- %s\n", result[i]->d_name);
						if(checkDirectory(fullPath)==0){
							flag = 0;
						}
					}
					else{
						thread_state=12;
						pthread_mutex_unlock(&mutex_lock_object);
						flag = 0;
						if(result[i]->d_type==DT_DIR){
							printf("ONLY FS 2 DIR  : %s/%s\n", dirName+rootFS2Len, result[i]->d_name);
						}
						else{
							printf("ONLY FS 2 FILE : %s/%s\n", dirName+rootFS2Len, result[i]->d_name);
						}
						//printf("HERE2\n");
						i++;
						continue;
					}
				}
				else if(thread_state==2){
					int printTillIndex = i;
					while(printTillIndex<n&&(strcmp(nodeName, result[printTillIndex]->d_name)>0)){
						flag = 0;
						if(result[printTillIndex]->d_type==DT_DIR){
							printf("ONLY FS 2 DIR  : %s/%s\n", dirName+rootFS2Len, result[printTillIndex]->d_name);
						}
						else{
							printf("ONLY FS 2 FILE : %s/%s\n", dirName+rootFS2Len, result[printTillIndex]->d_name);
						}
						//printf("HERE3\n");
						printTillIndex++;
					}
					i = printTillIndex;
					if(i>=n){
						thread_state=33;
						pthread_mutex_unlock(&mutex_lock_object);
						/*while(thread_state!=3){

						}*/
						//thread_state = 0;
					}
					else if(strcmp(result[i]->d_name, nodeName)==0){
						thread_state=21;
						pthread_mutex_unlock(&mutex_lock_object);
						checkFileCopy(fullPath);
					}
					else{
						thread_state=22;
						pthread_mutex_unlock(&mutex_lock_object);
						flag = 0;
						if(result[i]->d_type==DT_DIR){
							printf("ONLY FS 2 DIR  : %s/%s\n", dirName+rootFS2Len, result[i]->d_name);
						}
						else{
							printf("ONLY FS 2 FILE : %s/%s\n", dirName+rootFS2Len, result[i]->d_name);
						}
						//printf("HERE4\n");
						i++;
						continue;
					}
				}
				else{
					//Print all your remaining files. They belong only to thread 2.
					//printf("GOT A 3 FROM THREAD 1\n");
					thread_state = 0;
					pthread_mutex_unlock(&mutex_lock_object);
					while(i<n){
						flag = 0;
						if(result[i]->d_type==DT_DIR){
							printf("ONLY FS 2 DIR  : %s/%s\n", dirName+rootFS2Len, result[i]->d_name);
						}
						else{
							printf("ONLY FS 2 FILE : %s/%s\n", dirName+rootFS2Len, result[i]->d_name);
						}
						//printf("HERE5\n");
						i++;
					}
					//printf("Thread 2 got 3. Returning\n");
					return flag;
				}
				i++;
		}
		//printf("THREAD 2 DONE\n");
	}
	//pthread_exit((void *)flag);
	return flag;
}
void * traverseAndCompare(void * root){
	/*
		Function to be run by threads, for comparing the file system from root assigned to it, with the filesystem from root assigned to
		another thread.
	*/
	char * fileSysRoot = (char *)root;
	struct stat * currFileStat;
	currFileStat = malloc(sizeof(struct stat));
	if(stat(fileSysRoot, currFileStat) < 0){
		perror("File stat : ");
	}
	else if(S_ISDIR(currFileStat->st_mode)){
		//Notify other thread for a directory check.
		if(checkDirectory(fileSysRoot)==1){
			//printf("FLAG 1\n");
			pthread_exit((void *)1);
		}
	}
	else{
		//Notify other thread for a file check.
		printf("PROVIDE DIRECTORIES AS INPUT. IGNORE IDENTICAL/DIFFERENT OUTPUT\n");
		exit(0);
	}
	pthread_exit((void *)0);
}
void compareFSK2(char *rootFS1, char *rootFS2){
	
	//int iret1, iret2;
	void * iret1;
	void * iret2;
	pthread_create(ids + 0, NULL, traverseAndCompare, (void *)rootFS1);
	pthread_create(ids + 1, NULL, traverseAndCompare, (void *)rootFS2);
	
	pthread_join( ids[0], &iret1);
	pthread_join( ids[1], &iret2);

	if(iret1==(void *)1&&iret2==(void *)1){
		printf("IDENTICAL FILESYSTEMS\n");
	}
	else{
		printf("DIFFERENT FILESYSTEMS\n");
	}
}
int main(int argc, char *argv[])
{
	if(argc!=3){
		printf("./ComparePthreadK2 <full path of fileSys 1> <full path of fileSys 2>\n");
		exit(0);
	}
	//setenv("VT_GNU_NM", "nm", 0);
	//setenv("VT_GNU_NMFILE", "f.nm", 0);
	//VT_USER_START("main");
	char *rootFS1 = argv[1]; 
	char *rootFS2 = argv[2];
	rootFS1Len = strlen(rootFS1);
	rootFS2Len = strlen(rootFS2);
	compareFSK2(rootFS1, rootFS2);
	//VT_USER_END("main");
}