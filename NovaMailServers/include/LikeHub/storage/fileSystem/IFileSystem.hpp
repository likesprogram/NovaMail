#pragma once
#include <ostream>
#include <istream>
#include <string>

namespace LikeHub  {
    namespace Storage {
         struct ObjectStat {
            uint64_t size{0};
            std::string etag;
            std::string contentType;
            bool exists{false};
        };
        class IFileSystem {
        public:
            virtual ~IFileSystem() = default;

            virtual bool Upload(const std::string& bucket, const std::string& object, std::istream& in, std::string& errorMsg) = 0;

            virtual bool Download(const std::string& bucket, const std::string& object, std::ostream& out, std::string& errorMsg) = 0;

            virtual bool Remove(const std::string& bucket, const std::string& object, std::string& errorMsg) = 0;

            virtual bool Exists(const std::string& bucket, const std::string& object) = 0;

            virtual bool Stat(const std::string& bucket, const std::string& object, ObjectStat& stat, std::string& errorMsg) = 0;

            virtual bool GetObjectSize(const std::string& bucket, const std::string& object, uint64_t& size, std::string& errorMsg) = 0;
        };
    }
}
