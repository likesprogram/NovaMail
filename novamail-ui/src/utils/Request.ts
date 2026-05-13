import axios from 'axios';
import type { AxiosInstance, AxiosRequestConfig, AxiosResponse, AxiosError } from 'axios';
import { TokenStore } from '@store/TokenStore';
import { AuthService } from '@service/AuthService';

export const baseUrl = 'http://127.0.0.1:9099';
export const wsBaseUrl  = 'ws://127.0.0.1:9099';
const timeout = 150000;

export interface ApiResponse<T> {
    code: number;
    message: string;
    data: T;
}
export class ApiException extends Error {
    code: number;
    details?: any;
    timestamp?: number;
    path?: string;

    constructor(code: number, message: string, details?: any) {
        super(message);
        this.code = code;
        this.details = details;
        this.timestamp = Date.now();
    }
}
export interface ApiError {
    code: number;
    message: string;
    details?: any;
    timestamp?: number;
    path?: string;
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
instance.interceptors.request.use(config => {
    const token = TokenStore.get();
    if (token) config.headers.Authorization = `Bearer ${token}`;
    return config;
}, (error) => {
    return Promise.reject(error);
});

export type SimpleCallback = (chunk: Uint8Array) => Uint8Array | void;

interface StreamOptions {
    data?: any;
    onChunk?: SimpleCallback;
}

// 响应拦截器
instance.interceptors.response.use((response: AxiosResponse<ApiResponse<any>>) => {
    // Token 刷新 (axios 会把 header key 全转小写)
    const newToken = response.headers['x-new-token'];
    if (newToken) TokenStore.set(newToken);

    // 文件下载直通
    if (response.config.responseType === 'blob' || response.config.responseType === 'arraybuffer') return response;

    const res = response.data;

    // 非标准响应直接放行
    if (!res || typeof res !== 'object' || !('code' in res)) return response;

    // 业务状态处理
    switch (res.code) {
        case 200:
        case 0: return response;
        case 401:
        case 4011:
        case 4012:
        case 4013: {
            AuthService.logout(false);
            return Promise.reject(new ApiException(res.code, res.message || '认证失败'));
        }
        default: return Promise.reject(new ApiException(res.code, res.message || '业务错误'));
    }
}, (error: AxiosError) => {
    // 网络层错误
    if (!error.response) return Promise.reject(new ApiException(-1, '网络异常，请检查连接'));

    // HTTP 状态码处理
    const status = error.response.status;

    switch (status) {
        case 400: return Promise.reject(new ApiException(400, '请求参数错误'));
        case 401: {
            AuthService.logout(false);
            return Promise.reject(new ApiException(401, '未授权，请重新登录'));
        }
        case 403: return Promise.reject(new ApiException(403, '无权限访问'));
        case 404: return Promise.reject(new ApiException(404, '接口不存在'));
        case 500: return Promise.reject(new ApiException(500, '服务器内部错误'));
        default: return Promise.reject(new ApiException(status, error.message || '请求失败'));
    }
});

// 封装请求方法
const request = {
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
                        "Authorization": `Bearer ${TokenStore.get() || ""}`
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