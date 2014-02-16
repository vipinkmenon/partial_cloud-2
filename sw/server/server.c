
#include <pcap.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "server.h"
#include "circ_queue.h"
#include "fpga.h"
#include <time.h> 
#include <sys/poll.h>

#define DEBUG

pcap_t *handle;    /* packet capture handle */
FILE *fptr;
u_char *pkt_data = NULL;    /* packet data including the link-layer header */
struct circ_queue * reque;
struct circ_queue * prque;
int process_stat_queue[100];
int listenfd = 0;
int connfd;
struct sockaddr_in serv_addr; 
//char recvBuff[1024];
time_t ticks; 
int process_id = 0;

int sock_errno();

int main(int argc, char **argv)
{
 char sendBuff[1025];
 char *dev = "eth0";   /* capture device name */
 char errbuf[PCAP_ERRBUF_SIZE];  /* error buffer */
 int status;
 int rtn;
 pthread_t rxthread;
 pthread_t txthread;

 listenfd = socket(AF_INET, SOCK_STREAM, 0);
 memset(&serv_addr, '0', sizeof(serv_addr));
 memset(sendBuff, '0', sizeof(sendBuff)); 
 //memset(recvBuff, '0',sizeof(recvBuff));

 serv_addr.sin_family = AF_INET;
 serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
 serv_addr.sin_port = htons(5000); 

 bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 

 listen(listenfd, 2); 

 //Create the circular queue
 reque = init_circ_queue(100);
 if(reque == NULL)
     printf("Failed to create the input queue\n");

 prque = init_circ_queue(100);
 if(prque == NULL)
     printf("Failed to create the output queue\n");

rtn= pthread_create(&txthread, NULL,process_loop,NULL);

while(1){
   connfd = accept(listenfd, (struct sockaddr*)NULL, NULL); 
   if(rtn == 0){
     rtn= pthread_create(&rxthread, NULL,recv_loop,(void *) connfd);
   }
   else{
      snprintf(sendBuff, sizeof(sendBuff), "%s\r\n", "NAK");
      write(connfd, sendBuff, strlen(sendBuff)); 
      close(connfd);
   }  
   sleep(1);
 }
 printf("End of program!!\n");
 free_circ_queue(reque);
 free_circ_queue(prque);
 return 0;
}

void *process_loop()
{
  int rtn;
  int proc;
  int n;
  int filesize;
  char command[100];
  char recvBuff[1024];
  char sendBuff[1024];
  char bitstreamName[100];
  char executeName[100];
  char inputFileName[100];
  char outFileName[100];
  int status;
  FILE *fptr;
  char *buffer;
  int fileLen;
  int sent;

  struct pollfd ufds;
  while(1){
     rtn = pop_circ_queue(prque, &proc);
     if(rtn == 0)
     {
        sent = 0;
        ufds.fd = proc;
        ufds.events = POLLIN | POLLHUP | POLLRDNORM;
        ufds.revents = 0;
        #ifdef DEBUG
            printf("Request to process : %d\n",proc);         //Get the connection number from the input 
        #endif
        rtn = poll(&ufds,1,3500);                         //queue for which data has to be processed
        if(rtn == -1){                                    //Poll on that description for read request
           printf("Poll error\n");                        //If no read request till 3.5 sec, disconnect it
           close(proc);
        }
        else if(rtn == 0){
           printf("Timeout\n");
           close(proc);
        }
        else if(ufds.revents & POLLIN){
            n = read(proc, recvBuff, sizeof(recvBuff));
            if(n < 0)
            {
                printf("tid : %d Read error \n",proc);
                close(proc);
            } 
            else if(n==0)
            {
                printf("Connection lost \n");
                close(proc);
            }
            else 
            {
                memcpy(command,recvBuff,sizeof(command));
                command[10] = 0;
                #ifdef DEBUG
                    printf("command: %s\n",command);
                #endif
                if(strcmp(command,"REQ_DATARD") == 0)     //Check whether the request received is data read
                {
                    #ifdef DEBUG
                        printf("Data read request received\n");
                    #endif
                    n = sscanf(recvBuff, "REQ_DATARD_%d", &filesize);
                    #ifdef DEBUG
                        printf("Data size is %d\n",filesize); //Get the request data size
                    #endif
                    if(n < 0)
                    {
                      printf("tid : %d Data size error\n",proc);
                      close(proc);
                    } 
                    snprintf(sendBuff, sizeof(sendBuff)-1, "%s", "DATA_RDACK");  //Acknowledge the read request
                    n = write(proc, sendBuff, strlen(sendBuff)); 
                    sprintf(bitstreamName, "%s%d%s", "bitfile_",proc,".bin");
                    sprintf(executeName, "%s%d", "software_",proc);
                    sprintf(inputFileName, "%s%d%s", "indata_",proc,".bin");
                    sprintf(outFileName, "%s%d%s", "outdata_",proc,".bin");
                    config_fpga(bitstreamName); //Configure the FPGA with the partial bitstream                                  
                    sprintf (command, "./%s %s %s",executeName,inputFileName,outFileName);
                    status = system(command);
                    rtn = WEXITSTATUS(status);
                    if(rtn != 0){
                       printf("Execution failed\n");
                       close(proc);
                    }
                    else{
                        fptr = fopen(outFileName,"rb");
                        fseek(fptr, 0, SEEK_END);
                        fileLen=ftell(fptr);
                        fseek(fptr, 0, SEEK_SET);
                        #ifdef DEBUG
                            printf("Data file size is %d\n",fileLen);
                        #endif
                        //Allocate memory
                        buffer=(char *)malloc(fileLen+1);
                        if (!buffer)
                        {
                            fprintf(stderr, "Memory error!\n");
                            fclose(fptr);
                        }
                        else
                        {
                            fread(buffer, 1, fileLen, fptr);
                            fclose(fptr);
                            if(filesize != 0)
                                fileLen = filesize;
                            n = write(proc, buffer, fileLen);
                            if(n!= fileLen)
                            {
                                printf("Data sending failed\n");
                                sprintf(sendBuff,"%s", "DTW_NAK");
                                n = write(proc, sendBuff, strlen(sendBuff)); 
                            }
                            else{
                                #ifdef DEBUG
                                    printf("Data sending success\n");
                                #endif
                                sprintf(sendBuff,"%s", "DTW_ACK");
                                n = write(proc, sendBuff, strlen(sendBuff));
                            }
                            close(proc);
                        }
                    }
                }
                else
                {
                    printf("Unknown request from client \n");
                    close(proc);
                }
            }
        }
     }
  }
}


void * recv_loop(void * thread_id)
{
  int local_id = thread_id;
  int rv;
  int t_cnt=0;
  int i=0;
  int received = 0;
  int filesize;
  FILE *fptr;
  int n = 0;
  int rtn;
  char recvBuff[1024];
  struct pollfd ufds;
  char sendBuff[1025];
  char command[10];
  char filenamebuffer[100];
  char executablebuffer[100];
  char compilecommand[100];
  int status;
  ufds.fd = local_id;
  ufds.events = POLLIN | POLLHUP | POLLRDNORM;
  ufds.revents = 0;
  snprintf(sendBuff, sizeof(sendBuff), "%s%d", "ACK_",local_id);
  n = write(local_id, sendBuff, strlen(sendBuff)); 

  while(1){

    n = read(local_id, recvBuff, sizeof(recvBuff));
    if(n < 0)
    {
      printf("tid : %d Read error \n",local_id);
      close(local_id);
      pthread_exit(NULL);
    } 
    else if(n==0)
    {
      printf("Connection lost \n");
      close(local_id);
      pthread_exit(NULL);
    }
    
    memcpy(command,recvBuff,sizeof(command));
    command[10] = 0;

    if(strcmp(command,"REQ_CONFIG") == 0)
    {
      #ifdef DEBUG
          printf("Configuration request received\n");
      #endif
      n = sscanf(recvBuff, "REQ_CONFIG_%d", &filesize);                //Find the configuration bitstream size
      #ifdef DEBUG
          printf("Bitstream size is %d\n",filesize);
      #endif
      if(n < 0)
      {
          printf("tid : %d Bitstream size error\n",local_id);
          close(local_id);
          pthread_exit(NULL);
      } 
      snprintf(filenamebuffer, sizeof(filenamebuffer), "%s%d%s", "bitfile_",local_id,".bin");  
      fptr = fopen(filenamebuffer,"wb");                               //create a file to store the configuration data
      memset(sendBuff, '0', sizeof(sendBuff));
      snprintf(sendBuff, sizeof(sendBuff)-1, "%s", "CONF_ACK");        //Send the acknowledgement for configuration request
      n = write(local_id, sendBuff, strlen(sendBuff)); 
      while(1) {
          n = read(local_id, recvBuff, sizeof(recvBuff));
          if(n < 0)
          {
              printf("tid : %d Read error \n",local_id);
              fclose(fptr);
              close(local_id);
              pthread_exit(NULL);
          } 
          else if(n==0)
          {
              printf("Connection lost \n");
              fclose(fptr);
              close(local_id);
              pthread_exit(NULL);
          }    
          received += n;
          //printf("Received data size %d\n",received);      
          if(received >= filesize){
              fwrite((char *)recvBuff,1,n-(received-filesize),fptr);
              #ifdef DEBUG
                  printf("Bitstream reception done\n");
              #endif
              memset(sendBuff, '0', sizeof(sendBuff));
              snprintf(sendBuff, sizeof(sendBuff)-1, "%s", "BS_ACK");
              n = write(local_id, sendBuff, strlen(sendBuff)); 
              fclose(fptr);
              break;
          }
          else
              fwrite((char *)recvBuff,1,n,fptr);
       }
    }
 
    received = 0;
    if(strcmp(command,"REQ_SOFTWR") == 0)
    {
      #ifdef DEBUG
          printf("Software request received\n");
      #endif
      n = sscanf(recvBuff, "REQ_SOFTWR_%d", &filesize);
      #ifdef DEBUG
          printf("Software size is %d\n",filesize);
      #endif
      if(n < 0)
      {
        printf("tid : %d Software size error\n",local_id);
        fclose(fptr);
        close(local_id);
      } 
      memset(sendBuff, '0', sizeof(sendBuff));
      snprintf(sendBuff, sizeof(sendBuff)-1, "%s", "SOFT_ACK");
      n = write(local_id, sendBuff, strlen(sendBuff));
      snprintf(filenamebuffer, sizeof(filenamebuffer), "%s%d%s", "software_",local_id,".c");
      snprintf(executablebuffer, sizeof(filenamebuffer), "%s%d", "software_",local_id);
      fptr = fopen(filenamebuffer,"wb");                             //create a file to store the software data
      while(1) {
        n = read(local_id, recvBuff, sizeof(recvBuff));
        if(n < 0)
        {
          printf("tid : %d Read error \n",local_id);
          fclose(fptr);
          close(local_id);
          pthread_exit(NULL);
        } 
        else if(n==0)
        {
          printf("Connection lost \n");
          fclose(fptr);
          close(local_id);
          pthread_exit(NULL);
        }          
        received += n;
        //printf("Received data size %d\n",received);
        if(received >= filesize){
            fwrite((char *)recvBuff,1,n-(received-filesize),fptr);
            #ifdef DEBUG
                printf("Software reception done\n");
            #endif
            memset(sendBuff, '0', sizeof(sendBuff));
            snprintf(sendBuff, sizeof(sendBuff)-1, "%s", "SW_ACK");
            n = write(local_id, sendBuff, strlen(sendBuff));     //Once software reception is over, acknowledge the client
            fclose(fptr);                                        //close the file
            sprintf (compilecommand, "gcc fpga.c %s -o %s",filenamebuffer,executablebuffer);
            status = system(compilecommand);
            rtn = WEXITSTATUS(status);
            if(rtn != 0){
                printf("Code compilation failed\n");
                close(local_id);
                pthread_exit(NULL);
            }
            break;
        }
        else
            fwrite((char *)recvBuff,1,n,fptr);
      }
    }

    received = 0;
    if(strcmp(command,"REQ_DATAWR") == 0)
    {
      #ifdef DEBUG
          printf("Data request received\n");
      #endif
      n = sscanf(recvBuff, "REQ_DATAWR_%d", &filesize);
      #ifdef DEBUG
          printf("Data size is %d\n",filesize);
      #endif
      if(n < 0)
      {
        printf("tid : %d Data size error\n",local_id);
        close(local_id);
        pthread_exit(NULL);
      } 
      memset(sendBuff, '0', sizeof(sendBuff));
      snprintf(sendBuff, sizeof(sendBuff)-1, "%s", "DATA_WRACK");
      n = write(local_id, sendBuff, strlen(sendBuff));
      snprintf(filenamebuffer, sizeof(filenamebuffer), "%s%d%s", "indata_",local_id,".bin");
      fptr = fopen(filenamebuffer,"wb");       //create a file to store the input data. file name is indata_connectionid.bin
      while(1) {
        n = read(local_id, recvBuff, sizeof(recvBuff));
        if(n < 0)
        {
          printf("tid : %d Read error \n",local_id);
          fclose(fptr);
          close(local_id);
          pthread_exit(NULL);
        } 
        else if(n==0)
        {
          printf("Connection lost \n");
          fclose(fptr);
          close(local_id);
          pthread_exit(NULL);
        }          
        received += n;
        if(received >= filesize){
            fwrite((char *)recvBuff,1,n-(received-filesize),fptr);           //Write exact number of bytes into the file
            #ifdef DEBUG
                printf("Data reception done\n");
            #endif
            memset(sendBuff, '0', sizeof(sendBuff));
            snprintf(sendBuff, sizeof(sendBuff)-1, "%s", "DTW_ACK");         //Acknowledge the reception.
            n = write(local_id, sendBuff, strlen(sendBuff)); 
            fclose(fptr);                                                    //Close the input data file and log it in the input queue
            n = push_circ_queue(prque,local_id);                             //then exit the thread
            pthread_exit(NULL);
            return;
        }
        else
            fwrite((char *)recvBuff,1,n,fptr);
      }
    }
  }
}

int config_fpga(char * partial_file)
{
   /*FILE *prfile;
   char *buffer;
   unsigned long fileLen;
   int rtn;
   prfile = fopen(partial_file, "rb");
   if (!prfile)
   {
      fprintf(stderr, "Unable to open partial bit file\n");
      return -1;
   }
   //Get file length
   fseek(prfile, 0, SEEK_END);
   fileLen=ftell(prfile);
   fseek(prfile, 0, SEEK_SET);
   //Allocate memory
   buffer=(char *)malloc(fileLen+1);
   if (!buffer)
   {
       fprintf(stderr, "Memory error!\n");
       fclose(prfile);
       return -1;
   }
   //Read file contents into buffer
   fread(buffer, 1, fileLen, prfile);
   fclose(prfile);
   //Send partial bitstream to FPGA
   rtn = fpga_send_data(ICAP, (unsigned char *) buffer, fileLen, 0);
   printf("FPGA reconfigured............\n");*/
   //sleep(1);
   return 0;
}
