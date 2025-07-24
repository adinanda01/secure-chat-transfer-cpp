# Secure Multithreaded Chat and File Transfer System

This project is a C++ TCP-based client-server system supporting secure private messaging and file transfers with built-in SHA256 integrity validation. It is developed using **socket programming**, **multithreading**, and **OpenSSL**, incorporating foundational concepts from **Operating Systems** and **Computer Networks**.

---

## Features

* **Multithreaded Server:** Handles multiple client connections concurrently using `std::thread`.
* **Secure Authentication:** User registration and login with SHA256-hashed password verification.
* **Private Messaging:** Send direct messages using `/msg <username> <message>`.
* **File Transfer Support:** Transfer files to other users via `/sendfile <username> <filename>`.
* **Integrity Check:** Every file transfer includes SHA256 checksum validation.
* **Custom Deadlock-Free Protocol:** Optimized pipelined approach instead of traditional ACK-based handshakes.
* **Command-Line Interface:** Simple and intuitive terminal-based interaction for both client and server.

---

## Build Instructions

### Prerequisites

* A C++11-compatible compiler (e.g., GCC or Clang)
* OpenSSL development libraries

#### On Ubuntu/Debian

```bash
sudo apt update
sudo apt install build-essential libssl-dev
```

#### On macOS (with Homebrew)

```bash
brew install openssl
```

---

### Compilation

#### Linux

```bash
# Compile Server
g++ server/main.cpp server/server_utils.cpp -o server/server -lpthread -lssl -lcrypto

# Compile Client
g++ client/main.cpp -o client/client -lpthread -lssl -lcrypto
```

#### macOS

```bash
# Compile Server (macOS)
g++ server/main.cpp server/server_utils.cpp -o server/server -pthread \
-I$(brew --prefix openssl)/include \
-L$(brew --prefix openssl)/lib \
-lssl -lcrypto

# Compile Client (macOS)
g++ client/main.cpp -o client/client -pthread \
-I$(brew --prefix openssl)/include \
-L$(brew --prefix openssl)/lib \
-lssl -lcrypto
```

#### Windows (using MSYS2 or MinGW with OpenSSL)

```bash
# Example using MSYS2 (assumes OpenSSL installed via pacman)
g++ server/main.cpp server/server_utils.cpp -o server.exe -lws2_32 -lssl -lcrypto -lpthread
g++ client/main.cpp -o client.exe -lws2_32 -lssl -lcrypto -lpthread
```

> Note: Windows builds may require additional configuration of OpenSSL paths and runtime DLLs.

---

## Usage

### Start Server

```bash
./server/server
```

### Start Client (in a separate terminal)

```bash
./client/client
```

---

## Client Commands

| Command                           | Description                                 |
| --------------------------------- | ------------------------------------------- |
| `/register <username> <password>` | Register a new user                         |
| `/login <username> <password>`    | Login with an existing account              |
| `/msg <username> <message>`       | Send a private message                      |
| `/sendfile <username> <filename>` | Send a file with SHA256 checksum validation |
| `/quit`                           | Disconnect from the server                  |

To inspect registered users:

```bash
cat server/users.db
```

---

## Example Workflow

```
Client A:
    /register alice pass123
    /login alice pass123
    /msg bob Hello Bob!

Client B:
    /register bob pass456
    /login bob pass456
    [PM from alice]: Hello Bob!

Client A:
    /sendfile bob test.txt

Client B:
    Incoming file from alice: test.txt
    File saved successfully: test.txt
```

---

## Technical Concepts Applied

### Operating System Concepts

* Multithreading with `std::thread`
* Synchronization using `std::mutex` and RAII patterns

### Networking Concepts

* TCP socket programming (POSIX sockets)
* Custom text-based command protocol

### Security

* SHA256 password and file hashing via OpenSSL
* Integrity verification on every file transfer

### Systems Programming

* Manual I/O buffering for file operations
* Cross-platform compilation support

---

## Future Improvements

* Add `/listusers` and `/help` command support
* Encrypt files before transfer
* Organize incoming files per user
* Prompt confirmation on file overwrites
* Introduce a Makefile for cross-platform build automation

---

## Author

Aditya Nanda
[LinkedIn](https://linkedin.com/in/aditya-nanda-8b0325252)
[Email](mailto:a.nanda@iitg.ac.in)

---

## License

Maintained by Author
