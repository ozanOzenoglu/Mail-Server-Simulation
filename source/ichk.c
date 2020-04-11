/*
 ============================================================================
 Name        : Experiment3.c
 Author      : Ozan
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */



#define DIRECTORYTYPE 4
#define FILETYPE 8
#define MOD_ADLER 65521
#define NOTFOUND 999999





#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>         /* defines options flags */
#include <sys/types.h>     /* defines types used by sys/stat.h */
#include <sys/stat.h>      /* defines S_IREAD & S_IWRITE  */
#include <sys/dir.h>
#include <unistd.h>
#include <time.h>



typedef struct {
	ino_t inode_number ; // file serial number
	unsigned long checksum ; //alder-32 checksum

}ichk_info;

typedef struct {
	ino_t inode_number;
	char file_name[32];
}my_file_struct;




static char *logName = "ichk.log";
static char *sumName = "ichk.sum";

ichk_info findFile(ino_t inode, int sum) {
	ichk_info current;
	ichk_info finded;
	finded.inode_number = NOTFOUND;
	lseek(sum,0l,0);
	while ( read(sum,&current,sizeof(ichk_info)) > 0 ) {
		if(current.inode_number == inode) finded = current;
	}
	lseek(sum,0l,0);
	return finded; // return 999999 inode number if file can't find in sum file.
}







int open_file(char * filePath , char** fileList , int totalFile,char * fileName , int * isNew) {
	char * buffer = malloc((strlen(filePath)+256)*sizeof(char)); // we asume maximum file name length is 256 in berkeley directory system
	int file;
	*isNew = 0 ; // we assume there is file in start.




			strcpy(buffer,filePath);
			strcat(buffer,fileName);
			if( strcmp(fileName,sumName) ) {
				file = open(buffer,O_RDWR | O_APPEND); // open existing log file for adding
				if(file == -1) { // if there is no file
					file = open( buffer, O_CREAT | O_RDWR | O_APPEND, S_IREAD | S_IWRITE);
					*isNew = 1;
				}
			}
			else {
				file = open(buffer,O_RDWR  ); // we will overwrite on sumfile. so we don't open file in append mode here if file is sum file.
				if(file == -1) { // if there is no file
					file = open( buffer, O_CREAT | O_RDWR , S_IREAD | S_IWRITE);
					*isNew = 1;
				}
			}
			free(buffer);

			return file;



}

unsigned long adler32(unsigned char *data, int len) /* where data is the location of the data in physical memory and len is the length of the data in bytes */
{
    unsigned long a = 1, b = 0;
    int index;

    /* Process each byte of the data in order */
    for (index = 0; index < len; ++index)
    {
        a = (a + data[index]) % MOD_ADLER;
        b = (b + a) % MOD_ADLER;
    }

    return (b << 16) | a;
}


char ** direc_reader(char* directoryPath , int * i) { // read files name and inode number in the given directory
	   DIR *dirp;
	   struct direct *dp;
	   char * ichkName = "ichk";




	   dirp = opendir(directoryPath);                   /* open the current directory */
	   *i = 0 ;
	   char ** fileList  = malloc ( sizeof(char*) * 64 );
	   while( (dp=readdir(dirp)) != NULL )  {

		   // printf("file inode_number: %ui file_name: %s\n", dp->d_ino , dp->d_name);
		   if(strcmp(dp->d_name,ichkName) && strcmp(dp->d_name,logName) && strcmp(dp->d_name,sumName) ) fileList[(*i)++] = dp->d_name; // don't save sum and log file name


	   }
	   return fileList;
}

void monitor(char* filePath ) {
	char slash = '/';
	int i = 0 ,infoListIndex = 0;
	int fd;
	struct stat mystat;
	char buffer[256];
	char * buffer2;
	char sumFilePath[64];
	int totalFile;
	int isNewLog , isNewSum; // we asume there is no log and sum file in start.
	int logFile;
	int sumFile;
	char * localFilePath = filePath;
	char * added = " added\n";
	char * modified =  " modified\n";

	char message[64];

	if ( filePath[strlen(filePath)-1] != slash ){ // if filepath like this ../reports then occurs this station ../reportsfilename then stat function make program crash.
		int strLength = strlen(filePath);
		char * newpath = malloc(sizeof(char)*(strLength+2));
		strcpy(newpath,filePath);
		newpath[strLength] = '/';
		newpath[strLength+1] = '\0';
		localFilePath = malloc((strlen(newpath)+2)*sizeof(char));
		strcpy(localFilePath,newpath);
		free(newpath);
	}

	char ** fileList = direc_reader(localFilePath,&totalFile);
	ichk_info * infoList = malloc(sizeof(ichk_info)*totalFile);
	my_file_struct * myList = malloc(sizeof(my_file_struct)*totalFile);



	logFile = open_file(localFilePath, fileList,totalFile,logName,&isNewLog);
	sumFile = open_file(localFilePath,fileList,totalFile,sumName,&isNewSum);


	for ( i = 0 ; i < totalFile ; i ++) {
		if(fileList[i][0] == '.') continue; // .. ve . için onlem
		strcpy(buffer,localFilePath);
		strcat(buffer,fileList[i]);
		stat(buffer,&mystat);

		if( mystat.st_mode & S_IFMT != S_IFDIR ) {

			monitor(buffer); // This is a directory so I recursive method in here.

		}else {

			buffer2 = malloc(mystat.st_size);
			fd = open(buffer,O_RDWR);
			read(fd,buffer2,mystat.st_size); // read file content into buffer2 to adler32
			close(fd);

			ichk_info fileinfo;
			fileinfo.checksum = adler32((unsigned char *)buffer2,mystat.st_size);
			fileinfo.inode_number = mystat.st_ino;
			free(buffer2);



			my_file_struct myFile;
			strcpy(myFile.file_name,fileList[i]);
			myFile.inode_number = mystat.st_ino;

			myList[infoListIndex] = myFile;
			infoList[infoListIndex++] = fileinfo;







		}
	}


	if(! isNewSum ) {
		for(i = 0 ; i < infoListIndex ; i ++)	{
			if(strcmp(myList[i].file_name,sumName) && strcmp(myList[i].file_name , logName) ) {
				ichk_info old = findFile(infoList[i].inode_number,sumFile); // sum is the sum file we are searching the inode number in the sum file here.
				if(old.inode_number == NOTFOUND) { // if new file added we check it in this scope
					strcpy(message,myList[i].file_name);

					strcat(message,added);
					write(logFile,message,strlen(message)*sizeof(char));

				}else if (old.checksum != infoList[i].checksum){ // if the file is edited we check it in this scope
					strcpy(message,myList[i].file_name);
					strcat(message,modified);
					write(logFile,message,strlen(message)*sizeof(char));
				}
			}
		}
		ichk_info isDeleted;
		int find = 0;

		lseek(sumFile,0l,0); // go to start of file we will search all sum file to find .
		while(  read(sumFile,&isDeleted,sizeof(ichk_info)) > 0 )  {
			find = 0;
			for(i = 0 ; i < infoListIndex ; i++) {
				if(isDeleted.inode_number == infoList[i].inode_number)  find = 1;
			}
			if(!find ) {
				sprintf(message,"%ld %s" , isDeleted.inode_number,"deleted\n");
				write(logFile,message,strlen(message)*sizeof(char));
			}

		}
	}
	close(logFile);

	strcpy(sumFilePath,localFilePath);
	strcat(sumFilePath,sumName);

	remove(sumFilePath);
	sumFile = open_file(localFilePath,fileList,totalFile,sumName,&isNewSum);
	for(i = 0 ; i < infoListIndex ; i++) { // we create new sum file here in other hand we update sum file ..
		write(sumFile,&infoList[i],sizeof(ichk_info));
	}



	close(sumFile);
	free(infoList);
	free(myList);



}






int main(int argc , char * argv[])
{
	char * filePath = argv[1];
    long start , now;
    int iterationCount , delay , newDelay;
    delay = atoi(argv[2]);
    iterationCount = atoi(argv[3]);


   time (&start);
   newDelay = delay;
   long fark = 0;
   monitor(filePath);
   iterationCount--;

   while(iterationCount) {
	   if(fark == newDelay) {
		   monitor(filePath);
		   iterationCount--;
		   newDelay += delay;
	   }
	   time(&now);
	   fark = now - start;
 }


return 0;

}
