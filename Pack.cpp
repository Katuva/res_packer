#include "Pack.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <string>
#include <algorithm>
#include "lz4hc.h"
#include "zstd.h"
#include "External/miniz/miniz.h"
#include "sodium.h"
#include "PakTypes.h"

bool Pack::CreatePakFile(
    const std::vector<PakTypes::PakFileItem>& files,
    const std::string& targetPath,
    PakTypes::CompressionType compressionType)
{
    std::vector<std::string> fileQueue;

    PakTypes::PakHeader header = {};

    header.NumEntries = 0;

    std::vector<char> dataBuffer;
    std::vector<PakTypes::PakFileTableEntry> fileEntries;

    ZSTD_CCtx* cctx = ZSTD_createCCtx();

    bool hasEncryptedItem = std::any_of(files.begin(), files.end(), [](const PakTypes::PakFileItem& item) {
        return item.encrypted;
    });

    if (hasEncryptedItem)
        Pack::GenerateEncryptionKey();

    std::filesystem::create_directory("temp");

    int index = 1;

    for (const auto& file : files) {
        std::ifstream fileStream(file.path, std::ios::ate | std::ios::binary);

        if (fileStream.fail()) {
            std::cout << "Warning: File not found '" << file.path << "'" << std::endl;
            continue;
        }

        PakTypes::PakFileTableEntry pakFileEntry;
        memset(&pakFileEntry, 0, sizeof(PakTypes::PakFileTableEntry));
        pakFileEntry.OriginalSize = (unsigned int)fileStream.tellg();
        memcpy(pakFileEntry.FilePath, file.packedPath.c_str(), file.packedPath.length() + 1);
        pakFileEntry.Offset = (unsigned int)dataBuffer.size();
        pakFileEntry.Compressed = file.compressed;

        if (pakFileEntry.Compressed) {
            pakFileEntry.CompressionType = compressionType;
        }

        fileStream.seekg(0);

        std::vector<char> fileData(4096);

        if (pakFileEntry.Compressed) {
            std::ofstream tempOutput("temp/temp" + std::to_string(index), std::ios::binary);
            fileQueue.emplace_back("temp/temp" + std::to_string(index));
            std::vector<char> compressedData;

            if (compressionType == PakTypes::CompressionType::ZLIB) {
                mz_ulong compressedSize = mz_compressBound(pakFileEntry.OriginalSize);
                compressedData.resize(compressedSize);
                int result = mz_compress2((unsigned char*)compressedData.data(), &compressedSize, (const unsigned char*)fileData.data(), pakFileEntry.OriginalSize, zlibCompressionLevel);
                if (result != MZ_OK)
                    return false;
                if (!file.encrypted) {
                    pakFileEntry.PackedSize = (unsigned int) compressedSize;
                    dataBuffer.insert(dataBuffer.end(), compressedData.data(), compressedData.data() + compressedSize);
                }
            }
            else if (compressionType == PakTypes::CompressionType::LZ4) {
                size_t compressedSize = LZ4_compressBound(pakFileEntry.OriginalSize);
                compressedData.resize(compressedSize);
                int compressed_size = LZ4_compress_HC(fileData.data(), compressedData.data(), fileData.size(), compressedData.size(), lz4CompressionLevel);
                if (compressed_size <= 0)
                    return false;
                compressedData.resize(compressed_size);
                if (!file.encrypted) {
                    pakFileEntry.PackedSize = (unsigned int) compressed_size;
                    dataBuffer.insert(dataBuffer.end(), compressedData.data(),
                                      compressedData.data() + compressed_size);
                }
            }
            else if (compressionType == PakTypes::CompressionType::ZSTD) {
                compressedData.resize(ZSTD_compressBound(4096));
                while (fileStream) {
                    fileStream.read(fileData.data(), fileData.size());
                    size_t read_size = fileStream.gcount();
                    if (read_size == 0) {
                        break;
                    }

                    size_t compressed_size = ZSTD_compressCCtx(cctx,
                                                               compressedData.data(), compressedData.size(),
                                                               fileData.data(), read_size,
                                                               zstdCompressionLevel);
                    if (ZSTD_isError(compressed_size)) {
                        std::cerr << "Zstd compression error: " << ZSTD_getErrorName(compressed_size) << '\n';
                        ZSTD_freeCCtx(cctx);
                    }

                    tempOutput.write(compressedData.data(), compressed_size);
                }

                if (!file.encrypted) {
                    size_t compressed_size = std::filesystem::file_size("temp/temp" + std::to_string(index));
                    pakFileEntry.PackedSize = compressed_size;
                }
            }
            else {
                std::cout << "Unknown compression type: " << compressionType << std::endl;
                return false;
            }

            if (file.encrypted) {
                Pack::Encrypt(compressedData);
                pakFileEntry.PackedSize = (unsigned int) compressedData.size();
                pakFileEntry.Encrypted = true;
                dataBuffer.insert(dataBuffer.end(), compressedData.data(), compressedData.data() + compressedData.size());
            }

            tempOutput.close();
        }
        else {
            pakFileEntry.PackedSize = pakFileEntry.OriginalSize;

            if (file.encrypted) {
                Pack::Encrypt(fileData);
                pakFileEntry.PackedSize = (unsigned int) fileData.size();
                pakFileEntry.Encrypted = true;
            } else {
                pakFileEntry.PackedSize = pakFileEntry.OriginalSize;
            }

            dataBuffer.insert(dataBuffer.end(), fileData.begin(), fileData.end());
        }

        fileEntries.push_back(pakFileEntry);
        fileStream.close();

        index++;
    }

    std::fill(key, key + sizeof(key), 0);
    std::fill(salt, salt + sizeof(salt), 0);

    header.NumEntries = fileEntries.size();

    std::ofstream output(targetPath, std::ios::binary);

    output.write(reinterpret_cast<char*>(&header), sizeof(PakTypes::PakHeader));

    const unsigned int baseOffset = sizeof(PakTypes::PakHeader) + (unsigned int)fileEntries.size() * sizeof(PakTypes::PakFileTableEntry);

    for (PakTypes::PakFileTableEntry& e : fileEntries) {
        e.Offset += baseOffset;
        output.write(reinterpret_cast<char*>(&e), sizeof(PakTypes::PakFileTableEntry));
    }

    for (auto& file : fileQueue) {
        std::ifstream tempFile(file, std::ios::binary);
        std::vector<char> buffer(4096);

        while (tempFile.read(buffer.data(), 4096) || tempFile.gcount() > 0) {
            output.write(buffer.data(), tempFile.gcount());
        }

        tempFile.close();
    }

    output.close();

    std::filesystem::remove_all("temp");

    return true;
}

void Pack::GenerateEncryptionKey()
{
    randombytes_buf(salt, sizeof salt);

    if (crypto_pwhash(key, sizeof key, password.c_str(), password.size(), salt,
                      encryptionOpsLimit, encryptionMemLimit,
                      crypto_pwhash_ALG_DEFAULT) != 0) {
        throw std::exception("Key derivation failed");
    }
}

void Pack::Encrypt(std::vector<char> &dataBuffer) const
{
    size_t message_len = dataBuffer.size();

    unsigned char nonce[crypto_secretbox_xchacha20poly1305_NONCEBYTES];
    randombytes_buf(nonce, sizeof nonce);

    auto* ciphertext = new unsigned char[message_len + crypto_secretbox_xchacha20poly1305_MACBYTES];

    if (crypto_secretbox_xchacha20poly1305_easy(ciphertext, reinterpret_cast<const unsigned char *>(dataBuffer.data()), message_len, nonce, key) != 0) {
        throw std::exception("Encryption failed");
    }

    dataBuffer.clear();
    dataBuffer.resize(message_len + crypto_secretbox_xchacha20poly1305_NONCEBYTES + crypto_secretbox_xchacha20poly1305_MACBYTES + crypto_pwhash_SALTBYTES);
    std::memcpy(dataBuffer.data(), salt, sizeof salt);
    std::memcpy(dataBuffer.data() + sizeof salt, nonce, sizeof nonce);
    std::memcpy(dataBuffer.data() + sizeof salt + sizeof nonce, ciphertext, message_len + crypto_secretbox_xchacha20poly1305_MACBYTES);

    delete[] ciphertext;
}

const char *Pack::CompressionTypeToString(PakTypes::CompressionType type)
{
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
