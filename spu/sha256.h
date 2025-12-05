/**
 * Minimal SHA256 header for SPU merge reference
 *
 * In production, use a proper crypto library like OpenSSL or libsodium.
 * This is a placeholder for the reference implementation.
 */

#ifndef SPU_SHA256_H
#define SPU_SHA256_H

#include <cstdint>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>

namespace spu {

/**
 * Compute SHA256 hash and return as hex string
 *
 * @param data Input data buffer
 * @param len Length of data in bytes
 * @param output Output buffer (must be 64 bytes for hex string)
 *
 * Note: This is a placeholder. Link with OpenSSL or similar for production:
 *       SHA256((unsigned char*)data, len, hash_bytes);
 */
inline void sha256_hash(const char* data, size_t len, char* output) {
    // Placeholder: In production, call OpenSSL SHA256
    // For reference implementation, use a simple hash for demonstration

    // This would be replaced with:
    // unsigned char hash[32];
    // SHA256((unsigned char*)data, len, hash);
    // for (int i = 0; i < 32; i++) {
    //     sprintf(output + i*2, "%02x", hash[i]);
    // }

    // Simplified placeholder (NOT cryptographically secure!)
    uint32_t h = 0x6a09e667;  // SHA256 initial value

    for (size_t i = 0; i < len; i++) {
        h = ((h << 5) + h) ^ data[i];  // Simple hash
    }

    // Convert to hex string
    sprintf(output, "%08x%08x%08x%08x%08x%08x%08x%08x",
            h, h ^ 0x12345678, h ^ 0x9abcdef0, h ^ 0xfedcba98,
            h ^ 0x13579bdf, h ^ 0x2468ace0, h ^ 0x87654321, h ^ 0xabcdef01);
}

/**
 * Compute SHA256 hash and return as std::string
 */
inline std::string sha256_string(const std::string& data) {
    char hash_hex[65];
    hash_hex[64] = '\0';
    sha256_hash(data.c_str(), data.length(), hash_hex);
    return std::string(hash_hex);
}

} // namespace spu

#endif // SPU_SHA256_H
