//
// Created by Tadeas Topinka (xtopint00) on 2025-11-15.
//

#ifndef PROJEKT_CIPHER_H
#define PROJEKT_CIPHER_H

#include <vector>
#include <string>

namespace Cipher {
    class Cipher {
    public:
        /**
         * Constructor used to create the Cipher class, it initializes the key with the login parameter
         * @param login string used to initialize the ciphering key
         */
        explicit Cipher(const std::string& login);

        /**
         * method used to encrypt data using the AES256 cipher
         * @param plainText data to be encrypted
         * @param iv initialization vector to be used for this encryption
         * @return encrypted data vector
         */
        std::vector<uint8_t> encrypt(const std::vector<uint8_t>& plainText, std::vector<uint8_t>& iv);

        /**
         * method used to decrypt data using the AES256 cipher
         * @param cipherText data to be decrypted
         * @param iv initialization vector to be used for this decryption
         * @return decrypted data vector
         */
        std::vector<uint8_t> decrypt(const std::vector<uint8_t>& cipherText, std::vector<uint8_t>& iv);

    private:

        /**
         * key used for ciphering by this instance of Cipher
         */
        uint8_t key[32] = {};
    };
} // Cipher

#endif //PROJEKT_CIPHER_H