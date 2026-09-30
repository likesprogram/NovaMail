<template>
    <div class="settings">
        <div class="header">
            <div class="userInfo">
                <div class="avatar">
                    <img v-if="userStore.isLogin" :src="userStore.avatarUrl" alt="avatar"></img>
                    <img v-else src="@assets/avatar.png" alt="avatar"></img>
                </div>
                <div class="userInfoMeta">
                    <div class="nickname">{{ userStore.nickname }}</div>
                    <div class="mailBox">{{ userStore.mailBoxView }}</div>
                    <div class="resume">{{ userStore.userInfo?.resume }}</div>
                </div>
                <div class="operation">
                    <button class="btn btn-text" @click="AuthService.logout()">
                        <span class="iconfont icon-likesprogramtuichudenglu"></span>
                        退出登录
                    </button>
                </div>
            </div>
            <div class="catalogue">
                <router-link class="catalogueItem" to="/settings/users">
                    个人资料
                    <span class="line"></span>
                </router-link>
                <router-link class="catalogueItem" to="/settings/mail">
                    邮箱管理
                    <span class="line"></span>
                </router-link>
                <template v-if="userStore.isAdmin">
                    <router-link class="catalogueItem" to="/settings/system">
                        系统配置
                        <span class="line"></span>
                    </router-link>
                    <router-link class="catalogueItem" to="/settings/services">
                        服务监控
                        <span class="line"></span>
                    </router-link>
                </template>
            </div>
        </div>
        <div class="content">
            <router-view></router-view>
        </div>
    </div>
</template>

<script lang="ts">
import { UserStore } from '@store/UserStore';
import { AuthService } from '@service/AuthService';

export default {
    name: 'Settings',
}
</script>

<script setup lang="ts">
// 用户
const userStore = UserStore();
</script>

<style scoped>
.settings {
    width: 100%;
    height: 100%;
    position: relative;
    z-index: 999;
    top: 0;
    left: 0;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
}

/* 头部 */
.settings .header {
    width: 100%;
    flex-shrink: 0;
    background-color: var(--bg-primary);
    box-shadow: var(--shadow-card);
}
/* 用户信息展示 */
.settings .header .userInfo {
    width: 100%;
    display: flex;
    align-items: center;
    gap: 20px;
    padding: 20px 25px;
}
/* 头像 */
.settings .header .userInfo .avatar {
    width: 80px;
    height: 80px;
    border-radius: 50%;
    flex-shrink: 0;
    overflow: hidden;
    display: flex;
    justify-content: center;
    align-items: center;
}
.settings .header .userInfo .avatar img {
    width: 100%;
    height: 100%;
    object-fit: cover;
}
/* 用户详细 */
.settings .header .userInfo .userInfoMeta {
    width: 100%;
    flex: 1;
    min-width: 0; 
    gap: 3px;
    display: flex;
    flex-direction: column;
    justify-content: center;
    align-items: flex-start;
}
.settings .header .userInfo .userInfoMeta div {
    width: 100%;
    font-size: 14px;
    /* 单行文本省略 */
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
/* 用户名 */
.settings .header .userInfo .userInfoMeta .nickname {
    font-size: 18px;
    font-weight: 700;
}
/* 默认邮箱 */
.settings .header .userInfo .userInfoMeta .mailBox {
    color: var(--text-secondary1);
}.settings .header .userInfo .userInfoMeta .mailBox::after {
    content: ">";
}.settings .header .userInfo .userInfoMeta .mailBox::before {
    content: "<";
}
/* 个人简介 */
.settings .header .userInfo .userInfoMeta .resume {
    margin-top: 5px;
    color: var(--text-secondary2);
}
/* 操作按钮 */
.settings .header .userInfo .operation {
    flex-shrink: 0;
    display: flex;
    justify-content: center;
    align-items: center;
}
.settings .header .userInfo .operation .btn {
    width: 100%;
    height: 40px;
    font-size: 14px;
    padding-left: 30px;
    border-left: 1px solid var(--bg-active-primary);
}
.settings .header .userInfo .operation .btn .iconfont {
    font-size: 20px;
}

/* 目录 */
.settings .header .catalogue {
    width: 100%;
    height: 40px;
    display: flex;
    justify-content: flex-start;
    align-items: center;
    gap: 30px;
    padding: 0px 25px;
}
.settings .header .catalogue .catalogueItem {
    display: flex;
    flex-direction: column;
    justify-content: center;
    align-items: center;
    position: relative;
    font-size: 14px;
    height: 100%;
    transition: all .3s;
    color: var(--text-primary);
}
.settings .header .catalogue .catalogueItem .line {
    position: absolute;
    bottom: 0px;
    left: 50%;
    width: 60%;
    height: 2px;
    background-color: var(--text-link-hover);
    transform: translateX(-50%) scaleX(0);
    transition: transform 0.3s ease;
}
.settings .header .catalogue .catalogueItem:hover {
    color: var(--text-link-hover);
}
.settings .header .catalogue .catalogueItem:hover .line {
    transform: translateX(-50%) scaleX(1);
}
.settings .header .catalogue .catalogueItem.router-link-active,
.settings .header .catalogue .catalogueItem.router-link-exact-active {
    color: var(--text-link);
}
.settings .header .catalogue .catalogueItem.router-link-active .line,
.settings .header .catalogue .catalogueItem.router-link-exact-active .line { 
    transform: translateX(-50%) scaleX(1);
    background-color: var(--text-link);
}

/* 内容 */
.settings .content {
    width: 100%;
    flex: 1;
    min-height: 0; /* 允许子元素收缩 */
    padding: 15px 20px;
    display: flex;
    justify-content: center;
    align-items: flex-start;
    overflow: hidden;
    overflow-y: auto;
}
</style>
