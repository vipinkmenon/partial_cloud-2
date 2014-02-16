#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 

int main(int argc, char *argv[])
{
    int sockfd = 0, n = 0;
    char recvBuff[1024];
    char sendBuff[1025];
    char command[3];
    int access_no;
    int i=0;
    FILE *file;
    char *buffer;
    int fileLen;
    int sent = 0;
    struct sockaddr_in serv_addr; 

    if(argc != 6)
    {
        printf("\n Usage: %s <ip of server>  <bitstream> <software> <inputdata> <outputdata>\n",argv[0]);
        return 1;
    } 

    memset(recvBuff, '0',sizeof(recvBuff));
    memset(sendBuff, '0', sizeof(sendBuff));
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    } 

    memset(&serv_addr, '0', sizeof(serv_addr)); 

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5000); 

    if(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        return 1;
    } 

    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
       printf("\n Error : Connect Failed \n");
       return 1;
    } 

    //Checkck whether connection request was accepted
    n = read(sockfd, recvBuff, sizeof(recvBuff));
    recvBuff[n] = 0;  

    if(n<=0){
        printf("link broken\n");
        return -1;
    }

    memcpy(command,recvBuff,sizeof(command));

    if(strcmp(command,"ACK") == 0) 
        ;//printf("Wow I got access...%d\n",access_no);
    else {
        printf("Server rejected request\n");   
        return -1;
    }
   
    //Send the bitstream///////////////////////////////////////////

    file = fopen(argv[2],"rb");
    fseek(file, 0, SEEK_END);
    fileLen=ftell(file);
    fseek(file, 0, SEEK_SET);
    //printf("Bitstream file size is %d\n",fileLen);
    //Allocate memory
    buffer=(char *)malloc(fileLen+1);
    if (!buffer)
    {
	fprintf(stderr, "Memory error!\n");
        fclose(file);
	return -1;
    }
    fread(buffer, 1, fileLen, file);
    fclose(file);

    snprintf(sendBuff, sizeof(sendBuff), "%s%d", "REQ_CONFIG_",fileLen);
    n = write(sockfd, sendBuff, strlen(sendBuff));  //Send bitstream request

    n = read(sockfd, recvBuff, sizeof(recvBuff));   //Wait for host ack
    recvBuff[n] = 0; 

    if(n<=0) {
        printf("link broken\n");
        return -1;
    }
    
    if(strcmp(recvBuff,"CONF_ACK") != 0)  {
        printf("Configuration request rejected\n");
        return -1;
    }

    n = write(sockfd, buffer,fileLen);       //Send bitstream

    if(n!= fileLen){
        printf("Bitstream sending failed requested %ld, got %d\n",sizeof(buffer),n);
        return -1;
    }
  
    n = read(sockfd, recvBuff, sizeof(recvBuff));   //wait for bitstream reception ack
    recvBuff[n] = 0; 

    if(n<=0){
        printf("link broken\n");
        return -1;
    }

    if(strcmp(recvBuff,"BS_ACK") != 0)  {
        printf("Bitstream not acknowledged\n");
        return -1;
    }
    free(buffer);
    ////Send software////////////////////////////////////////////////
    file = fopen(argv[3],"rb");
    fseek(file, 0, SEEK_END);
    fileLen=ftell(file);
    fseek(file, 0, SEEK_SET);
    //Allocate memory
    buffer=(char *)malloc(fileLen+1);
    if (!buffer)
    {
	fprintf(stderr, "Memory error!\n");
        fclose(file);
	return -1;
    }
    fread(buffer, 1, fileLen, file);
    fclose(file);

    snprintf(sendBuff, sizeof(sendBuff), "%s%d", "REQ_SOFTWR_",fileLen);
    n = write(sockfd, sendBuff, strlen(sendBuff)); //Send software send request
  
    n = read(sockfd, recvBuff, sizeof(recvBuff));
    recvBuff[n] = 0; 

    if(n<=0){
        printf("link broken\n");
        return -1;
    }

    if(strcmp(recvBuff,"SOFT_ACK") != 0) {
        printf("Software request not acknowledged\n");
        return -1;
    }

    n = write(sockfd, buffer, fileLen);  //Send the software

    if( n!= fileLen) {
        printf("Software sending failed..\n");
        return -1;
    }

    n = read(sockfd, recvBuff, sizeof(recvBuff));
    recvBuff[n] = 0; 

    if(n<=0){
        printf("link broken\n");
        return -1;
    }

    if(strcmp(recvBuff,"SW_ACK") != 0)  {
        printf("Software not acknowledged\n");
        return -1;
    }
    free(buffer);
    //Send input data////////////////////////////////////////////////////////////
    file = fopen(argv[4],"rb");
    fseek(file, 0, SEEK_END);
    fileLen=ftell(file);
    fseek(file, 0, SEEK_SET);
    //Allocate memory
    buffer=(char *)malloc(fileLen+1);
    if (!buffer)
    {
	fprintf(stderr, "Memory error!\n");
        fclose(file);
	return -1;
    }
    fread(buffer, 1, fileLen, file);
    fclose(file);

    memset(sendBuff, '0', sizeof(sendBuff));
    snprintf(sendBuff, sizeof(sendBuff), "%s%d", "REQ_DATAWR_",fileLen);

    n = write(sockfd, sendBuff, strlen(sendBuff)); //Send data request
  
    n = read(sockfd, recvBuff, sizeof(recvBuff));  //Wait for host ack
    recvBuff[n] = 0; 

    if(n<=0){
        printf("link broken\n");
        return -1;
    }

    if(strcmp(recvBuff,"DATA_WRACK") != 0) {
        printf("Data request not acknowledged\n");
        return -1;
    }

    n = write(sockfd, buffer, fileLen);  //Send the data

    if( n!= fileLen) {
        printf("Data sending failed..\n");
        return -1;
    }

    n = read(sockfd, recvBuff, sizeof(recvBuff)); //Wait for host ack
    recvBuff[n] = 0; 

    if(n<=0){
        printf("link broken\n");
        return -1;
    }


    if(strcmp(recvBuff,"DTW_ACK") != 0)  {
        printf("Data not acknowledged\n");
        return -1;
    }
    free(buffer);
    //Read input data/////////////////////////////////////////////////
    file = fopen(argv[5],"wb");
    memset(sendBuff, '0', sizeof(sendBuff));
    snprintf(sendBuff, sizeof(sendBuff), "%s%d", "REQ_DATARD_",0); //Set data size as 0 so that whole processed data is received
 
    n = write(sockfd, sendBuff, strlen(sendBuff)); //send request

    n = read(sockfd, recvBuff, sizeof(recvBuff)); //Wait for host ack
    recvBuff[n] = 0; 

    if(n<=0){
        printf("link broken\n");
        return -1;
    }


    if(strcmp(recvBuff,"DATA_RDACK") != 0)  {
        printf("Data read not acknowledged\n");
        return -1;
    }


    while(1) {
        n = read(sockfd, recvBuff, sizeof(recvBuff));
        if(n<=0){
            printf("link broken\n");
            fclose(file);
            return -1;
        }

        if(strstr(recvBuff,"DTW_ACK") == NULL){
            fwrite(recvBuff,1,n,file);
        }
        else {
            fwrite(recvBuff,1,n-7,file);
            fclose(file);
            break;
        }
    }
 
    close(sockfd);
    return 0;
}
