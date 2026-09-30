import axios from 'axios';
import type { AxiosError, AxiosInstance, AxiosRequestConfig, AxiosResponse } from 'axios';
import { TokenStore } from '@store/TokenStore';
import { AuthService } from '@service/AuthService';

export const baseUrl = 'http://127.0.0.1:9099';
export const wsBaseUrl = 'ws://127.0.0.1:9099';
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

export interface SearchResult<T> {
    data: T;
    participle: string[];
}

export type SimpleCallback = (chunk: Uint8Array) => Uint8Array | void;

interface StreamOptions {
    data?: any;
    onChunk?: SimpleCallback;
}

const instance: AxiosInstance = axios.create({
    baseURL: baseUrl,
    timeout: timeout,
    headers: {
        'Content-Type': 'application/json',
    },
});

let refreshTokenPromise: Promise<boolean> | null = null;

const refreshAccessToken = async (): Promise<boolean> => {
    if (refreshTokenPromise) return refreshTokenPromise;

    const refreshToken = TokenStore.getRefreshToken();
    if (!refreshToken) return false;

    refreshTokenPromise = instance.post<ApiResponse<any>>('authentication/refresh', { refreshToken }, {
        headers: { Authorization: undefined },
    }).then(response => {
        const res = response.data;
        if (res?.code === 401 || res?.code === 4011 || res?.code === 4012 || res?.code === 4013) {
            return false;
        }
        if (res?.code === 200 && res.data?.accessToken && res.data?.refreshToken) {
            TokenStore.setTokenPair(res.data);
            return true;
        }
        return false;
    }).catch(() => false).finally(() => {
        refreshTokenPromise = null;
    });

    return refreshTokenPromise;
};

const retryWithFreshToken = async (config?: AxiosRequestConfig): Promise<AxiosResponse<ApiResponse<any>>> => {
    const refreshed = await refreshAccessToken();
    if (!refreshed || !config) {
        AuthService.logout(false);
        throw new ApiException(401, '未授权，请重新登录');
    }

    config.headers = config.headers || {};
    config.headers.Authorization = `Bearer ${TokenStore.get() || ''}`;
    return instance(config);
};

instance.interceptors.request.use(config => {
    const token = TokenStore.get();
    if (token) config.headers.Authorization = `Bearer ${token}`;
    return config;
}, (error) => Promise.reject(error));

instance.interceptors.response.use((response: AxiosResponse<ApiResponse<any>>) => {
    const newToken = response.headers['x-new-token'];
    if (newToken) TokenStore.setAccessToken(newToken);

    if (response.config.responseType === 'blob' || response.config.responseType === 'arraybuffer') return response;

    const res = response.data;
    if (!res || typeof res !== 'object' || !('code' in res)) return response;
    if (response.config.url?.includes('authentication/refresh')) return response;

    switch (res.code) {
        case 200:
        case 0:
            return response;
        case 401:
        case 4011:
            return retryWithFreshToken(response.config);
        case 4012:
        case 4013:
            AuthService.logout(false);
            return Promise.reject(new ApiException(res.code, res.message || '认证失败'));
        default:
            return Promise.reject(new ApiException(res.code, res.message || '业务错误'));
    }
}, (error: AxiosError) => {
    if (!error.response) return Promise.reject(new ApiException(-1, '网络异常，请检查连接'));
    if (error.config?.url?.includes('authentication/refresh')) {
        AuthService.logout(false);
        return Promise.reject(new ApiException(401, '未授权，请重新登录'));
    }

    const status = error.response.status;
    switch (status) {
        case 400: return Promise.reject(new ApiException(400, '请求参数错误'));
        case 401: return retryWithFreshToken(error.config);
        case 403: return Promise.reject(new ApiException(403, '无权限访问'));
        case 404: return Promise.reject(new ApiException(404, '接口不存在'));
        case 500: return Promise.reject(new ApiException(500, '服务器内部错误'));
        default: return Promise.reject(new ApiException(status, error.message || '请求失败'));
    }
});

const request = {
    setToken(token: string) {
        TokenStore.setAccessToken(token, true);
    },
    clearToken() {
        TokenStore.remove();
    },
    get<T = any>(url: string, config?: AxiosRequestConfig): Promise<ApiResponse<T>> {
        return instance.get(url, config);
    },
    post<T = any>(url: string, data?: any, config?: AxiosRequestConfig): Promise<ApiResponse<T>> {
        return instance.post(url, data, config);
    },
    put<T = any>(url: string, data?: any, config?: AxiosRequestConfig): Promise<ApiResponse<T>> {
        return instance.put(url, data, config);
    },
    delete<T = any>(url: string, config?: AxiosRequestConfig): Promise<ApiResponse<T>> {
        return instance.delete(url, config);
    },
    upload<T = any>(url: string, file: File, params?: Record<string, any>, onProgress?: (progress: number) => void): Promise<ApiResponse<T>> {
        const formData = new FormData();
        formData.append('file', file);

        if (params) {
            for (const key in params) {
                if (params.hasOwnProperty(key)) formData.append(key, params[key]);
            }
        }

        return instance.post(url, formData, {
            headers: { 'Content-Type': 'multipart/form-data' },
            timeout: 0,
            onUploadProgress: (progressEvent) => {
                if (onProgress && progressEvent.total) {
                    const progress = (progressEvent.loaded / progressEvent.total) * 100;
                    onProgress(Math.round(progress));
                }
            },
        });
    },
    getProgress(sessionId: string): Promise<ApiResponse<ApiResponse<number>>> {
        return instance.post('transmissionProgress/getProgress', { sessionId: sessionId });
    },
    download(url: string, filename?: string, params?: Record<string, any>): Promise<void> {
        return instance.get(url, {
            responseType: 'blob',
            params,
            timeout: 0,
        }).then(response => {
            let finalFilename = filename;
            const disposition = response.headers['content-disposition'];
            if (!finalFilename && disposition) {
                const match = disposition.match(/filename[^;=\n]*=((['"]).*?\2|[^;\n]*)/);
                if (match && match[1]) finalFilename = decodeURIComponent(match[1].replace(/['"]/g, ''));
            }
            if (!finalFilename) finalFilename = 'downloaded-file';

            const blob = new Blob([response.data]);
            const downloadUrl = window.URL.createObjectURL(blob);
            const link = document.createElement('a');
            link.href = downloadUrl;
            link.download = finalFilename;
            document.body.appendChild(link);
            link.click();
            document.body.removeChild(link);
            window.URL.revokeObjectURL(downloadUrl);
        }).catch(() => Promise.reject(new Error('文件下载失败')));
    },
    stream(url: string, options: StreamOptions = {}): Promise<Uint8Array> {
        return new Promise<Uint8Array>(async (resolve, reject) => {
            try {
                const response = await fetch(baseUrl + '/' + url, {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json',
                        Authorization: `Bearer ${TokenStore.get() || ''}`,
                    },
                    body: JSON.stringify(options.data),
                });

                if (!response.body) throw new Error('服务端未返回流数据');

                const reader = response.body.getReader();
                let buffer = new Uint8Array(0);
                let offset = 0;
                const chunkSize = 65536;

                while (true) {
                    const { done, value } = await reader.read();
                    if (done) break;

                    let processedChunk = value;
                    if (options.onChunk) {
                        try {
                            const result = options.onChunk(value);
                            processedChunk = result instanceof Uint8Array ? result : value;
                        } catch {
                            processedChunk = value;
                        }
                    }

                    const newLength = offset + processedChunk.length;
                    if (newLength > buffer.length) {
                        const newBuffer = new Uint8Array(Math.max(newLength, buffer.length + chunkSize));
                        newBuffer.set(buffer);
                        buffer = newBuffer;
                    }

                    buffer.set(processedChunk, offset);
                    offset += processedChunk.length;
                }
                resolve(buffer.slice(0, offset));
            } catch (error) {
                reject(error);
            }
        });
    },
};

export default request;
