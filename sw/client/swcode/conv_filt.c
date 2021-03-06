#include <stdio.h>
#include <stdlib.h>
#include "fpga.h"

#define header_size 1078
#define image_size (512*512)

unsigned int gDATA[512*512];  //Buffer to hold the send data

int main(int argc, char *argv[]) {

    FILE *in_file;
    FILE *out_file;
    char *file_header;
    char *file_data;
    int rtn;
    int sent = 0;
    int recv = 0;
    int len = 512*512;
    FILE *file;
    char *buffer;
    unsigned long fileLen;
    if(argc != 3) {
        printf("Wrong number of arguments\n");
        return -1;
    }
    in_file = fopen(argv[1],"rb");
    if (!in_file)
    {
	fprintf(stderr, "Unable to open image file\n");
	return;
    }
    file_header=(char *)malloc(header_size);
    fread(file_header, 1, header_size, in_file);
    //Save file data
    file_data=(char *)malloc(image_size);
    fread(file_data, 4, image_size, in_file);
    //Close the image file
    fclose(in_file);
    //Open a new file to store the result
    out_file = fopen(argv[2],"wb");
    //Store image header
    fwrite(file_header, 1, header_size, out_file);
    //Open partial bitstream file
    //Reset user logic
    rtn = fpga_reg_wr(UCTR_REG,0x0);
    rtn = fpga_reg_wr(UCTR_REG,0x1);
    rtn = fpga_reg_wr(CTRL_REG,0x0);
    rtn = fpga_reg_wr(CTRL_REG,0x1);
    while(sent < len){
        rtn = fpga_send_data(USERPCIE1,(unsigned char *) file_data+sent,512,1);
        rtn = fpga_send_data(USERPCIE2,(unsigned char *) file_data+sent+512,512,1);
        rtn = fpga_send_data(USERPCIE3,(unsigned char *) file_data+sent+1024,512,1);
        rtn = fpga_reg_wr(0x400,0x1);
        rtn = fpga_recv_data(USERPCIE1,(unsigned char *) gDATA+recv,512,1);
	rtn = fpga_reg_wr(0x400,0x0);
	sent += 512;
        recv += 512;
    }
    fwrite(gDATA,1,image_size+2,out_file);
    fclose(out_file);
    free(file_header);
    free(file_data);
    return 0;
}
