import request from '@utils/Request';
import type { ApiResponse } from '@utils/Request';
import type { DomainDkimConfigData, DomainDnsVerificationData, DomainNameData, DomainTlsConfigData } from '@api/email/MailBox';

export interface SystemSettingData {
    settingId: string;
    settingName: 'DefaultAvatar' | 'DefaultSetting' | 'ClientDownload';
    settingValue: string;
    settingType: string;
    createTime: string;
    updateTime: string;
}

export interface SystemClientDownloadInfo {
    fileName: string;
    fileType: string;
    fileSize: number;
    version: string;
    description: string;
    storageBucket: string;
    objectName: string;
    uploadTime: string;
}

export interface SystemServiceLoadInfo {
    cpuPct?: number;
    memPct?: number;
    stressIndicators?: number;
    delayMs?: number;
    cpu?: number;
    mem?: number;
    stress?: number;
    lat?: number;
    load?: number;
    effectiveLoad?: number;
}

export interface SystemServiceMonitorInfo {
    serviceId: string;
    serviceName: string;
    serviceAddress: string;
    lifecycle?: number;
    lifecycleText: string;
    health?: number;
    healthText: string;
    load?: SystemServiceLoadInfo;
    lastUpdateTime: string;
    currentNode?: boolean;
    source?: 'hub' | 'event' | 'cache' | string;
}

export const SystemSetting = {
    getDomainList: async (): Promise<ApiResponse<DomainNameData[]>> => {
        const response = await request.post<ApiResponse<DomainNameData[]>>('system/getDomainList');
        return response.data;
    },
    createDomain: async (domain: Pick<DomainNameData, 'domainName' | 'userId' | 'hasCertificate' | 'hasDKIM' | 'isDefault'>): Promise<ApiResponse<DomainNameData>> => {
        const response = await request.post<ApiResponse<DomainNameData>>('system/createDomain', domain);
        return response.data;
    },
    updateDomain: async (domain: Pick<DomainNameData, 'domainNameId' | 'domainName' | 'userId' | 'hasCertificate' | 'hasDKIM' | 'isDefault'>): Promise<ApiResponse<DomainNameData>> => {
        const response = await request.post<ApiResponse<DomainNameData>>('system/updateDomain', domain);
        return response.data;
    },
    deleteDomain: async (domainNameId: string): Promise<ApiResponse<string>> => {
        const response = await request.post<ApiResponse<string>>('system/deleteDomain', { domainNameId });
        return response.data;
    },
    setDefaultDomain: async (domainNameId: string): Promise<ApiResponse<string>> => {
        const response = await request.post<ApiResponse<string>>('system/setDefaultDomain', { domainNameId });
        return response.data;
    },
    getDomainDkimConfig: async (domainNameId: string): Promise<ApiResponse<DomainDkimConfigData>> => {
        const response = await request.post<ApiResponse<DomainDkimConfigData>>('system/getDomainDkimConfig', { domainNameId });
        return response.data;
    },
    configureDomainTls: async (config: DomainTlsConfigData): Promise<ApiResponse<string>> => {
        const response = await request.post<ApiResponse<string>>('system/configureDomainTls', config);
        return response.data;
    },
    verifyDomainDns: async (domainNameId: string): Promise<ApiResponse<DomainDnsVerificationData>> => {
        const response = await request.post<ApiResponse<DomainDnsVerificationData>>('system/verifyDomainDns', { domainNameId });
        return response.data;
    },
    getDefaultAvatar: async (): Promise<ApiResponse<SystemSettingData>> => {
        const response = await request.post<ApiResponse<SystemSettingData>>('system/getDefaultAvatar');
        return response.data;
    },
    setDefaultAvatar: async (settingValue: string): Promise<ApiResponse<SystemSettingData>> => {
        const response = await request.post<ApiResponse<SystemSettingData>>('system/setDefaultAvatar', { settingValue });
        return response.data;
    },
    uploadDefaultAvatar: async (file: File, onProgress?: (progress: number) => void): Promise<ApiResponse<SystemSettingData>> => {
        const response = await request.upload<ApiResponse<SystemSettingData>>('system/uploadDefaultAvatar', file, undefined, onProgress);
        return response.data;
    },
    getDefaultSetting: async (): Promise<ApiResponse<SystemSettingData>> => {
        const response = await request.post<ApiResponse<SystemSettingData>>('system/getDefaultSetting');
        return response.data;
    },
    setDefaultSetting: async (settingValue: string): Promise<ApiResponse<SystemSettingData>> => {
        const response = await request.post<ApiResponse<SystemSettingData>>('system/setDefaultSetting', { settingValue });
        return response.data;
    },
    uploadClientPackage: async (
        file: File,
        params: { version: string; description: string },
        onProgress?: (progress: number) => void
    ): Promise<ApiResponse<SystemClientDownloadInfo>> => {
        const response = await request.upload<ApiResponse<SystemClientDownloadInfo>>('system/uploadClientPackage', file, params, onProgress);
        return response.data;
    },
    getClientDownloadInfo: async (): Promise<ApiResponse<SystemClientDownloadInfo>> => {
        const response = await request.post<ApiResponse<SystemClientDownloadInfo>>('system/getClientDownloadInfo');
        return response.data;
    },
    downloadClient: async (fileName?: string): Promise<void> => {
        await request.download('system/downloadClient', fileName || 'NovaMail-Client');
    },
    getServiceMonitorList: async (): Promise<ApiResponse<SystemServiceMonitorInfo[]>> => {
        const response = await request.post<ApiResponse<SystemServiceMonitorInfo[]>>('system/getServiceMonitorList');
        return response.data;
    },
};
