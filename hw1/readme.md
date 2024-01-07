tar tjf (解壓縮的檔案.tbz) ---extract

curl http://localhost:10801/        ---有加密
curl -k https://localhost:10841/    ---沒加密(-k會不管憑證)

 if(strstr(directory, "/%")){
    //     char* decoded_path = url_decode(directory);
    //     // printf("%s\n", decoded_path);
    //     memset(directory, 0, BUFSIZE);
    //     // printf("%s\n", directory);
    //     strcat(directory, decoded_path);
    //     // printf("%s\n", directory);
    //     char *file_extension = strrchr(decoded_path, '.');
      
    //     char response[] = "HTTP/1.1 200 OK\r\n";
    //     send(client_socket, response, strlen(response), 0);
    //     fd = open(decoded_path, O_RDONLY);
    //     if (fd < 0) {
    //         char response[] = "HTTP/1.1 404 Not Found\r\n\r\n";
    //         send(client_socket, response, strlen(response), 0);
    //     } else if (file_extension != NULL) {
    //         if (strcmp(file_extension, ".html") == 0) {
    //             char content_type[] = "Content-Type: text/html\r\n\r\n";
    //             send(client_socket, content_type, strlen(content_type), 0);
    //         } else if (strcmp(file_extension, ".css") == 0) {
    //             char content_type[] = "Content-Type: text/css\r\n\r\n";
    //             send(client_socket, content_type, strlen(content_type), 0);
    //         } else if (strcmp(file_extension, ".txt") == 0) {
    //             char content_type[] = "Content-Type: text/plain\r\n\r\n";
    //             send(client_socket, content_type, strlen(content_type), 0);
    //         } else if (strcmp(file_extension, ".jpg") == 0 || strcmp(file_extension, ".jpeg") == 0) {
    //             char content_type[] = "Content-Type: image/jpeg\r\n\r\n";
    //             send(client_socket, content_type, strlen(content_type), 0);
    //         } else if (strcmp(file_extension, ".png") == 0) {
    //             char content_type[] = "Content-Type: image/png\r\n\r\n";
    //             send(client_socket, content_type, strlen(content_type), 0);
    //         } else if (strcmp(file_extension, ".gif") == 0) {
    //             char content_type[] = "Content-Type: image/gif\r\n\r\n";
    //             send(client_socket, content_type, strlen(content_type), 0);
    //         } else if (strcmp(file_extension, ".mp3") == 0) {
    //             char content_type[] = "Content-Type: audio/mpeg\r\n\r\n";
    //             send(client_socket, content_type, strlen(content_type), 0);
    //         }
    //     }
    // }
    
    // if (path_len > 0 && file_path[path_len-1] != '/') {
    //     // Redirect to the same path with a trailing slash
    //     char redirect_path[BUFSIZE];
    //     snprintf(redirect_path, BUFSIZE, "HTTP/1.1 301 Moved Permanently\r\nLocation: %s/\r\n\r\n", file_path);
    //     send(client_socket, redirect_path, strlen(redirect_path), 0);
    //     return;
    // }
    // if (strcmp(http_method, "GET") != 0) {
    //     // Unsupported method, return 501 Not Implemented
    //     char response[] = "HTTP/1.1 501 Not Implemented\r\n\r\n";
    //     send(client_socket, response, strlen(response), 0);
    //     return;
    // }
    // else if (strstr(directory, ".") == NULL){
    //     if(directory[len - 1] == '/'){
    //         char response[] = "HTTP/1.1 403 Forbidden\r\n\r\n";
    //         send(client_socket, response, strlen(response), 0);
    //     } 
        // else if (directory[len] != '/'){
        //     // char redirect_path[BUFSIZE];
        //     printf("301\n");
        //     char response[BUFSIZE] = "HTTP/1.1 301 Moved Permanently\r\nLocation: ";
        //     strcat(response, directory);
        //     strcat(response, "/");
        //     strcat(response, "\r\n\r\n");
        //     // snprintf(redirect_path, BUFSIZE, "HTTP/1.1 301 Moved Permanently\r\nLocation: %s/\r\n\r\n", directory);
        //     send(client_socket, response, strlen(response), 0);
        //     printf("redirect: %s", response);
        // }

http(不管加密)
https(有加密)

demo的server放在demo的目錄

server.c 10802 <-> port80

10842 bonus+20 如果可以支援https

restart the 3 docker -> 才不會連不到server
docker-compose stop     # stop all containers
docker-compose rm       # remove all containers
docker-compose up -d --build

To test implementation :
    docker exec -it sw_tester /testcase/run.sh lighttpd
    docker exec -it sw_tester /testcase/run.sh demo 

501 error message

docker根目錄有html會寫有所有想要的網頁
GET去下載靜態內容

處理字串: fgets()   --- get line per time
FILE *fp;
fp = fdopen(c, "r+"); ---c is socket 包裝成file pointer

setvbuf(fp, NULL, _IONBF, 0)  ---set buffer mode(不做buffer可以比較即時)

while(fgets(buf, sizeof(buf), fp) != null){
    ...
    recv line by line
}

fprintf(fp, ...)
...
fclose(fp)