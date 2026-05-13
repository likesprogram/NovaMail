#pragma once
#include <curl/curl.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>

namespace LikeHub {
    namespace Storage {
        class CurlPool {
        public:
            static CurlPool& Instance();
            // 禁止拷贝
            CurlPool(const CurlPool&) = delete;
            CurlPool& operator=(const CurlPool&) = delete;

            // 禁止移动
            CurlPool(CurlPool&&) = delete;
            CurlPool& operator=(CurlPool&&) = delete;

            CURL* Acquire();
            void Release(CURL* curl);
            ~CurlPool();
        private:
            CurlPool(size_t maxPoolSize);

            static std::unique_ptr<CurlPool> s_instance;
            static std::once_flag s_initFlag;

            size_t m_maxPoolSize;
            std::vector<CURL*> m_pool;
            std::mutex m_mutex;
        };
    }
}
