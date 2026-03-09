import axios from 'axios';
import type { AxiosInstance, AxiosRequestConfig, AxiosResponse } from 'axios';

export const baseUrl = 'http://127.0.0.1:9099';
const timeout = 150000;

export interface ApiResponse<T> {
    code: number;
    message: string;
    data: T;
}

// 搜索结果类型
export interface SearchResult<T>  {
    data:T;       // 搜索数据
    participle: string[]; // 分词结果
};

// 创建 axios 实例
const instance: AxiosInstance = axios.create({
    baseURL: baseUrl,
    timeout: timeout,
    headers: {
        'Content-Type': 'application/json'
    }
});

// 请求拦截器
instance.interceptors.request.use(
    (config) => {
        let token = request.getToken();
        if (token) {
            config.headers.Authorization = `Bearer ${token}`;
        }
        return config;
    },
    (error) => {
        return Promise.reject(error);
    }
);

export type SimpleCallback = (chunk: Uint8Array) => Uint8Array | void;

interface StreamOptions {
    data?: any;
    onChunk?: SimpleCallback;
}

// 响应拦截器
instance.interceptors.response.use(
    (response: AxiosResponse<ApiResponse<any>>) => {
        // 处理文件上传和下载
        if (response.config.responseType === 'blob' || 
            String(response.config.headers?.['Content-Type']).includes('multipart/form-data')) {
            return response;
        }

        const res = response.data;
        if (res && typeof res === 'object' && ('code' in res) && 
            (res.code === 200 || res.code === 0)) {
            return response;
        }
        const message = typeof res === 'object' && res !== null && 'message' in res 
            ? res.message 
            : '请求失败';
        return Promise.reject(new Error(message));
    },
    (error) => {
        return Promise.reject(error);
    }
);

// 封装请求方法
const request = {
    getToken():string {
        return localStorage.getItem('token') ?? '';
    },
    setToken(token: string) {
        localStorage.setItem('token', token);
    },
    clearToken() {
        localStorage.removeItem('token');
    },
    // GET 请求
    get<T = any>(url: string, config?: AxiosRequestConfig): Promise<ApiResponse<T>> {
        return instance.get(url, config);
    },
    // POST 请求
    post<T = any>(url: string, data?: any, config?: AxiosRequestConfig): Promise<ApiResponse<T>> {
        return instance.post(url, data, config);
    },
    // PUT 请求
    put<T = any>(url: string, data?: any, config?: AxiosRequestConfig): Promise<ApiResponse<T>> {
        return instance.put(url, data, config);
    },
    // DELETE 请求
    delete<T = any>(url: string, config?: AxiosRequestConfig): Promise<ApiResponse<T>> {
        return instance.delete(url, config);
    },
    // 文件上传
    upload<T = any>(url: string, file: File, params?: Record<string, any>, onProgress?: (progress: number) => void): Promise<ApiResponse<T>> {
        const formData = new FormData();
        formData.append('file', file);
        
        // 添加额外参数
        if (params) {
            for (const key in params) {
                if (params.hasOwnProperty(key)) {
                    formData.append(key, params[key]);
                }
            }
        }
        
        return instance.post(url, formData, {
            headers: {
                'Content-Type': 'multipart/form-data'
            },
            timeout: 0,
            onUploadProgress: (progressEvent) => {
                if (onProgress && progressEvent.total) {
                    const progress = (progressEvent.loaded / progressEvent.total) * 100;
                    onProgress(Math.round(progress));
                }
            }
        });
    },
    // 获取传输进度
    getProgress(sessionId: String): Promise<ApiResponse<ApiResponse<number>>> {
        return instance.post("transmissionProgress/getProgress", {sessionId: sessionId});
    },
    // 文件下载
    download(url: string, filename?: string, params?: Record<string, any>): Promise<void> {
        return instance.get(url, {
            responseType: 'blob',
            params, timeout: 0
        }).then(response => {
            // 从响应头中提取文件名（后端必须设置 Content-Disposition）
            let finalFilename = filename;
            const disposition = response.headers['content-disposition'];
            if (!finalFilename && disposition) {
                const match = disposition.match(/filename[^;=\n]*=((['"]).*?\2|[^;\n]*)/);
                if (match && match[1]) {
                    finalFilename = decodeURIComponent(match[1].replace(/['"]/g, ''));
                }
            }
            if (!finalFilename) finalFilename = 'downloaded-file';
    
            // 创建 blob 链接并下载
            const blob = new Blob([response.data]);
            const downloadUrl = window.URL.createObjectURL(blob);
            const link = document.createElement('a');
            link.href = downloadUrl;
            link.download = finalFilename;
            document.body.appendChild(link);
            link.click();
            document.body.removeChild(link);
            window.URL.revokeObjectURL(downloadUrl);
        }).catch(() => {
            return Promise.reject(new Error('文件下载失败'));
        });
    },
    // 流式请求
    stream(url: string, options: StreamOptions = {}): Promise<Uint8Array> {
        return new Promise<Uint8Array>(async (resolve, reject) => {
            try {
                const response = await fetch(baseUrl + "/" + url, {
                    method: "POST",
                    headers: {
                        "Content-Type": "application/json",
                        "Authorization": `Bearer ${request.getToken() || ""}`
                    },
                    body: JSON.stringify(options.data)
                });

                if (!response.body) {
                    throw new Error("服务器未返回流数据");
                }

                // 创建文本解码器
                const reader = response.body.getReader();
                let buffer = new Uint8Array(0); // 初始空缓冲区
                let offset = 0; // 当前写入位置
                const CHUNK_SIZE = 65536; // 每次扩展的固定大小 (64KB)

                // 读取数据流
                while (true) {
                    const { done, value } = await reader.read();
                    if (done) break;

                    let processedChunk = value; 
                    // 调用 onChunk 回调函数处理返回数据
                    if (options.onChunk) {
                        try {
                            const result = options.onChunk(value);
                            // 处理回调返回的二进制数据
                            processedChunk = result instanceof Uint8Array 
                              ? result 
                              : value;
                        } catch (callbackError) {
                            processedChunk = value;
                        }
                    }

                    // 动态扩容缓冲区
                    const newLength = offset + processedChunk.length;
                    if (newLength > buffer.length) {
                        const newBuffer = new Uint8Array(Math.max(newLength, buffer.length + CHUNK_SIZE));
                        newBuffer.set(buffer);
                        buffer = newBuffer;
                    }

                    // 写入数据
                    buffer.set(processedChunk, offset);
                    offset += processedChunk.length;
                }
                resolve(buffer.slice(0, offset));
            } catch (error) {
                reject(error);
            }
        });
    }
};

export default request; 