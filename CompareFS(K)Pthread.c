/*
	Written by Piyush Jain
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>	
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>
#include "Md5.c"

// All shared variables are global variables.
char nodeName[255]; //shared variable for name of file/dir to be compared
char fileHash[16]; //shared variable for file hash to be compared
pthread_t *ids;
pthread_mutex_t mutex_lock_object = PTHREAD_MUTEX_INITIALIZER;

int fileSize = 0; //to communicate size of file to be shared
int * thread_state;
int k; // Number of threads
int * sharedThreadsList;
int sharedThreadCount;
int * rootFSLen;
struct initialData
{
	int selfID;
	char * fileSysRoot;
};
/*
	An object is any directory other file. The mutex_lock_object can be used to sync -
	1) Entering/Leaving the directory in both filesystems.
	2) Start checking a regular file for same content in the file.
	
	For the different states required in this syncronization, we use an integer - thread_state having values -
	1) 0  - Thread 2 waits for Thread 1 to modify this integer.
	2) 1  - Thread 1 has sent the name of a directory. Thread 2 is to check for it in its node.
	3) 11 - Thread 2 found the directory with the name specified. Both threads go ahead and recurse in that directory.
	4) 12 -  Thread 2 couldn't find the directory in its current node. Thread 1 to move ahead.
	5) 2  - Thread 1 has sent the name of a regular file. Thread 2 is to check for it in its node.
	6) 21 - Thread 2 found the file with the name specified. Both threads go ahead and check if the files are same.
	7) 22 - Thread 2 couldn't find the file in its current node. Thread 1 to move ahead.
	8) 3  - Search in current node is done. Move back up a level in the traversal i.e., backtrack.
	9) 33 - Thread 2 done checking in node.
	10) 4 - Matched found and is in majority. Move in.
	11) -1 - Slave thread/ Wait for Command.
*/

int checkFileCopy(char *fileName, int selfID, int * threadsToContact, int numOfThreadsToContact){
	

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
  	if(selfID==threadsToContact[1]){
  		//MASTER THREAD
  		//printf("FULLPATH %s\n", fileName);
  		
  		fileSize = mySize;
  		for (i = 2; i <= numOfThreadsToContact; i++)
  		{
  			while(thread_state[threadsToContact[i]]!=-1){
  				//printf("HERE\n");
  			}
  			pthread_mutex_lock(&mutex_lock_object);
  				thread_state[threadsToContact[i]] = 2;
  			pthread_mutex_unlock(&mutex_lock_object);
  		}

  		for (i = 2; i <= numOfThreadsToContact; i++)
  		{
  			while(thread_state[threadsToContact[i]]==2){
  				//printf("HERE\n");
  			}
  			if(thread_state[threadsToContact[i]]==21){
	  			
	  		}
	  		else{
	  			flag = 0;
	  			//printf("SIZE UNEQUAL\n");
	  		}			
  		}
  		if(flag==0){
  			for (i = 2; i <= numOfThreadsToContact; i++)
			{
				pthread_mutex_lock(&mutex_lock_object);
					thread_state[threadsToContact[i]] = -1;
				pthread_mutex_unlock(&mutex_lock_object);
			}
			return flag;	
  		}
		for (i = 0; i < 16; i++){
			fileHash[i] = md5Hash[i];
		}
		for (i = 2; i <= numOfThreadsToContact; i++)
		{
			pthread_mutex_lock(&mutex_lock_object);
				thread_state[threadsToContact[i]] = 2;
			pthread_mutex_unlock(&mutex_lock_object);
		}
		for (i = 2; i <= numOfThreadsToContact; i++)
		{
			while(thread_state[threadsToContact[i]]==2){

			}
			if(thread_state[threadsToContact[i]]==21){
  			
	  		}
	  		else{
	  			flag = 0;
	  			//printf("HASH UNEQUAL\n");
	  		}			
		}
		for (i = 2; i <= numOfThreadsToContact; i++)
		{
			pthread_mutex_lock(&mutex_lock_object);
				thread_state[threadsToContact[i]] = -1;
			pthread_mutex_unlock(&mutex_lock_object);
		}
			//printf("GOT RESPONSE 2\n");

  		//printf("GOT RESPONSE 1\n");
  		
  		//printf("THREAD 1 EXITS\n");
  	}
  	else{
  		//SLAVE THREAD
  		//printf("FULLPATH %s\n", fileName);
  		while(thread_state[selfID]!=2){
  			
  		}

  		if(mySize == fileSize){
  			pthread_mutex_lock(&mutex_lock_object);
  				thread_state[selfID] = 21;
  			pthread_mutex_unlock(&mutex_lock_object);
  			while(thread_state[selfID]!=2&&thread_state[selfID]!=-1){

  			}
  			if(thread_state[selfID]==-1){
  				return flag;
  			}
  			if(strncmp(md5Hash, fileHash, 16)==0){
	  			pthread_mutex_lock(&mutex_lock_object);
	  				thread_state[selfID] = 21;
	  			pthread_mutex_unlock(&mutex_lock_object);
  			}
  			else{
	  			pthread_mutex_lock(&mutex_lock_object);
	  				thread_state[selfID] = 22;
	  			pthread_mutex_unlock(&mutex_lock_object);
  			}
  		}
  		else{
  			pthread_mutex_lock(&mutex_lock_object);
  				thread_state[selfID] = 22;
  			pthread_mutex_unlock(&mutex_lock_object);
  		}
  		//printf("THREAD 2 EXITS\n");
  	}
	return flag;
}
int checkDirectory(char *dirName, int selfID, int * threadsToContact, int numOfThreadsToContact){

	int flag, i = 0, n = 0, j = 1, crwl, filesMore = 0, flagTotal = 1;
	struct dirent **result;
	if ((n = scandir(dirName, &result, NULL, alphasort)) < 0)
	{
		perror("scandir");
	}
	while(j<=numOfThreadsToContact && threadsToContact[j]!=selfID){
		j++;
	}
	int posInThreadsToContact = j; // Position of self in set of threads
	int *objectsDone;
	objectsDone = (int *) malloc((n)*sizeof(int));
	bzero(objectsDone, (n)*sizeof(int));
	/*
	Check if name of sub-file is same in both filesystems. 
	If so call this function recursively for that sub-file in both directory.
	*/			
	int cntRedun = 0;
	while(i<n && cntRedun<2){
		if((strcmp(result[i]->d_name, ".")==0)  || (strcmp(result[i]->d_name, "..")==0)){
			cntRedun++;
			objectsDone[i]=1;
		}
		i++;
	}
	i=0;
	while(1){
		if(thread_state[selfID]==0){
			while(i<n){
				//printf("THREAD %d in CONTROL\n", selfID);
				//printf("YYYY %d %d %d\n", numOfThreadsToContact, j, (k+1)/2);
				while(i<n&&(objectsDone[i]==1)){
					i++;
				}
				if(i==n){
					//printf("THREAD %d BREAKS\n", selfID);
					break;
				}
				if(selfID>threadsToContact[1]){
					filesMore = 1;
				}
				if((numOfThreadsToContact - posInThreadsToContact+1) < ((k+1)/2)){
					//printf("BREAK DUE TO LESS THREADS %d - %d %d\n", numOfThreadsToContact, posInThreadsToContact, ((k+1)/2));
					//printf("FLAG TOTAL 0 DUE TO NO MAJORITY\n");
					flagTotal = 0;
					break;
				}	
				//Iterate through files in node.
				char fullPath[1000] = {0};
				strncpy(fullPath, dirName, strlen(dirName));
				strncat(fullPath, "/", 1);
				strncat(fullPath, result[i]->d_name, strlen(result[i]->d_name));

				bzero(nodeName, 255);
				strcpy(nodeName, result[i]->d_name);
				objectsDone[i] = 1;
				flag = 1;
				//printf("THREAD %d SENT PATH NAME %s\n", selfID, result[i]->d_name);
				if(result[i]->d_type == DT_DIR){
					j = posInThreadsToContact + 1;
					for(j;j<=numOfThreadsToContact;j++){
						while(thread_state[threadsToContact[j]]!=-1){
						}
						pthread_mutex_lock(&mutex_lock_object);
							thread_state[threadsToContact[j]] = 1;
						pthread_mutex_unlock(&mutex_lock_object);
					}
					int counterConfirmed = 0;
					int * threadsThatMatch;
					threadsThatMatch = (int *) malloc((numOfThreadsToContact+1)*sizeof(int));
					bzero(threadsThatMatch, (numOfThreadsToContact+1)*sizeof(int));
					threadsThatMatch[1] = selfID;
					counterConfirmed=1;
					j = posInThreadsToContact + 1;
					for(j;j<=numOfThreadsToContact;j++){
						while(thread_state[threadsToContact[j]]==1){
						}
						if(thread_state[threadsToContact[j]]==11){
							/*pthread_mutex_lock(&mutex_lock_object);
							thread_state[threadsToContact[j]] = -1;
							pthread_mutex_unlock(&mutex_lock_object);*/
							counterConfirmed++;
							threadsThatMatch[counterConfirmed] = threadsToContact[j];
						}
						else{
							//printf("FLAG TOTAL 0 DUE TO FOLDER NOT FOUND\n");
							flagTotal = 0;
							pthread_mutex_lock(&mutex_lock_object);
								thread_state[threadsToContact[j]] = -1;
							pthread_mutex_unlock(&mutex_lock_object);
						}
					}
					if(counterConfirmed >= ((k+1)/2)){
						sharedThreadCount = counterConfirmed;
						sharedThreadsList =(int*) malloc((counterConfirmed+1)*sizeof(int));
						for (j = 1; j <= sharedThreadCount; j++)
						{
							sharedThreadsList[j] = threadsThatMatch[j];
						}
						j = 2;
						while(j<=counterConfirmed){
							pthread_mutex_lock(&mutex_lock_object);
								thread_state[threadsThatMatch[j]] = 4;
							pthread_mutex_unlock(&mutex_lock_object);
							j++;
						}
						pthread_mutex_lock(&mutex_lock_object);
							thread_state[selfID] = 0;
						pthread_mutex_unlock(&mutex_lock_object);
						//printf("FOLDER %s present in %d files\n", result[i]->d_name, counterConfirmed);
						if((counterConfirmed!=1)&&checkDirectory(fullPath, selfID, threadsThatMatch, counterConfirmed)==0){
							flag = 0;
							flagTotal = 0;
							//printf("FLAG TOTAL 0 DUE TO UNIDENTICAL FOLDERS\n");
						}
						/*if(flag==1){
							printf("FOLDER IN MAJORITY : %s\n", result[i]->d_name);
						}*/
						j = 2;
						
						while(j<=counterConfirmed){
							while(thread_state[threadsThatMatch[j]]!=72&&thread_state[threadsThatMatch[j]]!=71){
							}
							if(thread_state[threadsThatMatch[j]]==72){
								//printf("FLAG TOTAL 0 DUE TO MORE FILES IN LATER THREADS\n");
								flagTotal = 0;
								flag = 0;
							}
							pthread_mutex_lock(&mutex_lock_object);
								thread_state[threadsThatMatch[j]] = -1;
							//printf("CHANGED %d to -1\n", j);
							pthread_mutex_unlock(&mutex_lock_object);
							j++;
						}
						if(flag==1){
							pthread_mutex_lock(&mutex_lock_object);
								printf("FOLDER IN MAJORITY : %s/%s in FSs ", dirName+rootFSLen[selfID], result[i]->d_name);
								int crw;
								for (crw = 1; crw <=counterConfirmed; crw++)
								{
									printf("%d ", threadsThatMatch[crw]);
								}
								printf("\n");
							pthread_mutex_unlock(&mutex_lock_object);
						}
						//free(sharedThreadsList);
						/*pthread_mutex_lock(&mutex_lock_object);
						thread_state[selfID] = 0;
						pthread_mutex_unlock(&mutex_lock_object);*/
					}
					else{
						j = posInThreadsToContact + 1;
						while(j<=numOfThreadsToContact){
							pthread_mutex_lock(&mutex_lock_object);
								thread_state[threadsToContact[j]] = -1;
							pthread_mutex_unlock(&mutex_lock_object);
							j++;
						}
						pthread_mutex_lock(&mutex_lock_object);
							thread_state[selfID] = 0;
						pthread_mutex_unlock(&mutex_lock_object);
					}
				}
				else{
					//Signal for a file and check for same copy of file.
					//Thread 1 has sent the name of a file. Thread 2 is to check for it in its node.
					j = posInThreadsToContact + 1;
					for(j;j<=numOfThreadsToContact;j++){
						while(thread_state[threadsToContact[j]]!=-1){
						}
						pthread_mutex_lock(&mutex_lock_object);
							thread_state[threadsToContact[j]] = 2;
						pthread_mutex_unlock(&mutex_lock_object);
					}
				
					int counterConfirmed = 0;
					int * threadsThatMatch;
					threadsThatMatch = (int *) malloc((numOfThreadsToContact+1)*sizeof(int));
					bzero(threadsThatMatch, (numOfThreadsToContact+1)*sizeof(int));
					threadsThatMatch[1] = selfID;
					counterConfirmed=1;
					j = posInThreadsToContact + 1;
					for(j;j<=numOfThreadsToContact;j++){
						while(thread_state[threadsToContact[j]]==2){
						}
						if(thread_state[threadsToContact[j]]==21){
							counterConfirmed++;
							threadsThatMatch[counterConfirmed] = threadsToContact[j];
						}
						else{
							flagTotal = 0;
							pthread_mutex_lock(&mutex_lock_object);
								thread_state[threadsToContact[j]] = -1;
							pthread_mutex_unlock(&mutex_lock_object);
						}
					}
					if(counterConfirmed >= ((k+1)/2)){
						sharedThreadCount = counterConfirmed;
						sharedThreadsList =(int*) malloc((counterConfirmed+1)*sizeof(int));
						for (j = 1; j <= sharedThreadCount; j++)
						{
							sharedThreadsList[j] = threadsThatMatch[j];
						}
						j = 2;
						while(j<=counterConfirmed){
							pthread_mutex_lock(&mutex_lock_object);
								thread_state[threadsThatMatch[j]] = 4;
							pthread_mutex_unlock(&mutex_lock_object);
							j++;
						}
						pthread_mutex_lock(&mutex_lock_object);
							thread_state[selfID] = 0;
						pthread_mutex_unlock(&mutex_lock_object);
						if(checkFileCopy(fullPath, selfID, threadsThatMatch, counterConfirmed)==1){
							pthread_mutex_lock(&mutex_lock_object);
							printf("FILE %s/%s in FSs ", dirName+rootFSLen[selfID], result[i]->d_name);	
							int crw;
								for (crw = 1; crw <=counterConfirmed; crw++)
								{
									printf("%d ", threadsThatMatch[crw]);
								}
								printf("\n");
							pthread_mutex_unlock(&mutex_lock_object);
						}
						else{
							flagTotal = 0;
						}
						/*j = 2;
						while(j<=counterConfirmed){
							pthread_mutex_lock(&mutex_lock_object);
							thread_state[threadsThatMatch[j]] = -1;
							pthread_mutex_unlock(&mutex_lock_object);
							j++;
						}*/
						//flag = checkFileCopy(fullPath, level+1);
						//free(sharedThreadsList);
						/*pthread_mutex_lock(&mutex_lock_object);
						thread_state[selfID] = 0;
						pthread_mutex_unlock(&mutex_lock_object);*/
					}
					else{
						j = posInThreadsToContact + 1;
						while(j<=numOfThreadsToContact){
							pthread_mutex_lock(&mutex_lock_object);
								thread_state[threadsToContact[j]] = -1;
							pthread_mutex_unlock(&mutex_lock_object);
							j++;
						}
						pthread_mutex_lock(&mutex_lock_object);
							thread_state[selfID] = 0;
						pthread_mutex_unlock(&mutex_lock_object);
					}
				}
				i++;
			}
		}
		else{
				//printf("ID %d\n",selfID);
				while(1){
					while((thread_state[selfID]==-1)){
					}
					while(!(thread_state[selfID]==1||thread_state[selfID]==2||thread_state[selfID]==0)){
					}
					if(thread_state[selfID]==0){
						i = 0;
						break;
					}
					else{
						//printf("THREAD %d in LISTEN STATE\n", selfID);
						int l=0;
						while(l<n){
							if((((result[l]->d_type == DT_DIR)&&thread_state[selfID]==1)||((result[l]->d_type != DT_DIR)&&thread_state[selfID]==2))&&(strcmp(result[l]->d_name, nodeName)==0)){ // Need to check for directory and file
								objectsDone[l]=1;
								pthread_mutex_lock(&mutex_lock_object);
									if(thread_state[selfID]==1){
										thread_state[selfID]=11;
									}
									else{
										thread_state[selfID]=21;
									}
								pthread_mutex_unlock(&mutex_lock_object);
								while(thread_state[selfID]==11||thread_state[selfID]==21){
								}

								if(thread_state[selfID]==4){
									
									char fullPath[1000] = {0};
									strncpy(fullPath, dirName, strlen(dirName));
									strncat(fullPath, "/", 1);
									strncat(fullPath, result[l]->d_name, strlen(result[l]->d_name));

									int counterConfirmed = sharedThreadCount;
									int * threadsThatMatch;
									threadsThatMatch = (int *) malloc((counterConfirmed+1)*sizeof(int));
									bzero(threadsThatMatch, (counterConfirmed+1)*sizeof(int));
									for (j = 1; j <= counterConfirmed; j++)
									{
										threadsThatMatch[j] = sharedThreadsList[j];
									}
									pthread_mutex_lock(&mutex_lock_object);
										thread_state[selfID]=-1;
									pthread_mutex_unlock(&mutex_lock_object);
									//Need to share list of threads to contact.
									
									if(result[l]->d_type==DT_DIR){
										checkDirectory(fullPath, selfID, threadsThatMatch, counterConfirmed);
									}
									else{
										checkFileCopy(fullPath, selfID, threadsThatMatch, counterConfirmed);
										//printf("THREAD %d CHECKING FILE %s\n", selfID, fullPath);
										//flag = checkFileCopy(fullPath, level+1);
									}
									/*pthread_mutex_lock(&mutex_lock_object);
									thread_state[selfID] = -1;
									pthread_mutex_unlock(&mutex_lock_object);*/
								}
								else{
									//Do nothing.
								}
								break;
							}
							l++;
						}
						if(l==n){
							if(thread_state[selfID]==1){
								pthread_mutex_lock(&mutex_lock_object);
										thread_state[selfID]=12;
								pthread_mutex_unlock(&mutex_lock_object);
							}
							else{
								pthread_mutex_lock(&mutex_lock_object);
										thread_state[selfID]=22;
								pthread_mutex_unlock(&mutex_lock_object);
							}
						}
					}
				}
			continue;
		}
		break;	
	}
	free(result);
	//Signal to end node check.
	//Wait for all states to be 33. Then Give a 3 from the minimum one in threadsToContact.
	/*pthread_mutex_lock(&mutex_lock_object);
		thread_state[selfID] = 33;
	pthread_mutex_unlock(&mutex_lock_object);*/
	if(numOfThreadsToContact==1){
		pthread_mutex_lock(&mutex_lock_object);
			thread_state[selfID] = 0;
		pthread_mutex_unlock(&mutex_lock_object);
		return flagTotal;
	}	

	j = posInThreadsToContact + 1;
	if(j<=numOfThreadsToContact){
		while(thread_state[threadsToContact[j]]!=-1){
		}
		//printf("FOUND NEXT MASTER TO BE : %d\n", j);
		pthread_mutex_lock(&mutex_lock_object);
			thread_state[threadsToContact[j]]=0;
		pthread_mutex_unlock(&mutex_lock_object);
	}

	if(selfID==threadsToContact[1]){
		while(thread_state[threadsToContact[numOfThreadsToContact]]!=71&&thread_state[threadsToContact[numOfThreadsToContact]]!=72){

		}
		pthread_mutex_lock(&mutex_lock_object);
			//pthread_cond_wait(&endCond, &mutex_lock_object);
			thread_state[selfID] = 0;
		pthread_mutex_unlock(&mutex_lock_object);
	}
	else{
		// Exit
		if(filesMore==1){
			pthread_mutex_lock(&mutex_lock_object);
				thread_state[selfID] = 72;
			pthread_mutex_unlock(&mutex_lock_object);
		}
		else{
			pthread_mutex_lock(&mutex_lock_object);
				thread_state[selfID] = 71;
			pthread_mutex_unlock(&mutex_lock_object);
		}
		/*if(selfID==threadsToContact[numOfThreadsToContact]){
			pthread_mutex_lock(&mutex_lock_object);
			pthread_cond_signal(&endCond);
			pthread_mutex_unlock(&mutex_lock_object);
		}
		else{
		}*/
		//printf("THREAD %d EXITS AS SLAVE with %d\n", selfID, thread_state[selfID]);
	}
	return flagTotal;
}
void * traverseAndCompare(void * threadData){
	/*
		Function to be run by threads, for comparing the file system from root assigned to it, with the filesystem from root assigned to
		another thread.
	*/

	struct initialData * data = ((struct initialData *)threadData);
	char * fileSysRoot = data->fileSysRoot;
	struct stat * currFileStat;
	currFileStat = malloc(sizeof(struct stat));
	int threadsToContact[k+1], i;
	/*if(data->selfID==2||data->selfID==)
	{
		sleep(4);
	}*/
	for (i = 1; i <=k; i++)
	{
		threadsToContact[i] = i;
	}
	if(stat(fileSysRoot, currFileStat) < 0){
		perror("File stat");
		//printf("%s\n", fileSysRoot);
	}
	else if(S_ISDIR(currFileStat->st_mode)){
		//Notify other thread for a directory check.
		//printf("THREAD'S ID : %d\n", data->selfID);
		if(checkDirectory(fileSysRoot, data->selfID, threadsToContact, k)==1){
			pthread_exit((void *)1);
		}
	}
	else{
		printf("PROVIDE DIRECTORIES AS INPUT. IGNORE IDENTICAL/DIFFERENT OUTPUT\n");
		exit(0);
	}
	pthread_exit((void *)0);
}
void compareFSK(int k, struct initialData forThreads[]){
	
	void  *iret;
	int i;
	ids = (pthread_t *) malloc((k+1)*sizeof(pthread_t));
	thread_state = (int *) malloc((k+1)*sizeof(int));
	thread_state[1]=0;
	for (i = 2; i <= k; i++)
	{
		thread_state[i]=-1;
	}
	for (i = 1; i <=k; i++)
	{
		pthread_create(ids + i, NULL, traverseAndCompare, (void *)(forThreads + i));
	}
	pthread_join( ids[1], &iret);
	for (i = 2; i <=k; i++)
	{
		pthread_join( ids[i], NULL);
	}
	int flag = 1;
	for (i = 1; i <= k; i++)
	{
		if(thread_state[i]==72){
			flag = 0;
		}
	}
	if(flag==1&&iret==(void *)1){
		printf("IDENTICAL\n");
	}
	else{
		printf("DIFFERENT (IF NO MAJORITY HAS BEEN SHOWN IN ANY FILES/DIRS, ALL FSs ARE DIVERGENT)\n");
	}
}
int main(int argc, char *argv[])
{
	if(argc==1){
		printf("./ComparePthreadK k <full path of fileSys 1> <full path of fileSys 2> <..> ... <full path of fileSys k>\n");
		exit(0);
	}
	int i;
	k = atoi(argv[1]);
	if(argc!=k+2){
		printf("LESS ARGUMENTS. TRY AGAIN\n");
		exit(0);
	}
	struct initialData forThreads[k+1];
	rootFSLen = (int *)malloc((k+1)*sizeof(int));
	for (i = 1; i <=k; i++)
	{
		forThreads[i].fileSysRoot = argv[i+1];
		rootFSLen[i] = strlen(argv[i+1]);
		forThreads[i].selfID = i;

	}
	compareFSK(k, forThreads); 
}
