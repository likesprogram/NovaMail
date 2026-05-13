#pragma once
#include <cstdint>
#include <cstring>

#if defined(_WIN32)
    #include <winsock2.h>
#else
    #include <arpa/inet.h>
#endif

namespace LikeHub {
    namespace NetEndian {
        // 64-bit endian convert
        inline uint64_t ntohll(uint64_t value) {
        #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            return (static_cast<uint64_t>(ntohl(static_cast<uint32_t>(value))) << 32) | ntohl(static_cast<uint32_t>(value >> 32));
        #else
            return value;
        #endif
        }

        inline uint64_t htonll(uint64_t value) {
        #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            return (static_cast<uint64_t>(htonl(static_cast<uint32_t>(value))) << 32) | htonl(static_cast<uint32_t>(value >> 32));
        #else
            return value;
        #endif
        }

        // Host → Network
        template<typename T>
        inline T HostToNet(T v);
        template<>
        inline uint8_t HostToNet<uint8_t>(uint8_t v) {
            return v;
        }

        template<>
        inline uint16_t HostToNet<uint16_t>(uint16_t v) {
            return htons(v);
        }

        template<>
        inline uint32_t HostToNet<uint32_t>(uint32_t v) {
            return htonl(v);
        }

        template<>
        inline uint64_t HostToNet<uint64_t>(uint64_t v) {
            return htonll(v);
        }

        // Network → Host
        template<typename T>
        inline T NetToHost(T v);
        template<>
        inline uint8_t NetToHost<uint8_t>(uint8_t v) {
            return v;
        }

        template<>
        inline uint16_t NetToHost<uint16_t>(uint16_t v) {
            return ntohs(v);
        }

        template<>
        inline uint32_t NetToHost<uint32_t>(uint32_t v) {
            return ntohl(v);
        }

        template<>
        inline uint64_t NetToHost<uint64_t>(uint64_t v) {
            return ntohll(v);
        }

        // 从 buffer 读取
        template<typename T>
        inline T ReadNet(const uint8_t* p) {
            static_assert(
                std::is_same_v<T, uint8_t>  ||
                std::is_same_v<T, uint16_t> ||
                std::is_same_v<T, uint32_t> ||
                std::is_same_v<T, uint64_t>,
                "ReadNet only supports uint8/16/32/64"
            );

            T v;
            std::memcpy(&v, p, sizeof(T));
            return NetToHost<T>(v);
        }

        // 写入 buffer 自动转网络序
        template<typename T>
        inline void WriteNet(uint8_t* p, T v) {
            static_assert(
                std::is_same_v<T, uint8_t>  ||
                std::is_same_v<T, uint16_t> ||
                std::is_same_v<T, uint32_t> ||
                std::is_same_v<T, uint64_t>,
                "WriteNet only supports uint8/16/32/64"
            );

            T net = HostToNet<T>(v);
            std::memcpy(p, &net, sizeof(T));
        }
    }
}
