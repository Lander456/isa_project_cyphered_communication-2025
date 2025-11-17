//
// Created by root on 2025-11-15.
//

#include "../../include/Cipher.h"

#include <iostream>
#include <openssl/sha.h>
#include <openssl/rand.h>

namespace Cipher {
    Cipher::Cipher(const std::string& login) {
        SHA256(reinterpret_cast<const uint8_t*>(login.data()), login.size(), key);
    }

    std::vector<uint8_t> Cipher::encrypt(const std::vector<uint8_t>& plainText, std::vector<uint8_t>& iv) {
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();

        std::vector<uint8_t> cipherText(plainText.size() + 32);

        int len;

        EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv.data());
        EVP_EncryptUpdate(ctx, cipherText.data(), &len, plainText.data(), plainText.size());
        int cypherTextLen = len;

        EVP_EncryptFinal_ex(ctx, cipherText.data() + len, &len);
        cypherTextLen += len;

        cipherText.resize(cypherTextLen);
        EVP_CIPHER_CTX_free(ctx);

        return cipherText;
    }

    std::vector<uint8_t> Cipher::decrypt(const std::vector<uint8_t> &cipherText, std::vector<uint8_t> &iv) {
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        std::vector<uint8_t> plainText(cipherText.size());

        int len;

        if (!EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv.data())) {
        }

        if (!EVP_DecryptUpdate(ctx, plainText.data(), &len, cipherText.data(), cipherText.size())) {
        }
        int plainTextLen = len;

        if (!EVP_DecryptFinal_ex(ctx, plainText.data() + len, &len)) {
        }
        plainTextLen += len;

        plainText.resize(plainTextLen);
        EVP_CIPHER_CTX_free(ctx);

        return plainText;
    }

} // Cipher