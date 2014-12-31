#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <sys/ioctl.h>

#define portnum 56366
#define maxclient 8
#define buffsize 1024
#define timeout 60000 //1 minute
extern int errno;

//server

int main(){
  char initials[maxclient][3];
  memset(initials, '\0', maxclient*2);
  char *buff=(char*) malloc(buffsize);
  memset(buff, '\0', buffsize);
  char *tmp=(char*) malloc(buffsize);
  memset(tmp, '\0', buffsize);

  int end_server=0;
  int len=0;

//1.create socket
  int main_socket=socket(AF_INET, SOCK_STREAM, 0);
  if (main_socket<0) {perror("socket creation"); exit(1);}

  int o;

//make your socket non-blocking so that it can proceed
//without having to wait for a new connection
//THIS IS VERY IMPORTANT!!! SAVES LIVES
  if(ioctl(main_socket, FIONBIO, (char*)&o)<0)
    {perror("ioctl"); close(main_socket); exit(1);}
  
//2. set server info and bind
  struct sockaddr_in server;
  memset(&server, 0, sizeof(server));
  server.sin_family=AF_INET;
  server.sin_len=sizeof(server);
  server.sin_port=htons(portnum);
  server.sin_addr.s_addr=INADDR_ANY;

  if(bind(main_socket, (struct sockaddr*)&server, sizeof(server))<0){
    perror("bind"); close(main_socket); exit(1);}

//3. listen
  if(listen(main_socket, 8)<0){perror("listen"); exit(1);}

//maxclient + 1 because we are going to use fdarr[0] as listening socket
  struct pollfd fdarr[maxclient+1];
  memset(fdarr, 0, sizeof(fdarr)); //initialize fdarr

//set up initial listening socket
  fdarr[0].fd = main_socket;
  fdarr[0].events=POLLIN;

//declaring int values to be used..
  int p,r,k;
//number of clients
  int n=1;
  int i;
//new client file descriptor
  int news=-1;
  int current_size;
  do{
    printf("Poll.....\n");
    p=poll(fdarr, n, timeout);

    if (p<0){perror("poll"); break; }
    if (p==0){printf("Timeout\n"); break;}
         
//loop through to see active connections    
    current_size=n;
    for(i=0; i<current_size; i++){
//if nothing happens
      if(fdarr[i].revents == 0) continue;
//unexpected error
      if(fdarr[i].revents !=POLLIN){printf("error\n"); end_server=1; break;}
//Check for new connections
      if(fdarr[i].fd == main_socket) {
//accept all connections that have been queued up
        do {
          news = accept(main_socket, NULL, NULL);
//if errno is EWOULDBLOCK we know we've accepted all possible
          if (news<0) {if(errno!=EWOULDBLOCK){perror("accept"); end_server=1;} break;}
	  printf("New connection %d\n", news);
//set up the new client connection, assign it to fdarr
          fdarr[n].fd=news;
          fdarr[n].events=POLLIN;
//send already used initials to the new client
//if there are previous connections
	  memset(buff, '\0', buffsize);
	  if(strcmp(initials[0],"\0\0\0")!=0){
	    strncpy(buff,"Initials used: ",15);
            for(k=0;k<(n-1);k++){
              buff[15+(k*3)]=initials[k][0];
              buff[16+(k*3)]=initials[k][1];
	      buff[17+(k*3)]=initials[k][2];
            }
            buff[15+(k*3)]='\n';
	    buff[16+(k*3)]='\0';
          }
//if the first connection, the user can type in whatever initials
	  else {strcpy(buff,"You're the first connection!\n");}
	  if(send(fdarr[n].fd, buff, strlen(buff), 0)<0)perror("initial send");
//notify exiting clients about the new connection
	  for(k=1; k<current_size; k++){
	    if(k==i) continue;
	    send(fdarr[k].fd, "New User Connected!\0", 20, 0);
	  }
          n++;
        }while (news != -1);
      }
      else {
//This fd is not the listening socket and has sent something
//therefore we must receive its message
        r=recv(fdarr[i].fd, buff, buffsize, 0);
        if (r <0){continue;}
//if the connection closed, reset everything and notify other users
        if (r == 0){
      	  memset(buff, '\0', buffsize);
	  initials[i-1][2]='\0';
	  sprintf(buff, "connection closed for user %s\n",initials[i-1]); 
	  initials[i-1][2]=' ';
	  close(fdarr[i].fd); 
	  fdarr[i].fd=-1; 
	  strncpy(initials[i-1], "\0\0\0", 3); 
	  for(k=1; k<current_size; k++){
//don't message to itself
	    if(k==i) continue;
	    r=send(fdarr[k].fd, buff, strlen(buff), 0);
	    if(r<0){perror("send"); continue;}
          }	  
	continue;
        }
//if initials were not entered yet, grab two characters that the user has entered
  	if(strcmp(initials[i-1],"\0\0\0")==0){strncpy(initials[i-1], buff, 3);}
//if initials were already entered
//we know data was received
//must message others
	else {	
	  len=r+5;
	  strncpy(tmp, buff, r);
	  printf("received: %s\n", buff);
    	  memset(buff, '\0', buffsize);
	  initials[i-1][2]='\0';
	  sprintf(buff, "%s: %s", initials[i-1], tmp);
	  initials[i-1][2]=' ';
	  for(k=1; k<current_size; k++){
//don't message to itself
	    if(k==i) continue;
	    printf("sending: %s", buff);
	    r=send(fdarr[k].fd, buff, len, 0);
	    if(r<0){perror("send"); continue;}
          }
	}
      }
    } 
  }while(!end_server);
//close any sockets that are still open
for (i = 0; i < n; i++)
  {
    if(fdarr[i].fd >= 0)
      close(fdarr[i].fd);
  }

return 0;
}
