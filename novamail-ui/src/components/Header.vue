<template>
    <div class="header">
        <!-- logo -->
        <a class="logo" href="/">
            <img src="@assets/icons/favicon.svg" alt="logo"></img>
            <div class="titles">
                <div class="title">{{ title }}</div>
                <div class="subtitle">{{ subtitle }}</div>
            </div>
        </a>
        <!-- 搜索栏 -->
        <div class="searchBox" ref="searchBox">
            <div class="search" v-if="!hideSearch" :class="{ 'searchClearShown': (searchText && searchText.length > 0) }" @click="showSearchPanel">
                <div class="searchInput">
                    <input type="text" ref="searchInput" v-model="searchText" placeholder="请输入搜索内容" />
                    <!-- 清空图标 -->
                    <div v-if="searchText && searchText.length > 0"
                        class="searchClear iconfont icon-likesprogramcancel"
                    @click="searchClear" @mousedown="blurFromInternal = true"></div>
                </div>
                <!-- 搜索图标 -->
                <div class="searchButton iconfont icon-likesprogramchaxun"></div>
            </div>
            <div class="minSearch" :class="{ 'searchPanelShown': searchPanelShown }" v-else>
                <div class="iconfont icon-likesprogramchaxun"></div>
            </div>
                
            <!-- 搜索提示面板 -->
            <SafePopover ref="searchPanel" :NoBoxShadow="['all']" :Width="'95%'">
                <div class="searchPanel">这是一个安全弹窗</div>
            </SafePopover>
        </div>
        <!-- 占位 -->
        <div class="placeholder"></div>
        <!-- 用户信息 -->
        <div class="userInfo" ref="userInfoButton" @click="showUserInfoPanel">
            <div class="avatar">
                <img src="@assets/avatar.png" alt="avatar"></img>
            </div>
            <div class="name">{{ "新星邮局" }}</div>
            <div class="email">{{ "novamail@lby.email" }}</div>
            
            <!-- 搜索提示面板 -->
            <SafePopover ref="userInfoPanel" :NoBoxShadow="['all']" :Width="'95%'">
                <div class="searchPanel">这是一个安全弹窗</div>
            </SafePopover>
        </div>
        <!-- 设置按钮 -->
        <router-link class="settings" to="/settings">
            <span class="iconfont icon-likesprogramshezhi"></span>
            <span class="title">设置</span>
            <div class="settingsPanelBox panelBox"></div>
        </router-link>
    </div>
</template>

<script lang="ts">
import { defineComponent, nextTick, ref, getCurrentInstance } from 'vue';
export default defineComponent({
    name: 'Header',
});
</script>

<script setup lang="ts">
import SafePopover from '@components/SafePopover.vue';

const instance = getCurrentInstance();
const title = instance?.appContext.config.globalProperties.$title || '新星邮箱';
const subtitle = instance?.appContext.config.globalProperties.$subtitle || 'NovaMail';

// 搜索框相关
// 搜索框
const searchBox = ref<HTMLElement | null>(null);
// 搜索输入框
const searchInput = ref<HTMLElement | null>(null);
// 搜索面板
const searchPanel = ref<InstanceType<typeof SafePopover> | null>(null);
// 搜索内容
const searchText = ref("");
// 是否显示清空按钮
const blurFromInternal = ref(false);
// 隐藏搜索框
const hideSearch = ref(false);
// 是否显示搜索面板
const searchPanelShown = ref(false);
// 清空搜索内容
const searchClear = () => {
    searchText.value = "";
    nextTick(() => {
        searchInput.value?.focus();
    });
}
// 显示搜索面板
const showSearchPanel = () => {
    searchPanel.value?.show(searchBox.value as HTMLElement);
}

// 用户信息相关
const userInfoButton = ref<HTMLElement | null>(null);
// 用户信息面板
const userInfoPanel = ref<InstanceType<typeof SafePopover> | null>(null);
// 显示用户信息面板
const showUserInfoPanel = () => {
    userInfoPanel.value?.show(userInfoButton.value as HTMLElement);
}
</script>

<style scoped>
/* header */
.header {
    width: 100%;
    height: 65px;
    padding: 15px;
    display: flex;
    flex-shrink: 0;
}

/* logo */
.header .logo {
    width: 200px;
    height: 100%;
    margin-top: -2px;
    display: flex;
    align-items: center;
    justify-content: center;
    flex-shrink: 0;
    cursor: pointer;
    text-decoration: none;
}
.header .logo img { 
    width: 45px;
    flex-shrink: 0;
}
.header .logo .titles {
    margin-left: 5px;
    margin-top: 7px;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: flex-end;
    font-family: 'artisticFonts';
    color: rgba(10, 150, 235, 1);
    line-height: 1;
}
.header .logo .titles .title {
    font-size: 20px;
}
.header .logo .titles .subtitle { 
    font-size: 12px;
}

/* 搜索框 */
.header .searchBox {
    flex-grow: 1;
    height: 35px;
    margin-left: 10px;
    flex-shrink: 0;
    max-width: 400px;
    min-width: 220px;
    display: flex;
    align-items: center;
    position: relative;
    transition: all .3s;
    justify-content: flex-start;
}
.header .searchBox .search {
    flex-shrink: 0;
    width: 100%;
    height: 100%;
    display: flex;
    align-items: center;
    justify-content: space-between;
    color: var(--primary-color);
    border: 1px solid transparent;
    border-radius: 10px;
    position: relative;
    overflow: hidden;
    transition: all .3s;
    background-color: var(--bg-hover-primary);
}
.header .searchBox .search input::placeholder {
    color: var(--input-primary-text-placeholder);
    font-size: 14px;
}
.header .searchBox .search:hover {
    border: 1px solid var(--border-hover);
}
.header .searchBox .search:focus-within,
.header .searchBox .search.searchClearShown {
    border: 1px solid var(--border-hover);
    background-color: rgba(255, 255, 255, 1);
}
.header .searchBox .search .searchInput {
    flex: 1;
    width: 100%;
    height: 100%;
    display: flex;
    align-items: center;
}
.header .searchBox .search .searchInput input {
    width: 100%;
    height: 100%;
    border: none;
    outline: none;
    padding-left: 10px;
    font-size: 14px;
    background-color: transparent;
}
.header .searchBox .search .iconfont {
    flex-shrink: 0;
    cursor: pointer;
    display: flex;
    align-items: center;
    transition: all .3s;
    color: rgba(60, 60, 60, 1);
}
.header .searchBox .search .searchInput .searchClear {
    height: 100%;
    width: 20px;
    justify-content: center;
}
.header .searchBox .search .searchButton {
    width: 35px;
    height: 100%;
    font-size: 20px;
    padding-left: 5px;
    justify-content: flex-start;
}
.header .searchBox .search .iconfont:hover {
    color: var(--hover-color);
}

/* 小搜索按钮 */
.header .searchBox .minSearch {
    flex-shrink: 0;
    padding: 0px 10px;
    height: 100%;
    display: flex;
    align-items: center;
    justify-content: center;
    cursor: pointer;
    color: var(--primary-color);
    transition: color 0.3s;
}
.header .searchBox .minSearch.searchPanelShown,
.header .searchBox .minSearch:hover { 
    color: var(--hover-color);
}
.header .searchBox .minSearch .iconfont {
    font-size: 28px;
}
.header .searchPanel {
    width: 100%;
    height: 200px;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    background-color: var(--bg-primary);
}

/* 占位 */
.header .placeholder {
    flex-grow: 1;
}

/* 用户信息 */
.header .userInfo { 
    max-width: 250px;
    height: 100%;
    flex-shrink: 0;
    padding: 0px 10px;
    margin-left: 5px;
    display: flex;
    align-items: center;
    justify-content: flex-start;
    cursor: pointer;
    transition: all .3s;
    color: var(--text-primary);
    border-radius: 10px;
    position: relative;
}
.header .userInfo .avatar {
    width: 35px;
    height: 35px;
    border-radius: 50%;
    overflow: hidden;
    flex-shrink: 0;
    background-color: var(--bg-primary);
}
.header .userInfo .avatar img { 
    width: 100%;
    height: 100%;
}
.header .userInfo .name {
    margin-left: 5px;
    font-size: 14px;
    font-weight: 700;
    max-width: 100px;
    flex-shrink: 0;
    color: var(--text-secondary1);
    /* 单行文本省略 */
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.header .userInfo .email {
    margin-left: 5px;
    font-size: 13px;
    flex: 1;
    color: var(--text-secondary2);
    /* 单行文本省略 */
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}

/* 设置按钮 */
.header .settings {
    height: 100%;
    padding: 0px 10px;
    flex-shrink: 0;
    margin-left: 5px;
    display: flex;
    gap: 3px;
    align-items: center;
    justify-content: center;
    cursor: pointer;
    border-radius: 10px;
    transition: all .3s;
    font-size: 14px;
    color: var(--text-secondary1);
}
.header .userInfo:hover,
.header .settings:hover { 
    background-color: var(--bg-hover-primary);
}
.header .userInfo:active,
.header .settings:active { 
    transition: all .1s;
    background-color: var(--bg-active-primary);
}
.header .userInfo.router-link-active,
.header .settings.router-link-active,
.header .userInfo.router-link-exact-active,  
.header .settings.router-link-exact-active { 
    background-color: var(--bg-hover-primary);
}
.header .settings .iconfont {
    font-size: 20px;
}
</style>
