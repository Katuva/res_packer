#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <exception>
#include <filesystem>
#include "PakTypes.h"

#ifdef USE_LZ4
#include "lz4hc.h"
#endif
#ifdef USE_ZSTD
#include "zstd.h"
#endif
#ifdef USE_ZLIB
#include "External/miniz/miniz.h"
#endif
#ifdef USE_ENCRYPTION
#include "sodium.h"
#endif

class Unpacker {
public:
    std::vector<char> ExtractFileToMemory(
            PakTypes::PakFile &pakFile,
            const std::string &filePath
    );

    void ExtractFileToDisk(
            PakTypes::PakFile &pakFile,
            const std::string &outputPath,
            const std::string &filePath
    );

    static PakTypes::PakFile ParsePakFile(const std::string &inputPath);

    void Decrypt(std::vector<char> &dataBuffer) const;

    [[nodiscard]] size_t getEncryptionOpsLimit() const { return encryptionOpsLimit; }

    void setEncryptionOpsLimit(size_t limit) { encryptionOpsLimit = limit; }

    [[nodiscard]] size_t getEncryptionMemLimit() const { return encryptionMemLimit; }

    void setEncryptionMemLimit(size_t limit) { encryptionMemLimit = limit; }

    [[nodiscard]] std::string getPassword() const { return password; }

    void setPassword(std::string &pwd) { password = pwd; }

private:
    size_t encryptionOpsLimit = crypto_pwhash_OPSLIMIT_MIN;
    size_t encryptionMemLimit = crypto_pwhash_MEMLIMIT_MIN;

    std::string password;
    unsigned char salt[crypto_pwhash_SALTBYTES];
    unsigned char key[crypto_secretbox_xchacha20poly1305_KEYBYTES];

    void GenerateEncryptionKey();
};