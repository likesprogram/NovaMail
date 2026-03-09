import request from '@utils/Request';
import type { ApiResponse } from '@utils/Request';
import type { ImageCode, SmsCode } from '@api/VerificationCode';

// 登录上传数据
export interface LoginData {
    account: string;        // 账号
    password: string;       // 密码
    imageCode: ImageCode;   // 图片验证码
}
// 注册上传数据
export interface RegisterData {
    account: string;        // 账号
    password: string;       // 密码
    smsCode: SmsCode;       // 短信验证码
}

export const Authentication = {
    // 登录
    login : async (loginData: LoginData): Promise<ApiResponse<string>> => {
        try {
            const response = await request.post<ApiResponse<string>>('authentication/login', loginData);
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    // 登出
    logout : async (): Promise<ApiResponse<void>> => {
        try {
            const response = await request.post<ApiResponse<void>>('authentication/logout');
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    // 注册
    register : async (registerData: RegisterData): Promise<ApiResponse<string>> => {
        try {
            const response = await request.post<ApiResponse<string>>('authentication/register', registerData);
            return response.data;
        } catch (error) {
            throw error;
        }
    },
}