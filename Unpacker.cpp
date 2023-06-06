#include "Unpacker.h"
#include <fstream>
#include <string>
#include <filesystem>

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

std::vector<char> Unpacker::ExtractFileToMemory(PakTypes::PakFile& pakFile, const std::string& filePath) {
    bool hasEncryptedItem = std::any_of(pakFile.FileEntries.begin(), pakFile.FileEntries.end(),
                                        [](const PakTypes::PakFileTableEntry &item) {
                                            return item.Encrypted;
                                        });
#ifdef USE_ENCRYPTION
    if (hasEncryptedItem) {
        memcpy(salt, pakFile.Header.Salt, crypto_pwhash_SALTBYTES);
        Unpacker::GenerateEncryptionKey();
    }
#else
    if (hasEncryptedItem)
        throw std::runtime_error("Encryption is not supported");
#endif

    auto fileEntryIt = std::find_if(pakFile.FileEntries.begin(), pakFile.FileEntries.end(),
                                    [&filePath](const PakTypes::PakFileTableEntry &entry) {
                                        return std::string(entry.FilePath) == filePath;
                                    });

    if (fileEntryIt != pakFile.FileEntries.end()) {
        const PakTypes::PakFileTableEntry &entry = *fileEntryIt;

        pakFile.File.seekg(entry.Offset);

        std::vector<char> dataBuffer(entry.PackedSize);
        pakFile.File.read(dataBuffer.data(), dataBuffer.size());

#ifdef USE_ENCRYPTION
        if (entry.Encrypted) {
            Decrypt(dataBuffer);
        }
#endif

        std::vector<char> buffer;
        if (entry.Compressed) {
            buffer.resize(entry.OriginalSize);

            if (entry.CompressionType == PakTypes::CompressionType::ZLIB) {
#ifdef USE_ZLIB
                mz_ulong uncompressedSize = entry.OriginalSize;
                int result = mz_uncompress(reinterpret_cast<unsigned char *>(buffer.data()), &uncompressedSize,
                                           reinterpret_cast<const unsigned char *>(dataBuffer.data()),
                                           entry.PackedSize);
                if (result != MZ_OK)
                    throw std::runtime_error("Failed to decompress file: " + filePath);
#else
                throw std::runtime_error("ZLIB compression is not supported");
#endif
            } else if (entry.CompressionType == PakTypes::CompressionType::LZ4) {
#ifdef USE_LZ4
                int decompressed_size = LZ4_decompress_safe(dataBuffer.data(), buffer.data(), dataBuffer.size(),
                                                            buffer.size());
                if (decompressed_size <= 0)
                    throw std::runtime_error("Failed to decompress file: " + filePath);
#else
                throw std::runtime_error("LZ4 compression is not supported");
#endif
            } else if (entry.CompressionType == PakTypes::CompressionType::ZSTD) {
#ifdef USE_ZSTD
                size_t decompressed_size = ZSTD_decompress(buffer.data(), buffer.size(), dataBuffer.data(),
                                                           dataBuffer.size());
                if (ZSTD_isError(decompressed_size))
                    throw std::runtime_error("Failed to decompress file: " + filePath);
#else
                throw std::runtime_error("ZSTD compression is not supported");
#endif
            } else {
                throw std::invalid_argument("Unknown compression type");
            }
        } else {
            buffer = std::move(dataBuffer);
        }

        return buffer;
    } else {
        throw std::runtime_error("File not found in pak file: " + filePath);
    }
}


void Unpacker::ExtractFileToDisk(PakTypes::PakFile &pakFile, const std::string &outputPath, const std::string &filePath) {
    std::vector<char> buffer = ExtractFileToMemory(pakFile, filePath);
    std::string filename = std::filesystem::path(filePath).filename().string();

    std::filesystem::path outputFile = std::filesystem::path(outputPath) / filename;
    std::ofstream file(outputFile, std::ios::binary);
    file.write(buffer.data(), buffer.size());
    file.close();
}

PakTypes::PakFile Unpacker::ParsePakFile(const std::string &inputPath) {
    PakTypes::PakFile file;
    std::ifstream pakFile(inputPath, std::ios::binary);

    if (!pakFile)
        throw std::runtime_error("Failed to open pak file: " + inputPath);

    file.File = std::move(pakFile);

    PakTypes::PakHeader header;
    if (!file.File.read(reinterpret_cast<char*>(&header), sizeof(PakTypes::PakHeader)))
        throw std::runtime_error("Failed to read header from pak file: " + inputPath);

    if (std::string(header.ID) != "PAK")
        throw std::runtime_error("Invalid PAK file format: " + inputPath);

    if (header.Version != PakTypes::PAK_FILE_VERSION)
        throw std::runtime_error("Unsupported PAK file version: " + inputPath);

    std::vector<PakTypes::PakFileTableEntry> entries(header.NumEntries);
    if (!file.File.read(reinterpret_cast<char*>(entries.data()), sizeof(PakTypes::PakFileTableEntry) * header.NumEntries))
        throw std::runtime_error("Failed to read file entries from pak file: " + inputPath);

    file.Header = header;
    file.FileEntries.reserve(entries.size() + header.NumEntries);
    file.FileEntries.insert(file.FileEntries.end(), entries.begin(), entries.end());

    return file;
}

#ifdef USE_ENCRYPTION
void Unpacker::GenerateEncryptionKey() {
    sodium_init();

    if (crypto_pwhash(key, sizeof key, password.c_str(), password.size(), salt,
                      encryptionOpsLimit, encryptionMemLimit,
                      crypto_pwhash_ALG_DEFAULT) != 0) {
        throw std::exception("Key derivation failed");
    }
}

void Unpacker::Decrypt(std::vector<char> &dataBuffer) const {
    unsigned char nonce[crypto_secretbox_xchacha20poly1305_NONCEBYTES];
    std::memcpy(nonce, dataBuffer.data(), sizeof nonce);

    size_t message_len = dataBuffer.size() - sizeof nonce - crypto_secretbox_xchacha20poly1305_MACBYTES;
    auto *decrypted = new unsigned char[message_len];

    if (crypto_secretbox_xchacha20poly1305_open_easy(decrypted,
                                                     reinterpret_cast<const unsigned char *>(dataBuffer.data()) +
                                                     sizeof nonce,
                                                     dataBuffer.size() - sizeof nonce,
                                                     nonce,
                                                     key) != 0) {
        throw std::exception("Decryption failed");
    }

    dataBuffer.clear();
    dataBuffer.resize(message_len);
    std::memcpy(dataBuffer.data(), decrypted, message_len);

    delete[] decrypted;
}
#endif
