import request from '@utils/Request';
import type { ApiResponse } from '@utils/Request';

// 邮箱数据
export interface MailBoxData {
    mailBoxId: string;          // 邮箱ID
    userId: string;             // 所属用户ID
    domainNameId: string;       // 域名ID
    localPart: string;          // 邮箱地址本地部分
    isOpen: boolean;             // 是否启用：0 禁用，1 启用，2 删除
    createTime: string;         // 创建时间
    updateTime: string;         // 更新时间
    mailBoxView: string;        // 邮箱显示地址
}

export interface DomainNameData {
    domainNameId: string;       // 域名ID
    domainName: string;         // 域名
    userId: string | null;      // 所属用户，null 表示公共域名
    hasCertificate: boolean;    // 是否配置 TLS
    hasDKIM: boolean;           // 是否配置 DKIM
    isDefault: boolean;         // 是否系统默认域名
    updateTime: string;         // 更新时间
    permission: 'PUBLIC' | 'OWNER' | 'USE' | 'ADMIN' | 'NONE' | string;
    editable: boolean;          // 当前用户是否可编辑
}

export interface DomainDkimConfigData {
    domainNameId: string;
    domainName: string;
    selector: string;
    recordType: string;
    recordName: string;
    recordValue: string;
    hasDKIM: boolean;
}

export interface DomainTlsConfigData {
    domainNameId: string;
    privateKey: string;
    certificate: string;
    caChain?: string | null;
}

export interface DomainDnsRecordCheckData {
    key: string;
    label: string;
    host: string;
    recordType: string;
    expectedValue: string;
    actualValue: string;
    passed: boolean;
    message: string;
}

export interface DomainDnsVerificationData {
    domainNameId: string;
    domainName: string;
    passed: boolean;
    records: DomainDnsRecordCheckData[];
}

export interface EmailAuthorizationData {
    emailAuthorizationId: string;
    userId: string;
    authCode: string;
    createTime: string;
    expireTime: string | null;
    isActivated: 0 | 1 | boolean;
    lastLoginTime: string | null;
    loginCount: number;
    clientInfo: string | null;
    updateTime: string;
}

export const MailBoxe = {
    // 登录
    getMailBoxList : async (userId: string): Promise<ApiResponse<MailBoxData[]>> => {
        try {
            const response = await request.post<ApiResponse<MailBoxData[]>>('mailBox/getMailBoxList', { userId: userId });
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    createMailBox: async (localPart: string, domainNameId: string): Promise<ApiResponse<string>> => {
        try {
            const response = await request.post<ApiResponse<string>>('mailBox/createMailBox', { localPart, domainNameId });
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    setOpenMailBox: async (mailBoxId: string, isOpen: 0 | 1): Promise<ApiResponse<string>> => {
        try {
            const response = await request.post<ApiResponse<string>>('mailBox/setOpenMailBox', { mailBoxId, isOpen });
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    setDefaultMailBox: async (mailBoxId: string): Promise<ApiResponse<string>> => {
        try {
            const response = await request.post<ApiResponse<string>>('mailBox/setDefaultMailBox', { mailBoxId });
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    removeMailBox: async (mailBoxId: string): Promise<ApiResponse<string>> => {
        try {
            const response = await request.post<ApiResponse<string>>('mailBox/removeMailBox', { mailBoxId });
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    getAvailableDomainList: async (): Promise<ApiResponse<DomainNameData[]>> => {
        try {
            const response = await request.post<ApiResponse<DomainNameData[]>>('mailBox/getAvailableDomainList');
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    createDomain: async (domain: Pick<DomainNameData, 'domainName'>): Promise<ApiResponse<DomainNameData>> => {
        try {
            const response = await request.post<ApiResponse<DomainNameData>>('mailBox/createDomain', domain);
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    updateDomain: async (domain: Pick<DomainNameData, 'domainNameId' | 'domainName'>): Promise<ApiResponse<DomainNameData>> => {
        try {
            const response = await request.post<ApiResponse<DomainNameData>>('mailBox/updateDomain', domain);
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    deleteDomain: async (domainNameId: string): Promise<ApiResponse<string>> => {
        try {
            const response = await request.post<ApiResponse<string>>('mailBox/deleteDomain', { domainNameId });
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    getDomainDkimConfig: async (domainNameId: string): Promise<ApiResponse<DomainDkimConfigData>> => {
        try {
            const response = await request.post<ApiResponse<DomainDkimConfigData>>('mailBox/getDomainDkimConfig', { domainNameId });
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    configureDomainTls: async (config: DomainTlsConfigData): Promise<ApiResponse<string>> => {
        try {
            const response = await request.post<ApiResponse<string>>('mailBox/configureDomainTls', config);
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    verifyDomainDns: async (domainNameId: string): Promise<ApiResponse<DomainDnsVerificationData>> => {
        try {
            const response = await request.post<ApiResponse<DomainDnsVerificationData>>('mailBox/verifyDomainDns', { domainNameId });
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    getEmailAuthorizationList: async (): Promise<ApiResponse<EmailAuthorizationData[]>> => {
        try {
            const response = await request.post<ApiResponse<EmailAuthorizationData[]>>('mailBox/getEmailAuthorizationList');
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    createEmailAuthorization: async (authorization: Pick<EmailAuthorizationData, 'clientInfo' | 'expireTime' | 'isActivated'>): Promise<ApiResponse<EmailAuthorizationData>> => {
        try {
            const response = await request.post<ApiResponse<EmailAuthorizationData>>('mailBox/createEmailAuthorization', authorization);
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    updateEmailAuthorization: async (authorization: Pick<EmailAuthorizationData, 'emailAuthorizationId' | 'clientInfo' | 'expireTime' | 'isActivated'>): Promise<ApiResponse<EmailAuthorizationData>> => {
        try {
            const response = await request.post<ApiResponse<EmailAuthorizationData>>('mailBox/updateEmailAuthorization', authorization);
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    setEmailAuthorizationActivated: async (emailAuthorizationId: string, isActivated: 0 | 1): Promise<ApiResponse<string>> => {
        try {
            const response = await request.post<ApiResponse<string>>('mailBox/setEmailAuthorizationActivated', { emailAuthorizationId, isActivated });
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    deleteEmailAuthorization: async (emailAuthorizationId: string): Promise<ApiResponse<string>> => {
        try {
            const response = await request.post<ApiResponse<string>>('mailBox/deleteEmailAuthorization', { emailAuthorizationId });
            return response.data;
        } catch (error) {
            throw error;
        }
    },
}
