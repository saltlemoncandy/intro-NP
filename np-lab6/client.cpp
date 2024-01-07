#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <cstring>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <filesystem>
#include <dirent.h>
#include <vector>
#include <unordered_map> 
#include <ctime>
#include <map>

using namespace std;
using pii = pair<int, int>;

map <pii, int> ACKed;
map <pii, string> send_failed;
int term_flag = 0;

pair<pii, string> decompose(string content){ // decompose a packet into 1.seq num, 2.checksum, 3.content(string)
	stringstream ss;
	string buf;
	int seq, offset;
	ss << content;
	ss >> seq >> offset >> buf;
	if(buf == "terminate"){
		term_flag = 1;
		cout << "term flag!\n";
	}
	return {{seq, offset}, buf};
}

static int sockfd = -1;
static struct sockaddr_in servin;
static unsigned seq;
static unsigned cnt = 0;

// Driver code
int main(int argc, char *argv[]){
    if(argc < 3) { // not enough parameters
		return -fprintf(stderr, "usage: %s ... <port> <ip>\n", argv[0]);
	}

	memset(&servin, 0, sizeof(servin));
	servin.sin_family = AF_INET;
	servin.sin_port = htons(strtol(argv[3], NULL, 0));
	if(inet_pton(AF_INET, argv[4], &servin.sin_addr) != 1) {
		return -fprintf(stderr, "** cannot convert IPv4 address for %s\n", argv[4]);
	}

    if((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
        perror("Socket Error\n");
        exit(0);
    }

	struct timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

	while (!term_flag){
		if(term_flag){
			close(sockfd);
			exit(0);
		}
		DIR *d = opendir(argv[1]) ;

		int num_file = strtol(argv[2], nullptr, 10);
		// int num_file = 800;
		
		struct dirent *dir;
		int cnt = 0;
		int seq_num = 0;
		if(d){
			while ((dir = readdir(d)) != NULL){
				//skip the directory "."
				if (!strcmp (dir->d_name, "."))
					continue;
				if (!strcmp (dir->d_name, ".."))    
					continue;
				if (dir->d_name[0] == '.') continue;
				
				if(cnt == num_file) break;
				cnt++;
				
				string path = string(argv[1]) + "/" + string(dir->d_name); //cout << "path = " << path << endl;
				FILE *fp = fopen(path.c_str(), "rb");
				// bzero(buf, sizeof(buf));
				if(fp){
					fseek(fp, 0, SEEK_END);
					size_t file_size = ftell(fp);
					fseek(fp, 0, SEEK_SET);

					// cout << "client size: " << file_size << "\n";

					vector <string> content;
					if(file_size < 1000){
						char* buf = static_cast<char*>(malloc(file_size + 1));
						size_t readd = fread(buf, 1, file_size, fp);
						buf[readd] = '\0';
						string converted(buf, buf + readd);
						content.emplace_back(converted);
						free(buf);
						// cout << "client s :" << converted.size() << "\n";
					}else{
						int n = file_size / 1000;
						char* content_buf = static_cast<char*>(malloc(1024));
						for(int i = 0; i < n; i++){
							bzero(content_buf, sizeof(content_buf));
							size_t readd = fread(content_buf, 1, 1000, fp);
							string converted(content_buf, content_buf + readd);
							content.emplace_back(converted);
						}
						if(file_size % 1000 != 0) {
							bzero(content_buf, sizeof(content_buf));
							size_t readd = fread(content_buf, 1, file_size - (1000 * n), fp);
							string converted(content_buf, content_buf + readd);
							content.emplace_back(converted);
						}
						free(content_buf);
					}

					
					
					char ackbuf[1024];
					pair<pii, string> packet = {{-1, -1}, ""};

					for(int offset = 0; offset < content.size(); offset++){
						packet = {{-1, -1}, ""};
						if(ACKed[{seq_num, offset}]) continue;
						string msg = "<" + to_string(seq_num) + " " + to_string(content.size()) + " " + to_string(offset) + " " + to_string(content[offset].size()) + " " + string(dir->d_name) + " >" + content[offset];
						
						packet = {{-1, -1}, ""};
						bzero(ackbuf, sizeof(ackbuf));

						int rlen = sendto(sockfd, msg.c_str(), msg.length(), 0, (struct sockaddr*)&servin, sizeof(servin));
						usleep(100);
						socklen_t servinlen = sizeof(servin);
						recvfrom(sockfd, ackbuf, 1024, MSG_DONTWAIT, (struct sockaddr*)&servin, &servinlen);
						packet = decompose(string(ackbuf));

						if(term_flag){
							fclose(fp);
							close(sockfd);
							exit(0);
						}
						if(packet.second == "ACK") {
							ACKed[{packet.first.first, packet.first.second}] = 1;
							// break;
						}

						
						if(ACKed.find({seq_num, offset}) == ACKed.end()){
							send_failed[{seq_num, offset}] = msg;
						}
					}
				}else{
					std::cout << "Cannot open file\n";
					exit(0);
				}
				fclose(fp);
				seq_num++;
				if(cnt == num_file) break;
			}

			while (!send_failed.empty()){ //cout << "left size = " << send_failed.size() << endl;
				for(auto &f : send_failed){
					string msg = f.second;
					int rlen = sendto(sockfd, msg.c_str(), msg.length(), 0, (struct sockaddr*)&servin, sizeof(servin));
					usleep(100);
					char ackbuf[1024];
					bzero(ackbuf, sizeof(ackbuf));
					socklen_t servinlen = sizeof(servin);
					recvfrom(sockfd, ackbuf, 1024, MSG_DONTWAIT, (struct sockaddr*)&servin, &servinlen);
					pair<pii, string> packet = decompose(string(ackbuf));

					if(packet.second == "ACK") {
						ACKed[{packet.first}] = 1;
						send_failed.erase(f.first);
					}
					if(term_flag){
						close(sockfd);
						exit(0);
					}
				}	
			}
			
		}
	}
	
	// close the descriptor
	close(sockfd);
}