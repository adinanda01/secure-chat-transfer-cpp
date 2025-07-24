#include "server_utils.h"
#include <openssl/sha.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <mutex>

// Add this:
std::mutex user_file_mutex;

std::string sha256(const std::string &str) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char *)str.c_str(), str.size(), hash);

    std::ostringstream oss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    return oss.str();
}

bool register_user(const std::string &username, const std::string &hashed_pass) {
    std::lock_guard<std::mutex> lock(user_file_mutex);
    std::ifstream infile("server/users.db");
    std::string line;
    while (std::getline(infile, line)) {
        if (line.substr(0, line.find(':')) == username)
            return false; // Already exists
    }
    infile.close();

    std::ofstream outfile("server/users.db", std::ios::app);
    outfile << username << ":" << hashed_pass << "\n";
    return true;
}

bool validate_user(const std::string &username, const std::string &hashed_pass) {
    std::lock_guard<std::mutex> lock(user_file_mutex);
    std::ifstream infile("server/users.db");
    std::string line;
    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        std::string user, pass;
        if (std::getline(iss, user, ':') && std::getline(iss, pass)) {
            if (user == username && pass == hashed_pass)
                return true;
        }
    }
    return false;
}
