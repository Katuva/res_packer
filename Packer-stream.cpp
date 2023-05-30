#include "Packer.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <string>
#include "lz4hc.h"
#include "zstd.h"
#include "External/miniz/miniz.h"
#include "External/hash-library/crc32.h"
#include "sodium.h"
#include "PakTypes.h"

bool Packer::CreatePakFile(
    const std::vector<PakTypes::PakFileItem>& files,
    const std::string& targetPath,
    PakTypes::CompressionType compressionType,
    const std::string& password) const
{
    PakTypes::PakHeader header = {};

    header.NumEntries = 0;

    std::vector<char> dataBuffer;
    std::vector<PakTypes::PakFileTableEntry> fileEntries;

    CRC32 crc32;

    ZSTD_CCtx* cctx = ZSTD_createCCtx();

    std::ofstream mainOutput("output_file", std::ios::binary);

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
            std::vector<char> compressedData;

            if (compressionType == PakTypes::CompressionType::ZLIB) {
                mz_ulong compressedSize = mz_compressBound(pakFileEntry.OriginalSize);
                compressedData.resize(compressedSize);
                int result = mz_compress2((unsigned char*)compressedData.data(), &compressedSize, (const unsigned char*)fileData.data(), pakFileEntry.OriginalSize, this->zlibCompressionLevel);
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
                int compressed_size = LZ4_compress_HC(fileData.data(), compressedData.data(), fileData.size(), compressedData.size(), this->lz4CompressionLevel);
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
                std::ofstream output("temp", std::ios::binary);
                while (fileStream) {
                    // Read from the input file
                    fileStream.read(fileData.data(), fileData.size());
                    size_t read_size = fileStream.gcount();
                    if (read_size == 0) {
                        break;
                    }

                    // Compress the data
                    size_t compressed_size = ZSTD_compressCCtx(cctx,
                                                               compressedData.data(), compressedData.size(),
                                                               fileData.data(), read_size,
                                                               this->zstdCompressionLevel);
                    if (ZSTD_isError(compressed_size)) {
                        std::cerr << "Zstd compression error: " << ZSTD_getErrorName(compressed_size) << '\n';
                        ZSTD_freeCCtx(cctx);
                    }

                    // Write the compressed data to the output file
                    output.write(compressedData.data(), compressed_size);
                }

                output.close();

                if (!file.encrypted) {
                    size_t compressed_size = std::filesystem::file_size("temp");
                    pakFileEntry.PackedSize = compressed_size;
                }

                std::ifstream input("temp", std::ios::binary);
                std::vector<char> buffer(4096);

                while (input.read(buffer.data(), 4096) || input.gcount() > 0) {
                    mainOutput.write(buffer.data(), input.gcount());
                }

                input.close();

                std::filesystem::remove("temp");
            }
            else {
                std::cout << "Unknown compression type: " << compressionType << std::endl;
                return false;
            }

            if (file.encrypted) {
                Packer::Encrypt(password, compressedData);
                pakFileEntry.PackedSize = (unsigned int) compressedData.size();
                pakFileEntry.Encrypted = true;
                dataBuffer.insert(dataBuffer.end(), compressedData.data(), compressedData.data() + compressedData.size());
            }

            std::ifstream sourceFile("output_file", std::ios::binary);

            std::vector<char> buffer(4096);
            while (sourceFile.read(buffer.data(), buffer.size()) || sourceFile.gcount() > 0) {
                crc32.add(buffer.data(), sourceFile.gcount());
            }

            memcpy(pakFileEntry.Hash, crc32.getHash().c_str(), 8);
        }
        else {
            pakFileEntry.PackedSize = pakFileEntry.OriginalSize;

            if (file.encrypted) {
                Packer::Encrypt(password, fileData);
                pakFileEntry.PackedSize = (unsigned int) fileData.size();
                pakFileEntry.Encrypted = true;
                memcpy(pakFileEntry.Hash, crc32((const char*)fileData.data(), pakFileEntry.PackedSize).c_str(), 8);
            } else {
                pakFileEntry.PackedSize = pakFileEntry.OriginalSize;
                memcpy(pakFileEntry.Hash, crc32((const char*)fileData.data(), pakFileEntry.OriginalSize).c_str(), 8);
            }

            dataBuffer.insert(dataBuffer.end(), fileData.begin(), fileData.end());
        }

        fileEntries.push_back(pakFileEntry);
        fileStream.close();
    }

    mainOutput.close();

    header.NumEntries = fileEntries.size();

    std::ofstream output(targetPath, std::ios::binary);

    output.write(reinterpret_cast<char*>(&header), sizeof(PakTypes::PakHeader));

    const unsigned int baseOffset = sizeof(PakTypes::PakHeader) + (unsigned int)fileEntries.size() * sizeof(PakTypes::PakFileTableEntry);

    for (PakTypes::PakFileTableEntry& e : fileEntries) {
        e.Offset += baseOffset;
        output.write(reinterpret_cast<char*>(&e), sizeof(PakTypes::PakFileTableEntry));
    }

    std::ifstream sourceFile("output_file", std::ios::binary);
    std::vector<char> buffer(4096);

    while (sourceFile.read(buffer.data(), 4096) || sourceFile.gcount() > 0) {
        output.write(buffer.data(), sourceFile.gcount());
    }

    sourceFile.close();
    output.close();

    std::filesystem::remove("output_file");

    return true;
}

void Packer::Encrypt(const std::string &password, std::vector<char> &dataBuffer) const
{
    if (sodium_init() < 0) {
        throw std::exception("Libsodium initialization failed");
    }

    size_t message_len = dataBuffer.size();

    unsigned char nonce[crypto_secretbox_xchacha20poly1305_NONCEBYTES];
    randombytes_buf(nonce, sizeof nonce);

    unsigned char salt[crypto_pwhash_SALTBYTES];
    randombytes_buf(salt, sizeof salt);

    unsigned char key[crypto_secretbox_xchacha20poly1305_KEYBYTES];
    if (crypto_pwhash(key, sizeof key, password.c_str(), password.size(), salt,
                      this->encryptionOpsLimit, this->encryptionMemLimit,
                      crypto_pwhash_ALG_DEFAULT) != 0) {
        throw std::exception("Key derivation failed");
    }

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

const char *Packer::CompressionTypeToString(PakTypes::CompressionType type)
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
