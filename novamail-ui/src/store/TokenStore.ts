const TOKEN_KEY = 'access_token';

export const TokenStore = {
    set(token: string) {
        localStorage.setItem(TOKEN_KEY, token);
    },

    get(): string | null {
        return localStorage.getItem(TOKEN_KEY);
    },

    remove() {
        localStorage.removeItem(TOKEN_KEY);
    }
};