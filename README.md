
# 🔐 Secure Multithreaded Chat & File Transfer System

A C++-based TCP client-server system for secure private messaging and file transfers with integrity validation. This project is designed using **socket programming**, **multithreading**, and **SHA256 checksum validation**, integrating core concepts from **Operating Systems** and **Computer Networks**.

---

## 🚀 Features

- ✅ **Multithreaded Server**: Handles multiple clients concurrently using `std::thread`
- 🔐 **Secure Authentication**: User registration/login with SHA256-hashed passwords
- 💬 **Private Messaging**: Use `/msg <username> <message>` to send personal messages
- 📁 **File Transfer**: Send files using `/sendfile <username> <filename>`
- 🛡️ **Data Integrity**: SHA256 checksum validation for every file transfer
- 🔄 **Deadlock-Free Protocol**: Replaces traditional ACK with a pipelined transfer system
- 📜 **Clean CLI UX**: Easy command-line interface for both clients and server

---

## 🏗️ Build Instructions

### 📦 Prerequisites

- C++11 compatible compiler (GCC/Clang)
- OpenSSL development libraries

#### 🛠️ On Ubuntu/Debian:
```bash
sudo apt update
sudo apt install build-essential libssl-dev
```

#### 🛠️ On macOS (using Homebrew):
```bash
brew install openssl
```

---

### 🔨 Compilation

```bash
# Build Server
g++ server/main.cpp server/server_utils.cpp -o server/server -lpthread -lssl -lcrypto

# Build Client
g++ client/main.cpp -o client/client -lpthread -lssl -lcrypto
```

---

## 🧪 Usage

### 🔧 Run Server
```bash
./server/server
```

### 💬 Run Client (in a new terminal)
```bash
./client/client
```

---

## 💻 Supported Commands (Client)

| Command | Description |
|---------|-------------|
| `/register <username> <password>` | Register a new user |
| `/login <username> <password>` | Login as an existing user |
| `/msg <username> <message>` | Send a private message |
| `/sendfile <username> <filename>` | Send a file with checksum verification |
| `/quit` | Disconnect from the server |
|> to clear the database
 cat server/users.db

---

## 🧾 Example Interaction

```
Client A: /register alice pass123
Client A: /login alice pass123
Client A: /msg bob Hello Bob!

Client B: /register bob pass456
Client B: /login bob pass456
Client B: [PM from alice]: Hello Bob!

Client A: /sendfile bob test.txt
Client B: [!] Incoming file from alice: test.txt
         [+] File saved successfully: test.txt
```

---

## 🧠 Concepts Applied

- Operating System:
  - Thread management (`std::thread`)
  - Synchronization (`std::mutex`, RAII)
- Computer Networks:
  - TCP socket programming
  - Custom text-based protocol
- Security:
  - SHA256 hashing via OpenSSL
  - File integrity checks
- Systems Programming:
  - Manual I/O buffering
  - Cross-platform POSIX socket API

---

## 📌 Future Enhancements (Ideas)

- Add `/listusers` and `/help` commands
- Enable file encryption before transfer
- Save incoming files to a user-specific directory
- Add file overwrite confirmation
- Makefile for build automation

---

## 👤 Author
Reach out: [LinkedIn](https://linkedin.com/in/aditya-nanda-8b0325252) • [Email](mailto:a.nanda@iitg.ac.in)

---

## 📝 License

to ME..
