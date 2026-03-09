#pragma once
#include <ServiceInstance.hpp>
#include <unordered_map>
#include <shared_mutex>
#include <string>

namespace LikeHub {
    class RegistryStore {
    public:
        RegistryStore();
        ~RegistryStore();
            
    private:
        std::unordered_map<
            std::string, // serviceName
            std::unordered_map<
                std::string, // instanceId
                ServiceInstance*
            >
        > instances; // 注册的服务实例列表，key 是 instanceId

        std::shared_mutex m_mutex;
    };
}
