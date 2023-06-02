#include "Packer.h"
#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>
#include "lz4hc.h"
#include "zstd.h"
#include "External/miniz/miniz.h"
#include "sodium.h"
#include "PakTypes.h"

bool Packer::CreatePakFile(
        const std::vector<PakTypes::PakFileItem> &files,
        const std::string &targetPath,
        PakTypes::CompressionType compressionType) {
    PakTypes::PakHeader header{};
    header.NumEntries = 0;

    std::vector<char> dataBuffer;
    std::vector<PakTypes::PakFileTableEntry> fileEntries;

    bool hasEncryptedItem = std::any_of(files.begin(), files.end(), [](const PakTypes::PakFileItem &item) {
        return item.encrypted;
    });

    if (hasEncryptedItem) {
        Packer::GenerateEncryptionKey();
        std::memcpy(header.Salt, salt, crypto_pwhash_SALTBYTES);
    }

    for (const auto &file: files) {
        std::ifstream fileStream(file.path, std::ios::ate | std::ios::binary);

        if (!fileStream) {
            std::cout << "Warning: File not found '" << file.path << "'" << std::endl;
            continue;
        }

        PakTypes::PakFileTableEntry pakFileEntry{};
        pakFileEntry.OriginalSize = static_cast<unsigned int>(fileStream.tellg());
        std::memcpy(pakFileEntry.FilePath, file.packedPath.c_str(), file.packedPath.length() + 1);
        pakFileEntry.Offset = static_cast<unsigned int>(dataBuffer.size());
        pakFileEntry.Compressed = file.compressed;

        if (pakFileEntry.Compressed) {
            pakFileEntry.CompressionType = compressionType;
        }

        fileStream.seekg(0);

        std::vector<char> fileData(pakFileEntry.OriginalSize);
        fileStream.read(fileData.data(), pakFileEntry.OriginalSize);

        if (pakFileEntry.Compressed) {
            std::vector<char> compressedData;

            if (compressionType == PakTypes::CompressionType::ZLIB) {
                mz_ulong compressedSize = mz_compressBound(pakFileEntry.OriginalSize);
                compressedData.resize(compressedSize);
                int result = mz_compress2(reinterpret_cast<unsigned char *>(compressedData.data()), &compressedSize,
                                          reinterpret_cast<const unsigned char *>(fileData.data()),
                                          pakFileEntry.OriginalSize, zlibCompressionLevel);
                if (result != MZ_OK)
                    return false;
                if (!file.encrypted) {
                    pakFileEntry.PackedSize = static_cast<unsigned int>(compressedSize);
                    dataBuffer.insert(dataBuffer.end(), compressedData.begin(),
                                      compressedData.begin() + compressedSize);
                }
            } else if (compressionType == PakTypes::CompressionType::LZ4) {
                size_t compressedSize = LZ4_compressBound(pakFileEntry.OriginalSize);
                compressedData.resize(compressedSize);
                int compressed_size = LZ4_compress_HC(fileData.data(), compressedData.data(), fileData.size(),
                                                      compressedData.size(), lz4CompressionLevel);
                if (compressed_size <= 0)
                    return false;
                compressedData.resize(compressed_size);
                if (!file.encrypted) {
                    pakFileEntry.PackedSize = static_cast<unsigned int>(compressed_size);
                    dataBuffer.insert(dataBuffer.end(), compressedData.begin(),
                                      compressedData.begin() + compressed_size);
                }
            } else if (compressionType == PakTypes::CompressionType::ZSTD) {
                size_t compressedSize = ZSTD_compressBound(pakFileEntry.OriginalSize);
                compressedData.resize(compressedSize);
                size_t compressed_size = ZSTD_compress(compressedData.data(), compressedData.size(), fileData.data(),
                                                       fileData.size(), zstdCompressionLevel);
                if (ZSTD_isError(compressed_size))
                    return false;
                compressedData.resize(compressed_size);
                if (!file.encrypted) {
                    pakFileEntry.PackedSize = static_cast<unsigned int>(compressed_size);
                    dataBuffer.insert(dataBuffer.end(), compressedData.begin(),
                                      compressedData.begin() + compressed_size);
                }
            } else {
                throw std::invalid_argument("Unknown compression type");
            }

            if (file.encrypted) {
                Packer::Encrypt(compressedData);
                pakFileEntry.PackedSize = static_cast<unsigned int>(compressedData.size());
                pakFileEntry.Encrypted = true;
                dataBuffer.insert(dataBuffer.end(), compressedData.begin(),
                                  compressedData.begin() + compressedData.size());
            }
        } else {
            pakFileEntry.PackedSize = pakFileEntry.OriginalSize;

            if (file.encrypted) {
                Packer::Encrypt(fileData);
                pakFileEntry.PackedSize = static_cast<unsigned int>(fileData.size());
                pakFileEntry.Encrypted = true;
            }

            dataBuffer.insert(dataBuffer.end(), fileData.begin(), fileData.end());
        }

        fileEntries.push_back(pakFileEntry);
        fileStream.close();
    }

    header.NumEntries = static_cast<unsigned int>(fileEntries.size());

    std::ofstream output(targetPath, std::ios::binary);
    if (!output) {
        throw std::runtime_error("Failed to open output file: " + targetPath);
    }

    output.write(reinterpret_cast<const char *>(&header), sizeof(PakTypes::PakHeader));
    if (!output) {
        throw std::runtime_error("Failed to write header to output file: " + targetPath);
    }

    const unsigned int baseOffset =
            sizeof(PakTypes::PakHeader) +
            static_cast<unsigned int>(fileEntries.size()) * sizeof(PakTypes::PakFileTableEntry);

    for (PakTypes::PakFileTableEntry &e: fileEntries) {
        e.Offset += baseOffset;
        output.write(reinterpret_cast<const char *>(&e), sizeof(PakTypes::PakFileTableEntry));
        if (!output) {
            throw std::runtime_error("Failed to write file entry to output file: " + targetPath);
        }
    }

    output.write(dataBuffer.data(), dataBuffer.size());
    if (!output) {
        throw std::runtime_error("Failed to write data buffer to output file: " + targetPath);
    }

    output.close();
    if (!output) {
        throw std::runtime_error("Failed to close output file: " + targetPath);
    }

    return true;
}

void Packer::GenerateEncryptionKey() {
    sodium_init();

    randombytes_buf(salt, sizeof salt);

    if (crypto_pwhash(key, sizeof key, password.c_str(), password.size(), salt,
                      encryptionOpsLimit, encryptionMemLimit,
                      crypto_pwhash_ALG_DEFAULT) != 0) {
        throw std::exception("Key derivation failed");
    }
}

void Packer::Encrypt(std::vector<char> &dataBuffer) const {
    size_t message_len = dataBuffer.size();

    unsigned char nonce[crypto_secretbox_xchacha20poly1305_NONCEBYTES];
    randombytes_buf(nonce, sizeof nonce);

    auto *ciphertext = new unsigned char[message_len + crypto_secretbox_xchacha20poly1305_MACBYTES];

    if (crypto_secretbox_xchacha20poly1305_easy(ciphertext, reinterpret_cast<const unsigned char *>(dataBuffer.data()),
                                                message_len, nonce, key) != 0) {
        throw std::exception("Encryption failed");
    }

    dataBuffer.clear();
    dataBuffer.resize(
            message_len + crypto_secretbox_xchacha20poly1305_NONCEBYTES + crypto_secretbox_xchacha20poly1305_MACBYTES);
    std::memcpy(dataBuffer.data(), nonce, sizeof nonce);
    std::memcpy(dataBuffer.data() + sizeof nonce, ciphertext,
                message_len + crypto_secretbox_xchacha20poly1305_MACBYTES);

    delete[] ciphertext;
}

const char *Packer::CompressionTypeToString(PakTypes::CompressionType type) {
    switch (type) {
        case PakTypes::CompressionType::LZ4:
            return "LZ4";
        case PakTypes::CompressionType::ZLIB:
            return "Zlib";
        case PakTypes::CompressionType::ZSTD:
            return "Zstd";
        default:
            return "Unknown";
    }
}
