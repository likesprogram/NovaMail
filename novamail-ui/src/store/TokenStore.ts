const ACCESS_TOKEN_KEY = 'access_token';
const REFRESH_TOKEN_KEY = 'refresh_token';
const TOKEN_MODE_KEY = 'access_token_mode';

type TokenMode = 'local' | 'session';

export interface StoredTokenPair {
    accessToken: string;
    refreshToken: string;
}

const getStorage = (mode: TokenMode) => mode === 'local' ? localStorage : sessionStorage;
const getOtherStorage = (mode: TokenMode) => mode === 'local' ? sessionStorage : localStorage;

export const TokenStore = {
    set(token: string, persistent?: boolean) {
        this.setAccessToken(token, persistent);
    },

    setAccessToken(token: string, persistent?: boolean) {
        const mode = persistent === undefined ? this.getMode() : (persistent ? 'local' : 'session');
        const storage = getStorage(mode);
        const otherStorage = getOtherStorage(mode);

        storage.setItem(ACCESS_TOKEN_KEY, token);
        storage.setItem(TOKEN_MODE_KEY, mode);
        otherStorage.removeItem(ACCESS_TOKEN_KEY);
        otherStorage.removeItem(TOKEN_MODE_KEY);
    },

    setTokenPair(token: StoredTokenPair, persistent?: boolean) {
        const mode = persistent === undefined ? this.getMode() : (persistent ? 'local' : 'session');
        const storage = getStorage(mode);
        const otherStorage = getOtherStorage(mode);

        storage.setItem(ACCESS_TOKEN_KEY, token.accessToken);
        storage.setItem(REFRESH_TOKEN_KEY, token.refreshToken);
        storage.setItem(TOKEN_MODE_KEY, mode);
        otherStorage.removeItem(ACCESS_TOKEN_KEY);
        otherStorage.removeItem(REFRESH_TOKEN_KEY);
        otherStorage.removeItem(TOKEN_MODE_KEY);
    },

    get(): string | null {
        return localStorage.getItem(ACCESS_TOKEN_KEY) || sessionStorage.getItem(ACCESS_TOKEN_KEY);
    },

    getRefreshToken(): string | null {
        return localStorage.getItem(REFRESH_TOKEN_KEY) || sessionStorage.getItem(REFRESH_TOKEN_KEY);
    },

    getMode(): TokenMode {
        if (localStorage.getItem(ACCESS_TOKEN_KEY) || localStorage.getItem(REFRESH_TOKEN_KEY)) return 'local';
        if (sessionStorage.getItem(ACCESS_TOKEN_KEY) || sessionStorage.getItem(REFRESH_TOKEN_KEY)) return 'session';
        return localStorage.getItem(TOKEN_MODE_KEY) === 'local' ? 'local' : 'session';
    },

    disablePersistence() {
        const accessToken = this.get();

        localStorage.removeItem(ACCESS_TOKEN_KEY);
        localStorage.removeItem(REFRESH_TOKEN_KEY);
        localStorage.removeItem(TOKEN_MODE_KEY);

        if (accessToken) {
            sessionStorage.setItem(ACCESS_TOKEN_KEY, accessToken);
            sessionStorage.setItem(TOKEN_MODE_KEY, 'session');
        }
        sessionStorage.removeItem(REFRESH_TOKEN_KEY);
    },

    remove() {
        localStorage.removeItem(ACCESS_TOKEN_KEY);
        localStorage.removeItem(REFRESH_TOKEN_KEY);
        localStorage.removeItem(TOKEN_MODE_KEY);
        sessionStorage.removeItem(ACCESS_TOKEN_KEY);
        sessionStorage.removeItem(REFRESH_TOKEN_KEY);
        sessionStorage.removeItem(TOKEN_MODE_KEY);
    }
};
