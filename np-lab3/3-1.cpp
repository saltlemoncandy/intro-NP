#include <netinet/in.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <cstring> 
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <iostream>
using namespace std;

char buff[100000001]; 
char ans[2000];
const char* host = "inp.zoolab.org";
// int maze[11][7];

int main(){
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in srv_address, client; 
    if (socketfd == -1) {
        printf("Socket Error\n"); 
        exit(1);
    }
    bzero(&srv_address, sizeof(srv_address));
    srv_address.sin_family = AF_INET;
    srv_address.sin_addr.s_addr = inet_addr(host);
    
    struct hostent *server = gethostbyname(host); 
    if(server == NULL){
        perror("error solving hostname");
        return 1;
    }
    bcopy((char*)server->h_addr, (char*)&srv_address.sin_addr, server->h_length);
    srv_address.sin_port = htons(10301);
    int check_connect = connect(socketfd, (struct sockaddr*)&srv_address, sizeof(srv_address));
    if (check_connect != 0) {
        printf("Connect Error\n"); 
        exit(1); 
    }
    
    while(1){
        bzero(buff, sizeof(buff));
        long long get_ = recv(socketfd, buff, sizeof(buff), 0);
        if(get_ <= 0){
            printf("server closed connect");
            exit(1);
        }
        printf("%s", buff);
        string buffer = buff;      

        int width = 11;
        int length= 7;
        size_t found = buffer.find("\n#");
        int sz = 12*7;
        string maze = buffer.substr(found+1, sz);
        cout << maze
        
       
    }
    
    

     // int maze[maze_len][maze_wid];
        // printf("drawing maze\n");
        // char* wall = strstr(buff, "##");
        // int idx = wall - buff;
        // for(int i=0; i<maze_len; i++){
        //     for(int j=0; j<maze_wid; j++){
        //         maze[i][j] = buff[idx];
        //         idx++;
        //     }
        //     idx++;
        // }
        // for(int i=0; i<maze_len; i++){
        //     for(int j=0; j<maze_wid; j++){
        //         cout << maze[i][j];
        //     }
        // }
        // break;
    

}

