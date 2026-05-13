import { Authentication, type LoginData } from '@api/users/Authentication';
import { userService } from '@service/UserService';
import { webSocketService } from '@service/WebSocketService';
import { TokenStore } from '@store/TokenStore';

export const AuthService = {
    // 登录
    login: async (loginData: LoginData) => {
        const res = await Authentication.login(loginData);
        // 保存 Token
        if (res.code === 200 && res.data) {
            TokenStore.set(res.data);
            // 启动 自动刷新 用户信息
            userService.startAutoRefresh();
            // 建立 WebSocket 连接
            webSocketService.connect();
        }
        return res;
    },
    // 退出登录
    logout: async (notifyBackend: boolean = true) => {
        if (notifyBackend) try { await Authentication.logout(); } catch (e) { }
        // 先关闭 WebSocket，避免退出后自动重连
        webSocketService.close();
        // 停止 自动刷新 用户信息
        userService.stopAutoRefresh();
        // 清空用户信息
        userService.clear();
        // 清理 token
        TokenStore.remove();

        // 刷新页面
        window.location.reload();
    }
};
