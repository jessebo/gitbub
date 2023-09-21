#ifndef COMMON_BASE64_H_
#define COMMON_BASE64_H_
#include <string>
 
std::string base64_encode_(unsigned char const* , unsigned int len);
std::string base64_decode_(std::string const& s);
std::string base64_Encode(const std::string&);
std::string base64_Decode(const std::string&);
std::string urlsafe_base64Encode(const std::string&s);
std::string urlsafe_base64Encode(unsigned char const* bytes_to_encode, unsigned int in_len);

#endif
