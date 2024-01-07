#include <iostream>
#include <vector>
#include <unordered_map>
#include <math.h>
#include <cstring>
#include <random>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <chrono>
#include <unistd.h>
using namespace std;
const int BOARD_SIZE = 30;
vector<pair<int, int>> target;
int board[30][30];

bool solve(vector<uint8_t> init_board, int8_t idx, uint8_t queen_position){
    if(idx>=0){
        init_board.at(idx) = queen_position;
    }
    unordered_map<uint8_t, uint8_t> queen_mapping;
    for(size_t sz=0; sz<init_board.size(); sz++){
        if(init_board.at(sz)){
            queen_mapping.insert(pair<uint8_t, uint8_t> (sz, init_board.at(sz)));
        }
    }
    // print board
    if(queen_mapping.size()==init_board.size()){
        // cout << "============ GOAL STATE ===========" << endl;
        for (size_t x=0; x<init_board.size(); x++){
            for (size_t y=0; y<init_board.size(); y++){
                if (init_board[y]==(x+1)){
                    // cout << "Q|";
                    target.push_back({x, y});
                }
                // else{
                //     cout << " |";
                // }
            }
            // cout << endl;
        }
        // cout << endl;
        // cout << "===================================" << endl;
        return true;
    }

    for(size_t sz=0; sz<init_board.size(); sz++){
        if(queen_mapping.find(sz)==queen_mapping.end()){
            for(size_t num=1; num<=init_board.size(); num++){
                bool conflict = false;
                for(const auto &queen:queen_mapping){
                    uint8_t oen = abs(queen.first-(uint8_t)sz);
                    uint8_t tow = abs(queen.second-(uint8_t)num);
                    if(num==queen.second || oen==tow){
                        conflict = true;
                        break;
                    }
                }
                if(conflict){
                    continue;
                }
                else{
                    if(solve(init_board, (int8_t)sz, num)){
                        return true;
                    }
                }
            }
            return false;
        }
    }
    return false;
}

int main() {
    int clientSocket, serverLen;
    struct sockaddr_un serverAddress;

    // Create a socket
    clientSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        cerr << "Error creating socket" << endl;
        return 1;
    }

    // Clear server address structure
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sun_family = AF_UNIX;
    strncpy(serverAddress.sun_path, "./queen.sock", sizeof(serverAddress.sun_path) - 1);

    // Connect to the server
    serverLen = sizeof(serverAddress);
    if (connect(clientSocket, (struct sockaddr *)&serverAddress, serverLen) == -1) {
        cerr << "Error connecting to the server" << endl;
        close(clientSocket);
        return 1;
    }

    // Send a message to the server
    string message = "s";
    send(clientSocket, message.c_str(), message.size(), 0);
    
    // Receive a response from the server
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived == -1) {
        cerr << "Error receiving response" << endl;
        close(clientSocket);
        return 1;
    }
    buffer[bytesReceived] = '\0';
    // cout << "Received from server: \n" << buffer << endl;
    // cout<<"---"<<endl;

    int index = 4;
    for(int i=0; i<BOARD_SIZE; ++i){
        for(int j=0; j<BOARD_SIZE; ++j){
            if(buffer[index]=='Q'){
                board[i][j] = 1;
            }
            else if(buffer[index]=='.'){
                board[i][j] = 0;
            }
            index++;
        }
    }
    // cout << "initial board" << endl;
    // for(int i=0; i<BOARD_SIZE; ++i){
    //     for(int j=0; j<BOARD_SIZE; ++j){
    //         if(board[i][j]==1){
    //             cout << " Q ";
    //         }
    //         else{
    //             cout << '.';
    //         }
    //     }
    //     cout << endl;
    // }
    vector<string> nList;
    for(int col=0; col<BOARD_SIZE; col++){
        bool flag = false;
        for(int row=0; row<BOARD_SIZE; row++){
            if(board[row][col]==1){
                flag = true;
                nList.push_back(to_string(row+1));
                break;
            }
        }
        if(!flag){
            nList.push_back(to_string(0));
        }
    }

    vector<uint8_t> init_board;
    for(const auto &n:nList){
        init_board.push_back((uint8_t)stoi(n));
    }
    cout << "solving ..." << endl;
    auto start_time = chrono::high_resolution_clock::now();
    bool solved = solve(init_board, -1, 0);
    auto end_time = chrono::high_resolution_clock::now();
    cout << "Execution time: " << chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count() << endl;
    for(const auto &i:target){
        char mark_queen[20];
        sprintf(mark_queen, "M %d %d", i.first, i.second);

        if(send(clientSocket, mark_queen, strlen(mark_queen), 0)==-1){
            perror("Error sending command M");
            exit(EXIT_FAILURE);
        }

        char buff[1024];
        memset(buff, 0, sizeof(buff));
        if (recv(clientSocket, buff, sizeof(buff), 0)==-1){
            perror("Error receiving response");
            exit(EXIT_FAILURE);
        }
        cout << "M - response " << buff << endl;

    }
    string check = "c";
    if(send(clientSocket, check.c_str(), check.size(), 0)==-1){
        perror("Error sending command M");
        exit(EXIT_FAILURE);
    }

    char buf[1024];
    memset(buf, 0, sizeof(buf));
    if (recv(clientSocket, buf, sizeof(buf), 0)==-1){
        perror("Error receiving response");
        exit(EXIT_FAILURE);
    }
    cout << "C - response " << buf << endl;

    // Close the socket
    close(clientSocket);

    return 0;
}
