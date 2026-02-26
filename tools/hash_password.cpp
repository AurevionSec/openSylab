#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

// Simple hash function matching User::hashPassword
std::string hashPassword(const std::string &password) {
  if (password.empty()) {
    return "";
  }

  // Simple hash based on DJB2 algorithm with salt
  const std::string salt = "OpenSylab_v0.2_Salt";
  std::string salted = salt + password + salt;

  unsigned long hash1 = 5381;
  unsigned long hash2 = 52711;

  for (char c : salted) {
    hash1 = ((hash1 << 5) + hash1) ^ static_cast<unsigned char>(c);
    hash2 = ((hash2 << 5) + hash2) + static_cast<unsigned char>(c);
  }

  // Combine and output as hex string
  std::ostringstream oss;
  oss << std::hex << std::setfill('0');
  oss << std::setw(16) << hash1;
  oss << std::setw(16) << hash2;

  return oss.str();
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <password>" << std::endl;
    return 1;
  }

  std::string password = argv[1];
  std::string hash = hashPassword(password);

  std::cout << hash << std::endl;

  return 0;
}
