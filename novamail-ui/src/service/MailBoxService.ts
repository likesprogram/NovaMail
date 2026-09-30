import { MailBoxe, type DomainDkimConfigData, type DomainDnsVerificationData, type DomainNameData, type DomainTlsConfigData, type EmailAuthorizationData, type MailBoxData } from '@api/email/MailBox';

export const MailBoxService = {
    // 创建签名
    getMailBoxList: async (userId: string): Promise<MailBoxData[]> => {
        try {
            const response = await MailBoxe.getMailBoxList(userId);
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    createMailBox: async (localPart: string, domainNameId: string): Promise<string> => {
        try {
            const response = await MailBoxe.createMailBox(localPart, domainNameId);
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    setOpenMailBox: async (mailBoxId: string, isOpen: 0 | 1): Promise<string> => {
        try {
            const response = await MailBoxe.setOpenMailBox(mailBoxId, isOpen);
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    setDefaultMailBox: async (mailBoxId: string): Promise<string> => {
        try {
            const response = await MailBoxe.setDefaultMailBox(mailBoxId);
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    removeMailBox: async (mailBoxId: string): Promise<string> => {
        try {
            const response = await MailBoxe.removeMailBox(mailBoxId);
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    getAvailableDomainList: async (): Promise<DomainNameData[]> => {
        try {
            const response = await MailBoxe.getAvailableDomainList();
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    createDomain: async (domain: Pick<DomainNameData, 'domainName'>): Promise<DomainNameData> => {
        try {
            const response = await MailBoxe.createDomain(domain);
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    updateDomain: async (domain: Pick<DomainNameData, 'domainNameId' | 'domainName'>): Promise<DomainNameData> => {
        try {
            const response = await MailBoxe.updateDomain(domain);
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    deleteDomain: async (domainNameId: string): Promise<string> => {
        try {
            const response = await MailBoxe.deleteDomain(domainNameId);
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    getDomainDkimConfig: async (domainNameId: string): Promise<DomainDkimConfigData> => {
        try {
            const response = await MailBoxe.getDomainDkimConfig(domainNameId);
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    configureDomainTls: async (config: DomainTlsConfigData): Promise<string> => {
        try {
            const response = await MailBoxe.configureDomainTls(config);
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    verifyDomainDns: async (domainNameId: string): Promise<DomainDnsVerificationData> => {
        try {
            const response = await MailBoxe.verifyDomainDns(domainNameId);
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    getEmailAuthorizationList: async (): Promise<EmailAuthorizationData[]> => {
        try {
            const response = await MailBoxe.getEmailAuthorizationList();
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    createEmailAuthorization: async (authorization: Pick<EmailAuthorizationData, 'clientInfo' | 'expireTime' | 'isActivated'>): Promise<EmailAuthorizationData> => {
        try {
            const response = await MailBoxe.createEmailAuthorization(authorization);
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    updateEmailAuthorization: async (authorization: Pick<EmailAuthorizationData, 'emailAuthorizationId' | 'clientInfo' | 'expireTime' | 'isActivated'>): Promise<EmailAuthorizationData> => {
        try {
            const response = await MailBoxe.updateEmailAuthorization(authorization);
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    setEmailAuthorizationActivated: async (emailAuthorizationId: string, isActivated: 0 | 1): Promise<string> => {
        try {
            const response = await MailBoxe.setEmailAuthorizationActivated(emailAuthorizationId, isActivated);
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    deleteEmailAuthorization: async (emailAuthorizationId: string): Promise<string> => {
        try {
            const response = await MailBoxe.deleteEmailAuthorization(emailAuthorizationId);
            return response.data;
        } catch (error) {
            throw error;
        }
    },
};
