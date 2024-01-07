#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fstream>
#include <iterator>


using namespace std;

int main() {
    #if DEPLOY
    const char* server_address = "172.21.0.4";
    const int server_port = 10001; 
    #else
    const char* server_address = "inp.zoolab.org";
    const int server_port = 10314;
    #endif

    // Create a socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        cerr << "Error creating socket." << endl;
        return 1;
    }

    // Resolve the server's hostname to an IP address
    struct hostent *serverInfo = gethostbyname(server_address);
    if (serverInfo == nullptr) {
        cerr << "Error resolving server hostname." << endl;
        close(clientSocket);
        return 1;
    }

    // Set up server address
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(server_port);
    memcpy(&serverAddr.sin_addr.s_addr, serverInfo->h_addr, serverInfo->h_length);

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        cerr << "Error connecting to the server." << endl;
        close(clientSocket);
        return 1;
    }
   


    char * s;
    while (1) {
        // For example, you can send a message to the server
        string httpRequest = "GET /otp?name=110550158 HTTP/1.1\r\n"
                                "Host: ";
        httpRequest += server_address;
        httpRequest += ":" + to_string(server_port) + "\r\n";
        httpRequest += "Connection: Keep-Alive\r\n\r\n";

        // Send the HTTP request
        if (send(clientSocket, httpRequest.c_str(), httpRequest.size(), 0) < 0) {
            cout << "Send failed\n";
            continue;
        }

        // Receive and print the HTTP response
        char buffer[4096];
        int bytesRead;
        while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0) > 0)) {
            if (strstr(buffer, "110550158") != nullptr)
                break;
            bzero(buffer, sizeof(buffer));
        }
        s = strstr(buffer, "110550158");
        cout<<"aaa"<<s<<endl;
        break;
    }

    // // After sending each request, close the socket
    close(clientSocket);

    // Re-establish the socket connection for the next request
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        cerr << "Error connecting to the server." << endl;
        close(clientSocket);
        return 1;
    }


    while (1) {
        // For example, you can send a message to the server
        string boundary = "------------------------799eae777d9d8bb2";
        string header = "POST /upload HTTP/1.1\r\n"
                        "Host: ";
        header += server_address; 
        header += ":" + to_string(server_port) + "\r\n";
        header += "Connection: Keep-Alive\r\n";

        string body = "--"+boundary+"\r\n"
                    "Content-Disposition: form-data; name=\"file\"; filename=\"otp.txt\"\r\n\r\n";
        body += s;
        body += "\r\n--"+boundary+"--\r\n";

        header += "Content-Length: " + to_string(body.length()) + "\r\nContent-Type: multipart/form-data; boundary="+boundary+"\r\n\r\n";
        
        send(clientSocket, header.c_str(), header.size(), 0);
        send(clientSocket, body.c_str(), body.size(), 0);

        
        char buf[1024];
        int bytesRead;
        while ((bytesRead = recv(clientSocket, buf, sizeof(buf), 0)) > 0) {
            buf[bytesRead] = '\0';
            cout << buf << "\n";
        }
        break;
    }
    
    close(clientSocket);

    return 0;

}
