/*
 *  Lab problem set for INP course
 *  by Chun-Ying Huang <chuang@cs.nctu.edu.tw>
 *  License: GPLv2
 */
#include <ctype.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <ifaddrs.h>
#include <poll.h>
using namespace std;

#define NIPQUAD(m)	((unsigned char*) &(m))[0], ((unsigned char*) &(m))[1], ((unsigned char*) &(m))[2], ((unsigned char*) &(m))[3]
#define errquit(m)	{ perror(m); exit(-1); }

#define MYADDR		0x0a0000fe
#define ADDRBASE	0x0a00000a
#define	NETMASK		0xffffff00
char TUN_NAME[] = "tun0";

int
tun_alloc(char *dev) {
	struct ifreq ifr;
	int fd, err;
	if((fd = open("/dev/net/tun", O_RDWR)) < 0 )
		return -1;
	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TUN | IFF_NO_PI;	/* IFF_TUN (L3), IFF_TAP (L2), IFF_NO_PI (w/ header) */
	if(dev && dev[0] != '\0') strncpy(ifr.ifr_name, dev, IFNAMSIZ);
	if((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ) {
		close(fd);
		return err;
	}
	if(dev) strcpy(dev, ifr.ifr_name);
	return fd;
}

int
ifreq_set_mtu(int fd, const char *dev, int mtu) {
	struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_mtu = mtu;
	if(dev) strncpy(ifr.ifr_name, dev, IFNAMSIZ);
	return ioctl(fd, SIOCSIFMTU, &ifr);
}

int
ifreq_get_flag(int fd, const char *dev, short *flag) {
	int err;
	struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifr));
	if(dev) strncpy(ifr.ifr_name, dev, IFNAMSIZ);
	err = ioctl(fd, SIOCGIFFLAGS, &ifr);
	if(err == 0) {
		*flag = ifr.ifr_flags;
	}
	return err;
}

int
ifreq_set_flag(int fd, const char *dev, short flag) {
	struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifr));
	if(dev) strncpy(ifr.ifr_name, dev, IFNAMSIZ);
	ifr.ifr_flags = flag;
	return ioctl(fd, SIOCSIFFLAGS, &ifr);
}

int
ifreq_set_sockaddr(int fd, const char *dev, int cmd, unsigned int addr) {
	struct ifreq ifr;
	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = addr;
	memset(&ifr, 0, sizeof(ifr));
	memcpy(&ifr.ifr_addr, &sin, sizeof(struct sockaddr));
	if(dev) strncpy(ifr.ifr_name, dev, IFNAMSIZ);
	return ioctl(fd, cmd, &ifr);
}

int
ifreq_set_addr(int fd, const char *dev, unsigned int addr) {
	return ifreq_set_sockaddr(fd, dev, SIOCSIFADDR, addr);
}

int
ifreq_set_netmask(int fd, const char *dev, unsigned int addr) {
	return ifreq_set_sockaddr(fd, dev, SIOCSIFNETMASK, addr);
}

int
ifreq_set_broadcast(int fd, const char *dev, unsigned int addr) {
	return ifreq_set_sockaddr(fd, dev, SIOCSIFBRDADDR, addr);
}

// int tun_fd_global = -1;
// void cleanup(int signum) {
//     if (tun_fd_global != -1) {
//         close(tun_fd_global);
//         fprintf(stderr, "## [server] tun0 interface closed\n");
//     }
//     exit(signum);
// }

int
tunvpn_server(int port) {
	// XXX: implement your server codes here ...

	// when tvn process is killed tun0 should be killed
	// signal(SIGINT, cleanup); // Handle Ctrl+C
    // signal(SIGTERM, cleanup); // Handle termination signal

	fprintf(stderr, "## [server] starts ...\n");

	int tun_fd = tun_alloc(TUN_NAME); 
	int server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); 

	int enable = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

    ifreq_set_mtu(server_socket, TUN_NAME, 1400);
    ifreq_set_addr(server_socket, TUN_NAME, htonl(MYADDR));
    ifreq_set_netmask(server_socket, TUN_NAME, htonl(NETMASK));
	ifreq_set_broadcast(server_socket, TUN_NAME, htonl(ADDRBASE | ~NETMASK));

    short flag;
    ifreq_get_flag(server_socket, TUN_NAME, &flag);
    flag |= IFF_UP; 
    ifreq_set_flag(server_socket, TUN_NAME, flag);

	char buffer[1400];
    struct sockaddr_in server_addr, client_addr1, client_addr2;
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr1, 0, sizeof(client_addr1));
    memset(&client_addr2, 0, sizeof(client_addr2));
    socklen_t client_len1 = sizeof(client_addr1);
    socklen_t client_len2 = sizeof(client_addr2);

	server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port);

	bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
	listen(server_socket, 1);
	// printf("Server listening on port %d...\n", port);

	int client1 = accept(server_socket, (struct sockaddr *)&client_addr1, &client_len1);
	int client2 = accept(server_socket, (struct sockaddr *)&client_addr2, &client_len2);

	struct pollfd poll_fd[3];
    poll_fd[0].fd = tun_fd;
    poll_fd[0].events = POLLIN;
    poll_fd[1].fd = client1;
    poll_fd[1].events = POLLIN;
    poll_fd[2].fd = client2;
    poll_fd[2].events = POLLIN;

	while(1) {
		int ret = poll(poll_fd, 3, -1);

        if(ret>0){
            if (poll_fd[0].revents & POLLIN) {
                int len = read(tun_fd, buffer, sizeof(buffer));
                if(len>=sizeof(struct iphdr)){
                    struct iphdr *iph = (struct iphdr *)buffer;
                    struct in_addr dest_addr;
                    dest_addr.s_addr = iph->daddr;
                    string dest_addr_str = inet_ntoa(dest_addr);
                    string client_addr_str1 = inet_ntoa(client_addr1.sin_addr);
                    string client_addr_str2 = inet_ntoa(client_addr2.sin_addr);

					size_t lastDotPosdest = dest_addr_str.find_last_of(".");
					size_t lastDotPos1 = client_addr_str1.find_last_of(".");
					size_t lastDotPos2 = client_addr_str1.find_last_of(".");

					string lastdest = dest_addr_str.substr(lastDotPosdest + 1);
					string lastone = client_addr_str1.substr(lastDotPos1 + 1);
					string lasttwo = client_addr_str2.substr(lastDotPos2 + 1);

					int lastNumdest = stoi(lastdest);
					int lastNumber1 = stoi(lastone);
					int lastNumber2 = stoi(lasttwo);

                    if(lastNumber1 == lastNumdest-10){
                        write(client1, buffer, len);
                    } 
					else if(lastNumber2 == lastNumdest-10){
                        write(client2, buffer, len);
                    }
                }
            }

            if (poll_fd[1].revents & POLLIN) {
                int len = read(client1, buffer, sizeof(buffer));

                if (len>=sizeof(struct iphdr)) {
                    struct iphdr *iph = (struct iphdr *)buffer;
                    struct in_addr dest_addr;
                    dest_addr.s_addr = iph->daddr;
                    string dest_addr_str = inet_ntoa(dest_addr);
                    string client_addr_str2 = inet_ntoa(client_addr2.sin_addr);

					size_t lastDotPosdest = dest_addr_str.find_last_of(".");
					size_t lastDotPos2 = client_addr_str2.find_last_of(".");

					string lastdest = dest_addr_str.substr(lastDotPosdest + 1);
					string lasttwo = client_addr_str2.substr(lastDotPos2 + 1);

					int lastNumdest = stoi(lastdest);
					int lastNumber2 = stoi(lasttwo);

                    if(lastNumber2 == lastNumdest-10){
                        write(client2, buffer, len); 
                    } 
					else{
                        write(tun_fd, buffer, len);
                    }
                }
            }
            if (poll_fd[2].revents & POLLIN) {
                int len = read(client2, buffer, sizeof(buffer));

                if(len>=sizeof(struct iphdr)){
                    struct iphdr *iph = (struct iphdr *)buffer;
                    struct in_addr dest_addr;
                    dest_addr.s_addr = iph->daddr;
                    string dest_addr_str = inet_ntoa(dest_addr);
                    string client_addr_str1 = inet_ntoa(client_addr1.sin_addr);

					size_t lastDotPosdest = dest_addr_str.find_last_of(".");
					size_t lastDotPos1 = client_addr_str1.find_last_of(".");
					
					string lastdest = dest_addr_str.substr(lastDotPosdest + 1);
					string lastone = client_addr_str1.substr(lastDotPos1 + 1);
					
					int lastNumdest = stoi(lastdest);
					int lastNumber1 = stoi(lastone);
					
                    if(lastNumber1 == lastNumdest-10){
                        write(client1, buffer, len);
                    } 
					else{
                        write(tun_fd, buffer, len);
                    }
                }
            }
        }
	}
	return 0;
}

int 
connect_to_server(int sockfd, const char *server, int port) {
    struct addrinfo hints, *res, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    char port_str[6];
    snprintf(port_str, sizeof(port_str), "%d", port);

    int status = getaddrinfo(server, port_str, &hints, &res);

    for(p=res; p!=NULL; p=p->ai_next){
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		connect(sockfd, p->ai_addr, p->ai_addrlen);
        break;
    }

    freeaddrinfo(res);
    return sockfd;    
}

int 
get_eth0_addr() {
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];

    getifaddrs(&ifaddr);

    for(ifa=ifaddr; ifa!=nullptr; ifa=ifa->ifa_next){
        if (ifa->ifa_addr == nullptr)
            continue;

        family = ifa->ifa_addr->sa_family;

        if (family==AF_INET || family==AF_INET6) {
            s = getnameinfo(ifa->ifa_addr, 
							(family==AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
                            host, NI_MAXHOST,
                            nullptr, 0, NI_NUMERICHOST);

            if (s!=0) {
                cout << "getnameinfo() failed: " << gai_strerror(s) << endl;
                continue;
            }

            if (strcmp(ifa->ifa_name, "eth0") == 0) {
                string host_str = host;
				size_t lastDotPos = host_str.find_last_of(".");
				string lastPart = host_str.substr(lastDotPos + 1);
				int lastNumber = stoi(lastPart);
				return lastNumber;
            }
        }
    }

    freeifaddrs(ifaddr);
    return 0;
}

int
tunvpn_client(const char *server, int port) {
	// XXX: implement your client codes here ...
	// 連到server(用eth0 tcp)收到assign ip
	// 設定tun0
	// tun/udp 互傳
	fprintf(stderr, "## [client] starts ...\n");

	int tun_fd = tun_alloc(TUN_NAME);
	int socketfd = connect_to_server(socketfd, server, port);
	
	ifreq_set_mtu(socketfd, TUN_NAME, 1400);

	short flag;
	ifreq_get_flag(socketfd, TUN_NAME, &flag);
    flag |= IFF_UP; 
    ifreq_set_flag(socketfd, TUN_NAME, flag);

	int offset = get_eth0_addr();

	ifreq_set_addr(socketfd, TUN_NAME, htonl(ADDRBASE+offset));
	ifreq_set_netmask(socketfd, TUN_NAME, htonl(NETMASK));
	ifreq_set_broadcast(socketfd, TUN_NAME, htonl(ADDRBASE | ~NETMASK));

	struct pollfd poll_fd[2];
    poll_fd[0].fd = tun_fd;
    poll_fd[0].events = POLLIN;
    poll_fd[1].fd = socketfd;
    poll_fd[1].events = POLLIN;

    char buffer[1400];
    while (1) {
        int ret = poll(poll_fd, 2, -1);

        if(poll_fd[0].revents & POLLIN){
            int len = read(tun_fd, buffer, sizeof(buffer));
            write(socketfd, buffer, len);
        }

        if(poll_fd[1].revents & POLLIN){
            int len = read(socketfd, buffer, sizeof(buffer));
            write(tun_fd, buffer, len); 
        }
    }
	return 0;
}

int
usage(const char *progname) {
	fprintf(stderr, "usage: %s {server|client} {options ...}\n"
		"# server mode:\n"
		"	%s server port\n"
		"# client mode:\n"
		"	%s client servername serverport\n",
		progname, progname, progname);
	return -1;
}

int main(int argc, char *argv[]) {
	if(argc < 3) {
		return usage(argv[0]);
	}
	if(strcmp(argv[1], "server") == 0) {
		if(argc < 3) return usage(argv[0]);
		return tunvpn_server(strtol(argv[2], NULL, 0));
	} else if(strcmp(argv[1], "client") == 0) {
		if(argc < 4) return usage(argv[0]);
		return tunvpn_client(argv[2], strtol(argv[3], NULL, 0));
	} else {
		fprintf(stderr , "## unknown mode %s\n", argv[1]);
	}
	return 0;
}
