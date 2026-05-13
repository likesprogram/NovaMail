#pragma once
#include <LikeHub/storage/fileSystem/IFileSystem.hpp>
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <string>
#include <memory>
#include <istream>
#include <ostream>

namespace LikeHub {
    namespace Storage {
        class MinioClient : public IFileSystem {
        public:
            MinioClient(const std::string& endpoint, const std::string& accessKey, const std::string& secretKey, bool https = false);
            ~MinioClient();

            bool Upload(const std::string& bucket, const std::string& object, std::istream& in, std::string& error) override;

            bool Download(const std::string& bucket, const std::string& object, std::ostream& out, std::string& error) override;

            bool Remove(const std::string& bucket, const std::string& object, std::string& error) override;

            bool Exists(const std::string& bucket, const std::string& object) override;

            bool Stat(const std::string& bucket, const std::string& object, ObjectStat& stat, std::string& errorMsg) override;

            bool GetObjectSize(const std::string& bucket, const std::string& object, uint64_t& size, std::string& errorMsg) override;
        private:
            Aws::SDKOptions m_options;
            std::shared_ptr<Aws::S3::S3Client> m_client;
        };
    }
}
