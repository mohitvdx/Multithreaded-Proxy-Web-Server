
---

**Proxy Parse Project Documentation**

---

### Overview

This project implements a basic HTTP proxy server capable of caching web responses and handling multiple client requests concurrently. It supports various networking tasks, including request parsing, connection management, and caching.

#### Key Features
- **Request Parsing**: Handles parsing of incoming HTTP requests.
- **Concurrent Handling**: Utilizes threads to manage multiple client requests simultaneously.
- **Caching**: Implements a caching mechanism to store and retrieve previous responses, reducing latency and network traffic.
- **Error Handling**: Provides robust error handling and logging for server operations.

---

### Project Structure
```
.
├── proxy_parse.h                # Header file containing declarations for proxy functions
├── proxy_parse.c                # Implementation of proxy server functionality
├── proxy_server_with_cache.c    # Main proxy server code with caching logic
├── Makefile                     # Build script for compiling and linking the project
└── README.md                    # Project documentation (this file)
```

---

### Functions

#### `parse_request(int client_fd)`
- **Description**: Parses an HTTP request from the client and forwards it to the appropriate handler.
- **Arguments**: 
  - `client_fd`: The file descriptor for the client's socket.
- **Returns**: None

---

#### `cache_request(char *url, char *data, int len)`
- **Description**: Caches a response for a given URL.
- **Arguments**: 
  - `url`: The URL of the request to cache.
  - `data`: The response data to store.
  - `len`: The length of the data.
- **Returns**: None

---

#### `get_cached_response(char *url)`
- **Description**: Retrieves the cached response for a given URL.
- **Arguments**: 
  - `url`: The URL for which to retrieve the cached response.
- **Returns**: A pointer to the cached response data, or `NULL` if no cached response is available.

---

#### `handle_client_request(int client_fd)`
- **Description**: Manages the process of receiving, parsing, and responding to a client's request.
- **Arguments**: 
  - `client_fd`: The file descriptor for the client's socket.
- **Returns**: None

---

### Constants

- `MAX_BYTES`: Maximum allowed size of request or response (default: 4096 bytes).
- `MAX_CLIENTS`: Maximum number of client requests that can be served concurrently (default: 400).
- `MAX_SIZE`: Maximum size of the cache (default: 200 MB).
- `MAX_ELEMENT_SIZE`: Maximum size of an individual cache element (default: 10 MB).

---

### Data Structures

#### `cache_element`
A structure to represent a cached response.

- **Fields**:
  - `data`: A pointer to the cached response data.
  - `len`: The length of the cached data.
  - `url`: The URL associated with the cached response.

```c
struct cache_element {
    char *data;
    int len;
    char *url;
};
```

---

### Dependencies
- **Standard Libraries**: `stdio.h`, `stdlib.h`, `string.h`, `unistd.h`, `time.h`, etc.
- **Networking Libraries**: `sys/socket.h`, `netinet/in.h`, `arpa/inet.h`.

---

### Compilation and Build

To compile the proxy server, use the provided `Makefile` by running the following command:
```
make
```
This will compile the project and generate the executable `proxy` along with the necessary object files.

#### Makefile Details

The `Makefile` includes the following targets:

##### `all`
This is the default target, which builds the proxy server executable.

```makefile
CC=g++
CFLAGS= -g -Wall

all: proxy

proxy: proxy_server_with_cache.c
	$(CC) $(CFLAGS) -o proxy_parse.o -c proxy_parse.c -lpthread
	$(CC) $(CFLAGS) -o proxy.o -c proxy_server_with_cache.c -lpthread
	$(CC) $(CFLAGS) -o proxy proxy_parse.o proxy.o -lpthread
```

##### `clean`
Removes all object files and the compiled executable.

```makefile
clean:
	rm -f proxy *.o
```

##### `tar`
Creates a tarball archive of the source code, including the proxy server files, `README`, `Makefile`, and header/source files.

```makefile
tar:
	tar -cvzf ass1.tgz proxy_server_with_cache.c README Makefile proxy_parse.c proxy_parse.h
```

---

### Usage

1. **Start the Proxy Server**: Run the compiled binary to start the proxy server.
   ```
   ./proxy
   ```

2. **Connect Clients**: Clients can connect to the proxy server to request cached data or fresh responses.

3. **Manage Cache**: The server will cache the responses to improve performance for future requests to the same URLs.

---

### Error Handling

- The proxy server logs errors such as connection failures, request parsing issues, and cache management problems.
- Proper error codes are returned for failed operations, including HTTP status codes for invalid requests.

---

### Contributing

If you'd like to contribute to this project, feel free to fork the repository and submit a pull request. Please ensure to write tests and document your changes.

---

### License

This project is licensed under the MIT License.

---
