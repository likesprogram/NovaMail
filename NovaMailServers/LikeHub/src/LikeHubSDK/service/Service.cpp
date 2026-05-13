#include <LikeHub/service/Service.hpp>

namespace LikeHub {
    IService::IService(LikeHub::Storage::DatabasePool& pool): m_dbPool(pool) { }
}
