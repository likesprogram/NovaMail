#include <LikeHub/storage/fileSystem/MinioClient.hpp>
#include <aws/core/auth/AWSCredentials.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/HeadObjectRequest.h>
#include <aws/s3/model/HeadBucketRequest.h>
#include <aws/s3/model/CreateBucketRequest.h>
#include <fstream>

namespace LikeHub {
    namespace Storage {
        MinioClient::MinioClient(const std::string& endpoint, const std::string& accessKey, const std::string& secretKey, bool https){
            Aws::InitAPI(m_options);
            Aws::Client::ClientConfiguration config;
            config.endpointOverride = endpoint;
            config.scheme = https ? Aws::Http::Scheme::HTTPS : Aws::Http::Scheme::HTTP;
            config.region = "us-east-1";
            config.verifySSL = false;

            Aws::Auth::AWSCredentials credentials(accessKey, secretKey);

            m_client = std::make_shared<Aws::S3::S3Client>(
                credentials, config, Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never, false
            );
        }

        MinioClient::~MinioClient() {
            Aws::ShutdownAPI(m_options);
        }

        bool MinioClient::Upload(const std::string& bucket, const std::string& object, std::istream& in, std::string& errorMsg) {
            // 检查 bucket 是否存在
            Aws::S3::Model::HeadBucketRequest headRequest;
            headRequest.SetBucket(bucket);
            auto headOutcome = m_client->HeadBucket(headRequest);

            if (!headOutcome.IsSuccess()) {
                // 如果不存在则创建
                Aws::S3::Model::CreateBucketRequest createRequest;
                createRequest.SetBucket(bucket);

                auto createOutcome = m_client->CreateBucket(createRequest);

                if (!createOutcome.IsSuccess()) {
                    errorMsg = "CreateBucket failed: " +
                        createOutcome.GetError().GetMessage();
                    return false;
                }
            }

            // 上传对象
            Aws::S3::Model::PutObjectRequest request;

            request.SetBucket(bucket);
            request.SetKey(object);

            auto body = Aws::MakeShared<Aws::IOStream>("NovaMail-S3Upload", in.rdbuf());

            request.SetBody(body);

            auto outcome = m_client->PutObject(request);

            if (!outcome.IsSuccess()) {
                errorMsg = outcome.GetError().GetMessage();
                return false;
            }

            return true;
        }

        bool MinioClient::Download(const std::string& bucket, const std::string& object, std::ostream& out, std::string& errorMsg) {
            Aws::S3::Model::GetObjectRequest request;

            request.SetBucket(bucket);
            request.SetKey(object);

            auto outcome = m_client->GetObject(request);

            if (!outcome.IsSuccess()) {
                errorMsg = outcome.GetError().GetMessage();
                return false;
            }

            out << outcome.GetResult().GetBody().rdbuf();

            return true;
        }

        bool MinioClient::Remove(const std::string& bucket, const std::string& object, std::string& errorMsg) {
            Aws::S3::Model::DeleteObjectRequest request;

            request.SetBucket(bucket);
            request.SetKey(object);

            auto outcome = m_client->DeleteObject(request);

            if (!outcome.IsSuccess()) {
                errorMsg = outcome.GetError().GetMessage();
                return false;
            }

            return true;
        }

        bool MinioClient::Exists(const std::string& bucket, const std::string& object) {
            Aws::S3::Model::HeadObjectRequest request;

            request.SetBucket(bucket);
            request.SetKey(object);

            auto outcome = m_client->HeadObject(request);

            return outcome.IsSuccess();
        }

        bool MinioClient::Stat(const std::string& bucket, const std::string& object, ObjectStat& stat, std::string& errorMsg) {
            stat = ObjectStat{};
                
            Aws::S3::Model::HeadObjectRequest request;
            request.SetBucket(bucket);
            request.SetKey(object);
                
            auto outcome = m_client->HeadObject(request);
                
            if (!outcome.IsSuccess()) {
                errorMsg = outcome.GetError().GetMessage();
                return false;
            }
        
            const auto& result = outcome.GetResult();
        
            const auto contentLength = result.GetContentLength();
            if (contentLength < 0) {
                errorMsg = "Invalid object content length";
                return false;
            }
        
            stat.exists = true;
            stat.size = static_cast<uint64_t>(contentLength);
            stat.etag = result.GetETag();
            stat.contentType = result.GetContentType();
        
            return true;
        }

        bool MinioClient::GetObjectSize(const std::string& bucket, const std::string& object, uint64_t& size, std::string& errorMsg) {
            size = 0;
        
            ObjectStat stat;
            if (!Stat(bucket, object, stat, errorMsg)) return false;
            size = stat.size;
            return true;
        }
    }
}
