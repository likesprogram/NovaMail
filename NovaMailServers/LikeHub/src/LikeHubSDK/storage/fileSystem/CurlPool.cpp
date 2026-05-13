#include <LikeHub/storage/fileSystem/CurlPool.hpp>
#include <LikesProgram/log/Logger.hpp>

namespace LikeHub {
    namespace Storage {
        std::unique_ptr<CurlPool> CurlPool::s_instance;
        std::once_flag CurlPool::s_initFlag;

        CurlPool& CurlPool::Instance() {
            if (!s_instance) {
                std::call_once(s_initFlag, [&](){
                    LogInfo(u"Initializing CurlPool, size={}", 64);
                    s_instance.reset(new CurlPool(64));
                });
            }

            return *s_instance;
        }

        CurlPool::CurlPool(size_t maxPoolSize): m_maxPoolSize(maxPoolSize) {
            curl_global_init(CURL_GLOBAL_DEFAULT);
        }

        CurlPool::~CurlPool() {
            for(auto c : m_pool) curl_easy_cleanup(c);

            curl_global_cleanup();
        }

        CURL* CurlPool::Acquire() {
            std::lock_guard<std::mutex> lock(m_mutex);

            if(!m_pool.empty())
            {
                CURL* curl = m_pool.back();
                m_pool.pop_back();
                return curl;
            }

            return curl_easy_init();
        }

        void CurlPool::Release(CURL* curl) {
            if(!curl) return;

            curl_easy_reset(curl);

            std::lock_guard<std::mutex> lock(m_mutex);
            
            if(m_pool.size() < m_maxPoolSize) m_pool.push_back(curl);
            else curl_easy_cleanup(curl);
        }
    }
}