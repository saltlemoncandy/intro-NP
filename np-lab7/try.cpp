#include <iostream>
#include <vector>
#include <unordered_map>
#include <math.h>
#include <cstring>
#include <random>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#define N 30
using namespace std;
const char *SOCKET_PATH = "/queen.sock";
typedef vector<uint8_t> ST_BOARD;

int board[30][30];
vector<pair<int, int>> target;

int connectSocket()
{
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("Error connecting to socket");
        exit(EXIT_FAILURE);
    }

    return sockfd;
}
void closeSocket(int sockfd)
{
    close(sockfd);
}
void sendCommand(int sockfd, const char *command)
{
    if (send(sockfd, command, strlen(command), 0) == -1)
    {
        perror("Error sending command");
        exit(EXIT_FAILURE);
    }

    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    if (recv(sockfd, buffer, sizeof(buffer), 0) == -1)
    {
        perror("Error receiving response");
        exit(EXIT_FAILURE);
    }

    cout << "Response from server: " << buffer << endl;
}
void get_board(int sockfd, const char *command)
{
    if (send(sockfd, command, strlen(command), 0) == -1)
    {
        perror("Error sending command");
        exit(EXIT_FAILURE);
    }
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    if (recv(sockfd, buffer, sizeof(buffer), 0) == -1)
    {
        perror("Error receiving response");
        exit(EXIT_FAILURE);
    }
    // cout << "Response from server: " << buffer << endl;
    int index = 4; // 前面有 " OK: "
    for (int i = 0; i < 30; ++i)
    {
        for (int j = 0; j < 30; ++j)
        {
            if (buffer[index] == 'Q')
            {
                board[i][j] = 1;
            }
            else if (buffer[index] == '.')
            {
                board[i][j] = 0;
            }
            index++;
        }
    }

    cout << "\ninitial board \n ";
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            if (board[i][j] == 1)
            {
                cout << " Q ";
            }
            else
            {
                cout << '.';
            }
        }
        cout << '\n';
    }
}

void printBoard(ST_BOARD &board)
{
    size_t count = board.size();
    for (size_t x = 0; x < count; x++)
    {
        for (size_t y = 0; y < count; y++)
        {
            if (board[y] == x + 1)
            {
                printf("Q|");
                target.push_back({x, y});
            }
            else
                printf(" |");
        }
        printf("\n");
    }
    printf("\n");
}
bool solve(ST_BOARD arg_init_queen, int8_t index, uint8_t queen_pos)
{
    if (index >= 0)
        arg_init_queen.at(index) = queen_pos;
    // (index, queen_pos)
    unordered_map<uint8_t, uint8_t> q_mapping;

    // find the queen then save to mapping
    for (size_t t = 0; t < arg_init_queen.size(); t++)
    {
        if (arg_init_queen.at(t))
        {
            q_mapping.insert(pair<uint8_t, uint8_t>(t, arg_init_queen.at(t)));
        }
    }

    uint8_t n = arg_init_queen.size();

    if (q_mapping.size() == n)
    {
        cout << "============ GOAL STATE ===========" << endl;
        printBoard(arg_init_queen);
        cout << "===================================" << endl;
        return true;
    }

    // solving problem
    for (size_t t = 0; t < n; t++)
    {
        auto it = q_mapping.find(t);
        if (it == q_mapping.end())
        {
            // select queen of position:
            for (size_t num = 1; num <= n; num++)
            {
                // check if available
                bool is_attacked = false;
                for (const auto &q : q_mapping)
                {
                    uint8_t cp1 = abs(q.first - (uint8_t)t);
                    uint8_t cp2 = abs(q.second - (uint8_t)num);
                    // printf("%u:%u:%u:%u:%u\n", num, q.first, q.second, cp1,cp2);
                    if (num == q.second || cp1 == cp2)
                    {
                        is_attacked = true;
                        break;
                    }
                }
                if (is_attacked)
                    continue;
                else
                {
                    // # put the queen
                    if (solve(arg_init_queen, (int8_t)t, num))
                    {
                        return true;
                    }
                }
            }
            // if no select any position, should be return previous index to select another position of queen.
            return false;
        }
    }
    return false;
}

vector<string> split(const string &str)
{
    vector<string> res;
    if ("" == str)
        return res;
    char *strs = new char[str.length() + 1];
    strcpy(strs, str.c_str());

    char *p = strtok(strs, ",");
    while (p)
    {
        string s = p;
        res.push_back(s);
        p = strtok(NULL, ",");
    }

    return res;
}

int randomInteger(int min, int max)
{
    random_device rd;
    mt19937 generator = mt19937(rd());
    uniform_int_distribution<int> distribution(min, max);

    return distribution(generator);
}

void problem(ST_BOARD &board)
{

    bool rs = solve(board, -1, 0);
    if (rs)
    {
        cout << "[V] Find the solution!" << endl;
    }
    else
        cout << "[X] No Answer!" << endl;
}

int main()
{
    cout << "\n\n\nstarting main \n\n";
    int sockfd = connectSocket();
    get_board(sockfd, "S");

    vector<string> n_list;
    for (int col = 0; col < 30; col++)
    {
        bool flag = false;
        for (int row = 0; row < 30; row++)
        {
            if (board[row][col] == 1)
            {
                n_list.push_back(to_string(row + 1));
                flag = true;
                break;
            }
        }
        if (!flag)
        {
            n_list.push_back(to_string(0));
        }
    }
    ST_BOARD init_board;
    for (const auto &n : n_list)
    {
        init_board.push_back((uint8_t)stoi(n));
    }
    problem(init_board);
    for (const auto &p : target)
    {
        char formattedString[20]; // Adjust the size accordingly
        sprintf(formattedString, "M %d %d", p.first, p.second);
        sendCommand(sockfd, formattedString);
    }
    sendCommand(sockfd, "c");
    closeSocket(sockfd);
    // sendCommand(sockfd, "P");

    return 0;
}
