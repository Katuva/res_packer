#include "Unpacker.h"
#include <fstream>
#include <string>
#include <filesystem>
#include "lz4hc.h"
#include "zstd.h"
#include "External/miniz/miniz.h"
#include "sodium.h"

std::vector<char> Unpacker::ExtractFileToMemory(PakTypes::PakFile &pakFile, const std::string& filePath) {
    bool hasEncryptedItem = std::any_of(pakFile.FileEntries.begin(), pakFile.FileEntries.end(), [](const PakTypes::PakFileTableEntry& item) {
        return item.Encrypted;
    });

    if (hasEncryptedItem) {
        memcpy(salt, pakFile.Header.Salt, crypto_pwhash_SALTBYTES);

        Unpacker::GenerateEncryptionKey();
    }

    for (int i = 0; i < pakFile.Header.NumEntries; i++) {
        PakTypes::PakFileTableEntry &entry = pakFile.FileEntries[i];
        if (std::string(entry.FilePath) == filePath) {
            pakFile.File.seekg(entry.Offset);

            std::vector<char> dataBuffer(entry.PackedSize);
            pakFile.File.read(dataBuffer.data(), dataBuffer.size());

            if (entry.Encrypted) {
                Decrypt(dataBuffer);
            }

            std::vector<char> buffer;
            if (entry.Compressed) {
                buffer.resize(entry.OriginalSize);
                if (entry.CompressionType == PakTypes::CompressionType::ZLIB) {
                    mz_ulong uncompressedSize = entry.OriginalSize;
                    int result = mz_uncompress((unsigned char*)buffer.data(), &uncompressedSize, (const unsigned char*)dataBuffer.data(), entry.PackedSize);
                    if (result != MZ_OK)
                        throw std::exception("Failed to decompress file");
                }
                else if (entry.CompressionType == PakTypes::CompressionType::LZ4) {
                    int decompressed_size = LZ4_decompress_safe(dataBuffer.data(), buffer.data(), dataBuffer.size(), buffer.size());
                    if (decompressed_size <= 0)
                        throw std::exception("Failed to decompress file");
                }
                else if (entry.CompressionType == PakTypes::CompressionType::ZSTD) {
                    size_t decompressed_size = ZSTD_decompress(buffer.data(), buffer.size(), dataBuffer.data(), dataBuffer.size());
                    if (ZSTD_isError(decompressed_size))
                        throw std::exception("Failed to decompress file");
                }
                else {
                    throw std::invalid_argument("Unknown compression type");
                }
            }
            else {
                buffer.resize(entry.OriginalSize);

                buffer = dataBuffer;
            }

            return buffer;
        }
    }

    throw std::exception("File not found in pak file");
}

void Unpacker::ExtractFileToDisk(PakTypes::PakFile &pakFile, const std::string& outputPath, const std::string& filePath) {
    std::vector<char> buffer = ExtractFileToMemory(pakFile, filePath);
    std::string filename = std::filesystem::path(filePath).filename().string();

    std::ofstream file(outputPath + "/" + filename, std::ios::binary);
    file.write(buffer.data(), buffer.size());
    file.close();
}

PakTypes::PakFile Unpacker::ParsePakFile(const std::string& inputPath)
{
    PakTypes::PakFile file;
    std::ifstream pakFile(inputPath, std::ios::binary);

    file.File = std::move(pakFile);

    if (file.File.fail())
        throw std::exception("Failed to open pak file");

    PakTypes::PakHeader header;
    file.File.read(reinterpret_cast<char*>(&header), sizeof(PakTypes::PakHeader));

    if (std::string(header.ID) != "PAK")
        throw std::exception("PAK file is malformed or corrupted");

    if (header.Version != PakTypes::PAK_FILE_VERSION)
        throw std::exception("Unsupported PAK file version");

    auto* pEntries = new PakTypes::PakFileTableEntry[header.NumEntries];
    file.File.read(reinterpret_cast<char*>(pEntries), sizeof(PakTypes::PakFileTableEntry) * header.NumEntries);

    file.Header = header;
    file.FileEntries = std::vector<PakTypes::PakFileTableEntry>(pEntries, pEntries + header.NumEntries);

    delete[] pEntries;

    return file;
}

void Unpacker::GenerateEncryptionKey()
{
    sodium_init();

    if (crypto_pwhash(key, sizeof key, password.c_str(), password.size(), salt,
                      encryptionOpsLimit, encryptionMemLimit,
                      crypto_pwhash_ALG_DEFAULT) != 0)
    {
        throw std::exception("Key derivation failed");
    }
}

void Unpacker::Decrypt(std::vector<char> &dataBuffer) const
{
    unsigned char nonce[crypto_secretbox_xchacha20poly1305_NONCEBYTES];
    std::memcpy(nonce, dataBuffer.data(), sizeof nonce);

    size_t message_len = dataBuffer.size() - sizeof nonce - crypto_secretbox_xchacha20poly1305_MACBYTES;
    auto* decrypted = new unsigned char[message_len];

    if (crypto_secretbox_xchacha20poly1305_open_easy(decrypted,
                                                     reinterpret_cast<const unsigned char *>(dataBuffer.data()) + sizeof nonce,
                                                     dataBuffer.size() - sizeof nonce,
                                                     nonce,
                                                     key) != 0)
    {
        throw std::exception("Decryption failed");
    }

    dataBuffer.clear();
    dataBuffer.resize(message_len);
    std::memcpy(dataBuffer.data(), decrypted, message_len);

    delete[] decrypted;
}
