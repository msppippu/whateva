#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
 
#define PORTNUM 56366
#define buffsize 1024
 
int main()
{
   int r, p;
   char in[3];//store initials
   memset(in, '\0', 3);
   char buff[1024];
   memset(buff, '\0', buffsize);

//set up the server struct
   int mysocket;
   struct sockaddr_in dest; 
   mysocket = socket(AF_INET, SOCK_STREAM, 0);
   memset(&dest, 0, sizeof(dest));                /* zero the struct */
   dest.sin_family = AF_INET;
   dest.sin_addr.s_addr = htonl(INADDR_LOOPBACK); /* set destination IP number - localhost, 127.0.0.1*/ 
   dest.sin_port = htons(PORTNUM);                /* set destination port number */
 
//connect to the server 
  if(connect(mysocket, (struct sockaddr *)&dest, sizeof(struct sockaddr))<0){perror("connect"); exit(1);}
//receive already existing initials
   if(recv(mysocket, buff, buffsize, 0)<0){perror("initial recv");} 
   printf("%s", buff);
//prompt the client to enter his own initials
   printf("Enter your initials (2 characters): ");
   if(scanf("%s", &in)<0){perror("scanf"); exit(1);}
   in[2]=' ';
//send it to the server
   if(send(mysocket, in, 3, 0)<0){perror("send initials"); exit(1);}

   int o;

//make your sockets NON-BLOCKING so that we can read/receive constantly
//IMPORTANT
   if(ioctl(mysocket, FIONBIO, (char*)&o)<0){perror("ioctl"); close(mysocket); exit(1);}
   if(ioctl(0, FIONBIO, (char*)&o)<0){perror("ioctl"); close(0); exit(1);}
   

   int i;
   
   while(1){
     r=read(0, buff, buffsize);
//if something has been entered, send
     if (r>0&&strcmp(buff,"\0")!=0){
       buff[r-1]='\0';
       send(mysocket, buff, r, 0);
       memset(buff, '\0', buffsize);
     }
//receive at any given time
     r=recv(mysocket, buff, buffsize, 0);
//if nothing to receive, skip the look
     if(r<0)continue;
//if the server disconnects, break
     if(r==0) break;
     buff[r-1] = '\0';
     printf("%s\n", buff);
     fflush(stdout);
     memset(buff, '\0', buffsize); 
   }
   printf("Server Disconnected\n");
   close(mysocket);
   return 0;
}
