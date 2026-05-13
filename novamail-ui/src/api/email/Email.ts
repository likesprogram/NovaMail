import { ValidateHtml } from '@utils/Utils';
import request from '@utils/Request';
import type { ApiResponse } from '@utils/Request';

// 文件夹
export interface MailBoxFolder {
    folderId: string;       // 文件夹ID
    floderName: string;     // 文件夹名称
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
    attachmentId: number;
    fileName: string;
    fileType: string;
    fileSize: number;
    isInline: boolean;
    downloadUrl?: string;
}

// 发件
export interface SendMailData {
    mailBoxId: number;          // 发件邮箱ID
    sendName: string;           // 发件人显示名称
    toAddressList: string[];    // 收件人地址列表
    subject: string;            // 邮件主题
    bodyData: string;           // 邮件正文HTML
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