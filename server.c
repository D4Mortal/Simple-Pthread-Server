#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>

#define HTTP404 "HTTP/1.0 404"
#define NOTFOUND "<html><body><h1>404 Not found</h1></body></html>"
#define HTML "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\
Connection: Closed\r\nContent-Length: "

#define JPG "HTTP/1.0 200 OK\r\nContent-Type: image/jpeg\r\n\
Connection: Closed\r\nContent-Length: "

#define CSS "HTTP/1.0 200 OK\r\nContent-Type: text/css\r\n\
Connection: Closed\r\nContent-Length: "

#define JAVASCRIPT "HTTP/1.0 200 OK\r\nContent-Type: text/javascript\r\n\
Connection: Closed\r\nContent-Length: "

#define MAX_PATH_LENGTH 200

pthread_mutex_t lock;
char root[MAX_PATH_LENGTH];

/*****************************************************************************/

/* reads through the requested file and sends it to the client
*/

int sendFile(int socket, FILE *file){
    char fileBuffer[8192];

    if (file == NULL){
        return 0;
    }

    memset(fileBuffer, 0, sizeof(fileBuffer));

    fseek(file, 0, SEEK_SET);
    while(!feof(file)){
        fread(fileBuffer, 1, sizeof(fileBuffer), file);
        write(socket, fileBuffer, sizeof(fileBuffer));
        memset(fileBuffer, 0, sizeof(fileBuffer));
    }
    return 1;
}

/*****************************************************************************/

/* identify the file type in the HTTP request
*/

int get_file_type(char* content){

    if(strstr(content, ".html") != NULL) {
        return 1;
    }
    if(strstr(content, ".jpg") != NULL) {
        return 2;
    }
    if(strstr(content, ".css") != NULL) {
        return 3;
    }
    if(strstr(content, ".js") != NULL) {
        return 4;
    }

    return 0;
}

/*****************************************************************************/

/* Retrieve the requested file path from the HTTP request
*/

char* get_file_path(char* content){
    char *file_path;
    int counter = 0, terminate = 0, i;

    file_path = malloc(sizeof(char) * 100);
    memset(file_path, 0, 100);


    for (i = 0; content[i] != '\0'; i++){
        if (terminate && content[i] == ' '){
            break;
        }
        if (content[i] == '/' || terminate){
            file_path[counter] = content[i];
            terminate = 1;
            counter++;
        }
    }
    return file_path;
}



/*****************************************************************************/

/* A pthread function that's in charge of sending the appropriate response
   to the client
*/
void* pthread_connection_thread(void *socket_info){
    /* once we've stored the client id, it's safe to unlock
    */
    int socket = *(int*)socket_info;
    pthread_mutex_unlock(&lock);

    char recieveBuffer[8192], temp[100], sendBuffer[2049];
    char *file_path;
    int recieve_size, file_size, file_type;
    FILE *requestedFile;

    memset(temp, 0, sizeof(temp));
    memset(recieveBuffer, 0, sizeof(recieveBuffer));
    memset(sendBuffer, 0, sizeof(sendBuffer));

    /* read data from client
    */
    recieve_size = read(socket,recieveBuffer,8192);

	if (recieve_size < 0)
	{
		perror("ERROR reading from socket");
		exit(1);
    }

    /* retrieve the file path and type from the request
    */
    file_path = get_file_path(recieveBuffer);
    file_type = get_file_type(recieveBuffer);
    strcat(root, file_path);
    requestedFile = fopen(root, "rb");

    /* If the file doesn't exist, return 404 message
    */
    if (requestedFile == NULL){
        write(socket, HTTP404, sizeof(HTTP404));
        write(socket, "\r\n", 2);
        write(socket, "\r\n", 2);
        write(socket, NOTFOUND, sizeof(NOTFOUND));
    }

    /* If the file requested is valid, respond with the appropriate headers
       and file info
    */
    else{
        /* bring the pointer to the end of the file, to find the file size
        */
        fseek(requestedFile, 0, SEEK_END);
        file_size = ftell(requestedFile);
        sprintf(temp, "%d", file_size);

        /* copy the appropriate headers into the send buffer
        */
        switch(file_type){
            case 1:
                memcpy(sendBuffer, HTML, sizeof(HTML));
                break;
            case 2:
                memcpy(sendBuffer, JPG, sizeof(JPG));
                break;
            case 3:
                memcpy(sendBuffer, CSS, sizeof(CSS));
                break;
            case 4:
                memcpy(sendBuffer, JAVASCRIPT, sizeof(JAVASCRIPT));
                break;
            default:
                break;
        }

        /* add content length info to the header
        */
        strcat(sendBuffer, temp);
        write(socket, sendBuffer, sizeof(char) * strlen(sendBuffer));
        write(socket, "\r\n", 2);
        write(socket, "\r\n", 2);

        sendFile(socket, requestedFile);
        memset(sendBuffer, 0, sizeof(sendBuffer));
        fclose(requestedFile);
    }

    /* close the connection once the work is done
    */
    close(socket);
    free(file_path);
    return 0;
}


/*****************************************************************************/

/* Main function body
*/
int main(int argc, char** argv){
    int port_num = atoi(argv[1]);
    int sockfd, newsockfd, temp_socket;
    struct sockaddr_in serv_addr;
    pthread_t client_thread;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    memset(&serv_addr, 0, sizeof(serv_addr));

    /* Initialize pthread_mutex_t
    */
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        perror("mutex init failed");
        exit(1);
    }

    /* Create TCP socket
    */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0){
		perror("ERROR opening socket");
		exit(1);
	}

    serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port_num);  // store in machine-neutral format

	 /* Bind address to the socket
     */
	if (bind(sockfd, (struct sockaddr *) &serv_addr,
			sizeof(serv_addr)) < 0){
		perror("ERROR on binding");
		exit(1);
	}

    listen(sockfd, 5);

    while(1){
        memset(root, 0, sizeof(char) * 200);
        strcpy(root, argv[2]);

        /* Accept a connection - block until a connection is ready to
    	   be accepted. Get back a new file descriptor to communicate on.
        */
        newsockfd = accept(sockfd, (struct sockaddr *) NULL, NULL);
        if (newsockfd < 0) {
            perror("unable to accept connection");
            continue;
        }

        /* lock so that the client id doesn't get over written by the next connection
        */
        pthread_mutex_lock(&lock);
        temp_socket = newsockfd;

        /* create threads to handle the connection
        */
        if( pthread_create( &client_thread, &attr, pthread_connection_thread,
            (void*) &temp_socket) < 0){
            perror("could not create thread");
            pthread_mutex_unlock(&lock);
            pthread_exit(NULL);
            }
    }

    close(sockfd);
    pthread_exit(NULL);
}
