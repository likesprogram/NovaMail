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

export const MailBoxe = {
    // 登录
    getMailBoxList : async (userId: string): Promise<ApiResponse<MailBoxData[]>> => {
        try {
            const response = await request.post<ApiResponse<MailBoxData[]>>('mailBox/getMailBoxList', { userId: userId });
            return response.data;
        } catch (error) {
            throw error;
        }
    }
}