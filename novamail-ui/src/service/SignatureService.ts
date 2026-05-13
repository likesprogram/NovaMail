import { ValidateHtml } from '@/utils/Utils';
import { baseUrl, type ApiResponse } from '@utils/Request';
import { Signature } from '@/api/users/Signature';

export const SignatureService = {
    // 创建签名
    createSignature: async (signature: Signature): Promise<ApiResponse<String>> => {
        try {
            return await Signature.createSignature(signature);
        } catch (error) {
            console.error('创建签名失败:', error);
            throw error;
        }
    },
    // 获取签名列表
    getSignatures: async (): Promise<ApiResponse<Signature[]>> => {
        try {
            return await Signature.getSignatures();
        } catch (error) {
            console.error('获取签名列表失败:', error);
            throw error;
        }
    },
    // 获取默认签名
    getDefaultSignature: async (): Promise<ApiResponse<Signature>> => {
        try {
            return await Signature.getDefaultSignature();
        } catch (error) {
            console.error('获取默认签名失败:', error);
            throw error;
        }
    },
    // 设置默认签名
    setDefaultSignature: async (signature: Signature): Promise<ApiResponse<String>> => {
        try {
            return await Signature.setDefaultSignature(signature);
        } catch (error) {
            console.error('设置默认签名失败:', error);
            throw error;
        }
    },
    // 获取签名内容
    getSignatureContent: async (signatureId: string): Promise<ApiResponse<string>> => {
        try {
            return await Signature.getSignatureContent(signatureId);
        } catch (error) {
            console.error('获取签名内容失败:', error);
            throw error;
        }
    },
    // 更新签名
    updateSignature: async (signature: Signature): Promise<ApiResponse<String>> => {
        try {
            return await Signature.updateSignature(signature);
        } catch (error) {
            console.error('更新签名失败:', error);
            throw error;
        }
    },
    // 删除签名
    deleteSignature: async (signature: Signature): Promise<ApiResponse<String>> => {
        try {
            return await Signature.deleteSignature(signature);
        } catch (error) {
            console.error('删除签名失败:', error);
            throw error;
        }
    },
    // 渲染签名内容
    renderSignature: (signature: string, account: string, nickname: string, email: string) => {
        if (!signature) return '';

        const variables: Record<string, string> = {
            nickname: nickname ?? '',
            email: email ?? '',
            avatarUrl: `${baseUrl}/userProfile/avatar?account=${account}`,
            profileCardUrl: `${baseUrl}/userProfile/profilecard?account=${account}`
        };
        
        // 占位符替换
        const content = signature.replace(/{{\s*([\w]+)\s*}}/g, (_, key) => {
            return variables[key] ?? '';
        });

        // 不安全元素或属性清理
        const { valid, clean } = ValidateHtml(content);
        if (!valid) console.warn('签名内容包含不安全的元素或属性，已自动清理');

        return clean;
    }
}