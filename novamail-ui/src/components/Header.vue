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
                    <input type="text" ref="searchInput" v-model="searchText" @keydown.enter.prevent="doSearch" placeholder="请输入搜索内容" />
                    <!-- 清空图标 -->
                    <div v-if="searchText && searchText.length > 0"
                        class="searchClear iconfont icon-likesprogramcancel"
                    @click="searchClear" @mousedown="blurFromInternal = true"></div>
                </div>
                <!-- 搜索图标 -->
                <div class="searchButton iconfont icon-likesprogramchaxun" @click.stop="doSearch"></div>
            </div>
            <div class="minSearch" :class="{ 'searchPanelShown': searchPanelShown }" v-else>
                <div class="iconfont icon-likesprogramchaxun"></div>
            </div>
                
            <!-- 搜索提示面板 -->
            <SafePopover ref="searchPanel" :NoBoxShadow="['all']" :Width="'95%'">
                <div class="searchPanel">
                    <div v-if="searchLoading" class="searchState">正在搜索...</div>
                    <div v-else-if="!searchText" class="searchState">输入主题、发件人或邮箱地址进行搜索</div>
                    <div v-else-if="searchSuggestions.length === 0" class="searchState">暂无匹配结果</div>
                    <div v-else v-for="item in searchSuggestions" :key="item.type + '-' + item.id" class="searchSuggestItem"
                    @click.stop
                    @mousedown.prevent="selectSuggestion(item)">
                        <div class="suggestSub" v-if="item.subText">{{ item.subText }}</div>
                        <div class="suggestText">{{ item.text }}</div>
                    </div>
                </div>
            </SafePopover>
        </div>
        <!-- 占位 -->
        <div class="placeholder"></div>
        <!-- 用户信息 -->
        <div class="userInformation" ref="userInfoButton" @click="showUserInfoPanel">
            <div class="avatar">
                <img v-if="userStore.isLogin" :src="userStore.avatarUrl" alt="avatar"></img>
                <img v-else src="@assets/avatar.png" alt="avatar"></img>
            </div>
            <div class="name">{{ userStore.isLogin ? userStore.nickname : "未登录" }}</div>
            <div class="email" v-if="userStore.isLogin">{{ userStore.mailBoxView }}</div>
            
            <!-- 用户信息面板 -->
            <SafePopover ref="userInfoPanel" :NoBoxShadow="['all']" :Width="'95%'">
                <div class="userInfoPanel">
                    <div class="userInfo">
                        <div class="nickname">
                            <span class="nameText">{{ userStore.nickname }}</span>
                            <span class="label" :class="userStore.isAdmin ? 'admin' : 'user'">
                                {{ userStore.isAdmin ? "管理员" : "普通用户" }}
                            </span>
                        </div>
                        <div class="mailBox">
                            <span class="mailBoxText">{{ userStore.mailBoxView }}</span>
                            <span
                                v-if="userStore.mailBoxView && userStore.mailBoxView != '无邮箱'" class="copyMailBox iconfont icon-likesprogramcopy"
                                @click="copyMailBox"
                            ></span>
                        </div>
                    </div>
                    <div class="buttonList">
                        <router-link class="button btn-text" to="/settings/users">
                            <span class="iconfont icon-likesprogramyonghu"></span>
                            个人资料
                        </router-link>
                        <router-link class="button btn-text" to="/settings/mail">
                            <span class="iconfont icon-likesprogramyoujian"></span>
                            邮箱管理
                        </router-link>
                        <router-link class="button btn-text" to="/files">
                            <span class="iconfont icon-likesprogramwenzhang"></span>
                            文件管理
                        </router-link>
                    </div>
                    <div class="bottom">
                        <router-link v-if="userStore.isAdmin" class="button btn-text" to="/settings/system">
                            <span class="iconfont icon-likesprogramjiaoseguanli"></span>
                            系统管理
                        </router-link>
                        <span v-else><!-- 占位符 --></span>
                        <button class="btn-text" @click="AuthService.logout()">
                            <span class="iconfont icon-likesprogramtuichudenglu"></span>
                            退出
                        </button>
                    </div>
                </div>
            </SafePopover>
        </div>
        <!-- 设置按钮 -->
        <router-link class="settings" to="/settings">
            <span class="iconfont icon-likesprogramshezhi"></span>
            <span class="title">设置</span>
            <div class="settingsPanelBox panelBox"></div>
        </router-link>
        <!-- 消息提示 -->
        <ShowMessage ref="showMessage"/>
    </div>
</template>

<script lang="ts">
import { defineComponent, nextTick, ref, getCurrentInstance, watch, onBeforeUnmount } from 'vue';
import { AuthService } from '@service/AuthService';
import { UserStore } from '@store/UserStore';
import { Email, type SearchSuggestion } from '@api/email/Email';
import ShowMessage from '@components/messageDialog/ShowMessage.vue';
import SafePopover from '@components/SafePopover.vue';
import { useRouter } from 'vue-router';

export default defineComponent({
    name: 'Header',
});
</script>

<script setup lang="ts">
const instance = getCurrentInstance();
const title = instance?.appContext.config.globalProperties.$title || '新星邮箱';
const subtitle = instance?.appContext.config.globalProperties.$subtitle || 'NovaMail';

const router = useRouter();

// 用户
const userStore = UserStore();

// 对话框实例
const showMessage = ref<InstanceType<typeof ShowMessage>>();

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
const searchLoading = ref(false);
const searchSuggestions = ref<SearchSuggestion[]>([]);
// 清空搜索内容
const searchClear = () => {
    searchText.value = "";
    searchSuggestions.value = [];
    nextTick(() => {
        searchInput.value?.focus();
        showSearchPanel();
    });
}
// 显示搜索面板
const showSearchPanel = () => {
    searchPanel.value?.show(searchBox.value as HTMLElement);
}
const hideSearchPanel = () => {
    searchPanelShown.value = false;
    searchPanel.value?.hide?.();
};

let searchTimer: number | undefined;
let requestSeq = 0;

const getSearchSuggestions = async (keyword: string): Promise<SearchSuggestion[]> => {
    try {
        const response = await Email.getSearchSuggestions(keyword);
        showSearchPanel();
        return response.data;
    } catch (error) {
        setTimeout(() => {
            showMessage.value?.ShowModal("搜索失败", ["搜索内容获取失败，是否重试"], "failure", (selectedButtonIndex) => {
                if(selectedButtonIndex === 1) getSearchSuggestions(keyword);
            });
        }, 300);
        return [];
    }
}

watch(searchText, (value) => {
    const keyword = value.trim();

    if (searchTimer) {
        clearTimeout(searchTimer);
    }

    if (!keyword) {
        searchSuggestions.value = [];
        searchLoading.value = false;
        return;
    }

    searchTimer = window.setTimeout(async () => {
        const currentSeq = ++requestSeq;

        try {
            searchLoading.value = true;

            const result = await getSearchSuggestions(keyword);

            // 防止旧请求覆盖新结果
            if (currentSeq !== requestSeq) return;

            searchSuggestions.value = result;
            showSearchPanel();
        } catch (error) {
            if (currentSeq !== requestSeq) return;
            searchSuggestions.value = [];
            console.error('搜索提示获取失败:', error);
        } finally {
            if (currentSeq === requestSeq) {
                searchLoading.value = false;
            }
        }
    }, 300);
});
const selectSuggestion = (item: SearchSuggestion) => {
    searchText.value = item.text;

    // 如果点击的是具体邮件，可以直接进入详情
    if (item.mappingId) {
        hideSearchPanel();
        router.replace({ name: 'inboxRead', params: { id: item.mappingId } });
        return;
    }

    // 否则执行搜索
    doSearch();
};
const doSearch = () => {
    const keyword = searchText.value.trim();
    if (!keyword) return;

    hideSearchPanel();

    // 这里换成你自己的邮件列表搜索逻辑
    // searchMailList(keyword);
};

onBeforeUnmount(() => {
    if (searchTimer) {
        clearTimeout(searchTimer);
    }
});

// 用户信息相关
const userInfoButton = ref<HTMLElement | null>(null);
// 用户信息面板
const userInfoPanel = ref<InstanceType<typeof SafePopover> | null>(null);
// 显示用户信息面板
const showUserInfoPanel = () => {
    userInfoPanel.value?.show(userInfoButton.value as HTMLElement);
}
// 邮箱拷贝
const copyMailBox = async () => {
    try {
        await navigator.clipboard.writeText(userStore.mailBoxView);
    } catch (e) {
        console.error(e);
    }
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
    justify-content: flex-start;
    background-color: var(--bg-primary);
    overflow: hidden;
    overflow-y: auto;
}.header .searchPanel .searchState {
    width: 100%;
    height: 100%;
    font-family: 'artisticFonts';
    font-size: 16px;
    color: var(--text-tertiary);
    display: flex;
    justify-content: center;
    align-items: center;
}.header .searchPanel .searchSuggestItem {
    padding: 10px 0px;
    width: 95%;
    transition: all .3s;
    display: flex;
    align-items: center;
    justify-content: flex-start;
    border-bottom: 1px solid var(--border-hover);
    gap: 5px;
}.header .searchPanel .searchSuggestItem:hover {
    background-color: var(--bg-hover-primary);
}.header .searchPanel .searchSuggestItem:active {
    background-color: var(--bg-active-primary);
}.header .searchPanel .searchSuggestItem:last-child {
    border-bottom: none;
}.header .searchPanel .searchSuggestItem .suggestSub,
.header .searchPanel .searchSuggestItem .suggestText {
    font-size: 14px;
    /* 单行文本省略 */
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}.header .searchPanel .searchSuggestItem .suggestSub {
    max-width: 120px;
    flex-shrink: 0;
    font-weight: 700;
    /* 单行文本省略 */
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}

/* 占位 */
.header .placeholder {
    flex-grow: 1;
}

/* 用户信息 */
.header .userInformation { 
    width: 250px;
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
.header .userInformation .avatar {
    width: 35px;
    height: 35px;
    border-radius: 50%;
    overflow: hidden;
    flex-shrink: 0;
    background-color: var(--bg-primary);
}
.header .userInformation .avatar img { 
    width: 100%;
    height: 100%;
}
.header .userInformation .name {
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
.header .userInformation .email {
    margin-left: 5px;
    font-size: 13px;
    flex: 1;
    color: var(--text-secondary2);
    /* 单行文本省略 */
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}

/* 用户信息面板 */
.header .userInformation .userInfoPanel {
    width: 100%;
    padding: 10px;
    cursor: default;
    display: flex;
    flex-direction: column;
    align-items: flex-start;
    justify-content: flex-start;
    background-color: var(--bg-primary);
}
/* 用户信息 */
.header .userInformation .userInfoPanel .userInfo {
    width: 100%;
    gap: 2px;
    padding: 0px 5px;
    padding-bottom: 10px;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    border-bottom: 1px solid var(--bg-active-primary);
}
/* 用户名称 */
.header .userInformation .userInfoPanel .userInfo .nickname,
.header .userInformation .userInfoPanel .userInfo .mailBox {
    width: 100%;
    display: flex;
    align-items: center;
    justify-content: flex-start;
    gap: 5px;
}
.header .userInformation .userInfoPanel .userInfo .nickname .nameText,
.header .userInformation .userInfoPanel .userInfo .mailBox .mailBoxText {
    font-size: 16px;
    font-weight: 700;
    color: var(--text-primary);
    /* 单行文本省略 */
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.header .userInformation .userInfoPanel .userInfo  .nickname .label {
    padding: 4px 8px;
    font-size: 12px;
    border-radius: 5px;
    flex-shrink: 0;
    background-color: var(--bg-tertiary);
}
/* 用户邮箱 */
.header .userInformation .userInfoPanel .userInfo .mailBox .mailBoxText {
    color: var(--text-secondary2);
    font-size: 12px;
    font-weight: normal;
}
.header .userInformation .userInfoPanel .userInfo .mailBox .copyMailBox {
    color: var(--text-secondary2);
    padding: 2px;
    border-radius: 5px;
    font-size: 18px;
    cursor: pointer;
    font-weight: normal;
    transition: all .3s;
}
.header .userInformation .userInfoPanel .userInfo .mailBox .copyMailBox:hover {
    color: var(--text-secondary1);
    background-color: var(--bg-hover-primary);
}
.header .userInformation .userInfoPanel .userInfo .mailBox .copyMailBox:active {
    transition: all .1s;
    background-color: var(--bg-active-primary);
}
/* 按钮列表 */
.header .userInformation .userInfoPanel .buttonList {
    width: 100%;
    padding: 5px 0px;
    display: flex;
    flex-direction: column;
    align-items: flex-start;
    justify-content: center;
}
.header .userInformation .userInfoPanel .buttonList .button {
    font-size: 14px;
    width: 100%;
    justify-content: flex-start;
}.header .userInformation .userInfoPanel .buttonList .button .iconfont {
    font-size: 18px;
}
/* 底部内容 */
.header .userInformation .userInfoPanel .bottom {
    font-size: 14px;
    width: 100%;
    display: flex;
    align-items: center;
    justify-content: space-between;
}.header .userInformation .userInfoPanel .bottom .iconfont {
    font-size: 18px;
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
.header .userInformation:hover,
.header .settings:hover { 
    background-color: var(--bg-hover-primary);
}
.header .userInformation:active,
.header .settings:active { 
    transition: all .1s;
    background-color: var(--bg-active-primary);
}
.header .userInformation.router-link-active,
.header .settings.router-link-active,
.header .userInformation .userInfoPanel .buttonList .button.router-link-active,
.header .userInformation .userInfoPanel .bottom .button.router-link-active,
.header .userInformation.router-link-exact-active,
.header .settings.router-link-exact-active,
.header .userInformation .userInfoPanel .buttonList .button.router-link-exact-active,
.header .userInformation .userInfoPanel .bottom .button.router-link-exact-active { 
    background-color: var(--bg-hover-primary);
}
.header .settings .iconfont {
    font-size: 20px;
}
</style>
