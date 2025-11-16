//
// Created by root on 2025-11-15.
//

#ifndef PROJEKT_CIPHER_H
#define PROJEKT_CIPHER_H

#include <vector>
#include <string>
#include <openssl/evp.h>

namespace Cipher {
    class Cipher {
    public:
        explicit Cipher(const std::string& login);

        std::vector<uint8_t> encrypt(const std::vector<uint8_t>& plainText, std::vector<uint8_t>& iv);

        std::vector<uint8_t> decrypt(const std::vector<uint8_t>& cipherText, std::vector<uint8_t>& iv);

    private:
        uint8_t key[32] = {0};
    };
} // Cipher

#endif //PROJEKT_CIPHER_H