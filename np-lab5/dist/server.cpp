#include <iostream>
#include <chrono>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
using namespace std;
int main() {
    int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket < 0) {
        cerr << "Error in socket creation" << endl;
        return -1;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8081);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Error in binding" << endl;
        close(listenSocket);
        return -1;
    }

    if(listen(listenSocket, 5)<0){
        cerr << "Error: Listen failed.\n";
        close(listenSocket);
        return 1;
    }

    cout << "Server is running..." << endl;
	
    while (true) {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int connSocket = accept(listenSocket, (struct sockaddr*)&clientAddr, &clientLen);
        if (connSocket < 0) {
            cerr << "Error in accepting connection" << endl;
            close(listenSocket);
            return 1;
        }

        char buffer[1024];
        int bytesReceived = recv(connSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived < 0) {
            cerr << "Receive fail." << endl;
            break;
        }else{
            buffer[bytesReceived] = '\0';
			string str = buffer;
			if(str=="Wake up"){
                const char* ackMsg = "let me sleep";
                int bytesSent = send(connSocket, ackMsg, strlen(ackMsg)+1, 0);
                if (bytesSent < 0) {
                    cerr << "Error in sending acknowledgment" << endl;
                    close(connSocket);
                    break;
                }
            }
        }


        long long int sent_num = 0;
        for(int i=0; i<50; i++){
            char message[1024*1024];
            memset(message, 'T', sizeof(message));
            int num = send(connSocket, message, sizeof(message), 0);
            if(num < 0){
                break;
            }
            sent_num += num;
        }
        // cout << "sent number = " << sent_num << endl;
        close(connSocket);
    }

    close(listenSocket);
    return 0;
}
