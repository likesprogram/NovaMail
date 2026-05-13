import request from '@utils/Request';
import type { ApiResponse } from '@utils/Request';

// 登录上传数据
export interface UserInfoData {
    userId: string;            // 用户ID（Snowflake，使用string避免精度丢失）
    account: string;           // 账号
    isAdmin: boolean;          // 是否管理员
    status: 'NORMAL' | 'DISABLED' | 'DELETED'; // 账号状态
    nickname: string;          // 用户昵称
    avatar: number;            // 头像索引
    address: string;           // 现居地址
    resume: string;            // 用户简介
    defaultMailBoxId: string;  // 默认发件邮箱ID
    defaultMailBoxView: string;// 默认邮箱显示
    updateTime: string;        // 最后更新时间（ISO时间字符串）
    registerTime: string;      // 注册时间
    lastLoginTime: string;     // 最后登录时间
}

// 历史头像
export interface HistoryAvatar {
    avatarName: string;      // 头像文件名
    avatarBase64: string;    // 头像的Base64字符串
    isCurrent: boolean;      // 是否为当前头像
    uploadTime: string;      // 上传时间（ISO时间字符串）
}

// 用户登录日志
export interface LoginLog {
    logId: string;              // 日志ID，Java Long 建议前端用 string 接收
    account: string | null;     // null 表示用户不存在（撞库/爆破）
    loginTime: string;          // 登录时间，ISO 时间字符串

    // 网络信息
    clientIp: string;
    ipLocation: string;
    networkType: string;
    carrier: string;

    // 设备信息
    userAgent: string;
    deviceType: 'PC' | 'Mobile' | 'Tablet' | string;

    // 系统信息
    osFamily: string;
    osVersion: string;
    osMajor: string;
    osMinor: string;

    // 浏览器信息
    browser: string;
    browserVersion: string;
    browserMajor: string;

    // 设备详细
    deviceBrand: string;
    deviceModel: string;
    deviceCode: string | null;

    // 安全与风控
    isBot: boolean;
    platform: 'Web' | 'Android' | 'iOS' | 'Windows' | 'macOS' | string;
    loginResult: 'SUCCESS' | 'FAIL' | 'LOCKED' | 'EXPIRED' | string;
    failReason: string | null;
    responseTime: number;       // ms
}

export interface UpdatePasswordData {
    oldPassword: string;
    newPassword: string;
    confirmPassword: string;
}

export const UserInfo = {
    // 获取用户信息
    getCurrentInfo : async (): Promise<ApiResponse<UserInfoData>> => {
        try {
            const response = await request.post<ApiResponse<UserInfoData>>('userProfile/getCurrentInfo');
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    // 获取历史头像
    getHistoryAvatars : async (): Promise<ApiResponse<HistoryAvatar[]>> => {
        try {
            const response = await request.post<ApiResponse<HistoryAvatar[]>>('userProfile/getHistoryAvatars');
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    // 更新头像
    updateAvatar: async (params: { fileName?: string; base64?: string }): Promise<ApiResponse<null>> => {
        try {
            const response = await request.post<ApiResponse<null>>('userProfile/updateAvatar', params);
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    // 更新用户昵称
    updateNickname: async (nickname: string): Promise<ApiResponse<null>> => {
        try {
            const response = await request.post<ApiResponse<null>>('userProfile/updateNickname', { nickname });
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    // 更新用户简介
    updateResume: async (resume: string): Promise<ApiResponse<null>> => {
        try {
            const response = await request.post<ApiResponse<null>>('userProfile/updateResume', { resume });
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    // 修改密码
    updatePassword: async (updatePasswordData: UpdatePasswordData): Promise<ApiResponse<null>> => {
        try {
            const response = await request.post<ApiResponse<null>>('user/updatePassword', updatePasswordData);
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    // 获取登录日志
    getLoginLogs: async (account: string): Promise<ApiResponse<LoginLog[]>> => {
        try {
            const response = await request.post<ApiResponse<LoginLog[]>>('userProfile/getLoginLogs', { account });
            return response.data;
        } catch (error) {
            throw error;
        }
    }
}