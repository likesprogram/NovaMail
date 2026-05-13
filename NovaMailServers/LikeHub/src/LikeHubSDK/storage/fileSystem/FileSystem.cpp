#include <LikeHub/storage/fileSystem/FileSystem.hpp>
#include <LikesProgram/log/Logger.hpp>

namespace LikeHub  {
    namespace Storage {
        std::unique_ptr<FileSystem> FileSystem::s_instance;
        std::once_flag FileSystem::s_initFlag;

        FileSystem& FileSystem::Instance() {
            if (!s_instance) {
                std::call_once(s_initFlag, []() {
                    LogInfo(u"Initializing FileSystem service");
                    s_instance.reset(new FileSystem(nullptr));
                });
            }

            return *s_instance;
        }

        FileSystem::FileSystem(std::shared_ptr<IFileSystem> iFileSystem) : m_iFileSystem(std::move(iFileSystem)) {}

        void FileSystem::Initialize(std::shared_ptr<IFileSystem> iFileSystem) {
            if (!s_instance) {
                LogFatal(u"FileSystem not initialized. Call Initialize() first.");
                return;
            }
            
            if (!iFileSystem) {
                LogError(u"FileSystem initialization failed: IFileSystem is null");
                return;
            }

            if (m_iFileSystem) {
                LogWarn(u"FileSystem already initialized, ignoring reinitialization request");
                return;
            }

            LogInfo(u"FileSystem backend registered");
            m_iFileSystem = std::move(iFileSystem);
        }

        IFileSystem* FileSystem::GetIFileSystem() {
            if (!m_iFileSystem) {
                LogError(u"FileSystem requested before initialization");
                return nullptr;
            }

            return m_iFileSystem.get();
        }
    }
}
