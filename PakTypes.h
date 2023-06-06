#pragma once

#include <vector>
#include <string>
#include <fstream>

#include "PackerConfig.h"

#ifdef USE_ENCRYPTION
#include <sodium.h>
#endif

class PakTypes {
public:
    static constexpr auto PAK_FILE_VERSION = 1;
    static constexpr auto CompressionCount = 3;

    enum CompressionType {
        ZLIB,
        LZ4,
        ZSTD
    };

    struct PakHeader {
        char ID[4] = {"PAK"};
        unsigned int Version = PAK_FILE_VERSION;
#ifdef USE_ENCRYPTION
        unsigned char Salt[crypto_pwhash_SALTBYTES];
#else
        unsigned char Salt[0];
#endif
        size_t NumEntries = 0;
    };

    struct PakFileTableEntry {
        char FilePath[255]{};
        bool Compressed = false;
        bool Encrypted = false;
        CompressionType CompressionType{};
        size_t OriginalSize = 0;
        size_t PackedSize = 0;
        size_t Offset = 0;
    };

    struct PakFile {
        PakHeader Header;
        std::vector<PakFileTableEntry> FileEntries;
        std::ifstream File;
    };

    struct PakFileItem {
        std::string name;
        std::string path;
        std::string packedPath;
        size_t size = 0;
        bool compressed = false;
        bool encrypted = false;
    };
};