import { MailBoxe, type MailBoxData } from '@api/email/MailBox';

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
};
