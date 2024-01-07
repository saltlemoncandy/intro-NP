#include <netinet/in.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <fstream> 
#include <cstring> 
#include <cctype>
#include <iomanip>
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <iostream>
using namespace std;


// const char* host = "inp.zoolab.org"; const int port = 10314;
const char* host = "172.21.0.4";    const int port = 10001;

int main() {
    
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);    
    if (clientSocket == -1) {
        cout << "Error creating socket" << endl;
        exit(1);
    }    
    struct hostent *server = gethostbyname(host); 
    if(server == nullptr ){
        cout << "hostname error" << endl;
        return 1;
    }
    struct sockaddr_in serverAddress;  
    serverAddress.sin_family = AF_INET; 
    serverAddress.sin_port = htons(port);
    memcpy(&serverAddress.sin_addr.s_addr, server->h_addr, server->h_length);

    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        cerr << "Error connecting to the server" << endl;
        close(clientSocket);
        return 1;
    }

    // string getOTP = "GET /otp?name=110550158 HTTP/1.1\r\nHost: inp.zoolab.org:10314\r\n";
    string getOTP = "GET /otp?name=110550158 HTTP/1.1\r\nHost: 172.21.0.4:10001\r\n";
    getOTP += "Connection: Keep-Alive\r\n\r\n";

    int sent = send(clientSocket, getOTP.c_str(), getOTP.size(), 0);
    if (sent == -1) {
        cout << "Error sending data to the server" << endl;
        close(clientSocket);
        return 1;
    }    

    char OTPbuffer[4096]; 
    int bytesRead = recv(clientSocket, OTPbuffer, sizeof(OTPbuffer), 0);
    if (bytesRead == -1) {
        cout << "Error receiving data from the server" << endl;
    }    
    while(recv(clientSocket, OTPbuffer, sizeof(OTPbuffer), 0)>0){
            OTPbuffer[bytesRead] = '\0'; 
            cout << "\nReceived from server: \n" << OTPbuffer << endl;

    }
    string buff = OTPbuffer;
    int startPos = 0;
    int endPos = buff.find("charset");
    string otp = buff.substr(startPos, endPos-startPos);
    cout << "otp: " << otp << endl;
    close(clientSocket);

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        cout << "Error connecting to the server" << endl;
        close(clientSocket);
        return 1;
    }

    string boundary = "------------------------23jrofmwl4oqdfeb2";
    string h = "POST /upload HTTP/1.1\r\nHost: inp.zoolab.org:10314\r\n";
    // string h = "POST /upload HTTP/1.1\r\nHost: 172.21.0.4:10001\r\n\r\n";
    h += "Connection: Keep-Alive\r\n";
    string b = "--" + boundary + "\r\n"
            "Content-Disposition: form-data; name=\"file\"; filename=\"otp.txt\"\r\n\r\n";
    b += otp;
    b += "\r\n--"+boundary+"--\r\n";
    h += "Content-Length: " + to_string(b.length()) + "\r\nContent-Type: multipart/form-data; boundary=" + boundary + "\r\n\r\n";
    send(clientSocket, h.c_str(), h.size(), 0);
    send(clientSocket, b.c_str(), b.size(), 0);

    char buf[1024];
    // int bytesRead;
    while ((bytesRead = recv(clientSocket, buf, sizeof(buf), 0)) > 0) {
        buf[bytesRead] = '\0';
        cout << buf << "\n";
    }
    close(clientSocket);
    return 0;
}
