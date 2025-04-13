#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <semaphore.h>
#include <pthread.h>
#include <vector>
#include <sys/stat.h>
#include <sys/sendfile.h> //sendfile()
#include <fcntl.h> //O_CREAT, O_WRONLY, O_RDONLY
#include <algorithm> // all_of(), remove()
#include <ctime> // For timestamp


using namespace std;

#include "server.h"

#define PORT 8080
#define client_message_SIZE 1024
#define MAX_THREADS 10


sem_t mutex;
int thread_count=0;

vector <string> serverData;

string getStr(string sql, char end) {
    int counter=0;
    string retStr="";
    while (sql[counter]!='\0') {
        if (sql[counter]==end) 
            break;
        retStr+=sql[counter];
        counter++;
    }
    return retStr;
}

string findFileExt (string fileEx) {
    for (int i=0;i<fileExtension.size();i++) {
        if (fileExtension[i]==fileEx) 
            return ContentType[i];
    }
    printf("Serving .%s as html\n", fileEx.c_str());
    return "Content-Type: text/html\r\n\r\n";
}

void log_event(const string& event, const string& file, const string& status) {
    FILE* logFile = fopen("server.log", "a");
    if (!logFile) return;

    // Get current timestamp
    time_t now = time(0);
    struct tm* t = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t); // e.g. 2025-04-12 12:15:45

    string logLevel = "[INFO]";

    // Determine log level
    if (status.find("404") != string::npos || status.find("500") != string::npos) {
        logLevel = "[ERROR]";
    } else if (event == "Client connected") {
        logLevel = "[INFO]";
    }

    if (event == "Client connected") {
        fprintf(logFile, "[%s] %s Received request from %s\n", timestamp, logLevel.c_str(), file.c_str());
    } else if (event == "Sent file") {
        fprintf(logFile, "[%s] %s Sent file %s with status %s\n", timestamp, logLevel.c_str(), file.c_str(), status.c_str());
    } else if (event == "File not found") {
        fprintf(logFile, "[%s] %s File not found: %s (404 Not Found)\n", timestamp, logLevel.c_str(), file.c_str());
    } else if (event == "Received POST field") {
        fprintf(logFile, "[%s] %s Received POST data: %s\n", timestamp, logLevel.c_str(), file.c_str());
    } else if (event == "Thread limit reached") {
        fprintf(logFile, "[%s] [ERROR] Thread limit reached: %s\n", timestamp,status.c_str());
    } else {
        // Generic fallback
        fprintf(logFile, "[%s] %s %s (%s)\n", timestamp, logLevel.c_str(), event.c_str(), file.c_str());
    }

    fclose(logFile);
}


void send_message(int fd, string filePath, string headerFile) {
    filePath = "./public" + filePath;

    struct stat stat_buff;

    // Open the file
    int fdimg = open(filePath.c_str(), O_RDONLY);
    if (fdimg < 0) {
        write(fd, Messages[NOT_FOUND].c_str(), Messages[NOT_FOUND].length());
        close(fd);
        perror(("Cannot open file: " + filePath).c_str());
        log_event("File not found", filePath, "404 Not Found");
        return;
    }

    // Get file info
    if (fstat(fdimg, &stat_buff) < 0) {
        perror("fstat failed");
        close(fdimg);
        return;
    }
    string header = Messages[HTTP_HEADER] + headerFile;
    write(fd, header.c_str(), header.length());


    size_t img_size = stat_buff.st_size;
    size_t block_size = stat_buff.st_blksize;
    size_t sent_size = 0;

    // Send file in chunks
    while (img_size > 0) {
        size_t send_bytes = (img_size < block_size) ? img_size : block_size;
        ssize_t done_bytes = sendfile(fd, fdimg, NULL, send_bytes);

        if (done_bytes <= 0) {
            perror("sendfile failed");
            break;
        }

        sent_size += done_bytes;
        img_size -= done_bytes;
    }

    close(fdimg);

    if (sent_size > 0) {
        printf("Sent file: %s\n", filePath.c_str());
        log_event("Sent file", filePath, "200 OK");

    } else {
        printf("No data sent for file: %s\n", filePath.c_str());
        log_event("No data sent",filePath.c_str(), "\t500 Internal Server Error");
    }
}

void getData (string requestType, string client_message) {
    string extract;
    string data = client_message;

    if (requestType=="GET") {
        data.erase(0,getStr(data, ' ').length()+1);
        data = getStr(data, ' ');
        data.erase(0,getStr(data,'?').length()+1);
    }
    else if (requestType=="POST") {
        int counter = data.length()-1;
        while (counter>0) {
            if (data[counter]==' '|| data[counter]=='\n') break;
            counter--;
        }
        data.erase(0, counter+1);
        int found = data.find("=");
        if (found==string::npos) data="";

    }

    while (data.length()>0) {
        extract = getStr(data, '&');
        serverData.clear();
        serverData.push_back(extract);
        data.erase(0,extract.length()+1); 
        for (auto s: serverData) {
            printf("Received POST field: %s\n", s.c_str());
            log_event("Received POST field", s, "200 OK");
        }
    }
}

void * connection_handler (void* socket_desc) {
    int newSock = *((int*)socket_desc);
    char client_message[client_message_SIZE];
    int request=read(newSock, client_message, client_message_SIZE);
    string message(client_message, request);
    
    if (request<0)
        puts("Receive failed.");
    else if (request==0)
        puts("Client disconnected unexpectedly.");
    else {
        string msg = message;
        string requestType = getStr(message, ' ');
        message.erase(0, requestType.length() + 1);
        string requestFile = getStr(message, ' ');

        if (requestFile.length() <= 1) {
            requestFile = "/index.html";
        }

        // Extract file extension properly
        string fileExt = requestFile.substr(requestFile.find_last_of('.') + 1);
        string fileEx = getStr(fileExt, '?'); // removes query params if present

        if (requestType == "GET" || requestType == "POST") {
            getData(requestType, client_message);
            size_t qmark = requestFile.find('?');
            if (qmark != string::npos) {
                requestFile = requestFile.substr(0, qmark);
            }

            string fileExt = requestFile.substr(requestFile.find_last_of('.') + 1);
            string fileEx = getStr(fileExt, '?');

            sem_wait(&mutex);
            send_message(newSock, requestFile, findFileExt(fileEx));
            sem_post(&mutex);
        }
    }
    sleep(5);
    close(newSock);
    sem_wait(&mutex);
    thread_count--;
    printf("Thread finished execution. Thread count: %d\n", thread_count);
    sem_post(&mutex);
    pthread_exit(NULL);
}

int main(int argc, char const *argv[]) {
    sem_init(&mutex, 0, 1);

    int server_socket, client_socket;
    int *thread_socket;
    int randomPORT = PORT;
    struct sockaddr_in server_address, client_address;
    char ip4[INET_ADDRSTRLEN];

    // 1. Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Error in socket");
        exit(EXIT_FAILURE);
    }

    // 2. Bind to a random port in [PORT..PORT+9]
    randomPORT = PORT + (rand() % 10);
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(randomPORT);

    while (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        randomPORT = PORT + (rand() % 10);
        server_address.sin_port = htons(randomPORT);
    }

    // 3. Listen
    if (listen(server_socket, 10) < 0) {
        perror("Error in listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", randomPORT);

    // 4. Accept loop
    while (true) {
        socklen_t len = sizeof(client_address);
        client_socket = accept(server_socket, (struct sockaddr *)&client_address, &len);
        if (client_socket < 0) {
            perror("Unable to accept connection");
            continue;
        }

        // Get client IP
        inet_ntop(AF_INET, &(client_address.sin_addr), ip4, INET_ADDRSTRLEN);
        printf("Connected to %s\n", ip4);
        log_event("Client connected", ip4, "CONNECTED");

        // Thread limit check
        sem_wait(&mutex);
        if (thread_count >= MAX_THREADS) {
            sem_post(&mutex);
            write(client_socket, Messages[BAD_REQUEST].c_str(), Messages[BAD_REQUEST].length());
            printf("Rejected connection: max threads reached (%d)\n", thread_count);
            log_event("Thread limit reached","", "400 Bad Request");
            close(client_socket);
            continue;
        }
        // Increment and spawn
        thread_count++;
        printf("Thread count: %d\n", thread_count);
        sem_post(&mutex);

        // Create worker thread
        pthread_t tid;
        thread_socket = new int(client_socket);
        if (pthread_create(&tid, NULL, connection_handler, (void*)thread_socket) != 0) {
            perror("Could not create thread");
            // Roll back count
            sem_wait(&mutex);
            thread_count--;
            printf("Thread count decremented (create fail): %d\n", thread_count);
            sem_post(&mutex);
            close(client_socket);
            delete thread_socket;
            continue;
        }
        pthread_detach(tid);
    }
    return 0;
}
