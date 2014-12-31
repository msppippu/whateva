#include <stdio.h> 
#include <string.h> 
#include <time.h> 
#include <sys/stat.h> 
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>

/*Stores the last modification time of the given file in string s.*/
/*Returns <0 if error occurs.*/
int get_mod_time(char s[], char fname[]){
	struct tm* mytime;
	struct stat st;
	int r;
	r = stat(fname, &st);
	mytime = gmtime(&(st.st_mtime));
	if (r < 0) perror("stat");
	else sprintf(s, "%d%0.2d%0.2d%0.2d%0.2d%0.2d", mytime->tm_year, mytime->tm_mon + 1, 
		mytime->tm_mday, mytime->tm_hour, mytime->tm_min, mytime->tm_sec);
	return r;
}

/*In order to find out the last time our utility was run, we create a text file.*/
/*If the text file does not exist, we know the utility has never been run. So last run time is set as 0.*/
/*After getting the mod time of the file, modify the file by putting an "a" so that mod time updates.*/
/*Use unix i/o functions in order to restrict permissions.*/
int get_run_time(char s[]){
	int r,i;

	r = get_mod_time(s, "last.txt");
	if (r < 0) sprintf(s, "0");
	i = open("last.txt", O_WRONLY|O_CREAT, 200);
	write(i, "a", 1);
	lseek(i, 0, SEEK_SET); //return to beginning so that we don't keep on writing "a"
	return r;
}

/*Checks whether a file has been modified since the last run time*/
/*Returns -1 if error, 1 if modified, 0 if not modified*/
int modified(char fname[], char run_time[]){
	char mod_time[20];
	int r;
	r = get_mod_time(mod_time, fname);
	if (r < 0) return -1;
	//if the modified time is larger than the run time
	else if (strcmp(mod_time, run_time) > 0) return 1;
	else return 0;
}

/*Stores a string name of the backup directory name with current time*/
void get_cur_time(char s[]){
	struct tm *mytime;
	time_t now;
	time(&now);
	mytime = gmtime(&now);
	sprintf(s, "backup%d%0.2d%0.2d-%0.2d:%0.2d", mytime->tm_year+1900, mytime->tm_mon+1, 
		mytime->tm_mday, mytime->tm_hour, mytime->tm_min);
}

/*makes a directory with a given name*/
void make_dir(char dname[]){
	int pid1, pid2, status;
	pid1 = fork();
	if (pid1 < 0) perror("mkdir_fork");
	else if (pid1 == 0)  //child
		if(execlp("mkdir", "mkdir", dname, NULL)<0) perror("mkdir_exec");
	else {//parent
		pid2 = wait(&status);
		if (pid2 < 0) perror("mkdir_wait");
	}
}

/*Copies the file of a given name to the directory of a given name*/
void copy_file(char fname[], char dname[]){
	int pid1, pid2, status;
	pid1 = fork();
	if (pid1 < 0) perror("cp_fork");
	else if (pid1 == 0)//child
		if (execlp("cp", "cp", fname, dname, NULL)<0) perror("cp_exec");
		else {//parent
			pid2 = wait(&status);
			if (pid2 < 0) perror("cp_wait");
		}
}

int main(int argc, char *argv[]){
	int r1, r2, count = 0, index=1;
	char lastrun[20];
	char curtime[20];

	r1 = get_run_time(lastrun); //get the latest run time

	//if the file names were given 
	if (argc > 1){ 
		while (index ++ < argc){ //file names start from argv[1] to argv[argc-1]
			r2 = modified(argv[index-1], lastrun);
			if (r2== 1){ //if the file has been modified
				count++;
				if (count == 1) { 
					get_cur_time(curtime);
					make_dir(curtime);
				}
				copy_file(argv[index-1], curtime);
			}
			else if (r2 == -1) printf("%s does not exist", argv[index-1]);
			else continue;
		}
	}
	else{ //if the file names were not given, check all files		
		DIR* d;
		d = opendir("."); //open current directory
		struct dirent *dir;
		//move dir by two to skip over . and .. files
		dir = readdir(d);
		dir = readdir(d);
		while ((dir = readdir(d)) != NULL){
		//ignore the files generated for the backup utility
			if (strcmp(dir->d_name, "assignment3.c") != 0 && strcmp(dir->d_name, "backup") != 0 && strcmp(dir->d_name, "last.txt") != 0){
			
				if (modified(dir->d_name, lastrun) == 1){
					count++;
					if (count == 1) {
						get_cur_time(curtime);
						make_dir(curtime);
					}
					copy_file(dir->d_name, curtime);
				}
			}
		}
	free(dir);
		    
	}
}
