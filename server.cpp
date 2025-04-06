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

using namespace std;

#include "server.h"

#define PORT 8080
#define client_message_SIZE 1024

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

// void (send_message) (int fd, string filePath, string headerFile) {
//     string header = Messages[HTTP_HEADER]+headerFile;
//     filePath = "/public"+filePath;
//     struct stat stat_buff; //holds info about our file to send

//     write(fd,header.c_str(), header.length());
//     int fdimg = open(filePath.c_str(), 0_RDONLY);

//     if (fdimg<0) {
//         printf("Cannot open file pat %s\n", filePath.c_str());
//         return;
//     }

//     //changed
//     else{
//         fstat(fdimg,&stat_buff);
//         int img_size = stat_buff.st_size;
//         int block_size = stat_buff.st_blksize;

//         size_t sent_size;
//         while (img_size > 0) {
//             int send_bytes = (img_size<block_size)?img_size:block_size;
//             int done_bytes =  sendfile(fd,fdimg,NULL,send_bytes);
//             img_size = img_size - done_bytes;
//         }
//         if (sent_size >=0) 
//             printf("Sent file: %s\n",filePath.c_str());
//         close(fdimg);
//     }
// }

void send_message(int fd, string filePath, string headerFile) {
    string header = Messages[HTTP_HEADER] + headerFile;
    filePath = "./public" + filePath;

    struct stat stat_buff;

    // Send HTTP header first
    write(fd, header.c_str(), header.length());

    // Open the file
    int fdimg = open(filePath.c_str(), O_RDONLY);
    if (fdimg < 0) {
        perror(("Cannot open file: " + filePath).c_str());
        return;
    }

    // Get file info
    if (fstat(fdimg, &stat_buff) < 0) {
        perror("fstat failed");
        close(fdimg);
        return;
    }

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
        printf("Sent %zu bytes from file: %s\n", sent_size, filePath.c_str());
    } else {
        printf("No data sent for file: %s\n", filePath.c_str());
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

    int found = client_message.find("cookie");
    if (found != string::npos) {
        client_message.erase(0,found+8); //erase upto space after cookie
        client_message = getStr(client_message, ' ');
        data = data+"&"+getStr(client_message, '\n');
    }

    while (data.length()>0) {
        extract = getStr(data, '&');
        serverData.push_back(extract);
        data.erase(0,extract.length()+1); // changed

    }
}

void * connection_handler (void* socket_desc) {
    int newSock = *((int*)socket_desc);
    char client_message[client_message_SIZE];
    int request=read(newSock, client_message, client_message_SIZE);
    string message(client_message, request);
    // string message = client_message;
    sem_wait(&mutex);
    thread_count++;
    printf("Thread counter %d\n", thread_count);
    if (thread_count>20) {
        write(newSock, Messages[BAD_REQUEST].c_str(), Messages[BAD_REQUEST].length());
        thread_count--;
        close(newSock);
        sem_post(&mutex);
        pthread_exit(NULL);
    }
    sem_post(&mutex);

    if (request<0)
        puts("Receive failed.");
    else if (request==0)
        puts("Client disconnected unexpectedly.");
    else {
        // string msg = client_message;
        string msg = message;
        int found = msg.find("multipart/form-data"); //checking if it is file upload request
        if (found!=string::npos) {
            found = msg.find("Content-Length:"); //: removed
            msg.erase(0,found+16);
            int length = stoi(getStr(msg, ' '));
            
            // found = msg.find("Content-Length");
            // if (found == string::npos) {
            //     printf("Content-Length header not found.\n");
            //     close(newSock);
            //     pthread_exit(NULL);
            // }
            // msg.erase(0, found + 15); // `Content-Length:` is 15 chars (not 16)

            // string lenStr = getStr(msg, '\n'); // till newline
            // lenStr.erase(remove(lenStr.begin(), lenStr.end(), '\r'), lenStr.end()); // strip \r

            // lenStr.erase(0, getStr(lenStr,' ').length());  // remove leading spaces
            // lenStr.erase(getStr(lenStr," \r\n").length() + 1); // remove trailing spaces and CR/LF



            // if (lenStr.empty() || !all_of(lenStr.begin(), lenStr.end(), ::isdigit)) {
            //     printf("Invalid Content-Length: '%s'\n", lenStr.c_str());
            //     close(newSock);
            //     pthread_exit(NULL);
            // }

            // int length = stoi(lenStr);

            found = msg.find("filename="); //= removed
            msg.erase(0,found+10);
            string new_file = getStr(msg, '"');
            new_file = "./public/downloads/"+new_file;
            found = msg.find("Content-Type:"); //removed :
            msg.erase(0,found+15);
            msg.erase(0, getStr(msg, '\n').length()+3); //\n\r\n\r

            char client_msg[client_message_SIZE];
            int fd, req, rec, counter=0;
            if ((fd = open(new_file.c_str(), O_CREAT | O_WRONLY, S_IRWXU))<0) {
                perror("Cannot open filepath");
            }
            write(fd, msg.c_str(), client_message_SIZE);
            
            // length-=msg.length();
            printf("Filesize: %d\n", length);

            while (length>0) {
                req = read(newSock, client_msg, client_message_SIZE);
                if ((rec = write(fd, client_msg, req)) < 0) {
                    perror("Write failed: ");
                    return NULL;
                }
                length -= req;
                counter += req;
                printf("Remaining size: %d Received size: %d Total size received: %d\n", length, req, counter); //swap counter, req
                if (req < 1000) break; // end of file
            }
            if ((rec=close(fd)) < 0) {
                perror("Close failed");
                return NULL;
            }
        }
        // printf("Client message: %s\n", client_message);
        // string requestType = getStr(message, ' ');
        // message.erase(0,requestType.length()+1);
        // string requestFile = getStr(message, ' ');

        // string requestF = requestFile;
        // string fileExt = requestF.erase(0,getStr(requestF,'.').length()+1);
        // string fileEx = getStr(getStr(fileExt,'/'),'?');
        // requestFile = getStr(requestFile,'.')+"."+fileEx;
        
        // if (requestType == "GET" || requestType =="POST") {
        //     if (requestFile.length() <=1) {
        //         requestFile = "/index.html";
        //     }
        //     if (fileEx == "php") {
        //         //do nothing
        //         getData(requestType, client_message);
        //     }
        //     sem_wait(&mutex);
        //     send_message(newSock, requestFile, findFileExt(fileEx));
        //     sem_post(&mutex);
        // }
        string requestType = getStr(message, ' ');
        message.erase(0, requestType.length() + 1);
        string requestFile = getStr(message, ' ');

        // Default to index.html if root is requested
        if (requestFile == "/") {
            requestFile = "/index.html";
        }

        // Extract file extension properly
        string fileExt = requestFile.substr(requestFile.find_last_of('.') + 1);
        string fileEx = getStr(fileExt, '?'); // removes query params if present

        if (requestType == "GET" || requestType == "POST") {
            // if (fileEx == "php") {
            //     getData(requestType, client_message);
            // }
            // else {
            serverData.clear();
                getData(requestType, client_message);
                for (auto s: serverData) {
                    printf("Received POST field: %s\n", s.c_str());
                }
            // }

            sem_wait(&mutex);
            send_message(newSock, requestFile, findFileExt(fileEx));
            sem_post(&mutex);
        }

    }
    printf("\n*****Exiting Server*****\n");
    sleep(2);
    close(newSock);
    sem_wait(&mutex);
    thread_count--;
    sem_post(&mutex);
    pthread_exit(NULL);
}

int main (int argc, char const *argv[]) {
    sem_init(&mutex,0,1);
    int server_socket, client_socket, *thread_socket;
    int randomPORT = PORT;
    struct sockaddr_in server_address, client_address;
    char ip4[INET_ADDRSTRLEN];

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket<0) {
        perror("Error in socket: ");
        exit(EXIT_FAILURE);
    }

    randomPORT = 8080 + (rand()%10);
    memset(&server_address,0,sizeof server_address);
    server_address.sin_family=AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(randomPORT);
    while (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address))<0) {
        randomPORT = 8080 + (rand()%10);
        server_address.sin_port = htons(randomPORT);
    }
    if (listen(server_socket,10) < 0) {
        perror("Error in listen: ");
        exit(EXIT_FAILURE);
    }

    while(1) {
        socklen_t len = sizeof(client_address);
        printf("Listening port: %d \n", randomPORT);
        client_socket = accept(server_socket, (struct sockaddr *)&client_address, &len );
        if (client_socket<0) {
            perror("Unable to accept connection: ");
            return 0;
        }
        else {
            inet_ntop(AF_INET, &(client_address.sin_addr), ip4, INET_ADDRSTRLEN);
            printf("Connected to %s \n", ip4);
        }
        pthread_t multi_thread;
        thread_socket = new int();
        *thread_socket = client_socket;

        if (pthread_create(&multi_thread, NULL, connection_handler, (void*)thread_socket)!=0) {
            perror("Could not create thread");
            return 0;
        }
    }
}   