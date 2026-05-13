#pragma once
#include <LikeHub/service/Service.hpp>
#include <string>

namespace NovaMail {
    namespace Imap::Service {
        class DomainNameService : public LikeHub::IService {
        public:
            explicit DomainNameService(LikeHub::Storage::DatabasePool& pool);

            std::string GetPrivateKey(uint64_t domainNameId);
        };
    }
}
