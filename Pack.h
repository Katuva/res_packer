#pragma once
#include <utility>
#include <vector>
#include <string>
#include <fstream>
#include "PakTypes.h"
#include "External/miniz/miniz.h"
#include "sodium.h"

class Pack
{
private:
    int zlibCompressionLevel = MZ_BEST_COMPRESSION;
    int lz4CompressionLevel = 8;
    int zstdCompressionLevel = 8;

    size_t encryptionOpsLimit = crypto_pwhash_OPSLIMIT_MIN;
    size_t encryptionMemLimit = crypto_pwhash_MEMLIMIT_MIN;

    std::string password;
    unsigned char salt[crypto_pwhash_SALTBYTES];
    unsigned char key[crypto_secretbox_xchacha20poly1305_KEYBYTES];

    void GenerateEncryptionKey();
public:
    [[nodiscard]] bool CreatePakFile(
        const std::vector<PakTypes::PakFileItem>& files,
        const std::string& targetPath,
        PakTypes::CompressionType compressionType = PakTypes::CompressionType::ZSTD
    );

    void Encrypt(std::vector<char> &dataBuffer) const;

    static const char *CompressionTypeToString(PakTypes::CompressionType type);

    [[nodiscard]] int getZlibCompressionLevel() const { return zlibCompressionLevel; }
    void setZlibCompressionLevel(int level) { zlibCompressionLevel = level; }

    [[nodiscard]] int getLz4CompressionLevel() const { return lz4CompressionLevel; }
    void setLz4CompressionLevel(int level) { lz4CompressionLevel = level; }

    [[nodiscard]] int getZstdCompressionLevel() const { return zstdCompressionLevel; }
    void setZstdCompressionLevel(int level) { zstdCompressionLevel = level; }

    [[nodiscard]] size_t getEncryptionOpsLimit() const { return encryptionOpsLimit; }
    void setEncryptionOpsLimit(size_t limit) { encryptionOpsLimit = limit; }

    [[nodiscard]] size_t getEncryptionMemLimit() const { return encryptionMemLimit; }
    void setEncryptionMemLimit(size_t limit) { encryptionMemLimit = limit; }

    [[nodiscard]] std::string getPassword() const { return password; }
    void setPassword(std::string &pwd) { password = pwd; }
};