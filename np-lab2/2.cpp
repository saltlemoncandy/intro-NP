#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <vector>
#include <math.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>
#include <fcntl.h> //open
#include <errno.h>
#include <unistd.h> //read write lseek
#include <arpa/inet.h> //htons honl ntohs ntohl
#include <sys/stat.h>
#include <sys/types.h>
using namespace std;

typedef struct {
    uint64_t magic;     /* 'BINFLAG\x00' */
    uint32_t datasize;  /* in big-endian */
    uint16_t n_blocks;  /* in big-endian */
    uint16_t zeros;
} __attribute((packed)) binflag_header_t;

typedef struct {
    uint32_t offset;        /* in big-endian */
    uint16_t cksum;         /* XOR'ed results of each 2-byte unit in payload */
    uint16_t length;        /* ranges from 1KB - 3KB, in big-endian */
    uint8_t*  payload;
} __attribute((packed)) block_t;

typedef struct {        
    uint16_t length;        
    uint32_t*  offset;
} __attribute((packed)) flag_t;

// vector<uint8_t> payload;
// vector<block_t> blocks;
// vector<uint16_t> answer;
// vector<uint32_t> fls;
// string ans;
vector<uint8_t> dict;

int main(int argc, char* argv[]){
    
    
    char getbin[1000];
    sprintf(getbin, "wget -O %s https://inp.zoolab.org/binflag/challenge?id=110550158", argv[1]);
    system(getbin);
    
    // int fd = open("demo1.bin", O_RDONLY);
    int fd = open("demo2.bin", O_RDONLY);
    // int fd = open(argv[1], O_RDONLY);

    if (fd == -1) {
        printf("error\n");
        return -1;
    }
    printf("Get Challenge.bin\n");
    

    // int buffer[128];
    // memset(buffer, '\0', sizeof(buffer));
    // read(fd, buffer, 8);
    // read(fd, &datasize, 4);
    // read(fd, &n_blocks, 2);
    // read(fd, buffer+8, 2);
    
    // printf("datasize : %d\nn_blocks : %d\n", datasize, n_blocks);

    size_t header_size = sizeof(binflag_header_t);
    binflag_header_t header;
    binflag_header_t tmp_header;
    
    int read_ = read(fd, &tmp_header, header_size);
    printf("Reading Header\n");

    header.magic = ntohl(tmp_header.magic);      //32
    header.datasize = ntohl(tmp_header.datasize);//32
    header.n_blocks = ntohs(tmp_header.n_blocks);//16
    header.zeros = ntohs(tmp_header.zeros);      //16

    printf("Header information\n");
    printf("-Magic : %x\n", header.magic);
    printf("-datasize : %d\n", header.datasize);    //string offset 
    printf("-n_blocks : %d\n", header.n_blocks);    //content offset
    

    block_t* blocks = (block_t*)malloc(sizeof(block_t) * header.n_blocks);
    bool* check = (bool*)malloc(sizeof(bool) * header.n_blocks);
    uint8_t* dict = (uint8_t*) malloc(sizeof(uint8_t) * header.datasize);
    
    


    for(int i=0; i<header.n_blocks; ++i){
        read_ = read(fd, &blocks[i], 8);
        blocks[i].offset = ntohl(blocks[i].offset); //4
        blocks[i].cksum = ntohs(blocks[i].cksum);   //2
        blocks[i].length = ntohs(blocks[i].length); //2
        blocks[i].payload = (uint8_t*) malloc(sizeof(uint8_t) * blocks[i].length);
        read_ = read(fd, blocks[i].payload, blocks[i].length);

        uint16_t checksum = 0;
        // for(int j=0;j<blocks[i].length/2;j++){
        //     read(fd, &payload[i][j], 2);
        // }
        for(int j=0; j<blocks[i].length; j+=2){
            uint16_t tocheck = blocks[i].payload[j] << 8;   // 8->16
            tocheck += blocks[i].payload[j+1];
            checksum ^= tocheck;    // XOR
        }
        if(checksum == blocks[i].cksum){
            check[i] = true;
        }
        else{
            check[i] = false;
        }
    }
    
    for (int i=0; i<header.n_blocks; ++i){
        if (check[i]){         
            for (int j=0; j<blocks[i].length; ++j)
                dict[blocks[i].offset + j] = blocks[i].payload[j];
        }
        
    }
    
    flag_t flag;
    read_ = read(fd, &flag, 2);
    flag.length = ntohs(flag.length);
    

    flag.offset = (uint32_t*) malloc(sizeof(uint32_t) * flag.length);

    read_ = read(fd, flag.offset, flag.length*4);
    // printf("Read Flag");

    printf("Flag : ");
    for (int i=0; i<flag.length; ++i){
        flag.offset[i] = ntohl(flag.offset[i]);
        printf("%c%c", dict[flag.offset[i]], dict[flag.offset[i]+1]);
    }   
    printf("\nHex flag : ");
    for (int i=0; i<flag.length; ++i){
        printf("%02x%02x", dict[flag.offset[i]], dict[flag.offset[i]+1]);
    }
    printf("\n");
    // printf("HEX : %d", flag.length);
    
    char* flags = (char*) malloc(sizeof(char) * flag.length*4 + 1);   // save to string
    for (int i=0; i<flag.length; ++i){
        sprintf(flags + i*4, "%02x", dict[flag.offset[i]]);
        sprintf(flags + i*4+2, "%02x", dict[flag.offset[i]+1]);
    }
    flags[flag.length * 4] = '\0';
    char* sendflag = (char*) malloc(sizeof(char) * flag.length*4 + 1);
    sprintf(sendflag, "curl https://inp.zoolab.org/binflag/verify?v=%s", flags);
    system(sendflag);
    // free memory
    for (int i=0; i<header.n_blocks; ++i){
        free(blocks[i].payload);
    }
    free(blocks);
    free(check);
    free(dict);
    free(flag.offset);    
    close(fd);
    return 0;
}