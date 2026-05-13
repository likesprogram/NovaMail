import { TokenStore } from '@store/TokenStore';
import { webSocketService } from '@service/WebSocketService';
import { UserInfo, type LoginLog, type UpdatePasswordData, type UserInfoData } from '@api/users/UserInfo';

class UserService {
    private userInfo: UserInfoData | null = null; // 缓存内容
    private lastFetchTime = 0; // 上次获取时间
    private cacheDuration = 30 * 60 * 1000; // 30 分钟缓存
    private refreshTimer: number | null = null; // 刷新计时器

    private currentRequest: Promise<UserInfoData | null> | null = null;
    private listeners: ((user: UserInfoData | null) => void)[] = []; // 订阅监听列表

    // 加入监听列表
    public subscribe(cb: (user: UserInfoData | null) => void) { this.listeners.push(cb) }
    // 通知 订阅者
    private notify() { for (const cb of this.listeners) cb(this.userInfo) }

    constructor() {
        console.log('UserService init', this);
    }

    // 内部获取用户信息
    private async fetchUser(force = false): Promise<UserInfoData | null> {
        if (!TokenStore.get()) return null;

        const now = Date.now();

        // 判断缓存是否有效
        const isValid = this.userInfo && (now - this.lastFetchTime < this.cacheDuration);
        if (!force && isValid) return this.userInfo;

        // 放并发
        if (this.currentRequest) return this.currentRequest;

        this.currentRequest = (async () => {
            try {
                const res = await UserInfo.getCurrentInfo();

                this.userInfo = res.data;
                this.lastFetchTime = Date.now();
                this.notify(); // 发送通知
                return this.userInfo;
            } finally {
                this.currentRequest = null;
            }
        })();

        return this.currentRequest;
    }

    // 检查并刷新
    private async checkAndRefresh() {
        if (!TokenStore.get()) return;

        const now = Date.now();

        // 未过期 → 不处理
        if (this.userInfo && (now - this.lastFetchTime < this.cacheDuration)) return;

        await this.fetchUser(true);
    }

    // 启动自动刷新
    public startAutoRefresh() {
        this.stopAutoRefresh(); // 先尝试停止

        this.checkAndRefresh(); // 立即执行一次

        this.refreshTimer = window.setInterval(() => {
            this.checkAndRefresh();
        }, 10 * 60 * 1000); // 每 10 分钟检查一次
    }

    // 停止自动刷新
    public stopAutoRefresh() {
        if (this.refreshTimer) {
            clearInterval(this.refreshTimer);
            this.refreshTimer = null;
        }
    }

    // 获取当前用户（带缓存）
    public async getUserInfo(): Promise<UserInfoData | null> {
        if (!TokenStore.get()) return null;

        const now = Date.now();

        // 命中缓存
        if (this.userInfo && (now - this.lastFetchTime < this.cacheDuration)) return this.userInfo;

        return this.fetchUser();
    }

    // 修改用户信息
    public async updateUserInfo(nickname?: string, resume?: string): Promise<null> {
        if (!nickname && !resume) return null;

        if (nickname !== undefined && nickname !== this.userInfo?.nickname) {
           UserInfo.updateNickname(nickname).then(() => { this.refreshUser(); }).catch((error) => {
                throw error;
            });
        }
        if (resume !== undefined && resume !== this.userInfo?.resume) {
            UserInfo.updateResume(resume).then(() => { this.refreshUser(); }).catch((error) => {
                throw error;
            });
        }
        return null;
    }

    // 修改密码
    public async updatePassword(updatePasswordData: UpdatePasswordData): Promise<null> {
        try {
            await UserInfo.updatePassword(updatePasswordData);
            return null;
        } catch (error) {
            throw error;
        }
    }

    // 获取用户登录日志
    public async getLoginLogs(account: string): Promise<LoginLog[]> {
        try {
            const response = await UserInfo.getLoginLogs(account);
            return response.data;
        } catch (error) {
            throw error;
        }
    }

    // 强制刷新用户信息
    public async refreshUser(): Promise<UserInfoData | null> {
        this.lastFetchTime = 0;
        return this.fetchUser(true);
    }

    // 是否已登录
    public isLogin(): boolean {
        return !!TokenStore.get();
    }

    // 清空数据
    public clear() {
        this.userInfo = null;
        this.lastFetchTime = 0;
        this.notify(); // 发送通知
    }

    // 恢复状态
    public recover() {
        if (!TokenStore.get()) return;

        // 恢复用户信息自动刷新
        userService.startAutoRefresh();

        // 恢复 WebSocket 连接
        webSocketService.connect();
    }
}

// 单例导出
export const userService = new UserService();
