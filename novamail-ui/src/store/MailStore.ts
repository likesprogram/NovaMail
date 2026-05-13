import { defineStore } from 'pinia';
import { type UserInfoData } from '@api/users/UserInfo';
import { userService } from '@service/UserService';
import { baseUrl } from '@utils/Request';

export const UserStore = defineStore('userInfo', {
    state: () => ({
        userInfo: null as UserInfoData | null
    }),
    getters: {
        isLogin: () => userService.isLogin(),
        isAdmin: (state) => state.userInfo?.isAdmin ?? false,
        account: (state) => state.userInfo?.account ?? '',
        avatarUrl: (state) => state.userInfo?.account ? `${baseUrl}/userProfile/avatar?account=${state.userInfo.account}&v=${state.userInfo.avatar}` : '',
        nickname: (state) => state.userInfo?.nickname ?? '',
        mailBoxView: (state) => state.userInfo?.defaultMailBoxView ?? '无邮箱',
        resume: (state) => state.userInfo?.resume ?? '这个人很懒，什么都没有留下~',
    },
    actions: {
        // 初始化订阅
        init() {
            userService.subscribe((userInfo) => { this.userInfo = userInfo }); // 添加监听列表
            userService.recover(); // 恢复状态
        }
    }
});
