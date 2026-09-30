<template>
    <div class="readMail">
        <div class="mailInfo">
            <div class="subject">
                <span class="subjectText">{{ mail?.subject || '无主题' }}</span>
                <button v-if="showStar" class="btn btn-text starButton" :class="{ active: starred }" @click="emit('toggleStarred')">
                    <span v-if="starred" class="iconfont icon-likesprograma-shoucang2"></span>
                    <span v-else class="iconfont icon-likesprograma-shoucang"></span>
                </button>
            </div>
            <div class="senderInfo">
                <div class="avatar">
                    <img :src="fromAvatar" alt="avatar">
                </div>
                <div class="sender">
                    <div class="mailAddressbox">
                        <span class="mailAddress">
                            <span class="name">{{ mail?.from.name || mail?.from.address }}</span>
                            <span class="address">{{ mail?.from.address }}</span>
                        </span>
                    </div>
                    <div class="mailAddressbox">
                        <span class="title">收件人</span>
                        <span v-if="mail?.tos.length === 0" class="emptyAddress">未填写</span>
                        <span v-for="mailAddress in mail?.tos" :key="mailAddress.address" class="mailAddress">
                            <span class="name">{{ mailAddress.name || mailAddress.address }}</span>
                            <span class="address">{{ mailAddress.address }}</span>
                        </span>
                    </div>
                    <div v-if="mail?.ccs?.length" class="mailAddressbox">
                        <span class="title">抄送</span>
                        <span v-for="mailAddress in mail.ccs" :key="mailAddress.address" class="mailAddress">
                            <span class="name">{{ mailAddress.name || mailAddress.address }}</span>
                            <span class="address">{{ mailAddress.address }}</span>
                        </span>
                    </div>
                    <div v-if="mail?.bccs?.length" class="mailAddressbox">
                        <span class="title">密送</span>
                        <span v-for="mailAddress in mail.bccs" :key="mailAddress.address" class="mailAddress">
                            <span class="name">{{ mailAddress.name || mailAddress.address }}</span>
                            <span class="address">{{ mailAddress.address }}</span>
                        </span>
                    </div>
                </div>
                <div class="otherInformation">
                    <div v-if="mail?.createTime" class="createTime">{{ formatEmailTime(mail.createTime) }}</div>
                </div>
            </div>
            <div v-if="visibleAttachments.length > 0" class="attachmentLine">
                <span class="attachmentLabel">附件</span>
                <div class="attachmentInlineList">
                    <button v-for="attachment in visibleAttachments" :key="getAttachmentKey(attachment)" class="attachmentInlineItem"
                        :title="attachment.fileName" @click="openAttachmentPreview(attachment)">
                        <img :src="getAttachmentIconUrl(attachment.fileName)" alt="file icon">
                        <span>{{ attachment.fileName }}</span>
                    </button>
                </div>
            </div>
            <div class="line">
                <span></span>
            </div>
        </div>

        <div class="content" v-if="mail?.htmlBody" v-html="renderMailDetail(mail.htmlBody)">
        </div>
        <div class="content" v-else-if="mail?.textBody" v-text="mail.textBody">
        </div>
        <div class="content" v-else-if="mail?.preview" v-text="mail.preview">
        </div>
        <div class="content renderError" v-else>
            <span class="iconfont icon-likesprogramjiazaizhong"></span>
            邮件内容为空
        </div>

        <MessageDialog ref="previewDialog" @closeEvent="closeAttachmentPreview">
            <template #title>
                <span>{{ previewAttachment?.fileName || '附件预览' }}</span>
            </template>
            <template #content>
                <div class="attachmentPreviewDialog">
                    <div class="previewHeader">
                        <span>{{ previewAttachment?.fileType || getAttachmentType(previewAttachment?.fileName || '') }}</span>
                        <span>{{ previewAttachment ? formatAttachmentSize(previewAttachment.fileSize) : '' }}</span>
                    </div>
                    <div class="previewBody">
                        <div v-if="isPreviewLoading" class="previewFallback">
                            <span>预览加载中</span>
                        </div>
                        <img v-else-if="previewAttachment && isImageAttachment(previewAttachment) && previewObjectUrl"
                            :src="previewObjectUrl" alt="attachment preview" @error="handlePreviewError">
                        <iframe v-else-if="previewAttachment && (isPdfAttachment(previewAttachment) || isTextLikeAttachment(previewAttachment)) && previewObjectUrl"
                            :src="previewObjectUrl" title="attachment preview"></iframe>
                        <div v-else class="previewFallback">
                            <img v-if="previewAttachment" :src="getAttachmentIconUrl(previewAttachment.fileName)" alt="file icon">
                            <span>{{ isPreviewFailed ? '预览加载失败' : '此文件暂不支持直接预览' }}</span>
                        </div>
                    </div>
                </div>
            </template>
            <template #footer>
                <div class="dialogFooterButton failure" @click="closePreviewDialog">
                    <span class="textBox">
                        <span class="iconfont icon-likesprogramcuowu"></span>
                        <span class="text">关闭</span>
                    </span>
                    <span class="line"></span>
                </div>
                <div class="dialogFooterButton success" :class="{ disabled: !previewAttachment }" @click="downloadPreviewAttachment">
                    <span class="textBox">
                        <span class="iconfont icon-likesprogramxiazai"></span>
                        <span class="text">下载</span>
                    </span>
                </div>
            </template>
        </MessageDialog>

        <ShowMessage ref="showMessage"/>
    </div>
</template>

<script lang="ts">
import { computed, onUnmounted, ref } from 'vue';
import { Email, type MailAttachment } from '@api/email/Email';
import MessageDialog from '@components/messageDialog/MessageDialog.vue';
import ShowMessage from '@components/messageDialog/ShowMessage.vue';
import type { MailPreviewAttachment, MailPreviewData } from '@typings/email/MailPreview';
import { baseUrl } from '@utils/Request';
import { FileProcessing } from '@utils/FileProcessing';
import { ValidateHtml } from '@utils/Utils';
import '@components/messageDialog/MessageDialog.css';

export default {
    name: 'ReadMail',
}
</script>

<script setup lang="ts">
const props = withDefaults(defineProps<{
    mail: MailPreviewData | null;
    showStar?: boolean;
    starred?: boolean;
}>(), {
    showStar: true,
    starred: false,
});
const emit = defineEmits<{
    (e: 'toggleStarred'): void;
}>();

const showMessage = ref<InstanceType<typeof ShowMessage>>();
const previewDialog = ref<InstanceType<typeof MessageDialog> | null>(null);
const previewAttachment = ref<MailPreviewAttachment | null>(null);
const previewObjectUrl = ref('');
const isPreviewLoading = ref(false);
const isPreviewFailed = ref(false);

const visibleAttachments = computed(() => (props.mail?.attachments ?? []).filter(attachment => !attachment.isInline));
const fromAvatar = computed(() => {
    const address = props.mail?.from.address || '';
    const encodedAddress = encodeURIComponent(address);
    return `${baseUrl}/userProfile/mailBoxAvatar?mailBoxAddress=${encodedAddress}&v=${encodedAddress}`;
});
const formatAttachmentSize = (size: number) => FileProcessing.getFileSize(Number(size ?? 0));
const getAttachmentIconUrl = (fileName: string) => FileProcessing.getFileIcon(fileName);
const getAttachmentKey = (attachment: MailPreviewAttachment) => {
    return attachment.id ?? attachment.attachmentId ?? `${attachment.source}-${attachment.partIndex ?? attachment.fileName}`;
};
const getAttachmentType = (fileName: string) => {
    const type = FileProcessing.getFileType(fileName);
    return type === 'unknown' ? '文件' : type.toUpperCase();
};
const getAttachmentMimeType = (attachment: MailPreviewAttachment) => (attachment.fileType || '').toLowerCase();
const isImageAttachment = (attachment: MailPreviewAttachment) => getAttachmentMimeType(attachment).startsWith('image/')
    || /\.(png|jpe?g|gif|webp|bmp|svg)$/i.test(attachment.fileName);
const isPdfAttachment = (attachment: MailPreviewAttachment) => getAttachmentMimeType(attachment) === 'application/pdf'
    || /\.pdf$/i.test(attachment.fileName);
const isTextLikeAttachment = (attachment: MailPreviewAttachment) => {
    const type = getAttachmentMimeType(attachment);
    return type.startsWith('text/')
        || ['application/json', 'application/xml', 'application/javascript'].includes(type)
        || /\.(txt|json|xml|js|ts|css|html)$/i.test(attachment.fileName);
};
const canPreviewAttachment = (attachment: MailPreviewAttachment) => isImageAttachment(attachment) || isPdfAttachment(attachment) || isTextLikeAttachment(attachment);

const renderMailDetail = (mailDetail: string) => {
    const { valid, clean } = ValidateHtml(mailDetail);
    if (!valid) console.warn('邮件内容包含不安全的元素或属性，已自动清理');
    return clean;
};
const cleanupPreviewUrl = () => {
    if (previewObjectUrl.value) {
        URL.revokeObjectURL(previewObjectUrl.value);
        previewObjectUrl.value = '';
    }
};
const handlePreviewError = () => {
    isPreviewFailed.value = true;
    cleanupPreviewUrl();
};
const localAttachmentBlob = (attachment: MailPreviewAttachment): Blob => {
    const fileData = attachment.fileData ?? '';
    if (!fileData) return new Blob([], { type: attachment.fileType || 'application/octet-stream' });
    const commaIndex = fileData.indexOf(',');
    const base64 = commaIndex >= 0 ? fileData.slice(commaIndex + 1) : fileData;
    const binary = atob(base64);
    const bytes = new Uint8Array(binary.length);
    for (let index = 0; index < binary.length; index += 1) bytes[index] = binary.charCodeAt(index);
    return new Blob([bytes], { type: attachment.fileType || 'application/octet-stream' });
};
const toRemoteAttachment = (attachment: MailPreviewAttachment): MailAttachment => ({
    attachmentId: attachment.attachmentId,
    fileName: attachment.fileName,
    fileType: attachment.fileType,
    fileSize: attachment.fileSize,
    isInline: Boolean(attachment.isInline),
    partIndex: attachment.partIndex,
    sourceType: attachment.sourceType,
    mappingId: attachment.mappingId,
});
const attachmentBlob = async (attachment: MailPreviewAttachment): Promise<Blob> => {
    if (attachment.source === 'LOCAL') return localAttachmentBlob(attachment);
    return await Email.attachmentBlob(toRemoteAttachment(attachment));
};
const openAttachmentPreview = async (attachment: MailPreviewAttachment) => {
    cleanupPreviewUrl();
    previewAttachment.value = attachment;
    isPreviewFailed.value = false;
    isPreviewLoading.value = canPreviewAttachment(attachment);
    previewDialog.value?.ShowModal();

    if (!canPreviewAttachment(attachment)) {
        isPreviewLoading.value = false;
        return;
    }

    try {
        const blob = await attachmentBlob(attachment);
        if (previewAttachment.value !== attachment) return;
        previewObjectUrl.value = URL.createObjectURL(blob);
    } catch {
        if (previewAttachment.value === attachment) isPreviewFailed.value = true;
    } finally {
        if (previewAttachment.value === attachment) isPreviewLoading.value = false;
    }
};
const closeAttachmentPreview = () => {
    cleanupPreviewUrl();
    previewAttachment.value = null;
    isPreviewLoading.value = false;
    isPreviewFailed.value = false;
};
const closePreviewDialog = () => {
    previewDialog.value?.Close();
};
const downloadLocalAttachment = (attachment: MailPreviewAttachment) => {
    const blob = localAttachmentBlob(attachment);
    const downloadUrl = URL.createObjectURL(blob);
    const link = document.createElement('a');
    link.href = downloadUrl;
    link.download = attachment.fileName;
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
    URL.revokeObjectURL(downloadUrl);
};
const downloadAttachment = async (attachment: MailPreviewAttachment) => {
    try {
        if (attachment.source === 'LOCAL') {
            downloadLocalAttachment(attachment);
            return;
        }
        await Email.downloadAttachment(toRemoteAttachment(attachment));
    } catch {
        showMessage.value?.ShowModal("附件下载", ["附件下载失败，请稍后重试"], "failure");
    }
};
const downloadPreviewAttachment = async () => {
    if (!previewAttachment.value) return;
    await downloadAttachment(previewAttachment.value);
};
const formatEmailTime = (rawTime: string): string => {
    const date = new Date(rawTime);
    if (Number.isNaN(date.getTime())) return rawTime;
    const now = new Date();

    const pad = (n: number) => n.toString().padStart(2, "0");
    const timeStr = `${pad(date.getHours())}:${pad(date.getMinutes())}`;

    const today = new Date(now.getFullYear(), now.getMonth(), now.getDate());
    const targetDay = new Date(date.getFullYear(), date.getMonth(), date.getDate());
    const diffMs = today.getTime() - targetDay.getTime();
    const diffDays = Math.floor(diffMs / (1000 * 60 * 60 * 24));

    if (diffDays === 0) return `今天 ${timeStr}`;
    if (diffDays === 1) return `昨天 ${timeStr}`;
    if (diffDays < 7 && diffDays > 1) {
        const weekdays = ["周日","周一","周二","周三","周四","周五","周六"];
        return `${weekdays[date.getDay()]} ${timeStr}`;
    }
    if (date.getFullYear() === now.getFullYear()) return `${pad(date.getMonth() + 1)}月${pad(date.getDate())}日`;
    return `${date.getFullYear()}年${pad(date.getMonth() + 1)}月${pad(date.getDate())}日`;
};

onUnmounted(() => {
    cleanupPreviewUrl();
});
</script>

<style scoped>
.readMail {
    width: 100%;
    height: 100%;
    min-height: 0;
    display: flex;
    flex-direction: column;
}
.readMail .mailInfo {
    width: 100%;
    padding: 0px 15px;
    flex-shrink: 0;
}
.readMail .mailInfo .subject {
    width: 100%;
    gap: 10px;
    padding: 15px 0px;
    padding-bottom: 10px;
    display: flex;
    justify-content: flex-start;
    align-items: center;
}
.readMail .mailInfo .subject .subjectText {
    font-size: 16px;
    font-weight: 700;
    color: var(--text-secondary1);
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.readMail .mailInfo .subject .btn {
    font-size: 20px;
    width: 30px;
    height: 30px;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.readMail .mailInfo .subject .btn:hover {
    background-color: var(--bg-secondary3);
}
.readMail .mailInfo .subject .btn:active {
    background-color: var(--bg-secondary4);
}
.readMail .mailInfo .subject .starButton.active {
    color: var(--color-warning);
}
.readMail .mailInfo .senderInfo {
    width: 100%;
    min-height: 0;
    gap: 10px;
    min-width: 0;
    display: flex;
    justify-content: flex-start;
    align-items: center;
}
.readMail .mailInfo .senderInfo .avatar {
    width: 40px;
    height: 40px;
    border-radius: 50%;
    overflow: hidden;
    flex-shrink: 0;
    background-color: var(--bg-primary);
}
.readMail .mailInfo .senderInfo .avatar img {
    width: 100%;
    height: 100%;
}
.readMail .mailInfo .sender {
    height: 100%;
    flex: 1;
    min-width: 0;
    display: flex;
    font-size: 14px;
    gap: 3px;
    flex-direction: column;
    justify-content: center;
    align-items: center;
}
.readMail .mailInfo .sender .mailAddressbox {
    width: 100%;
    flex-shrink: 0;
    gap: 5px;
    display: flex;
    justify-content: flex-start;
    align-items: center;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.readMail .mailInfo .sender .mailAddressbox .mailAddress {
    gap: 5px;
    display: flex;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.readMail .mailInfo .sender .mailAddressbox .title {
    flex-shrink: 0;
    font-size: 14px;
}
.readMail .mailInfo .sender .mailAddressbox .name {
    min-width: 20px;
    font-weight: 700;
    color: var(--text-secondary1);
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.readMail .mailInfo .sender .mailAddressbox .address,
.readMail .mailInfo .sender .mailAddressbox .emptyAddress {
    color: var(--text-secondary2);
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.readMail .mailInfo .sender .mailAddressbox .address::after {
    content: ">";
}
.readMail .mailInfo .sender .mailAddressbox .address::before {
    content: "<";
}
.readMail .mailInfo .otherInformation {
    width: 200px;
    align-self: stretch;
    height: auto;
    flex-shrink: 0;
    display: flex;
    justify-content: center;
    align-items: flex-end;
}
.readMail .mailInfo .otherInformation .createTime {
    width: 200px;
    font-size: 14px;
    flex-shrink: 0;
    gap: 5px;
    display: flex;
    justify-content: flex-end;
    align-items: center;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.readMail .mailInfo .attachmentLine {
    width: calc(100% - 50px);
    min-height: 38px;
    margin-top: 10px;
    margin-left: 50px;
    padding: 7px 10px;
    display: flex;
    align-items: center;
    gap: 8px;
    font-size: 14px;
    border-radius: 8px;
    background-color: rgba(255, 255, 255, .88);
    box-shadow: var(--shadow-card);
    backdrop-filter: blur(8px);
}
.readMail .mailInfo .attachmentLabel {
    flex-shrink: 0;
    padding-right: 2px;
    font-size: 14px;
}
.readMail .mailInfo .attachmentInlineList {
    min-width: 0;
    flex: 1;
    display: flex;
    align-items: center;
    flex-wrap: wrap;
    gap: 6px 10px;
}
.readMail .mailInfo .attachmentInlineItem {
    max-width: 260px;
    min-width: 0;
    height: 26px;
    padding: 0px 8px 0px 6px;
    display: flex;
    align-items: center;
    gap: 5px;
    border-radius: 6px;
    color: var(--text-link);
    font-size: 13px;
    border: 1px solid transparent;
    background-color: rgba(232, 246, 255, .78);
    transition: background-color .3s, color .3s, border-color .3s, box-shadow .3s;
}
.readMail .mailInfo .attachmentInlineItem:hover {
    color: var(--text-link-hover);
    border-color: rgba(72, 173, 238, .32);
    background-color: rgba(214, 239, 255, .95);
    box-shadow: 0 2px 6px rgba(67, 132, 178, .1);
}
.readMail .mailInfo .attachmentInlineItem img {
    width: 18px;
    height: 18px;
    flex-shrink: 0;
    object-fit: contain;
}
.readMail .mailInfo .attachmentInlineItem span {
    min-width: 0;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.readMail .line {
    width: 100%;
    height: 1px;
    padding: 0px 15px;
    margin-top: 12px;
    display: flex;
    justify-content: center;
    align-items: center;
}
.readMail .line span {
    width: 100%;
    height: 1px;
    background-color: var(--bg-secondary3);
}
.readMail .content {
    width: 100%;
    flex: 1;
    min-height: 0;
    padding: 0px 20px;
    overflow: hidden;
    overflow-x: auto;
    overflow-y: auto;
}
.readMail .content.renderError {
    width: 100%;
    height: 100%;
    font-family: 'artisticFonts';
    font-size: 48px;
    color: var(--text-tertiary);
    display: flex;
    flex-direction: column;
    justify-content: center;
    align-items: center;
}
.readMail .content.renderError .iconfont {
    font-size: 100px;
}
.attachmentPreviewDialog {
    width: min(900px, calc(100vw - 80px));
    height: min(620px, calc(100vh - 180px));
    min-height: 360px;
    padding: 0px 14px 14px;
    display: flex;
    flex-direction: column;
    gap: 10px;
    overflow: hidden;
    background-color: var(--bg-primary);
}
.attachmentPreviewDialog .previewHeader {
    height: 32px;
    flex-shrink: 0;
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
    color: var(--text-secondary2);
    font-size: 13px;
}
.attachmentPreviewDialog .previewHeader span {
    min-width: 0;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.attachmentPreviewDialog .previewBody {
    min-height: 0;
    flex: 1;
    border-radius: 8px;
    overflow: hidden;
    background-color: var(--bg-secondary4);
    display: flex;
    align-items: center;
    justify-content: center;
}
.attachmentPreviewDialog .previewBody > img,
.attachmentPreviewDialog .previewBody > iframe {
    width: 100%;
    height: 100%;
    border: none;
    object-fit: contain;
}
.attachmentPreviewDialog .previewFallback {
    width: 100%;
    height: 100%;
    padding: 24px;
    display: flex;
    align-items: center;
    justify-content: center;
    flex-direction: column;
    gap: 12px;
    color: var(--text-tertiary);
    font-size: 14px;
}
.attachmentPreviewDialog .previewFallback img {
    width: 72px;
    height: 72px;
    object-fit: contain;
}
</style>
