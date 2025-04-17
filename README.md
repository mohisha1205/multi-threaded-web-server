# Multithreaded Web Server in C++

##  Introduction

This project is a lightweight multithreaded server built in C++ that handles multiple clients concurrently, serves static files, and logs request details efficiently. It's designed for simplicity, scalability, and educational clarity.


## Key Concepts

### Server
A **server** is a system (hardware or software) that listens for and responds to requests from **clients**, like web browsers. In the context of the web, a server receives **HTTP requests** and returns resources such as HTML files, CSS, images, or data.

For example, when you visit a website, your browser (the client) sends an HTTP request to a server, which replies with the content of the webpage.


### Multithreading

**Multithreading** allows a program to perform multiple tasks concurrently using separate threads. In a web server, multithreading enables handling multiple client requests **at the same time**.

Without multithreading, a server could serve only one client at a time, leading to poor performance or timeouts. This project uses **POSIX threads (pthreads)** to implement multithreading so that each client is handled in its own thread, improving scalability and responsiveness.


## Project Overview


This project is a simple yet educational multithreaded web server implemented in C++, showcasing core systems programming concepts. It includes:

- **Socket Programming** – Establishes a server that listens on a port and accepts incoming client connections.
- **Multithreading** – Handles multiple client requests concurrently using POSIX threads for improved responsiveness.
- **HTTP Request Handling** – Parses and processes both `GET` and `POST` HTTP requests, serving appropriate content.
- **Static File Serving** – Delivers static resources like HTML, CSS, and image files from a designated `./public` directory.
- **Basic Logging** – Records each request with method, client IP, status code, and timestamp for debugging and analytics.
- **Thread Limit Enforcement** – Limits the number of concurrent threads (default: 10) to prevent server overload.


## How It Works

1. The server listens on a **random port** between `8080` and `8089` to avoid conflicts with other services.
2. When a **client connects**, the server logs the client's IP address and spawns a new thread to handle the request.
3. The thread **reads and parses** the incoming HTTP request (either `GET` or `POST`).
4. Based on the request type:
   - If it's a `GET` request, the server serves the corresponding static file from the `./public/` directory.
   - If it's a `POST` request, the server logs the submitted form data.
   - If the request is invalid or cannot be processed, the server responds with an error message (e.g., `404 Not Found` or `400 Bad Request`).
5. After processing the request, the thread **sends the appropriate response** back to the client and closes the connection.
6. All interactions, including requests, responses, and errors, are **logged** in `server.log` for debugging and analytics.
7. The server **manages thread count** to ensure it doesn't exceed the `MAX_THREADS` limit (default: 10), preventing server overload.


## Core Concepts

### Socket Programming

The server uses the **Berkeley sockets API** to create TCP connections:
- `socket()`, `bind()`, `listen()`, and `accept()` are used to set up a server.
- Data is received using `read()` and sent using `write()` or `sendfile()`.

### POSIX Threads

Each client connection is managed by a separate thread using:
- `pthread_create()` to launch a thread
- `pthread_detach()` to release resources once done
- A `semaphore` is used to safely manage `thread_count`

### Logging

Events like connections, requests, errors, and responses are written to `server.log` in this format:

```
[YYYY-MM-DD HH:MM:SS] [INFO] <informational message>
[YYYY-MM-DD HH:MM:SS] [ERROR] <error message>
```


## Example Output

### Client `GET` Request
```
GET /index.html HTTP/1.1 
Host: localhost:8083
```
**Server Output**
```
Sent file: ./public/index.html
```

### Client `POST` Request
```
POST /index.html HTTP/1.1
Host: localhost:8083
Content-Type: application/x-www-form-urlencoded
Content-Length: 132

Name=Mohisha&People=2&date=2025-04-17T20%3A00&Message=No+onions+please
```

**Server Output**
```
Thread count: 1
Received POST field: Name=Mohisha
Received POST field: People=2
Received POST field: date=2025-04-17T20%3A00
Received POST field: Message=No+onions+please
```



