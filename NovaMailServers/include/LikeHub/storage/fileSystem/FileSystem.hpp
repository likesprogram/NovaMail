#pragma once
#include <LikeHub/storage/fileSystem/IFileSystem.hpp>
#include <condition_variable>
#include <memory>
#include <mutex>


namespace LikeHub  {
    namespace Storage {
        class FileSystem {
        public:
            // 获取单例
            static FileSystem& Instance();            
            // 禁止拷贝
            FileSystem(const FileSystem&) = delete;
            FileSystem& operator=(const FileSystem&) = delete;

            // 禁止移动
            FileSystem(FileSystem&&) = delete;
            FileSystem& operator=(FileSystem&&) = delete;

            void Initialize(std::shared_ptr<IFileSystem> iFileSystem);

            // 获取
            IFileSystem* GetIFileSystem();
        private:
            FileSystem(std::shared_ptr<IFileSystem> iFileSystem);
        private:
            static std::unique_ptr<FileSystem> s_instance;
            static std::once_flag s_initFlag;
            std::shared_ptr<IFileSystem> m_iFileSystem;
        };
    }
}

