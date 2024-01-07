#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <mutex>
#include <algorithm>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <thread>

using namespace std;

const int MAX_CLIENTS = 10;
const int BUFFER_SIZE = 1024;
const int MAX_CHAT_HISTORY_SIZE = 10;

mutex client_sockets_mutex;
vector<int> client_sockets;

map<string, string> registered_users;

set<int> logged_in_clients;
set<string> logged_in_usernames;

map<int, string> client_usernames;
map<string, string> client_statuses;
map<int, int> client_chat_rooms;

map<int, vector<string>> chat_rooms;
map<int, set<int>> chat_room_clients;
map<int, string> chat_room_creators;
map<int, bool> chat_room_pins;
map<int, string> room_pin_message;
// bool ppin = false;
bool in = false;

vector<string> filtered_keywords = {
    "superpie",
    "hello",
    "starburst stream",
    "domain expansion",
    "=="
};

// Keep track of whether a user has already exited a chat room
map<int, bool> user_exited_chat_room;

void send_message(int client_socket, const char* message) {
    send(client_socket, message, strlen(message), 0);
}

void send_chat_history(int client_socket, int room_number) {
    if (chat_rooms.find(room_number) != chat_rooms.end()) {
        int history_size = chat_rooms[room_number].size();
        int start_index = max(0, history_size - MAX_CHAT_HISTORY_SIZE);
        for (int i = start_index; i < history_size; ++i) {
            send_message(client_socket, chat_rooms[room_number][i].c_str());
            send_message(client_socket, "\n");
        }
    }
    if(chat_room_pins[room_number]){
        send_message(client_socket, room_pin_message[room_number].c_str());
        send_message(client_socket, "\n");
    }
}

// Function to apply content filtering to a message
string filter_message(const string& message) {
    string filtered_message;
    string lowercase_message = message;
    transform(lowercase_message.begin(), lowercase_message.end(), lowercase_message.begin(), ::tolower);

    for (size_t i = 0; i < message.length(); ++i) {
        bool keyword_found = false;
        for (const string& keyword : filtered_keywords) {
            string lower_keyword = keyword;
            transform(lower_keyword.begin(), lower_keyword.end(), lower_keyword.begin(), ::tolower);
            size_t pos = lowercase_message.find(lower_keyword, i);
            if (pos != string::npos && (pos == i || !isalpha(message[pos - 1]))) {
                filtered_message += string(keyword.length(), '*');
                i = pos + keyword.length() - 1;
                keyword_found = true;
                break;
            }
        }
        if (!keyword_found) {
            filtered_message += message[i];
        }
    }
    return filtered_message;
}

void handle_command(int client_socket, const char* command) {
    char cmd[BUFFER_SIZE];
    strncpy(cmd, command, BUFFER_SIZE);

    const char* token = strtok(cmd, " \n");

    if (token == nullptr) {
        send_message(client_socket, "Usage: Enter a valid command.\n% ");
        return;
    } else if (token == nullptr && in == true) {
        send_message(client_socket, "Usage: Enter a valid command.\n");
        return;
    }

    if (strcmp(token, "register") == 0) {
        const char* username = strtok(nullptr, " \n");
        const char* password = strtok(nullptr, " \n");

        if (username == nullptr || password == nullptr) {
            send_message(client_socket, "Usage: register <username> <password>\n% ");
        } else if (registered_users.find(username) != registered_users.end()) {
            send_message(client_socket, "Username is already used.\n% ");
        } else {
            registered_users[username] = password;
            send_message(client_socket, "Register successfully.\n% ");
        }
    } else if (strcmp(token, "login") == 0) {
        if (logged_in_clients.find(client_socket) != logged_in_clients.end()) {
            send_message(client_socket, "Please logout first.\n% ");
            return;
        }

        const char* username = strtok(nullptr, " \n");
        const char* password = strtok(nullptr, " \n");

        if (username == nullptr || password == nullptr) {
            send_message(client_socket, "Usage: login <username> <password>\n% ");
        } else if (registered_users.find(username) == registered_users.end() ||
                   registered_users[username] != password) {
            send_message(client_socket, "Login failed.\n% ");
        } else if (logged_in_usernames.find(username) != logged_in_usernames.end()) {
            send_message(client_socket, "Login failed.\n% ");
        } else {
            logged_in_clients.insert(client_socket);
            client_usernames[client_socket] = username;
            client_statuses[username] = "online";
            logged_in_usernames.insert(username);

            send_message(client_socket, "Welcome, ");
            send_message(client_socket, username);
            send_message(client_socket, ".\n% ");
        }
    } else if (strcmp(token, "logout") == 0) {
         // Check if there are any extra parameters
        token = strtok(nullptr, " \n");
        if (token != nullptr) {
            send_message(client_socket, "Usage: logout\n% ");
        } else {
            if (logged_in_clients.find(client_socket) == logged_in_clients.end()) {
                send_message(client_socket, "Please login first.\n% ");
            } else {
                const string& username = client_usernames[client_socket];
                logged_in_clients.erase(client_socket);
                logged_in_usernames.erase(username);
                client_usernames.erase(client_socket);
                client_statuses[username] = "offline";
                send_message(client_socket, "Bye, ");
                send_message(client_socket, username.c_str());
                send_message(client_socket, ".\n% ");
            }
        }
    } else if (strcmp(token, "exit") == 0 && in == false) {
        // Check if there are any extra parameters
        token = strtok(nullptr, " \n");
        if (token != nullptr) {
            send_message(client_socket, "Usage: exit\n% ");
        } else {
            // Check if the user is logged in
            const string& username = client_usernames[client_socket];
            if (logged_in_clients.find(client_socket) != logged_in_clients.end()) {
                // The user is logged in, automatically log them out
                logged_in_clients.erase(client_socket);
                client_usernames.erase(client_socket);
                client_statuses[username] = "offline";

                // Remove the user's status from the set of logged-in usernames
                logged_in_usernames.erase(username);

                send_message(client_socket, "Bye, ");
                send_message(client_socket, username.c_str());
                send_message(client_socket, ".\n");
                close(client_socket);
            } else {
                // The user is not logged in, just close the socket
            
                close(client_socket);
            }

        }     
    } else if (strcmp(token, "whoami") == 0) {
        // Check if there are any extra parameters
        token = strtok(nullptr, " \n");
        if (token != nullptr) {
            send_message(client_socket, "Usage: whoami\n% ");
        } else {
            if (logged_in_clients.find(client_socket) == logged_in_clients.end()) {
                send_message(client_socket, "Please login first.\n% ");
            } else {
                const string& username = client_usernames[client_socket];
                send_message(client_socket, username.c_str());
                send_message(client_socket, "\n% ");
            }
        }
    } else if (strcmp(token, "set-status") == 0) {
        // token = strtok(nullptr, " \n");
        const char* status = strtok(nullptr, " \n");
        if (status == nullptr) {
            send_message(client_socket, "Usage: set-status <status>\n% ");
        } else {
            if (logged_in_clients.find(client_socket) == logged_in_clients.end()) {
                send_message(client_socket, "Please login first.\n% ");
            } else {
                const string& username = client_usernames[client_socket];
                // const char* status = strtok(nullptr, " \n");               
                if (strcmp(status, "online") == 0 || strcmp(status, "offline") == 0 || strcmp(status, "busy") == 0) {
                    client_statuses[username] = status;
                    send_message(client_socket, username.c_str());
                    send_message(client_socket, " ");
                    send_message(client_socket, status);
                    send_message(client_socket, "\n% ");
                } else {
                    send_message(client_socket, "set-status failed\n% ");
                }                
            }
        }
    } else if (strcmp(token, "list-user") == 0) {
        token = strtok(nullptr, " \n");
        if (token != nullptr) {
            send_message(client_socket, "Usage: list-user\n% ");
        } else {
            if (logged_in_clients.find(client_socket) == logged_in_clients.end()) {
                send_message(client_socket, "Please login first.\n% ");
            } else {
                // Send a list of users and their statuses alphabetically
                for (const auto& entry : registered_users) {
                    const string& username = entry.first;
                    const string& status = client_statuses.find(username) != client_statuses.end()
                                            ? client_statuses[username]
                                            : "offline";
                    send_message(client_socket, username.c_str());
                    send_message(client_socket, " ");
                    send_message(client_socket, status.c_str());
                    send_message(client_socket, "\n");
                }
                send_message(client_socket, "% ");
            }
        }
    } else if (strcmp(token, "enter-chat-room") == 0) {
        if (logged_in_clients.find(client_socket) == logged_in_clients.end()) {
            send_message(client_socket, "Please login first.\n% ");
        } else {
            const char* room_number_str = strtok(nullptr, " \n");
            if (room_number_str == nullptr) {
                send_message(client_socket, "Usage: enter-chat-room <number>\n% ");
            } else {
                int room_number = atoi(room_number_str);
                if (room_number < 1 || room_number > 100) {
                    send_message(client_socket, "Number ");
                    send_message(client_socket, to_string(room_number).c_str());
                    send_message(client_socket, " is not valid.\n% ");
                } else {
                    if (chat_room_creators[room_number].empty()) {
                        // Set the current user as the owner
                        chat_room_creators[room_number] = client_usernames[client_socket];
                    }

                    client_chat_rooms[client_socket] = room_number;

                    send_message(client_socket, "Welcome to the public chat room.\n");
                    send_message(client_socket, "Room number: ");
                    send_message(client_socket, to_string(room_number).c_str());
                    send_message(client_socket, "\n");
                    send_message(client_socket, "Owner: ");
                    send_message(client_socket, chat_room_creators[room_number].c_str());
                    send_message(client_socket, "\n");

                    in = true;

                    // Send chat history
                    if (chat_rooms[room_number].size() > 0 || chat_room_pins[room_number]) {
                        send_chat_history(client_socket, room_number);
                    }

                    // Notify other clients in the room
                    for (const auto& entry : logged_in_clients) {
                        int other_client_socket = entry;
                        if (other_client_socket != client_socket &&
                            client_chat_rooms[other_client_socket] == room_number) {
                            send_message(other_client_socket, client_usernames[client_socket].c_str());
                            send_message(other_client_socket, " had enter the chat room.\n");
                        }
                    }
                }
            }
        }
    } else if (strcmp(token, "list-chat-room") == 0) {
        token = strtok(nullptr, " \n");
        if (token != nullptr) {
            send_message(client_socket, "Usage: list-chat-room\n% ");
        } else {
            if (logged_in_clients.find(client_socket) == logged_in_clients.end()) {
                send_message(client_socket, "Please login first.\n% ");
            } else {
                for (const auto& entry : chat_rooms) {
                    int room_number = entry.first;
                    const string& owner = chat_room_creators[room_number];
                    string room_info = owner + " " + to_string(room_number) + "\n";
                    send_message(client_socket, room_info.c_str());
                }
                send_message(client_socket, "% ");
            }
        }
    } else if (strcmp(token, "close-chat-room") == 0) {
        if (logged_in_clients.find(client_socket) == logged_in_clients.end()) {
            send_message(client_socket, "Please login first.\n% ");
        } else {
            const char* room_number_str = strtok(nullptr, " \n");
            if (room_number_str == nullptr) {
                send_message(client_socket, "Usage: close-chat-room <number>\n% ");
            } else {
                int room_number = atoi(room_number_str);
                if (chat_rooms.find(room_number) == chat_rooms.end()) {
                    send_message(client_socket, "Chat room ");
                    send_message(client_socket, room_number_str);
                    send_message(client_socket, " does not exist.\n% ");
                } else {
                    const string& room_owner = chat_room_creators[room_number];
                    const string& client_username = client_usernames[client_socket];
                    if (room_owner != client_username) {
                        send_message(client_socket, "Only the owner can close this chat room.\n% ");
                    } else {
                        // Notify all users in the chat room and close it
                        for (const auto& entry : logged_in_clients) {
                            int other_client_socket = entry;
                            if (client_chat_rooms[other_client_socket] == room_number) {
                                send_message(other_client_socket, "Chat room ");
                                send_message(other_client_socket, room_number_str);
                                send_message(other_client_socket, " was closed.\n% ");
                                client_chat_rooms.erase(other_client_socket);
                            }
                        }
                        chat_rooms.erase(room_number);
                        chat_room_creators.erase(room_number);
                        send_message(client_socket, "Chat room ");
                        send_message(client_socket, room_number_str);
                        send_message(client_socket, " was closed.\n% ");
                    }
                }
            }
        }
    } else if (client_chat_rooms.find(client_socket) != client_chat_rooms.end()) {
        int room_number = client_chat_rooms[client_socket];
        if(token[0] == '/'){
            if (strcmp(token, "/pin") == 0) {
                const char* message = strtok(nullptr, "\n");
                string token_message = message;
                string filtered_message = filter_message(token_message);
                if (message != nullptr && strlen(message) <= 150) {
                    string pinned_message = "Pin -> [" + client_usernames[client_socket] + "]: " + filtered_message;
                    room_pin_message[room_number] = pinned_message;
                    chat_room_pins[room_number] = true;
                    send_message(client_socket, (pinned_message + "\n").c_str());
                    
                    // Notify all users in the chat room
                    for (const auto& entry : logged_in_clients) {
                        int other_client_socket = entry;
                        if (other_client_socket != client_socket &&
                            client_chat_rooms[other_client_socket] == room_number) {
                            send_message(other_client_socket, (pinned_message + "\n").c_str());
                        }
                    }
                } else {
                    send_message(client_socket, "Message length exceeds 150 characters or is empty.\n");
                }
            } else if (strcmp(token, "/delete-pin") == 0) {
                if (chat_room_pins[room_number]) {
                    room_pin_message[room_number] = "";
                } else {
                    send_message(client_socket, "No pin message in chat room ");
                    send_message(client_socket, to_string(room_number).c_str());
                    send_message(client_socket, "\n");
                }
            } else if (strcmp(token, "/exit-chat-room") == 0) {
                if (client_chat_rooms.find(client_socket) != client_chat_rooms.end()) {
                    int room_number = client_chat_rooms[client_socket];
                    const string& client_username = client_usernames[client_socket];
                    
                    // Notify other clients in the chat room
                    for (const auto& entry : logged_in_clients) {
                        int other_client_socket = entry;
                        if (other_client_socket != client_socket &&
                            client_chat_rooms[other_client_socket] == room_number) {
                            send_message(other_client_socket, (client_username + " had left the chat room.\n").c_str());
                        }
                    }
                    
                    // Remove the client from the chat room
                    chat_room_clients[room_number].erase(client_socket);
                    client_chat_rooms.erase(client_socket);
                    send_message(client_socket, "% ");
                    in = false;
                } else {
                    send_message(client_socket, "You are not in a chat room.\n");
                }
            } else if (strcmp(token, "/list-user") == 0) {
                // Create a vector to store usernames of users in the chat room
                vector<string> usernames_in_room;

                for (const auto& entry : logged_in_clients) {
                    int other_client_socket = entry;
                    if (client_chat_rooms[other_client_socket] == room_number) {
                        const string& username = client_usernames[other_client_socket];
                        usernames_in_room.push_back(username);
                    }
                }

                // Sort the usernames alphabetically
                sort(usernames_in_room.begin(), usernames_in_room.end());

                // Send the sorted list of users and their statuses
                for (const string& username : usernames_in_room) {
                    const string& status = client_statuses[username];
                    send_message(client_socket, username.c_str());
                    send_message(client_socket, " ");
                    send_message(client_socket, status.c_str());
                    send_message(client_socket, "\n");
                }
            } else {
                send_message(client_socket, "Error: Unknown command\n");
            }
        } else {
            // Regular message, broadcast to all users in the chat room
            const string& client_username = client_usernames[client_socket];
            const char* message = strtok(nullptr, "\n");
            string token_message = token;

            if (message != nullptr) {
                token_message += string(" ") + message;
            }
            
            string filtered_message = filter_message(token_message);

            if (message == nullptr || strlen(message) <= 150) {
                string chat_message = "[" + client_username + "]: " + filtered_message;
                chat_rooms[room_number].push_back(chat_message);

                for (const auto& entry : logged_in_clients) {
                    int other_client_socket = entry;
                    if (client_chat_rooms[other_client_socket] == room_number) {
                        send_message(other_client_socket, chat_message.c_str());
                        send_message(other_client_socket, "\n");
                    }
                }
            } else {
                send_message(client_socket, "Message length exceeds 150 characters or is empty.\n");
            }
        }
    } else {
        send_message(client_socket, "Error: Unknown command\n% ");
    }
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    
    // Place your code for handling client communication here.
    
    while (true) {
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            cout << "Client disconnected..." << endl;
            lock_guard<mutex> lock(client_sockets_mutex);
            client_sockets.erase(remove(client_sockets.begin(), client_sockets.end(), client_socket), client_sockets.end());
            // Check if the client was logged in and perform logout
            if (logged_in_clients.find(client_socket) != logged_in_clients.end()) {
                const string& username = client_usernames[client_socket];
                logged_in_clients.erase(client_socket);
                logged_in_usernames.erase(username);
                client_usernames.erase(client_socket);
                client_statuses[username] = "offline";
            }
            close(client_socket);
            break;  // Exit the loop when the client disconnects.
        } else {
            buffer[bytes_received] = '\0';
            handle_command(client_socket, buffer);
        }
    }
    
    // Close the socket and perform any cleanup when the client disconnects.
    close(client_socket);
}

int main() {
    int server_socket, new_socket, max_socket;
    struct sockaddr_in server_addr, new_addr;
    socklen_t addr_size;
    char buffer[BUFFER_SIZE];

    for (int r = 1; r <= 100; r++) {
        chat_room_pins[r] = false;
    }

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Error in socket");
        exit(1);
    }
    cout << "Server socket created..." << endl;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8888);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error in binding");
        exit(1);
    }
    cout << "Binding success..." << endl;

    if (listen(server_socket, MAX_CLIENTS) == 0) {
        cout << "Listening..." << endl;
    } else {
        cout << "Error in listening" << endl;
        exit(1);
    }

    addr_size = sizeof(new_addr);
    while (true) {
        new_socket = accept(server_socket, (struct sockaddr*)&new_addr, &addr_size);
        {
            lock_guard<mutex> lock(client_sockets_mutex);
            client_sockets.push_back(new_socket);
        }
        cout << "New client connected..." << endl;
        send_message(new_socket, "*********************************\n");
        send_message(new_socket, "** Welcome to the Chat server. **\n");
        send_message(new_socket, "*********************************\n% ");
      
        thread client_thread(handle_client, new_socket);
        client_thread.detach(); 
    }
    return 0;
}
