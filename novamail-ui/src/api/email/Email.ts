import { ValidateHtml } from '@utils/Utils';
import request from '@utils/Request';
import type { ApiResponse } from '@utils/Request';

// 文件夹
export interface MailBoxFolder {
    folderId: string | number;       // 文件夹ID
    folderName?: string;     // 文件夹名称
    floderName?: string;     // 兼容旧拼写
    noReadCont: number;     // 未读邮件数量
}

// 邮件预览
export type MailMappingStatus = 'NORMAL' | 'QUEUED' | 'SENDING' | 'SENT' | 'FAILED';
export interface MailBoxMailMapping {
    select?: boolean;
    mappingId: number;
    mailBoxId: number;
    mailId: number;
    fromName: string;
    fromMail: string;
    subject: string;
    preview: string;
    status: MailMappingStatus;
    folderId: number;
    read: boolean;
    starred: boolean;
    answered: boolean;
    createTime: string;
    updateTime: string;
}

// 邮箱信息
export interface MailAddressInfo {
    name: string;
    address: string
}

// 邮件内容
export interface MailDetailResponse {
    mappingId: number;
    mailId: number;
    folderId?: number;
    from: MailAddressInfo;
    tos: MailAddressInfo[];
    ccs: MailAddressInfo[];
    subject: string;
    preview: string;
    htmlBody?: string;
    textBody?: string;
    isRead: boolean;
    isStarred: boolean;
    isAnswered: boolean;
    createTime: string;
    attachments: MailAttachment[];
}

// 附件
export interface MailAttachment {
    attachmentId?: number | null;
    fileName: string;
    fileType: string;
    fileSize: number;
    isInline: boolean;
    downloadUrl?: string;
    partIndex?: number;
    sourceType?: 'DATABASE' | 'MIME_PART' | string;
    mappingId?: number;
}

export interface SendMailAttachment {
    fileName: string;
    fileType: string;
    fileSize: number;
    fileData: string;
    sendType: 'MIME_ATTACHMENT' | 'TRANSFER_LINK';
}

// 发件
export interface SendMailData {
    mailBoxId: number;          // 发件邮箱ID
    sendName: string;           // 发件人显示名称
    toAddressList: string[];    // 收件人地址列表
    ccAddressList: string[];
    bccAddressList: string[];
    subject: string;            // 邮件主题
    bodyData: string;           // 邮件正文HTML
    attachments?: SendMailAttachment[];
    replySourceMappingId?: number | null;
}

export interface SaveDraftData {
    draftMappingId?: number | null;
    mailBoxId: number;
    sendName: string;
    toAddressList: string[];
    ccAddressList: string[];
    bccAddressList: string[];
    subject: string;
    bodyData: string;
    attachments?: SendMailAttachment[];
}

export interface SaveDraftResponse {
    mappingId: number;
    mailId: number;
    mailBoxId: number;
    folderId: number;
    imapUid: number;
}

export interface DraftDetailResponse {
    mappingId: number;
    mailId: number;
    mailBoxId: number;
    sendName: string;
    sendMailBoxView: string;
    subject: string;
    bodyData: string;
    toAddressList: string[];
    ccAddressList: string[];
    bccAddressList: string[];
    attachments: SendMailAttachment[];
}

// 搜索框相关
export interface SearchSuggestion {
    id: number;
    type: 'subject' | 'sender' | 'email' | 'keyword';
    text: string;
    subText?: string;
    mailId?: number;
    mappingId?: number;
}

export interface MailStatusUpdateData {
    mappingId: number;
    data: boolean;
}

export interface MailMoveData {
    mappingId: number;
    targetFolderId: number;
}

const createDownloadLink = (blob: Blob, fileName: string): void => {
    const downloadUrl = window.URL.createObjectURL(blob);
    const link = document.createElement('a');
    link.href = downloadUrl;
    link.download = fileName;
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
    window.URL.revokeObjectURL(downloadUrl);
};

export const Email = {
    // 发送邮件
    sendMail: async (data: SendMailData): Promise<ApiResponse<string>> => {
        try {
            const response = await request.post<ApiResponse<string>>('email/sendMail', data);
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    saveDraft: async (data: SaveDraftData): Promise<ApiResponse<SaveDraftResponse>> => {
        try {
            const response = await request.post<ApiResponse<SaveDraftResponse>>('email/saveDraft', data);
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    getDraft: async (mappingId: number): Promise<ApiResponse<DraftDetailResponse>> => {
        try {
            const response = await request.post<ApiResponse<DraftDetailResponse>>('email/getDraft', { mappingId });
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    // 获取邮件文件夹
    getFolderList : async (): Promise<ApiResponse<MailBoxFolder[]>> => {
        try {
            const response = await request.post<ApiResponse<MailBoxFolder[]>>('email/getFolderList');
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    // 获取邮件列表
    getEmailList : async (mailBoxFolder: MailBoxFolder): Promise<ApiResponse<MailBoxMailMapping[]>> => {
        try {
            const response = await request.post<ApiResponse<MailBoxMailMapping[]>>('email/getEmailList',  {
                folderId: mailBoxFolder.folderId
            });
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    // 获取邮件内容
    getMailDetail: async (mappingId: number): Promise<ApiResponse<MailDetailResponse>> => {
        try {
            const response = await request.post<ApiResponse<MailDetailResponse>>('email/getMailDetail', { mappingId });
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    setRead: async (data: MailStatusUpdateData): Promise<ApiResponse<void>> => {
        try {
            const response = await request.post<ApiResponse<void>>('email/setRead', data);
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    setStarred: async (data: MailStatusUpdateData): Promise<ApiResponse<void>> => {
        try {
            const response = await request.post<ApiResponse<void>>('email/setStarred', data);
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    setAnswered: async (data: MailStatusUpdateData): Promise<ApiResponse<void>> => {
        try {
            const response = await request.post<ApiResponse<void>>('email/setAnswered', data);
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    moveMail: async (data: MailMoveData): Promise<ApiResponse<void>> => {
        try {
            const response = await request.post<ApiResponse<void>>('email/move', data);
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    deleteMail: async (mappingId: number): Promise<ApiResponse<void>> => {
        try {
            const response = await request.post<ApiResponse<void>>('email/delete', { mappingId, data: true });
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    notTrash: async (mappingId: number): Promise<ApiResponse<void>> => {
        try {
            const response = await request.post<ApiResponse<void>>('email/notrash', { mappingId, data: true });
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    noAdvertisement: async (mappingId: number): Promise<ApiResponse<void>> => {
        try {
            const response = await request.post<ApiResponse<void>>('email/noadvertisement', { mappingId, data: true });
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    downloadAttachment: async (attachment: MailAttachment): Promise<void> => {
        if (attachment.sourceType === 'MIME_PART') {
            if (!attachment.mappingId || attachment.partIndex == null) throw new Error('附件缺少下载索引');
            const response = await request.post<Blob>('email/downloadParsedAttachment', {
                mappingId: attachment.mappingId,
                partIndex: attachment.partIndex,
            }, {
                responseType: 'blob',
                timeout: 0,
            });
            createDownloadLink(new Blob([response.data]), attachment.fileName);
            return;
        }
        if (!attachment.attachmentId) throw new Error('附件缺少下载索引');
        const response = await request.post<Blob>('email/downloadAttachment', { attachmentId: attachment.attachmentId }, {
            responseType: 'blob',
            timeout: 0,
        });
        createDownloadLink(new Blob([response.data]), attachment.fileName);
    },
    attachmentBlob: async (attachment: MailAttachment): Promise<Blob> => {
        if (attachment.sourceType === 'MIME_PART') {
            if (!attachment.mappingId || attachment.partIndex == null) throw new Error('附件缺少下载索引');
            const response = await request.post<Blob>('email/previewParsedAttachment', {
                mappingId: attachment.mappingId,
                partIndex: attachment.partIndex,
            }, {
                responseType: 'blob',
                timeout: 0,
            });
            return response.data;
        }
        if (!attachment.attachmentId) throw new Error('附件缺少下载索引');
        const response = await request.post<Blob>('email/previewAttachment', { attachmentId: attachment.attachmentId }, {
            responseType: 'blob',
            timeout: 0,
        });
        return response.data;
    },
    // 渲染邮件
    renderMailDetail: (mailDetail: string) => {
        // 不安全元素或属性清理
        const { valid, clean } = ValidateHtml(mailDetail);
        if (!valid) console.warn('邮件内容包含不安全的元素或属性，已自动清理');
        return clean;
    },
    // 搜索匹配提示
    getSearchSuggestions: async (keyword: string): Promise<ApiResponse<SearchSuggestion[]>> => {
        try {
            const response = await request.get<ApiResponse<SearchSuggestion[]>>('/email/search/suggest',  { params: {
                keyword,
                limit: 8,
            }});
            return response.data;
        } catch (error) {
            throw error;
        }
    },
}
