import request from '@utils/Request';
import type { ApiResponse } from '@utils/Request';
// 图片验证码
export interface ImageCode {
    code: string;       // 图片验证码
    sessionId: string;  // 验证码会话ID
}

// 短信验证码
export interface SmsCode {
    phoneNumber: string;  // 手机号
    code: string;         // 短信验证码
    imageCode: ImageCode; // 图片验证码
}

export const VerificationCode = {
	// 获取图片验证码
	getImageCode : async (): Promise<ApiResponse<ImageCode>> => {
		try {
			const response = await request.post<ApiResponse<ImageCode>>('verification/image');
			return response.data;
		} catch (error) {
			throw error;
		}
	},
    // 获取短信验证码
    getSmsCode : async (smsCode: SmsCode): Promise<ApiResponse<SmsCode>> => {
        try {
            console.log(smsCode);
            const response = await request.post<ApiResponse<SmsCode>>('verification/sms', smsCode);
            return response.data;
        } catch (error) {
            throw error;
        }
    },
}
