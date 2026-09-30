<template>
    <div class="read">
        <div class="operation">
            <div class="sendButton" @click="goBack">
                <span class="iconfont icon-likesprograma-zuojiantoufanhuishangyibu"></span>
                <span class="actionTitle">返回</span>
            </div>
            <div class="actions">
                <ActionBar :actionItems="actionItems"></ActionBar>
            </div>
        </div>

        <div v-if="markMenuVisible" class="mailActionMenu" :style="markMenuStyle" @click.stop>
            <button v-for="item in markMenuItems" :key="item.key" type="button" class="mailActionMenuItem" @click="item.onClick">
                <span class="iconfont" :class="item.icon"></span>
                <span>{{ item.title }}</span>
            </button>
        </div>

        <div v-if="moveMenuVisible" class="mailActionMenu" :style="moveMenuStyle" @click.stop>
            <button v-for="item in movableFolders" :key="item.folderId" type="button" class="mailActionMenuItem" @click="moveCurrentTo(Number(item.folderId))">
                <span class="iconfont icon-likesprogramArrows_Arrows_Right_Left"></span>
                <span>{{ getFolderDisplayName(item.folderId) }}</span>
            </button>
        </div>

        <ReadMail :mail="previewMail" :starred="mail?.isStarred ?? false" @toggleStarred="toggleCurrentStarred" />
        <ShowMessage ref="showMessage"/>
    </div>
</template>

<script lang="ts">
import { computed, onBeforeUnmount, onMounted, ref, watch } from 'vue';
import { useRoute, useRouter } from 'vue-router';
import { Email, type MailAttachment, type MailBoxFolder, type MailDetailResponse } from '@api/email/Email';
import ActionBar, { type ActionItem } from '@components/ActionBar.vue';
import ReadMail from '@components/ReadMail.vue';
import ShowMessage from '@components/messageDialog/ShowMessage.vue';
import { MailService } from '@service/MailService';
import { UserStore } from '@store/UserStore';
import { EmailActions } from '@typings/emailList/EmailActions';
import type { MailPreviewAttachment, MailPreviewData } from '@typings/email/MailPreview';

export default {
    name: 'Read',
}
</script>

<script setup lang="ts">
const router = useRouter();
const route = useRoute();
const userStore = UserStore();

const showMessage = ref<InstanceType<typeof ShowMessage>>();
const mail = ref<MailDetailResponse | null>(null);
const folderOptions = ref<MailBoxFolder[]>([]);
const markMenuVisible = ref(false);
const moveMenuVisible = ref(false);
const markMenuStyle = ref<Record<string, string>>({});
const moveMenuStyle = ref<Record<string, string>>({});
let latestMailRequestId = 0;

const SYSTEM_FOLDER_NAME: Record<string, string> = {
    '1': '草稿箱',
    '2': '收件箱',
    '3': '已发送',
    '4': '垃圾邮件',
    '5': '广告邮件',
    '6': '已删除',
    '7': '彻底删除',
};

const mappingId = computed(() => {
    const id = route.params.id;
    if (Array.isArray(id)) return Number(id[0]);
    return Number(id);
});
const currentFolderId = computed(() => {
    const parent = String(route.matched[0]?.name ?? route.path.split('/')[1] ?? '');
    const routeFolderMap: Record<string, number> = {
        draft: 1,
        inbox: 2,
        sent: 3,
        trash: 4,
        advertisement: 5,
        deleted: 6,
    };
    return routeFolderMap[parent] ?? 0;
});
const movableFolders = computed(() => folderOptions.value.filter(folder => {
    const folderId = Number(folder.folderId);
    const actualFolderId = mail.value?.folderId ?? currentFolderId.value;
    return Number.isFinite(folderId) && folderId > 0 && folderId !== actualFolderId && folderId !== 7;
}));
const goBack = () => {
    if (window.history.length > 1) {
        router.back();
    } else {
        router.push('/inbox');
    }
};
const closeActionMenus = () => {
    markMenuVisible.value = false;
    moveMenuVisible.value = false;
};
const placeMenu = (target?: HTMLElement): Record<string, string> => {
    if (!target) return { top: '52px', left: '20px' };
    const rootRect = target.closest('.read')?.getBoundingClientRect();
    const rect = target.getBoundingClientRect();
    const top = rootRect ? rect.bottom - rootRect.top + 6 : rect.bottom + 6;
    const left = rootRect ? rect.left - rootRect.left : rect.left;
    return {
        top: `${top}px`,
        left: `${Math.max(8, left)}px`,
    };
};
const loadFolderOptions = async () => {
    if (folderOptions.value.length > 0) return;
    try {
        const response = await Email.getFolderList();
        folderOptions.value = response.data ?? [];
    } catch {
        showMessage.value?.ShowModal("文件夹加载失败", ["文件夹列表加载失败，请稍后重试"], "failure", undefined, [
            { text: '确定', type: 'success', autoClose: true }
        ]);
    }
};
const showMarkMenu = (target?: HTMLElement) => {
    markMenuStyle.value = placeMenu(target);
    markMenuVisible.value = !markMenuVisible.value;
    moveMenuVisible.value = false;
};
const showMoveMenu = (target?: HTMLElement) => {
    void loadFolderOptions();
    moveMenuStyle.value = placeMenu(target);
    moveMenuVisible.value = !moveMenuVisible.value;
    markMenuVisible.value = false;
};
const getFolderDisplayName = (folderId: string | number): string => {
    const key = String(folderId);
    const folder = folderOptions.value.find(folder => String(folder.folderId) === key);
    return SYSTEM_FOLDER_NAME[key] ?? folder?.folderName ?? folder?.floderName ?? `文件夹 ${key}`;
};
const requireCurrentMappingId = (): number | null => {
    if (Number.isFinite(mappingId.value) && mappingId.value > 0) return mappingId.value;
    showMessage.value?.ShowModal("邮件操作", ["当前邮件ID无效"], "warning", undefined, [
        { text: '确定', type: 'success', autoClose: true }
    ]);
    return null;
};
const runCurrentOperation = async (operation: (id: number) => Promise<void>, successMessage: string, afterMoved = false) => {
    const id = requireCurrentMappingId();
    if (!id) return;
    closeActionMenus();
    try {
        await operation(id);
        showMessage.value?.ShowModal("邮件操作", [successMessage], "success", undefined, [
            { text: '确定', type: 'success', autoClose: true }
        ]);
        if (afterMoved) {
            goBack();
        } else {
            await getMailDetail(id);
        }
    } catch {
        showMessage.value?.ShowModal("邮件操作失败", ["邮件操作失败，请稍后重试"], "failure");
    }
};
const setCurrentRead = async (read: boolean) => {
    await runCurrentOperation(async id => {
        await Email.setRead({ mappingId: id, data: read });
        if (mail.value) mail.value.isRead = read;
    }, read ? "已标记为已读" : "已标记为未读");
};
const setCurrentStarred = async (starred: boolean) => {
    await runCurrentOperation(async id => {
        await Email.setStarred({ mappingId: id, data: starred });
        if (mail.value) mail.value.isStarred = starred;
    }, starred ? "已添加星标" : "已取消星标");
};
const moveCurrentTo = async (targetFolderId: number) => {
    await runCurrentOperation(async id => {
        await Email.moveMail({ mappingId: id, targetFolderId });
    }, `已移动到${getFolderDisplayName(targetFolderId)}`, true);
};
const deleteCurrent = async () => {
    await runCurrentOperation(async id => {
        await Email.deleteMail(id);
    }, (mail.value?.folderId ?? currentFolderId.value) === 6 ? "已移入彻底删除" : "已移入已删除", true);
};
const toggleCurrentStarred = async () => {
    await setCurrentStarred(!(mail.value?.isStarred ?? false));
};
const getCurrentUserAddresses = (): string[] => {
    const addresses = [String(userStore.mailBoxView ?? '')];
    return addresses.map(address => address.trim()).filter(Boolean);
};
const forwardCurrent = async () => {
    const id = requireCurrentMappingId();
    if (!id) return;
    closeActionMenus();
    try {
        const detail = mail.value ?? (await Email.getMailDetail(id)).data;
        const { draft, skippedAttachments } = await MailService.createForwardDraft(detail);
        const tempDraft = MailService.storeTemporaryDraft(draft);
        await router.push({ name: 'composeBox', query: { tempDraft } });
        if (skippedAttachments.length > 0) {
            setTimeout(() => {
                const previewNames = skippedAttachments.slice(0, 3).join('、');
                const suffix = skippedAttachments.length > 3 ? ` 等 ${skippedAttachments.length} 个附件` : '';
                showMessage.value?.ShowModal("邮件转发", [`部分附件因读取失败或超出大小限制未加入：${previewNames}${suffix}`], "warning", undefined, [
                    { text: '确定', type: 'success', autoClose: true }
                ]);
            }, 300);
        }
    } catch {
        showMessage.value?.ShowModal("邮件转发失败", ["转发邮件准备失败，请稍后重试"], "failure");
    }
};
const replyCurrent = async (replyAll = false) => {
    const id = requireCurrentMappingId();
    if (!id) return;
    closeActionMenus();
    try {
        const detail = mail.value ?? (await Email.getMailDetail(id)).data;
        const draft = MailService.createReplyDraft(detail, {
            replyAll,
            currentUserAddresses: getCurrentUserAddresses(),
        });
        const tempDraft = MailService.storeTemporaryDraft(draft);
        await router.push({ name: 'composeBox', query: { tempDraft } });
    } catch {
        showMessage.value?.ShowModal("邮件回复失败", ["回复邮件准备失败，请稍后重试"], "failure");
    }
};
const markMenuItems = [
    { key: 'star', icon: 'icon-likesprograma-shoucang2', title: '星标', onClick: () => setCurrentStarred(true) },
    { key: 'unstar', icon: 'icon-likesprograma-shoucang', title: '取消星标', onClick: () => setCurrentStarred(false) },
    { key: 'read', icon: 'icon-likesprogramyoujian2', title: '已读', onClick: () => setCurrentRead(true) },
    { key: 'unread', icon: 'icon-likesprogramyoujian', title: '未读', onClick: () => setCurrentRead(false) },
    { key: 'spam', icon: 'icon-likesprogramkuloutou', title: '垃圾邮件', onClick: () => moveCurrentTo(4) },
    { key: 'ad', icon: 'icon-likesprogramjubao', title: '广告邮件', onClick: () => moveCurrentTo(5) },
];
const actionItems: ActionItem[] = [
    { icon: "icon-likesprogramhuishouzhan", title: "删除", action: EmailActions.Delete, onClick: deleteCurrent },
    { icon: "icon-likesprograma-shangyibu1", title: "回复", action: EmailActions.Revoke, onClick: () => replyCurrent(false) },
    { icon: "icon-likesprograma-shangyibu1", title: "回复全部", action: EmailActions.ReplyAll, onClick: () => replyCurrent(true) },
    { icon: "icon-likesprogramxiayibu", title: "转发", action: EmailActions.Forward, onClick: forwardCurrent },
    { icon: "icon-likesprograma-shoucang", title: "标记为", action: EmailActions.Mark, onClick: showMarkMenu },
    { icon: "icon-likesprogramArrows_Arrows_Right_Left", title: "移动到", action: EmailActions.MoveTo, onClick: showMoveMenu },
];
const mapAttachment = (attachment: MailAttachment): MailPreviewAttachment => ({
    id: attachment.attachmentId ?? `${attachment.sourceType || 'MIME_PART'}-${attachment.partIndex ?? attachment.fileName}`,
    fileName: attachment.fileName,
    fileType: attachment.fileType,
    fileSize: attachment.fileSize,
    isInline: attachment.isInline,
    source: 'REMOTE',
    attachmentId: attachment.attachmentId,
    partIndex: attachment.partIndex,
    sourceType: attachment.sourceType,
    mappingId: attachment.mappingId,
});
const previewMail = computed<MailPreviewData | null>(() => {
    if (!mail.value) return null;
    return {
        subject: mail.value.subject,
        from: mail.value.from,
        tos: mail.value.tos,
        ccs: mail.value.ccs,
        htmlBody: mail.value.htmlBody,
        textBody: mail.value.textBody,
        preview: mail.value.preview,
        createTime: mail.value.createTime,
        attachments: (mail.value.attachments ?? []).map(mapAttachment),
    };
});
const getMailDetail = async (id: number) => {
    const requestId = latestMailRequestId + 1;
    latestMailRequestId = requestId;
    if (!Number.isFinite(id) || id <= 0) {
        mail.value = null;
        return;
    }

    try {
        const response = await Email.getMailDetail(id);
        if (requestId !== latestMailRequestId) return;
        mail.value = response.data;
    } catch {
        if (requestId !== latestMailRequestId) return;
        mail.value = null;
        setTimeout(() => {
            showMessage.value?.ShowModal("获取邮件内容", ["获取邮件内容失败，是否重试"], "failure", (selectedButtonIndex) => {
                if(selectedButtonIndex === 1) void getMailDetail(mappingId.value);
            });
        }, 300);
    }
};

watch(mappingId, id => {
    void getMailDetail(id);
}, { immediate: true });

onMounted(() => {
    void loadFolderOptions();
    document.addEventListener('click', closeActionMenus);
});

onBeforeUnmount(() => {
    document.removeEventListener('click', closeActionMenus);
});
</script>

<style scoped>
.read {
    width: 100%;
    height: 100%;
    min-height: 0;
    position: relative;
    z-index: 999;
    top: 0;
    left: 0;
    display: flex;
    flex-direction: column;
    justify-content: center;
    align-items: center;
}
.read .operation {
    width: 100%;
    display: flex;
    gap: 8px;
    padding: 12px 15px;
    padding-bottom: 0px;
    flex-shrink: 0;
    justify-content: flex-start;
    align-items: flex-start;
}
.read .operation .sendButton {
    display: flex;
    justify-content: center;
    align-items: center;
    gap: 4px;
    padding: 4px 7px;
    border-radius: 5px;
    font-size: 14px;
    flex-shrink: 0;
    color: var(--button-text-primary);
    transition: color 0.3s, background-color 0.3s, border-color 0.3s;
    border: 1px solid var(--border-facus);
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
    background-color: var(--button-bg-primary);
}
.read .operation .sendButton .iconfont {
    font-size: 20px;
    color: var(--button-text-primary);
}
.read .operation .sendButton:hover {
    background-color: var(--button-bg-primary-hover);
}
.read .operation .sendButton:active {
    transition: all .1s;
    background-color: var(--button-bg-primary-active);
}
.read .operation .actions {
    flex: 1;
    min-width: 0;
    height: 100%;
    display: flex;
    align-items: center;
}
.mailActionMenu {
    position: absolute;
    z-index: 1200;
    min-width: 150px;
    max-height: min(320px, calc(100vh - 120px));
    padding: 6px;
    display: flex;
    flex-direction: column;
    gap: 4px;
    border: 1px solid var(--border-primary);
    border-radius: 8px;
    background-color: var(--bg-primary);
    box-shadow: var(--shadow-card);
    overflow-y: auto;
}
.mailActionMenuItem {
    width: 100%;
    min-height: 34px;
    padding: 6px 8px;
    border: none;
    border-radius: 6px;
    display: flex;
    align-items: center;
    justify-content: flex-start;
    gap: 8px;
    color: var(--text-primary);
    background-color: transparent;
    cursor: pointer;
    font-size: 14px;
    text-align: left;
}
.mailActionMenuItem:hover {
    background-color: var(--bg-hover-primary);
}
.mailActionMenuItem:active {
    background-color: var(--bg-active-primary);
}
.mailActionMenuItem .iconfont {
    width: 20px;
    flex-shrink: 0;
    font-size: 18px;
}
</style>
