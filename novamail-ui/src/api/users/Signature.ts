import request from '@utils/Request';
import type { ApiResponse } from '@utils/Request';

// 签名列表
export interface Signature {
    signatureId: string;        // 签名ID
    signatureName: string;      // 签名标题
    signatureContent: string;   // 签名内容
    default: boolean;           // 是否为默认签名
    editable: boolean;          // 是否可编辑
    createTime: string;         // 创建时间（ISO时间字符串）
    updateTime: string;         // 最后更新时间（ISO时间字符串）
}

export const Signature = {
    // 创建签名
    createSignature: async (signature: Signature): Promise<ApiResponse<String>> => {
        try {
            const response = await request.post<ApiResponse<String>>('userSignature/createSignature', {
                signatureName: signature.signatureName,
                signatureContent: signature.signatureContent
            });
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    // 获取签名列表
    getSignatures : async (): Promise<ApiResponse<Signature[]>> => {
        try {
            const response = await request.post<ApiResponse<Signature[]>>('userSignature/getSignatures');
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    // 获取默认签名
    getDefaultSignature: async (): Promise<ApiResponse<Signature>> => {
        try {
            const response = await request.post<ApiResponse<Signature>>('userSignature/getDefaultSignature');
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    // 设置默认签名
    setDefaultSignature: async (signature: Signature): Promise<ApiResponse<String>> => {
        try {
            const response = await request.post<ApiResponse<String>>('userSignature/setDefaultSignature', signature);
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    // 获取签名内容
    getSignatureContent: async (signatureId: string): Promise<ApiResponse<string>> => {
        try {
            const response = await request.post<ApiResponse<string>>('userSignature/getSignatureContent', { signatureId });
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    // 更新签名
    updateSignature: async (signature: Signature): Promise<ApiResponse<String>> => {
        try {
            const response = await request.post<ApiResponse<String>>('userSignature/updateSignature', signature);
            return response.data;
        } catch (error) {
            throw error;
        }
    },
    // 删除签名
    deleteSignature: async (signature: Signature): Promise<ApiResponse<String>> => {
        try {
            const response = await request.post<ApiResponse<String>>('userSignature/deleteSignature', signature);
            return response.data;
        } catch (error) {
            throw error;
        }
    }
}