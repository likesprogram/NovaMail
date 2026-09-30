import { Authentication, type LoginData } from '@api/users/Authentication';
import { userService } from '@service/UserService';
import { webSocketService } from '@service/WebSocketService';
import { TokenStore } from '@store/TokenStore';

export const AuthService = {
    login: async (loginData: LoginData, options: { autoLogin?: boolean } = {}) => {
        const res = await Authentication.login(loginData);
        if (res.code === 200 && res.data) {
            TokenStore.setTokenPair(res.data, options.autoLogin === true);
            userService.startAutoRefresh();
            webSocketService.connect();
        }
        return res;
    },

    logout: async (notifyBackend: boolean = true) => {
        const refreshToken = TokenStore.getRefreshToken();

        if (notifyBackend) {
            if (refreshToken) try { await Authentication.revokeRefresh(refreshToken); } catch (e) { }
            try { await Authentication.logout(); } catch (e) { }
        }

        webSocketService.close();
        userService.stopAutoRefresh();
        userService.clear();
        TokenStore.remove();

        window.location.reload();
    },

    cancelAutoLogin: async () => {
        const refreshToken = TokenStore.getRefreshToken();
        if (!refreshToken) {
            TokenStore.disablePersistence();
            return;
        }

        try { await Authentication.revokeRefresh(refreshToken); } catch (e) { }
        TokenStore.disablePersistence();
    },
};
