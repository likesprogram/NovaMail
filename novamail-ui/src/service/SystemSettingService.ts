import { SystemSetting, type SystemClientDownloadInfo, type SystemServiceMonitorInfo, type SystemSettingData } from '@api/SystemSetting';
import type { DomainDkimConfigData, DomainDnsVerificationData, DomainNameData, DomainTlsConfigData } from '@api/email/MailBox';

export const SystemSettingService = {
    getDomainList: async (): Promise<DomainNameData[]> => {
        const response = await SystemSetting.getDomainList();
        return response.data;
    },
    createDomain: async (domain: Pick<DomainNameData, 'domainName' | 'userId' | 'hasCertificate' | 'hasDKIM' | 'isDefault'>): Promise<DomainNameData> => {
        const response = await SystemSetting.createDomain(domain);
        return response.data;
    },
    updateDomain: async (domain: Pick<DomainNameData, 'domainNameId' | 'domainName' | 'userId' | 'hasCertificate' | 'hasDKIM' | 'isDefault'>): Promise<DomainNameData> => {
        const response = await SystemSetting.updateDomain(domain);
        return response.data;
    },
    deleteDomain: async (domainNameId: string): Promise<string> => {
        const response = await SystemSetting.deleteDomain(domainNameId);
        return response.data;
    },
    setDefaultDomain: async (domainNameId: string): Promise<string> => {
        const response = await SystemSetting.setDefaultDomain(domainNameId);
        return response.data;
    },
    getDomainDkimConfig: async (domainNameId: string): Promise<DomainDkimConfigData> => {
        const response = await SystemSetting.getDomainDkimConfig(domainNameId);
        return response.data;
    },
    configureDomainTls: async (config: DomainTlsConfigData): Promise<string> => {
        const response = await SystemSetting.configureDomainTls(config);
        return response.data;
    },
    verifyDomainDns: async (domainNameId: string): Promise<DomainDnsVerificationData> => {
        const response = await SystemSetting.verifyDomainDns(domainNameId);
        return response.data;
    },
    getDefaultAvatar: async (): Promise<SystemSettingData> => {
        const response = await SystemSetting.getDefaultAvatar();
        return response.data;
    },
    setDefaultAvatar: async (settingValue: string): Promise<SystemSettingData> => {
        const response = await SystemSetting.setDefaultAvatar(settingValue);
        return response.data;
    },
    uploadDefaultAvatar: async (file: File, onProgress?: (progress: number) => void): Promise<SystemSettingData> => {
        const response = await SystemSetting.uploadDefaultAvatar(file, onProgress);
        return response.data;
    },
    getDefaultSetting: async (): Promise<SystemSettingData> => {
        const response = await SystemSetting.getDefaultSetting();
        return response.data;
    },
    setDefaultSetting: async (settingValue: string): Promise<SystemSettingData> => {
        const response = await SystemSetting.setDefaultSetting(settingValue);
        return response.data;
    },
    uploadClientPackage: async (
        file: File,
        params: { version: string; description: string },
        onProgress?: (progress: number) => void
    ): Promise<SystemClientDownloadInfo> => {
        const response = await SystemSetting.uploadClientPackage(file, params, onProgress);
        return response.data;
    },
    getClientDownloadInfo: async (): Promise<SystemClientDownloadInfo> => {
        const response = await SystemSetting.getClientDownloadInfo();
        return response.data;
    },
    downloadClient: async (fileName?: string): Promise<void> => {
        await SystemSetting.downloadClient(fileName);
    },
    getServiceMonitorList: async (): Promise<SystemServiceMonitorInfo[]> => {
        const response = await SystemSetting.getServiceMonitorList();
        return response.data;
    },
    getServiceMonitorListResponse: async () => {
        return await SystemSetting.getServiceMonitorList();
    },
};
