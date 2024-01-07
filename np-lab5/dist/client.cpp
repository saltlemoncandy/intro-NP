#include <iostream>
#include <chrono>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;
int main() {
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        cerr << "Error in socket creation" << std::endl;
        return 1;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8081); // Port number
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Server's IP address

    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Error in connecting to server" << endl;
        return -1;
    }

    char buffer[1024] = "Wake up";

	auto start = chrono::steady_clock::now();
    ssize_t bytesSent = send(clientSocket, buffer, strlen(buffer), 0);
    if (bytesSent < 0) {
        cerr << "Error in sending data" << endl;
        close(clientSocket);
        return -1;
    }
	memset(buffer, 0, sizeof(buffer));
    ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    auto end = chrono::steady_clock::now();
    if (bytesReceived<0) {
        cerr << "Error in receiving data" << endl;
        return -1;
    }
	chrono::duration<double> latency = (end - start)/2;

    int byte_all=0, get_time=0;
    long long int get_byte=0;
    double recv_time=0;

    auto start_bw = chrono::steady_clock::now();
    while(byte_all<50*1024*1024){
        char buffer[1024*1024];
        auto start_recv = chrono::steady_clock::now();
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        auto end_recv = chrono::steady_clock::now();
        chrono::duration<double> cur_recv_time = end_recv-start_recv;
        if (bytesRead<0){
            cerr << "Error: Receive failed.\n";
        }else{
            buffer[bytesRead] = '\0';
            if(cur_recv_time.count()<0.005){
                recv_time += cur_recv_time.count();
                get_byte += (bytesRead+66);
            }
            byte_all += bytesRead;
            get_time++;
        }
    }
    auto end_bw = chrono::steady_clock::now();
    double time = recv_time;
    double bw = get_byte/time;

    int delay = int (latency.count() * 1000);
    int bandwidth = int(8*bw/1e6);
   
    cout << "# RESULTS: delay = " << delay << " ms, bandwidth = " << bandwidth << " Mbps";

    close(clientSocket);
    return 0;
}
