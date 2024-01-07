#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <fcntl.h>
// #define HTTPS_PORT 443
#define errquit(m) \
    do { \
        perror(m); \
        exit(EXIT_FAILURE); \
    } while (0)

#define BUFSIZE 1024
char buffer[BUFSIZE];
ssize_t read_bytes;
char* decoded_url;

char* url_decode(const char* url) {
    decoded_url = malloc(strlen(url) + 1);
    int j = 0;

    for (int i = 0; url[i] != '\0'; ++i) {
        if (url[i] == '%' && url[i+1] && url[i+2]) {
            int hex;
            sscanf(&url[i + 1], "%2x", &hex);   // read hexadecimal value after %
            decoded_url[j++] = (char)hex;       // hexadecimal value -> character
            i += 2;                             // move to the next encoded character
        } else if (url[i] == '+') {
            decoded_url[j++] = ' ';             // '+' -> space
        } else {
            decoded_url[j++] = url[i];          // copy other characters as is
        }
    }

    decoded_url[j] = '\0';                      // add null to the decoded URL
    return decoded_url;
}


void serve_file(SSL* client_socket, const char *file_path) {
    // struct stat path_stat;
    int fd;
    char *query_string = strchr(file_path, '?');
    if (query_string != NULL&& query_string != file_path) {
        *query_string = '\0';
    }

    char directory[BUFSIZE];
    strcpy(directory, file_path);
    size_t len = strlen(directory);
    char last = directory[len - 1];
    // printf("%c\n", last);
    const char *src = "index.html";
    if(strcmp(directory, "/html/") == 0){
        strcat(directory, src);
    }

    // printf("File path: %s\n", directory);
    // printf("len : %zu\n", len);

    fd = open(directory, O_RDONLY);
    char *dot = strrchr(directory, '.');

    if(fd>0 && dot == NULL && last == '/'){  
    // if(strcmp(m, "music/") == 0||strstr(directory, "/image/") != NULL){  
        // If index.html doesn't exist, return 403 Forbidden
        size_t length = strlen(directory);
		if (length >= 5) {
			memmove(directory, directory+5, length-4);
			directory[length - 4] = '\0';
		}
        char response[] = "HTTP/1.1 403 Forbidden\r\nLocation: ";
        strcat(response, directory);
        strcat(response, "/");
        strcat(response, "\r\n\r\n");
        SSL_write(client_socket, response, strlen(response));        
    } 
    else if (fd>0 && dot == NULL && last != '/'){
    // else if (strstr(directory, "/music") != NULL||strstr(directory, "/image") != NULL){
        // char redirect_path[BUFSIZE];
        // printf("301\n");
        size_t length = strlen(directory);
		if (length >= 5) {
			memmove(directory, directory+5, length-4);
			directory[length - 4] = '\0';
		}
        char response[BUFSIZE] = "HTTP/1.1 301 Moved Permanently\r\nLocation: ";
        strcat(response, directory);
        strcat(response, "/");
        strcat(response, "\r\n\r\n");

        // snprintf(redirect_path, BUFSIZE, "HTTP/1.1 301 Moved Permanently\r\nLocation: %s/\r\n\r\n", directory);
        SSL_write(client_socket, response, strlen(response));
        printf("redirect: %s", response);
    }    
    else if(strstr(directory, "/%") && dot == NULL){
        size_t length = strlen(directory);
		if (length >= 5) {
			memmove(directory, directory+5, length-4);
			directory[length - 4] = '\0';
		}
        char response[] = "HTTP/1.1 403 Forbidden\r\nLocation: ";
        strcat(response, directory);
        strcat(response, "/");
        strcat(response, "\r\n\r\n");
        SSL_write(client_socket, response, strlen(response));       
    }
    else if(strstr(directory, "%")){
        size_t length = strlen(directory);
        if(directory[length-1]=='/'){
            directory[length-1]='\0';
        }
        char* decoded_path = url_decode(directory);
        
        // printf("%s\n", decoded_path);
        memset(directory, 0, BUFSIZE);
        // printf("%s\n", directory);
        strcat(directory, decoded_path);
        // printf("%s\n", directory);
        fd = open(decoded_path, O_RDONLY);
        char *file_extension = strrchr(decoded_path, '.');
      
        char response[] = "HTTP/1.1 200 OK\r\n";
        SSL_write(client_socket, response, strlen(response));
        
        if (fd < 0) {
            char response[] = "HTTP/1.1 404 Not Found\r\n\r\n";
            SSL_write(client_socket, response, strlen(response));
        } else if (file_extension != NULL) {
            if (strcmp(file_extension, ".txt") == 0) {
                char content_type[] = "Content-Type: text/plain; charset=utf-8\r\n\r\n";
                SSL_write(client_socket, content_type, strlen(content_type));
            } else if (strcmp(file_extension, ".html") == 0) {
                char content_type[] = "Content-Type: text/html\r\n\r\n";
                SSL_write(client_socket, content_type, strlen(content_type));
            } else if (strcmp(file_extension, ".css") == 0) {
                char content_type[] = "Content-Type: text/css\r\n\r\n";
                SSL_write(client_socket, content_type, strlen(content_type));
            } else if (strcmp(file_extension, ".jpg") == 0 || strcmp(file_extension, ".jpeg") == 0) {
                char content_type[] = "Content-Type: image/jpeg\r\n\r\n";
                SSL_write(client_socket, content_type, strlen(content_type));
            } else if (strcmp(file_extension, ".png") == 0) {
                char content_type[] = "Content-Type: image/png\r\n\r\n";
                SSL_write(client_socket, content_type, strlen(content_type));
            } else if (strcmp(file_extension, ".gif") == 0) {
                char content_type[] = "Content-Type: image/gif\r\n\r\n";
                SSL_write(client_socket, content_type, strlen(content_type));
            } else if (strcmp(file_extension, ".mp3") == 0) {
                char content_type[] = "Content-Type: audio/mpeg\r\n\r\n";
                SSL_write(client_socket, content_type, strlen(content_type));
            }
        }

        while ((read_bytes = read(fd, buffer, BUFSIZE)) > 0) {
            SSL_write(client_socket, buffer, read_bytes);
        }
        close(fd);
        free(decoded_url);
        decoded_url = NULL;
    } 
    else if(dot != NULL) {    
        char response[] = "HTTP/1.1 200 OK\r\n";
        SSL_write(client_socket, response, strlen(response));

        char *file_extension = strrchr(directory, '.');
        if (file_extension != NULL) {
            if (strcmp(file_extension, ".html") == 0) {
                char content_type[] = "Content-Type: text/html\r\n\r\n";
                SSL_write(client_socket, content_type, strlen(content_type));
            } else if (strcmp(file_extension, ".css") == 0) {
                char content_type[] = "Content-Type: text/css\r\n\r\n";
                SSL_write(client_socket, content_type, strlen(content_type));
            } else if (strcmp(file_extension, ".txt") == 0) {
                char content_type[] = "Content-Type: text/plain; charset=utf-8\r\n\r\n";
                SSL_write(client_socket, content_type, strlen(content_type));
            } else if (strcmp(file_extension, ".jpg") == 0 || strcmp(file_extension, ".jpeg") == 0) {
                char content_type[] = "Content-Type: image/jpeg\r\n\r\n";
                SSL_write(client_socket, content_type, strlen(content_type));
            } else if (strcmp(file_extension, ".png") == 0) {
                char content_type[] = "Content-Type: image/png\r\n\r\n";
                SSL_write(client_socket, content_type, strlen(content_type));
            } else if (strcmp(file_extension, ".gif") == 0) {
                char content_type[] = "Content-Type: image/gif\r\n\r\n";
                SSL_write(client_socket, content_type, strlen(content_type));
            } else if (strcmp(file_extension, ".mp3") == 0) {
                char content_type[] = "Content-Type: audio/mpeg\r\n\r\n";
                SSL_write(client_socket, content_type, strlen(content_type));
            }
        }
        while ((read_bytes = read(fd, buffer, BUFSIZE)) > 0) {
            SSL_write(client_socket, buffer, read_bytes);
        }
        close(fd);
    }
    else{

        char response[] = "HTTP/1.1 404 Not Found\r\n\r\n";
        SSL_write(client_socket, response, strlen(response));
    }
}

int main(int argc, char *argv[]) {
    SSL_CTX *ssl_ctx;
    int server_socket;
    struct sockaddr_in server_addr;

    // SSL initialization
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    // Create SSL context
    ssl_ctx = SSL_CTX_new(TLS_server_method());
    if (!ssl_ctx) {
        fprintf(stderr, "Error creating SSL context\n");
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_certificate_file(ssl_ctx, "../cert/server.crt", SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_use_PrivateKey_file(ssl_ctx, "../cert/server.key", SSL_FILETYPE_PEM) <= 0) {
        fprintf(stderr, "Error loading certificate or private key\n");
        exit(EXIT_FAILURE);
    }

    // int server_socket;
    // struct sockaddr_in server_addr;

    if ((server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        errquit("socket");
    }

    int opt_val = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val)) < 0) {
        errquit("setsockopt");
    }

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(80);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        errquit("bind");
    }

    if (listen(server_socket, SOMAXCONN) < 0) {
        errquit("listen");
    }
    // SSL *ssl;

    while (1) {
        int client_socket;
        struct sockaddr_in client_addr;
        socklen_t client_addrlen = sizeof(client_addr);
        SSL *ssl;
        if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addrlen)) < 0) {
            perror("accept");
            continue;
        }
        ssl = SSL_new(ssl_ctx);
        if (!ssl) {
            fprintf(stderr, "Error creating SSL structure\n");
            close(client_socket);
            continue; // Handle the error and move on to the next connection
        }
        SSL_set_fd(ssl, client_socket);

        if (SSL_accept(ssl) <= 0) {
            perror("SSL_accept");
            SSL_free(ssl);
            close(client_socket);
            continue;
        }
        pid_t pid = fork();

        if(pid<0){
            // perror("fork");
            SSL_free(ssl);
            close(client_socket);
            exit(0);
        }else if(pid==0){
            close(server_socket);
            char request[BUFSIZE];
            ssize_t recv_len = SSL_read(ssl, request, BUFSIZE);
            if (recv_len < 0) {
                perror("recv");
                SSL_free(ssl);
                close(client_socket);
                continue;
            }

            char *http_method = strtok(request, " \t\n");
            char *directory = strtok(NULL, " \t\n");

            if (strcmp(http_method, "GET") == 0) {
                char full_path[BUFSIZE];
                snprintf(full_path, BUFSIZE, "/html%s", directory);
            
                serve_file(ssl, full_path);
            } else {
                char response[] = "HTTP/1.1 501 Not Implemented\r\n\r\n";
                SSL_write(ssl, response, strlen(response));
            }
            SSL_shutdown(ssl);
            SSL_free(ssl);
            close(client_socket);
            exit(0);
        }else{
            SSL_shutdown(ssl);
            SSL_free(ssl);
            close(client_socket);
        }
        
    }
    SSL_CTX_free(ssl_ctx);
    return 0;
}
