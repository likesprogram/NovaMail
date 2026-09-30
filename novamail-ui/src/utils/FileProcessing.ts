export const FileProcessing = {
    /**
     * 打开文件
     * @param {string} accept 文件类型（默认接受所有文件）
     * @param {boolean} multiple 是否允许选择多个文件（默认单选）
     * @param {number} delay 超时时间（默认500ms）（单位为毫秒）
     * @returns {Promise<File | FileList>} 返回一个 Promise，解析为选择的文件或文件列表
     */
    openFiles: async (
        accept: string = '*',
        multiple: boolean = false,
        delay: number = 500 // 默认延迟设置为 500 毫秒
    ): Promise<File | FileList> => {
        return new Promise((resolve, reject) => {
            try {
                // 创建文件输入框
                const input = document.createElement('input');
                input.type = 'file';
                input.accept = accept; // 限制选择文件类型
                input.multiple = multiple; // 是否允许选择多个文件
    
                let fileCanceled = true; // 标记用户是否取消选择
    
                // 当用户选择文件后触发
                input.onchange = (event: Event) => {
                    fileCanceled = false; // 用户有交互
                    const target = event.target as HTMLInputElement;
                    if (target && target.files && target.files.length > 0) {
                        const files = multiple ? target.files : target.files[0];
                        cleanup(); // 移除事件监听
                        resolve(files as File | FileList);
                    } else {
                        cleanup(); // 移除事件监听
                        reject({ error: new Error('用户未选择任何文件'), code: 0 });
                    }
                };
    
                // 检测用户取消选择
                const onFocus = () => {
                    setTimeout(() => {
                        if (fileCanceled) {
                            cleanup(); // 移除事件监听
                            reject({ error: new Error('用户取消选择'), code: 0 });
                        }
                    }, delay); // 延迟以确保 onchange 已被触发
                };
    
                // 清理函数，用于移除事件监听器
                const cleanup = () => {
                    window.removeEventListener('focus', onFocus);
                };
    
                // 触发文件选择框
                input.click();
    
                // 监听 focus 事件以检测“取消”行为
                window.addEventListener('focus', onFocus);
    
            } catch (err) {
                reject({ error: err, code: -1 });
            }
        });
    },
    
    /**
     * 将文件转换为 Base64 数据
     * @param {File} file - 文件对象
     * @returns {Promise<string>} - 返回 Promise，解析为 Base64 数据
     */
    fileToBase64: async (file: File) => {
        return new Promise((resolve, reject) => {
            const reader = new FileReader();
            reader.readAsDataURL(file);
            reader.onload = () => resolve(reader.result);
            reader.onerror = error => reject(error);
        });
    },

    /**
     * 下载文件
     * @param {string} base64 - 文件的 Base64 数据
     * @param {string} fileName - 文件名
     */
    base64ToFile: async (base64: string, fileName:string) => {
        const arr = base64.split(',');
        
        if (!arr[0] || !arr[1]) throw new Error('无效的 Base64 数据格式');
        const mimeMatch = arr[0].match(/:(.*?);/);
        if (!mimeMatch || !mimeMatch[1]) throw new Error('无效的 Base64 数据格式');
        const mime = mimeMatch[1];
        const bstr = atob(arr[1]);  // 将 Base64 部分解码
        let n = bstr.length;
        const u8arr = new Uint8Array(n);

        // 将解码后的字节数据放入 Uint8Array
        while (n--) {
            u8arr[n] = bstr.charCodeAt(n);
        }

        // 创建 Blob 对象
        const blob = new Blob([u8arr], { type: mime });

        // 创建一个临时下载链接
        const link = document.createElement('a');
        link.href = URL.createObjectURL(blob);  // 创建指向 Blob 对象的 URL
        link.download = fileName;  // 设置文件名
        link.click();  // 触发下载
        URL.revokeObjectURL(link.href);  // 释放 URL 对象
    },
    
    /**
     * 获取视频封面
     * @param {File} videoFile - 视频文件
     * @param {number} [time=1] - 获取封面的时间点（默认为 1 秒）
     * @returns {Promise<string>} - 返回 Promise，解析为封面图的 Base64 数据
     */
    getVideoPoster: (videoFile: File, time = 1): Promise<string> => {
        return new Promise((resolve, reject) => {
            const video = document.createElement('video');
            const canvas = document.createElement('canvas');
            const ctx = canvas.getContext('2d');
            
            // 设置 video 元素
            video.src = URL.createObjectURL(videoFile);
            video.load();
            
            video.onloadeddata = () => {
                // 等待视频加载完毕
                video.currentTime = time; // 设置为 1 秒时，稍微延迟一小段时间获取帧
                
                video.onseeked = () => {
                    // 将画布大小设置为视频大小
                    canvas.width = video.videoWidth;
                    canvas.height = video.videoHeight;
    
                    // 在 canvas 上绘制视频的第30帧
                    ctx?.drawImage(video, 0, 0, canvas.width, canvas.height);
    
                    // 获取 canvas 图像的 Base64 格式
                    const posterBase64 = canvas.toDataURL('image/jpeg');
                    resolve(posterBase64); // 返回封面图的 Base64 数据
                };
            };
            
            video.onerror = (err) => reject(err);
        });
    },
    // 获取视频时长
    getVideoDuration(file: File): Promise<number> {
        return new Promise((resolve, reject) => {
            const video = document.createElement('video');
            video.preload = 'metadata';
    
            video.onloadedmetadata = () => {
                window.URL.revokeObjectURL(video.src);
                resolve(video.duration); // 单位：秒
            };
    
            video.onerror = () => {
                reject(new Error("无法加载视频元数据"));
            };
    
            video.src = URL.createObjectURL(file);
        });
    },
    // 计算文件大小
    getFileSize: (file: File | number): string => {
        let size: number;
        if (typeof file === 'number') {
            size = file;
        } else if (file instanceof File) {
            size = file.size;
        } else {
            return '0B';
        }
        const units = ['B', 'KB', 'MB', 'GB', 'TB'];
        let i = 0;
        while (size >= 1024 && i < units.length - 1) {
            size /= 1024;
            i++;
        }
        return `${size.toFixed(2)} ${units[i]}`;
    },
    // 获取文件类型
    getFileType: (file: File | string): string => {
        let fileName: string;
        if (typeof file === 'string') {
            fileName = file;
        } else if (file instanceof File) {
            fileName = file.name;
        } else {
            return 'unknown';
        }
        const lastDotIndex = fileName.lastIndexOf('.');
        if (lastDotIndex === -1) {
            return 'unknown';
        }
        return fileName.substring(lastDotIndex + 1).toLowerCase();
    },
    // 获取文件图标
    getFileIcon: (file: File | string): string => {
        const fileType = FileProcessing.getFileType(file);
        let iconName = 'Misc';
        switch (fileType) {
            case 'pdf':
                iconName = 'PDF';
                break
            case 'doc':
            case 'docx':
                iconName = 'Word';
                break;
            case 'ppt':
            case 'pptx':
                iconName = 'PPT';
                break;
            case 'xls':
            case 'xlsx':
                iconName = 'Excel';
                break;
            case 'txt':
                iconName = 'Text';
                break;
            case 'zip':
            case '7z':
                iconName = 'ZIP_54';
                break;
            case 'rar':
                iconName = 'RAR';
                break;
            case 'jpg':
            case 'jpeg':
            case 'png':
            case 'gif':
            case 'bmp':
            case 'webp':
                iconName = 'Picture';
                break;
            case 'mp4':
            case 'avi':
            case 'mov':
            case 'wmv':
            case 'flv':
                iconName = 'Video';
                break;
            case 'mp3':
            case 'wav':
            case 'aac':
            case 'ogg':
                iconName = 'Music';
                break;
            case 'html':
            case 'htm':
                iconName = 'Web';
                break;
            case 'js':
            case 'ts':
                iconName = 'Code';
                break;
            case 'exe':
                iconName = 'EXE';
                break;
            default:
                iconName = 'Misc'; // 默认图标
                break;
        }

        // 通过 URL 返回图标路径
        try {
          const urlStr = new URL(`../assets/icons/fileIcons/${iconName}.png`, import.meta.url).href;
          return urlStr;
        } catch (e) {
          console.warn('默认图标路径解析失败:', e);
          return ''; // 如果默认图标解析失败，返回空字符串
        }
    }
    
}
