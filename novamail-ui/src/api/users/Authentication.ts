import request from '@utils/Request';
import type { ApiResponse } from '@utils/Request';
import type { ImageCode, SmsCode } from '@api/VerificationCode';

export interface LoginData {
    account: string;
    password: string;
    imageCode: ImageCode;
}

export interface LoginTokenData {
    tokenType: string;
    accessToken: string;
    refreshToken: string;
    accessTokenExpiresIn: number;
    refreshTokenExpiresIn: number;
}

export interface RegisterData {
    account: string;
    password: string;
    smsCode: SmsCode;
}

export interface ResetPasswordData {
    account: string;
    password: string;
    smsCode: SmsCode;
}

export const Authentication = {
    login: async (loginData: LoginData): Promise<ApiResponse<LoginTokenData>> => {
        const response = await request.post<ApiResponse<LoginTokenData>>('authentication/login', loginData);
        return response.data;
    },

    refresh: async (refreshToken: string): Promise<ApiResponse<LoginTokenData>> => {
        const response = await request.post<ApiResponse<LoginTokenData>>('authentication/refresh', { refreshToken });
        return response.data;
    },

    revokeRefresh: async (refreshToken: string): Promise<ApiResponse<string>> => {
        const response = await request.post<ApiResponse<string>>('authentication/revokeRefresh', { refreshToken });
        return response.data;
    },

    logout: async (): Promise<ApiResponse<void>> => {
        const response = await request.post<ApiResponse<void>>('authentication/logout');
        return response.data;
    },

    register: async (registerData: RegisterData): Promise<ApiResponse<string>> => {
        const response = await request.post<ApiResponse<string>>('authentication/register', registerData);
        return response.data;
    },

    resetPassword: async (resetPasswordData: ResetPasswordData): Promise<ApiResponse<string>> => {
        const response = await request.post<ApiResponse<string>>('authentication/resetPassword', resetPasswordData);
        return response.data;
    },
};
