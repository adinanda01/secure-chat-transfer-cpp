#ifndef SERVER_UTILS_H
#define SERVER_UTILS_H

#include <string>

std::string sha256(const std::string &str);
bool register_user(const std::string &username, const std::string &hashed_pass);
bool validate_user(const std::string &username, const std::string &hashed_pass);

#endif