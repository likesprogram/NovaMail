import request from '@utils/Request';
import type { ApiResponse } from '@utils/Request';
import { baseUrl } from '@utils/Request';

export type FileStatus = 'ACTIVE' | 'DELETED' | 'UPLOADING' | 'ARCHIVED';

export interface UserFile {
    fileId: string;
    userId: string;
    fileName: string;
    fileType: string | null;
    filePath: string;
    parentPath: string | null;
    storageBucket: string;
    fileHash: string;
    fileSize: number;
    version: number;
    fileStatus: FileStatus;
    description: string;
    uploadTime: string;
    updateTime: string;
}

export interface UpdateUserFileData {
    fileId: string;
    fileName: string;
    description: string;
}

export interface UserFileAttachmentAuthorization {
    attachmentId: string;
    mailId: string;
    fileName: string;
    fileType: string | null;
    fileSize: number;
    isInline: boolean;
    fileUrl: string;
    mailSubject: string;
    senderName: string;
    senderEmail: string;
    createTime: string;
}

const normalizeUserFile = (file: UserFile): UserFile => ({
    ...file,
    fileId: String(file.fileId),
    userId: String(file.userId),
    description: file.description ?? '',
});

const normalizeAuthorization = (item: UserFileAttachmentAuthorization): UserFileAttachmentAuthorization => ({
    ...item,
    attachmentId: String(item.attachmentId),
    mailId: String(item.mailId),
    fileType: item.fileType ?? null,
    fileSize: Number(item.fileSize ?? 0),
    isInline: Boolean(item.isInline),
    mailSubject: item.mailSubject ?? '',
    senderName: item.senderName ?? '',
    senderEmail: item.senderEmail ?? '',
});

const buildUrl = (path: string, params: Record<string, string | number>) => {
    const searchParams = new URLSearchParams();
    Object.entries(params).forEach(([key, value]) => searchParams.set(key, String(value)));
    return `${baseUrl}/${path}?${searchParams.toString()}`;
};

export const UserFiles = {
    list: async (): Promise<ApiResponse<UserFile[]>> => {
        const response = await request.post<ApiResponse<UserFile[]>>('userFiles/list');
        return {
            ...response.data,
            data: response.data.data.map(normalizeUserFile),
        };
    },
    upload: async (
        file: File,
        params: { filePath: string; description: string },
        onProgress?: (progress: number) => void
    ): Promise<ApiResponse<UserFile>> => {
        const response = await request.upload<ApiResponse<UserFile>>('userFiles/upload', file, params, onProgress);
        return {
            ...response.data,
            data: normalizeUserFile(response.data.data),
        };
    },
    update: async (data: UpdateUserFileData): Promise<ApiResponse<UserFile>> => {
        const response = await request.post<ApiResponse<UserFile>>('userFiles/update', data);
        return {
            ...response.data,
            data: normalizeUserFile(response.data.data),
        };
    },
    delete: async (fileId: string): Promise<ApiResponse<string>> => {
        const response = await request.post<ApiResponse<string>>('userFiles/delete', { fileId });
        return response.data;
    },
    listAttachmentAuthorizations: async (fileId: string): Promise<ApiResponse<UserFileAttachmentAuthorization[]>> => {
        const response = await request.post<ApiResponse<UserFileAttachmentAuthorization[]>>('userFiles/attachmentAuthorizations', { fileId });
        return {
            ...response.data,
            data: response.data.data.map(normalizeAuthorization),
        };
    },
    revokeAttachmentAuthorization: async (fileId: string, attachmentId: string): Promise<ApiResponse<string>> => {
        const response = await request.post<ApiResponse<string>>('userFiles/revokeAttachmentAuthorization', { fileId, attachmentId });
        return response.data;
    },
    previewBlob: async (fileId: string): Promise<Blob> => {
        const response = await request.get<Blob>('userFiles/preview', {
            responseType: 'blob',
            params: { fileId },
            timeout: 0,
        });
        return response.data;
    },
    downloadBlob: async (fileId: string): Promise<Blob> => {
        const response = await request.get<Blob>('userFiles/download', {
            responseType: 'blob',
            params: { fileId },
            timeout: 0,
        });
        return response.data;
    },
    previewUrl: (fileId: string): string => buildUrl('userFiles/preview', { fileId }),
    downloadUrl: (fileId: string): string => buildUrl('userFiles/download', { fileId }),
    referenceUrl: (fileId: string): string => `user-file:${fileId}`,
    download: async (file: UserFile): Promise<void> => {
        await request.download('userFiles/download', file.fileName, { fileId: file.fileId });
    },
};
