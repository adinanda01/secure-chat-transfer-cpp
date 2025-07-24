#include <iostream>
#include <thread>
#include <vector>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include "../server/server_utils.h"  // Assumed to contain register_user, validate_user, sha256 functions
#include <sstream>
#include <unordered_map>
#include <mutex>
#include <chrono>

std::unordered_map<std::string, int> username_to_socket;
std::mutex user_map_mutex;

#define PORT 9000
#define MAX_CLIENTS 100

// Helper function to read a line from socket
bool read_line(int socket, std::string& line) {
    line.clear();
    char ch;
    while (true) {
        int res = recv(socket, &ch, 1, 0);
        if (res <= 0) {
            return false;
        }
        if (ch == '\n') {
            // Remove trailing \r if present
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            return true;
        }
        line += ch;
    }
}

void handle_client(int client_socket)
{
    std::string recv_buffer;
    char chunk[1024];
    std::string current_user;
    bool authenticated = false;

    std::cout << "[+] Client connected: socket " << client_socket << std::endl;

    while (true)
    {
        memset(chunk, 0, sizeof(chunk));
        int bytes_received = recv(client_socket, chunk, sizeof(chunk), 0);
        if (bytes_received <= 0)
        {
            std::cout << "[-] Client disconnected: socket " << client_socket << std::endl;
            break;
        }

        recv_buffer.append(chunk, bytes_received);

        // Process complete lines
        size_t newline_pos;
        while ((newline_pos = recv_buffer.find('\n')) != std::string::npos)
        {
            std::string input = recv_buffer.substr(0, newline_pos);
            recv_buffer.erase(0, newline_pos + 1);

            if (!input.empty() && input.back() == '\r')
                input.pop_back(); // Trim \r if any

            std::istringstream iss(input);
            std::string command;
            iss >> command;

            // Command to register on to the server  -----------------------------------------------------

            if (command == "/register")
            {
                std::string username, password;
                iss >> username >> password;
                std::string hashed = sha256(password); // ✅ HASH IT HERE
                if (username.empty() || password.empty()) {
                    send(client_socket, "ERROR: Username and password required\n", 38, 0);
                    continue;
                }
                
                if (register_user(username, hashed))
                {
                    send(client_socket, "REGISTER_SUCCESS\n", 17, 0);
                }

                // We can add like for couple of attempt of regesitering the same person > ALREADY REGISTERED
                else
                {
                    send(client_socket, "REGISTER_FAILED\n", 16, 0);
                }
            }
            

            // Command to Login to the server -----------------------------------------------------

            else if (command == "/login")
            {
                std::string username, password;
                iss >> username >> password;
                std::string hashed = sha256(password);
                if (username.empty() || password.empty()) {
                    send(client_socket, "ERROR: Username and password required\n", 38, 0);
                    continue;
                }
                
                // Command to Validate User to the server -----------------------------------------------------

                if (validate_user(username, hashed))
                {
                    std::lock_guard<std::mutex> lock(user_map_mutex);
                    
                    // Check if user is already logged in  -----------------------------------------------------

                    if (username_to_socket.find(username) != username_to_socket.end()) {
                        send(client_socket, "ERROR: User already logged in\n", 31, 0);
                        continue;
                    }
                    
                    username_to_socket[username] = client_socket;
                    current_user = username;
                    authenticated = true;
                    send(client_socket, "LOGIN_SUCCESS\n", 14, 0);
                }
                else
                {
                    send(client_socket, "LOGIN_FAILED\n", 13, 0);
                }
            }

            // Command to Msg -----------------------------------------------------

            else if (command == "/msg")
            {
                // if the client has not logged in and !authenticated -----------------------------------------------------

                if (!authenticated) {
                    send(client_socket, "ERROR: Please login first.\n", 28, 0);
                    continue;
                }

                // Read the correct command and message written  -----------------------------------------------------
                size_t first_space = input.find(' ');
                size_t second_space = input.find(' ', first_space + 1);
                if (second_space == std::string::npos)
                {
                    send(client_socket, "ERROR: Usage: /msg <username> <message>\n", 41, 0);
                    continue;
                }

                

                std::string target_user = input.substr(first_space + 1, second_space - first_space - 1);
                std::string msg_body = input.substr(second_space + 1);

                // IF MESSGAGE BODY IS EMPTY  -----------------------------------------------------

                if (msg_body.empty())
                {
                    send(client_socket, "ERROR: Message cannot be empty.\n", 33, 0);
                    continue;
                }

                // CHECK ACTIVE USERS AND FIND THE CORRECT DESTINATION CLIENT TO SEND MESSAGE  -----------------------------------------------------

                std::lock_guard<std::mutex> lock(user_map_mutex);
                if (username_to_socket.find(target_user) == username_to_socket.end())
                {
                    send(client_socket, "ERROR: User not online.\n", 25, 0);
                }
                else
                {
                    std::string full_msg = "[PM from " + current_user + "]: " + msg_body + "\n";
                    int receiver_socket = username_to_socket[target_user];
                    send(receiver_socket, full_msg.c_str(), full_msg.length(), 0);
                    send(client_socket, "[PM sent]\n", 10, 0);
                }
            }

            // COMMAND TO SEND FILE -----------------------------------------------------

            else if (command == "/sendfile")
            {
                if (!authenticated) {
                    send(client_socket, "ERROR: Please login first.\n", 28, 0);
                    continue;
                }

                std::string target_user, filename;
                iss >> target_user >> filename;

                if (target_user.empty() || filename.empty()) {
                    send(client_socket, "ERROR: Usage: /sendfile <username> <filename>\n", 47, 0);
                    continue;
                }

                int receiver_socket = -1;
                {
                    std::lock_guard<std::mutex> lock(user_map_mutex);
                    if (username_to_socket.find(target_user) == username_to_socket.end()) {
                        send(client_socket, "ERROR: Target user not online\n", 31, 0);
                        continue;
                    }
                    receiver_socket = username_to_socket[target_user];
                }

                std::cout << "[DEBUG] Processing file transfer from " << current_user << " to " << target_user << std::endl;

                // Step 1: Read file size
                std::string size_line;
                size_t size_newline_pos = recv_buffer.find('\n');
                if (size_newline_pos != std::string::npos) {
                    size_line = recv_buffer.substr(0, size_newline_pos);
                    recv_buffer.erase(0, size_newline_pos + 1);
                    if (!size_line.empty() && size_line.back() == '\r')
                        size_line.pop_back();
                } else {
                    if (!read_line(client_socket, size_line)) {
                        std::cerr << "[-] Failed to read file size\n";
                        send(client_socket, "ERROR: Failed to read file size\n", 33, 0);
                        continue;
                    }
                }
                // NOTE THE SERVER CALCULATES THE FILE SIZE AND CHECK IT 
                // IT DOES NOT STORES THE FILE HENCE WE DON'T REQUIRE THE STORAGE HERE AND WE ARE EFFICIENT IN STORAGE -----------------------------------------------------

                size_t file_size = 0;
                try {
                    file_size = std::stoul(size_line);
                } catch (...) {
                    std::cerr << "[-] Invalid file size: " << size_line << "\n";
                    send(client_socket, "ERROR: Invalid file size\n", 26, 0);
                    continue;
                }

                std::cout << "[DEBUG] File size: " << file_size << " bytes\n";

                // Step 2: Receive file data
                std::vector<char> file_data(file_size);
                size_t bytes_copied = 0;

                // First, copy any data already in buffer
                if (!recv_buffer.empty()) {
                    size_t to_copy = std::min(file_size, recv_buffer.size());
                    std::copy(recv_buffer.begin(), recv_buffer.begin() + to_copy, file_data.begin());
                    recv_buffer.erase(0, to_copy);
                    bytes_copied = to_copy;
                }

                // Then receive remaining data
                while (bytes_copied < file_size) {
                    int chunk = recv(client_socket, file_data.data() + bytes_copied, file_size - bytes_copied, 0);
                    if (chunk <= 0) {
                        std::cerr << "[-] Failed to receive file content\n";
                        send(client_socket, "ERROR: File transfer interrupted\n", 34, 0);
                        goto next_command;
                    }
                    bytes_copied += chunk;
                }

                std::cout << "[DEBUG] File content received successfully (" << bytes_copied << " bytes)\n";

                // FILE INTEGRITY CHECK USING SHA256 CHECKSUM -----------------------------------------------------
                // Step 3: Read checksum
                std::string checksum;
                size_t checksum_newline_pos = recv_buffer.find('\n');
                if (checksum_newline_pos != std::string::npos) {
                    checksum = recv_buffer.substr(0, checksum_newline_pos);
                    recv_buffer.erase(0, checksum_newline_pos + 1);
                    if (!checksum.empty() && checksum.back() == '\r')
                        checksum.pop_back();
                } else {
                    if (!read_line(client_socket, checksum)) {
                        std::cerr << "[-] Failed to read checksum\n";
                        send(client_socket, "ERROR: Failed to read checksum\n", 32, 0);
                        continue;
                    }
                }

                std::cout << "[DEBUG] Checksum received: " << checksum << "\n";

                // Step 4: Send file directly to receiver (simplified approach)
                std::string notify_msg = "FILE_INCOMING " + current_user + " " + filename + " " + std::to_string(file_size) + "\n";
                if (send(receiver_socket, notify_msg.c_str(), notify_msg.length(), 0) != (ssize_t)notify_msg.length()) {
                    std::cerr << "[-] Failed to send notification to receiver\n";
                    send(client_socket, "ERROR: File transfer failed\n", 29, 0);
                    continue;
                }
                std::cout << "[DEBUG] Notification sent to receiver\n";

                // Small delay to let receiver process the notification
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                // Step 5: Send file data to receiver
                if (send(receiver_socket, file_data.data(), file_data.size(), 0) != (ssize_t)file_data.size()) {
                    std::cerr << "[-] Failed to send file data to receiver\n";
                    send(client_socket, "ERROR: File transfer failed\n", 29, 0);
                    continue;
                }

                // Step 6: Send checksum to receiver
                std::string checksum_line = checksum + "\n";
                if (send(receiver_socket, checksum_line.c_str(), checksum_line.length(), 0) != (ssize_t)checksum_line.length()) {
                    std::cerr << "[-] Failed to send checksum to receiver\n";
                    send(client_socket, "ERROR: File transfer failed\n", 29, 0);
                    continue;
                }

                std::cout << "[DEBUG] File forwarded successfully\n";
                send(client_socket, "[+] File transferred successfully\n", 35, 0);
            }

            else
            {
                if (!authenticated)
                {
                    send(client_socket, "ERROR: Please login first.\n", 28, 0);
                }
                else
                {
                    std::string msg = "[Echo] " + input + "\n";
                    send(client_socket, msg.c_str(), msg.length(), 0);
                }
            }
            
            next_command:;
        }
    }

    // Remove user from map when disconnecting
    {
        std::lock_guard<std::mutex> lock(user_map_mutex);
        for (auto it = username_to_socket.begin(); it != username_to_socket.end(); ++it) {
            if (it->second == client_socket) {
                std::cout << "[DEBUG] Removing user " << it->first << " from active users\n";
                username_to_socket.erase(it);
                break;
            }
        }
    }

    close(client_socket);
}

int main()
{
    
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    std::vector<std::thread> client_threads;

   // 1. Create socket -----------------------------------------------------
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // 2. Set socket options (like SO_REUSEADDR) -----------------------------------------------------
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // 3. Bind the socket to an address and port -----------------------------------------------------
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // 4. Listen for incoming connections -----------------------------------------------------
    if (listen(server_fd, MAX_CLIENTS) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "[+] Server started on port " << PORT << std::endl;

    // 5. Accept and handle each connection -----------------------------------------------------
    while (true)
    {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                                 (socklen_t *)&addrlen)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // Resource Management -----------------------------------------
        std::thread client_thread(handle_client, new_socket);
        client_thread.detach();  //  Let it run independently

    }

    // Join threads before shutdown (not strictly needed now)
    for (auto &t : client_threads)
    {
        if (t.joinable())
            t.join();
    }

    return 0;
}
