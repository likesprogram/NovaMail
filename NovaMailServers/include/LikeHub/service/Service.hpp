#pragma once
#include <LikeHub/storage/database/DatabaseConnection.hpp>
#include <LikeHub/storage/database/DatabasePool.hpp>

namespace LikeHub {
    class IService {
    public:
        explicit IService(LikeHub::Storage::DatabasePool& pool);
    protected:
        LikeHub::Storage::DatabasePool& m_dbPool;
    };
}
