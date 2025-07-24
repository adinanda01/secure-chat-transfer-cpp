#include <iostream>
#include <thread>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>
#include <sys/stat.h>
#include <vector>

#define PORT 9000
#define SERVER_IP "127.0.0.1"

std::string sha256_bytes(const std::vector<char> &data);

void receive_messages(int sock)
{
    char buffer[4096] = {0};
    while (true)
    {
        memset(buffer, 0, sizeof(buffer));
        std::string input_line;
        char ch;

        // Read line character by character
        while (true)
        {
            int res = recv(sock, &ch, 1, 0);
            if (res <= 0)
            {
                std::cout << "[-] Server disconnected.\n";
                return;
            }
            if (ch == '\n')
                break;
            input_line += ch;
        }

        // Remove trailing \r if present
        if (!input_line.empty() && input_line.back() == '\r')
        {
            input_line.pop_back();
        }

        // Check for incoming file notification
        if (input_line.rfind("FILE_INCOMING", 0) == 0)
        {
            std::istringstream iss(input_line);
            std::string cmd, sender, filename;
            size_t filesize;

            iss >> cmd >> sender >> filename >> filesize;
            std::cout << "[!] Incoming file from " << sender << ": " << filename << " (" << filesize << " bytes)\n";
            std::cout << "[!] Receiving file from " << sender << "...\n";

            // Receive the file data directly (no ACCEPT needed)
            std::vector<char> file_data(filesize);
            size_t received = 0;
            while (received < filesize)
            {
                int chunk = recv(sock, &file_data[received], filesize - received, 0);
                if (chunk <= 0)
                {
                    std::cerr << "[-] Error receiving file data.\n";
                    return;
                }
                received += chunk;
            }
            std::cout << "[DEBUG] Received " << received << " / " << filesize << " bytes\n";

            // Receive checksum (read line by line)
            std::string received_checksum;
            char checksum_ch;
            while (true)
            {
                int res = recv(sock, &checksum_ch, 1, 0);
                if (res <= 0)
                {
                    std::cerr << "[-] Failed to receive checksum.\n";
                    break;
                }
                if (checksum_ch == '\n')
                    break;
                received_checksum += checksum_ch;
            }

            // Remove trailing \r if present
            if (!received_checksum.empty() && received_checksum.back() == '\r')
            {
                received_checksum.pop_back();
            }

            // Compare with local checksum
            std::string local_checksum = sha256_bytes(file_data);
            std::cout << "[DEBUG] Local checksum: " << local_checksum << "\n";
            std::cout << "[DEBUG] Received checksum: " << received_checksum << "\n";

            if (local_checksum != received_checksum)
            {
                std::cerr << "[-] Checksum mismatch! File corrupted.\n";
                std::cerr << "    Expected: " << received_checksum << "\n";
                std::cerr << "    Got: " << local_checksum << "\n";
                continue;
            }

            // Save file
            std::ofstream out(filename, std::ios::binary);
            if (!out)
            {
                std::cerr << "[-] Failed to open file for writing: " << filename << "\n";
                perror("Reason");
                continue;
            }

            out.write(file_data.data(), file_data.size());
            out.close();

            std::cout << "[+] File saved successfully: " << filename << "\n";
        }
        else
        {
            // Regular message
            std::cout << input_line << "\n";
            std::cout.flush();
        }
    }
}



std::string sha256_bytes(const std::vector<char> &data)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char *>(data.data()), data.size(), hash);

    std::ostringstream oss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    return oss.str();
}

int main()
{
    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        std::cerr << "Socket creation error\n";
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0)
    {
        std::cerr << "Invalid address / Address not supported\n";
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        std::cerr << "Connection Failed\n";
        return -1;
    }

    std::cout << "[+] Connected to server at " << SERVER_IP << ":" << PORT << "\n";
    std::cout << "👉 Use /register <username> <password> or /login <username> <password> to continue\n";

    std::thread receiver(receive_messages, sock);

    std::string message;
    while (true)
    {
        std::getline(std::cin, message);

        if (message == "/quit")
            break;

        // Handle sendfile command
        if (message.rfind("/sendfile ", 0) == 0)
        {
            std::istringstream iss(message);
            std::string cmd, target_user, filepath;
            iss >> cmd >> target_user >> filepath;

            if (target_user.empty() || filepath.empty())
            {
                std::cerr << "[-] Usage: /sendfile <username> <filepath>\n";
                continue;
            }

            std::ifstream file(filepath, std::ios::binary);
            if (!file.is_open())
            {
                std::cerr << "[-] Could not open file: " << filepath << "\n";
                continue;
            }

            // Read file into buffer
            std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();

            size_t filesize = buffer.size();
            std::string checksum = sha256_bytes(buffer);

            // Extract filename from path
            std::string filename = filepath.substr(filepath.find_last_of("/\\") + 1);

            std::cout << "[DEBUG] Sending file: " << filename << " (" << filesize << " bytes)\n";
            std::cout << "[DEBUG] Checksum: " << checksum << "\n";

            // 1. Send command line
            std::string cmdline = "/sendfile " + target_user + " " + filename + "\n";
            send(sock, cmdline.c_str(), cmdline.length(), 0);

            // 2. Send file size
            std::string sizeline = std::to_string(filesize) + "\n";
            send(sock, sizeline.c_str(), sizeline.length(), 0);

            // 3. Send raw file bytes
            size_t sent = 0;
            while (sent < filesize)
            {
                int chunk = send(sock, buffer.data() + sent, filesize - sent, 0);
                if (chunk <= 0)
                {
                    std::cerr << "[-] Error sending file data.\n";
                    break;
                }
                sent += chunk;
            }

            // 4. Send checksum
            std::string checksum_line = checksum + "\n";
            send(sock, checksum_line.c_str(), checksum_line.length(), 0);

            std::cout << "[+] File sent: " << filename << " (" << filesize << " bytes)\n";
        }
        else
        {
            // Normal message
            message += "\n";
            send(sock, message.c_str(), message.length(), 0);
        }
    }

    close(sock);
    if (receiver.joinable())
        receiver.join();

    return 0;
}
