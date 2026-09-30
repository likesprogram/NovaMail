<template>
    <div class="sidebar">
        <div class="menuBox">
            <div class="menuList">
                <router-link class="menuItem" to="/compose">
                    <span class="iconfont icon-likesprogramxiexin"></span>
                    <span class="title">写信</span>
                </router-link>
                <router-link class="menuItem" to="/inbox">
                    <span class="iconfont icon-likesprogramyoujian" style="font-size: 20px; padding: 1px;"></span>
                    <span class="title">收件箱</span>
                    <span class="count">{{ formatCount(getFolderCount('2')) }}</span>
                </router-link>
                <router-link class="menuItem" to="/sent">
                    <span class="iconfont icon-likesprogramfasong"></span>
                    <span class="title">已发送</span>
                    <span class="count">{{ formatCount(getFolderCount('3')) }}</span>
                </router-link>
            </div>
            <div class="menuList">
                <router-link class="menuItem" to="/star">
                    <span class="iconfont icon-likesprograma-shoucang"></span>
                    <span class="title">星标件</span>
                    <span class="count">{{ formatCount(getFolderCount('-1')) }}</span>
                </router-link>
                <router-link class="menuItem" to="/draft">
                    <span class="iconfont icon-likesprogramcaogaoxiang"></span>
                    <span class="title">草稿箱</span>
                    <span class="count">{{ formatCount(getFolderCount('1')) }}</span>
                </router-link>
                <router-link class="menuItem" to="/deleted">
                    <span class="iconfont icon-likesprogramhuishouzhan"></span>
                    <span class="title">已删除</span>
                    <span class="count">{{ formatCount(getFolderCount('6')) }}</span>
                </router-link>
                <router-link class="menuItem" to="/trash">
                    <span class="iconfont icon-likesprogramkuloutou"></span>
                    <span class="title">垃圾箱</span>
                    <span class="count">{{ formatCount(getFolderCount('4')) }}</span>
                </router-link>
                <router-link class="menuItem" to="/advertisement">
                    <span class="iconfont icon-likesprogramjubao"></span>
                    <span class="title">广告箱</span>
                    <span class="count">{{ formatCount(getFolderCount('5')) }}</span>
                </router-link>
            </div>
            <div class="menuList" :class="{ 'menuListNoBorder': inNovaShell }">
                <router-link class="menuItem" to="/address">
                    <span class="iconfont icon-likesprogramtongxunlu"></span>
                    <span class="title">通讯录</span>
                </router-link>
                <router-link class="menuItem" to="/files">
                    <span class="iconfont icon-likesprogramwenzhang"></span>
                    <span class="title">文件中转</span>
                </router-link>
            </div>
            <!-- 下载按钮 -->
            <button v-if="!inNovaShell" class="download btn-primary button" @click="downloadClient">
                <span class="iconfont icon-likesprogramxiazai"></span>
                <span class="title">下载客户端</span>
            </button>
        </div>
        <Footer></Footer>
        
        <!-- 消息提示 -->
        <ShowMessage ref="showMessage"/>
    </div>
</template>

<script lang="ts">
import { computed, defineComponent, onBeforeUnmount, onMounted, ref } from 'vue';
import Footer from '@components/Footer.vue';
import ShowMessage from '@components/messageDialog/ShowMessage.vue';
import { Email, type MailBoxFolder } from '@api/email/Email';
import { webSocketService, type WebSocketMessage, type NewEmailMessageData } from '@service/WebSocketService';
import { useRouter } from 'vue-router';
import { isRunningInNovaShell } from '@utils/NovaShellSdk';
import { SystemSettingService } from '@service/SystemSettingService';

let unsubscribeNewWebSocket: (() => void) | null = null;
let unsubscribStatuseWebSocket: (() => void) | null = null;

export default defineComponent({
    name: 'Sidebar',
});
</script>

<script setup lang="ts">
const router = useRouter();

const inNovaShell = ref(isRunningInNovaShell());

// 对话框实例
const showMessage = ref<InstanceType<typeof ShowMessage>>();

// 邮箱列表
const folderList = ref<MailBoxFolder[]>([]);
// 邮箱数量映射表
const folderCountMap = computed<Record<string, number>>(() => {
    const map: Record<string, number> = {};
    for (const folder of folderList.value) map[folder.folderId] = folder.noReadCont ?? 0;
    return map;
});

// 根据文件夹名称获取数量
const getFolderCount = (folderId: string): number => {
    return folderCountMap.value[folderId] ?? 0;
};

// 数量为 0 时不显示
const formatCount = (count: number): string => {
    return count > 0 ? String(count) : '';
};

const downloadClient = async () => {
    try {
        const clientInfo = await SystemSettingService.getClientDownloadInfo();
        await SystemSettingService.downloadClient(clientInfo.fileName);
    } catch (error) {
        showMessage.value?.ShowModal('客户端下载失败', ['管理员暂未配置客户端安装包，或文件暂时不可用'], 'failure');
    }
};

// 获取签名列表
const getFolderList = async () => {
    try {
        const response = await Email.getFolderList();
        folderList.value = response.data;
    } catch (error) {
        setTimeout(() => {
            showMessage.value?.ShowModal("获取邮箱列表", ["获取邮箱列表失败，是否重试"], "failure", (selectedButtonIndex) => {
                if(selectedButtonIndex === 1) getFolderList();
            });
        }, 300);
    }
};

onMounted(() => {
    // 获取邮箱列表
    getFolderList();
    // 注册新邮件通知事件
    unsubscribeNewWebSocket = webSocketService.onMessage((message: WebSocketMessage) => {
        if (message.type === 'NEW_EMAIL') {
            const data = message.data as NewEmailMessageData;
            // 刷新侧边栏文件夹未读数
            getFolderList();
            
            // 如果不是已删除、回收站、广告、垃圾邮件就弹窗通知
            if (data.folderId !== 7 && data.folderId !== 6 && data.folderId !== 5 && data.folderId !== 4) {
                showMessage.value?.ShowModal("新邮件", ["有新邮件送达，是否查看"], "success", (selectedButtonIndex) => {
                    if(selectedButtonIndex === 1) router.replace({ name: 'inboxRead', params: { id: data.mappingId } });
                });
            }
        }
    });
        // 注册新邮件通知事件
    unsubscribStatuseWebSocket = webSocketService.onMessage((message: WebSocketMessage) => {
        if (message.type === 'MAIL_STATUS_CHANGED' || message.type === 'MAIL_MOVED') {
            // 刷新侧边栏文件夹未读数
            getFolderList();
        }
    });
});

onBeforeUnmount(() => {
    unsubscribeNewWebSocket?.();
    unsubscribeNewWebSocket = null;
    unsubscribStatuseWebSocket?.();
    unsubscribStatuseWebSocket = null;
});
</script>

<style scoped>
.sidebar {
    width: 220px;
    flex-shrink: 0;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: space-between;
    padding-bottom: 20px;
}
.sidebar .menuBox {
    width: 100%;
    flex: 1;
    display: flex;
    flex-direction: column;
    align-items: center;
}
.sidebar .menuBox .menuList { 
    width: 100%;
    padding: 5px 15px;
    padding-top: 0px;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: flex-start;
    gap: 5px;

}.sidebar .menuBox .menuList::after { 
    content: '';
    width: 80%;
    height: 1px;
    background-color: var(--bg-active-primary);
}.sidebar .menuBox .menuList.menuListNoBorder::after { 
    display: none;
}
.sidebar .menuBox .menuItem { 
    width: calc(100% - 30px);
    padding: 5px 10px;
    font-size: 14px;
    display: flex;
    align-items: center;
    justify-content: flex-start;
    gap: 5px;
    cursor: pointer;
    transition: all 0.3s;
    border-radius: 10px;
    color: var(--text-primary);
} .sidebar .menuBox .menuList .menuItem { 
    width: 100%;
}.sidebar .menuBox .menuItem:hover { 
    background-color: var(--bg-hover-primary);
}.sidebar .menuBox .menuItem:active {
    transition: all .1s; 
    background-color: var(--bg-active-primary);
}
.sidebar .menuBox .menuItem .iconfont { 
    font-size: 22px;
    flex-shrink: 0;
}
.sidebar .menuBox .menuItem .title { 
    font-size: 14px;
    flex: 1;
}
.sidebar .menuBox .menuItem .count { 
    font-size: 12px;
    flex-shrink: 0;
    font-weight: 700;
    color: var(--text-link);
}
.sidebar .menuBox .menuItem.router-link-active,
.sidebar .menuBox .menuItem.router-link-exact-active { 
    background-color: var(--bg-hover-primary);
}.sidebar .menuBox .menuItem.router-link-active .title,
.sidebar .menuBox .menuItem.router-link-exact-active .title { 
    font-weight: 700;
}

/* 下载按钮 */
.sidebar .menuBox .download {
    width: calc(100% - 50px);
    padding: 8px;
}.sidebar .menuBox .download .title { 
    flex: none;
    font-size: 14px;
}.sidebar .menuBox .download .iconfont { 
    font-size: 20px;
}
</style>
