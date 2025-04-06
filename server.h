#include <string.h>
#include <string>
#include <vector>
using namespace std;

typedef enum {
    HTTP_HEADER, //file ok
    BAD_REQUEST, //400 
    NOT_FOUND    //404
}messageType;
//changed string (name) [] = {} to vector <string> (name) ={}
vector <string> Messages = {
    "HTTP/1.1 200 OK\r\n",
    "HTTP/1.0 400 Bad Request\r\nContent-Type: text/html\r\n\r\n<!doctype html><html><body>System is busy right now.</body></html> ",
    "HTTP/1.0 404 File not found\r\nContent-Type: text/html\r\n\r\n<!doctype html><html><body>The requested file does not exist on this server.</body></html> "
};

vector <string> fileExtension = {
    "aac","avi", "bmp","css","gif","ico","js","json","mp3","mp4",
    "otf","png","php","rtf","svg","txt","webm","webp","woff","woff",
    "zip","html","htm","jpeg", "jpg"
};

vector <string> ContentType= {
    "Content-Type: audio/aac\r\n\r\n",
    "Content-Type: video/x-msvideo\r\n\r\n",
    "Content-Type: image/bmp\r\n\r\n",
    "Content-Type: text/css\r\n\r\n",
    "Content-Type: image/gif\r\n\r\n",
    "Content-Type: image/vnd.microsoft.icon\r\n\r\n",
    "Content-Type: text/javascript\r\n\r\n",
    "Content-Type: application/json\r\n\r\n",
    "Content-Type: audio/mpeg\r\n\r\n",
    "Content-Type: video/mp4\r\n\r\n",
    "Content-Type: font/otf\r\n\r\n",
    "Content-Type: image/png\r\n\r\n",
    "Content-Type: application/x-httpd-php\r\n\r\n",
    "Content-Type: application/rtf\r\n\r\n",
    "Content-Type: image/svg+xml\r\n\r\n",
    "Content-Type: text/plain\r\n\r\n",
    "Content-Type: video/webm\r\n\r\n",
    "Content-Type: video/webp\r\n\r\n",
    "Content-Type: font/woff\r\n\r\n",
    "Content-Type: font/woff2\r\n\r\n",
    "Content-Type: application/zip\r\n\r\n",
    "Content-Type: text/html\r\n\r\n",
    "Content-Type: text/html\r\n\r\n",
    "Content-Type: image/jpeg\r\n\r\n",
    "Content-Type: image/jpg\r\n\r\n"
};