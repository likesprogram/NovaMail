<template>
    <div class="compose">
        <!-- 操作列表 -->
        <div class="operation">
            <div class="sendButton" @click="sendMail">
                <span class="iconfont icon-likesprogramfasong"></span>
                <span class="actionTitle">发送</span>
            </div>
            <div class="actions">
                <ActionBar :actionItems="actionItems"></ActionBar>
            </div>
        </div>
        <SafePopover ref="attachmentPanel" :NoBoxShadow="['all']" :Placement="'Bottom,Left'">
            <div class="attachmentPanel" @click.stop>
                <button class="attachmentOption" @click="uploadLocalAttachments">
                    <span class="optionIcon iconfont icon-likesprogramshangchuan"></span>
                    <span class="optionText">
                        <span class="optionTitle">上传本地文件</span>
                        <span class="optionDesc">作为邮件附件发送</span>
                    </span>
                </button>
                <button class="attachmentOption" @click="openAttachmentFileBoxPicker">
                    <span class="optionIcon iconfont icon-likesprogramwenzhang"></span>
                    <span class="optionText">
                        <span class="optionTitle">从文件中转站选择</span>
                        <span class="optionDesc">发送时追加附件链接</span>
                    </span>
                </button>
            </div>
        </SafePopover>
        <div class="content" ref="contentRef">
            <!-- 输入框 -->
            <div class="inputBox">
                <!-- 写邮件组件 -->
                <div class="write">
                    <WriteMail v-model="mailDraft"></WriteMail>
                </div>
                <div v-if="mailDraft.attachment.length > 0" class="attachmentStrip">
                    <div v-for="(attachment, index) in mailDraft.attachment" :key="`${attachment.sendType}-${attachment.fileName}-${index}`" class="attachmentChip">
                        <img :src="getAttachmentIconUrl(attachment.fileName)" alt="file icon">
                        <span class="attachmentInfo">
                            <span class="attachmentName">{{ attachment.fileName }}</span>
                            <span class="attachmentMeta">{{ formatFileSize(attachment.fileSize) }} · {{ attachment.sendType === 'TRANSFER_LINK' ? '中转站链接' : '本地附件' }}</span>
                        </span>
                        <button class="button btn-text iconfont icon-likesprogramcancel" title="移除附件" @click="removeAttachment(index)"></button>
                    </div>
                </div>
                <!-- 发件人设置 -->
                <div class="sendMailSetting">
                    <span class="title">发件人：</span>
                    <span class="sendMail" ref="sendMailSetting" @click="showSendMailSettingPanel">
                        <span class="nickname">{{ mailDraft.sendNickname }}</span>
                        <span class="mailAddress">{{ mailDraft.sendMailBoxView }}</span>
                        <span class="expandIcon iconfont icon-likesprogramxiangyouxiayiye"></span>
                        <!-- 发件人设置面板 -->
                        <SafePopover ref="sendMailSettingPanel" :NoBoxShadow="['all']" :Width="'95%'" :Placement="'Top'">
                            <div class="sendMailSettingPanel" @click.stop>
                                <div class="mailBoxInfo" v-for="mailBox in mailBoxes" @click="selectedSendMailBox(mailBox)">
                                    <div class="button btn-text mailBox">
                                        <span class="address">{{ mailBox.mailBoxView }}</span>
                                        <span class="iconfont icon-likesprogramzhengque" v-if="mailBox.mailBoxId == mailDraft.sendMailBoxId"></span>
                                    </div>
                                </div>
                                <div class="sendNickname" v-if="editSendNickname">
                                    <input type="text" ref="nicknameEdit" v-model="editSendNicknameValue" class="value" placeholder="请输入发件名称">
                                    <button class="btn-text" @click.stop="saveSendNickname">
                                        <span></span>
                                        <span>完成</span>
                                    </button>
                                </div>
                                <div class="sendNickname" v-else>
                                    <span class="title">发件人名称：</span>
                                    <span class="value">{{ mailDraft.sendNickname }}</span>
                                    <button class="btn-text" @click.stop="openEditSendNickname">修改</button>
                                </div>
                            </div>
                        </SafePopover>
                    </span>
                </div>
            </div>
            <!-- 联系人列表 -->
            <div class="contact" v-if="showContact">
                <div class="contactHeader">
                    <div class="contactTitleGroup">
                        <span class="iconfont icon-likesprogramtongxunlu"></span>
                        <span class="contactTitle">联系人</span>
                    </div>
                    <span class="contactCount">{{ isContactsLoading ? '加载中' : `${filteredContactTotal}/${contacts.length}` }}</span>
                </div>

                <div class="searchInput" :class="{ searchClearShown: contactSearch.trim().length > 0 }">
                    <input type="text" v-model="contactSearch" placeholder="搜索联系人或邮箱">
                    <button v-if="contactSearch.trim().length > 0" class="button btn-text iconfont icon-likesprogramcancel"
                        title="清空搜索" @click="contactSearch = ''"></button>
                    <div v-else class="button btn-text iconfont icon-likesprogramchaxun"></div>
                </div>

                <div class="recipientSwitch" aria-label="选择添加位置">
                    <button v-for="option in recipientTargetOptions" :key="option.value"
                        :class="{ selected: recipientTarget === option.value }"
                        @click="recipientTarget = option.value">
                        {{ option.label }}
                    </button>
                </div>

                <div class="contactTree">
                    <div v-for="group in filteredContactGroups" :key="group.id" class="contactGroupSection">
                        <button class="contactGroupHeader" @click="toggleContactGroup(group.id)">
                            <span class="groupTitle">
                                <span class="expandIcon iconfont icon-likesprogramxiangyouxiayiye"
                                    :class="{ expanded: isContactGroupExpanded(group.id) }"></span>
                                <span class="groupName">{{ group.name }}</span>
                            </span>
                            <span class="groupCount">{{ group.contacts.length }}</span>
                        </button>

                        <div v-if="isContactGroupExpanded(group.id)" class="contactList">
                            <button v-for="contact in group.contacts" :key="contact.id" class="contactItem"
                                :class="{ selected: isContactSelected(contact.email) }"
                                :title="`${contact.name} <${contact.email}>`"
                                @click="addContactToDraft(contact)">
                                <span class="avatar">
                                    <img :src="getContactAvatarUrl(contact.email)" alt="avatar">
                                </span>
                                <span class="contactInfo">
                                    <span class="contactName">{{ contact.name }}</span>
                                    <span class="contactEmail">{{ contact.email }}</span>
                                </span>
                                <span class="contactAction">
                                    <span v-if="isContactSelected(contact.email)" class="iconfont icon-likesprogramzhengque"></span>
                                    <span v-else class="iconfont icon-likesprogramplus"></span>
                                </span>
                            </button>
                        </div>
                    </div>

                    <div v-if="filteredContactTotal === 0" class="emptyContact">
                        <span class="iconfont icon-likesprogramchaxun"></span>
                        <span>没有匹配的联系人</span>
                    </div>
                </div>

                <div class="contactFooter">
                    <span>{{ selectedRecipientCount }} 位已添加</span>
                    <button class="btn-text" :class="{ 'btn-disabled': selectedRecipientCount === 0 }"
                        @click="clearSelectedRecipients">
                        清空
                    </button>
                </div>
            </div>
        </div>

        <MessageDialog ref="fileBoxDialog" @closeEvent="cleanupPreviewUrl">
            <template #title>
                <span>从文件中转站选择</span>
            </template>
            <template #content>
                <div class="filePickerDialog">
                    <div class="filePickerSidebar">
                        <div class="filePickerSearch">
                            <input v-model="filePickerKeyword" type="text" placeholder="搜索文件">
                            <span class="iconfont icon-likesprogramchaxun"></span>
                        </div>
                        <div class="filePickerList">
                            <button v-for="file in filteredFilePickerFiles" :key="file.fileId" class="filePickerItem"
                                :class="{ selected: file.fileId === selectedPickerFileId }"
                                @click="selectPickerFile(file.fileId)">
                                <span class="filePickerIcon">
                                    <img :src="getAttachmentIconUrl(file.fileName)" alt="file icon">
                                </span>
                                <span class="filePickerInfo">
                                    <span class="filePickerName">{{ file.fileName }}</span>
                                    <span class="filePickerMeta">{{ formatFileSize(file.fileSize) }} · {{ file.filePath }}</span>
                                </span>
                            </button>
                            <div v-if="filteredFilePickerFiles.length === 0" class="filePickerEmpty">
                                {{ isLoadingPickerFiles ? '加载中' : '暂无可选文件' }}
                            </div>
                        </div>
                    </div>
                    <div class="filePickerPreview">
                        <template v-if="selectedPickerFile">
                            <div class="filePickerPreviewHeader">
                                <span>{{ selectedPickerFile.fileName }}</span>
                                <span>{{ selectedPickerFile.fileType || '未知类型' }}</span>
                            </div>
                            <div class="filePickerPreviewBody">
                                <div v-if="isLoadingPreview" class="filePickerPreviewFallback">
                                    <span>预览加载中</span>
                                </div>
                                <img v-else-if="isImageFile(selectedPickerFile) && previewObjectUrl" :src="previewObjectUrl" alt="preview"
                                    @error="handlePreviewLoadError">
                                <iframe v-else-if="(isPdfFile(selectedPickerFile) || isTextLikeFile(selectedPickerFile)) && previewObjectUrl"
                                    :src="previewObjectUrl" title="file preview"></iframe>
                                <div v-else class="filePickerPreviewFallback">
                                    <img :src="getAttachmentIconUrl(selectedPickerFile.fileName)" alt="file icon">
                                    <span>{{ isPreviewFailed ? '预览加载失败' : '此文件暂不支持直接预览' }}</span>
                                </div>
                            </div>
                        </template>
                        <div v-else class="filePickerPreviewFallback">
                            <span class="iconfont icon-likesprogramwenzhang"></span>
                            <span>选择文件后预览</span>
                        </div>
                    </div>
                </div>
            </template>
            <template #footer>
                <div class="dialogFooterButton failure" @click="closeFileBoxDialog">
                    <span class="textBox">
                        <span class="iconfont icon-likesprogramcuowu"></span>
                        <span class="text">取消</span>
                    </span>
                    <span class="line"></span>
                </div>
                <div class="dialogFooterButton success" :class="{ disabled: !selectedPickerFile }" @click="addSelectedFileBoxAttachment">
                        <span class="textBox">
                            <span class="iconfont icon-likesprogramzhengque"></span>
                        <span class="text">添加附件链接</span>
                    </span>
                </div>
            </template>
        </MessageDialog>

        <MessageDialog ref="mailPreviewDialog">
            <template #title>
                <span>邮件预览</span>
            </template>
            <template #content>
                <div class="mailPreviewDialog">
                    <ReadMail :mail="previewMail" :showStar="false" />
                </div>
            </template>
            <template #footer>
                <div class="dialogFooterButton success" @click="closeMailPreview">
                    <span class="textBox">
                        <span class="iconfont icon-likesprogramzhengque"></span>
                        <span class="text">完成</span>
                    </span>
                </div>
            </template>
        </MessageDialog>
        
        <!-- 消息提示 -->
        <ShowMessage ref="showMessage"/>
    </div>
</template>

<script lang="ts">
import { ref, computed, onMounted, onUnmounted, watch } from 'vue';
import ActionBar, { type ActionItem } from '@components/ActionBar.vue';
import { EmailActions } from '@typings/emailList/EmailActions';
import { UserStore } from '@store/UserStore';
import SafePopover from '@components/SafePopover.vue';
import WriteMail from '@components/WriteMail.vue';
import ReadMail from '@components/ReadMail.vue';
import ShowMessage from '@components/messageDialog/ShowMessage.vue';
import MessageDialog from '@components/messageDialog/MessageDialog.vue';
import { MailService, type Attachment, type MailDraft, type MailExtraData } from '@service/MailService';
import { useRoute, useRouter } from 'vue-router';
import type { MailBoxData } from '@api/email/MailBox';
import { MailBoxService } from '@service/MailBoxService';
import { Email, type SaveDraftData, type SendMailData } from '@api/email/Email';
import { SignatureService } from '@service/SignatureService'
import { ApiException, baseUrl } from '@utils/Request';
import { AddressBookApi } from '@api/users/AddressBook';
import type { Contact as AddressBookContact, ContactGroup as AddressBookGroup } from '@api/users/AddressBook';
import { FileProcessing } from '@utils/FileProcessing';
import { UserFiles, type UserFile } from '@api/users/UserFiles';
import type { MailPreviewData } from '@typings/email/MailPreview';
import '@components/messageDialog/MessageDialog.css';

export default {
    name: 'Compose',
}
</script>

<script setup lang="ts">
type RecipientTarget = 'to' | 'cc' | 'bcc';
type ContactItem = {
    id: string;
    name: string;
    email: string;
};
type ComposeContactGroup = {
    id: string;
    name: string;
    contacts: ContactItem[];
};

const recipientTargetOptions: Array<{ label: string; value: RecipientTarget }> = [
    { label: '收件', value: 'to' },
    { label: '抄送', value: 'cc' },
    { label: '密送', value: 'bcc' },
];

// 路由
const route = useRoute();
const router = useRouter();

// 用户
const userStore = UserStore();

// 对话框实例
const showMessage = ref<InstanceType<typeof ShowMessage>>();
const attachmentPanel = ref<InstanceType<typeof SafePopover> | null>(null);
const fileBoxDialog = ref<InstanceType<typeof MessageDialog> | null>(null);
const mailPreviewDialog = ref<InstanceType<typeof MessageDialog> | null>(null);
const filePickerFiles = ref<UserFile[]>([]);
const selectedPickerFileId = ref('');
const filePickerKeyword = ref('');
const isLoadingPickerFiles = ref(false);
const isLoadingPreview = ref(false);
const isPreviewFailed = ref(false);
const previewObjectUrl = ref('');

// 邮件草稿
const mailDraft = ref<MailDraft>(MailService.createDefaultMailDraft());
// 当前草稿ID
const mailDraftId = ref<string>('');
const previewMail = ref<MailPreviewData | null>(null);

const showAttachmentPanel = (target?: HTMLElement) => {
    if (target) attachmentPanel.value?.show(target);
};
const actionItems = computed<ActionItem[]>(() => [
    { icon: "icon-likesprogramtianxie", title: "预览", action: EmailActions.Edit, onClick: showMailPreview },
    { icon: "icon-likesprogrambaocun", title: "保存草稿", action: EmailActions.Edit, onClick: saveDraft },
    { icon: "icon-likesprogramclip", title: "附件", action: EmailActions.Revoke, onClick: showAttachmentPanel },
]);

const formatFileSize = (size: number) => FileProcessing.getFileSize(Number(size ?? 0));
const getAttachmentIconUrl = (fileName: string) => FileProcessing.getFileIcon(fileName);
const addAttachmentToDraft = (attachment: Attachment) => {
    mailDraft.value.attachment = [...mailDraft.value.attachment, attachment];
};
const removeAttachment = (index: number) => {
    mailDraft.value.attachment = mailDraft.value.attachment.filter((_item, itemIndex) => itemIndex !== index);
};
const uploadLocalAttachments = async () => {
    try {
        const result = await FileProcessing.openFiles('*', true);
        const files = Array.from(result instanceof FileList ? result : [result]);
        if (files.length === 0) return;
        const attachments: Attachment[] = [];
        const pendingData: MailExtraData[] = [];
        for (const file of files) {
            const fileData = String(await FileProcessing.fileToBase64(file) ?? '').trim();
            if (!fileData) continue;
            const attachment: Attachment = {
                fileName: file.name,
                fileType: file.type || 'application/octet-stream',
                fileSize: file.size,
                fileData,
                sendType: 'MIME_ATTACHMENT',
            };
            pendingData.push({ type: 'attachment', attachment });
            const estimateMailSize = MailService.estimateMailSize(mailDraft.value, pendingData);
            const maxMailSize = MailService.getMaxMailSize();
            if (estimateMailSize > maxMailSize) {
                showMessage.value?.ShowModal("附件添加失败", ["邮件大小已经超出限制，请改用文件中转站链接发送"], "failure");
                return;
            }
            attachments.push(attachment);
        }
        if (attachments.length === 0) {
            showMessage.value?.ShowModal("附件添加失败", ["未读取到有效的文件数据"], "failure");
            return;
        }
        mailDraft.value.attachment = [...mailDraft.value.attachment, ...attachments];
        const finalSize = MailService.estimateMailSize(mailDraft.value);
        const maxMailSize = MailService.getMaxMailSize();
        if (finalSize >= maxMailSize * 0.8) {
            setTimeout(() => {
                showMessage.value?.ShowModal("邮件大小警告", ["邮件大小即将超出限制，建议将较大的文件改用文件中转站链接发送"], "warning");
            }, 0);
        }
        attachmentPanel.value?.hide();
    } catch (error) {
        if ((error as { code?: number })?.code === 0) return;
        showMessage.value?.ShowModal("附件添加失败", ["文件读取失败，请重新选择文件"], "failure");
    }
};
const cleanupPreviewUrl = () => {
    if (previewObjectUrl.value) {
        URL.revokeObjectURL(previewObjectUrl.value);
        previewObjectUrl.value = '';
    }
};
const isImageFile = (file: UserFile) => /^(image\/)/i.test(file.fileType || '') || /\.(png|jpe?g|gif|webp|bmp)$/i.test(file.fileName);
const isPdfFile = (file: UserFile) => (file.fileType || '').toLowerCase() === 'application/pdf' || /\.pdf$/i.test(file.fileName);
const isTextLikeFile = (file: UserFile) => {
    const type = (file.fileType || '').toLowerCase();
    return type.startsWith('text/') || ['application/json', 'application/xml', 'application/javascript'].includes(type) || /\.(txt|json|xml|js|ts|css|html)$/i.test(file.fileName);
};
const selectedPickerFile = computed(() => filePickerFiles.value.find(file => file.fileId === selectedPickerFileId.value) ?? null);
const filteredFilePickerFiles = computed(() => {
    const keyword = filePickerKeyword.value.trim().toLowerCase();
    return filePickerFiles.value.filter(file => {
        return !keyword
            || file.fileName.toLowerCase().includes(keyword)
            || file.filePath.toLowerCase().includes(keyword)
            || (file.fileType || '').toLowerCase().includes(keyword);
    });
});
const handlePreviewLoadError = () => {
    isPreviewFailed.value = true;
    cleanupPreviewUrl();
};
const loadPickerPreview = async (file: UserFile) => {
    cleanupPreviewUrl();
    isPreviewFailed.value = false;
    isLoadingPreview.value = true;
    try {
        if (isImageFile(file) || isPdfFile(file) || isTextLikeFile(file)) {
            const blob = await UserFiles.previewBlob(file.fileId);
            previewObjectUrl.value = URL.createObjectURL(blob);
        }
    } catch {
        isPreviewFailed.value = true;
    } finally {
        isLoadingPreview.value = false;
    }
};
const selectPickerFile = async (fileId: string) => {
    selectedPickerFileId.value = fileId;
    const file = filePickerFiles.value.find(item => item.fileId === fileId);
    if (file) await loadPickerPreview(file);
};
const openAttachmentFileBoxPicker = async () => {
    attachmentPanel.value?.hide();
    isLoadingPickerFiles.value = true;
    filePickerFiles.value = [];
    selectedPickerFileId.value = '';
    filePickerKeyword.value = '';
    isPreviewFailed.value = false;
    isLoadingPreview.value = false;
    cleanupPreviewUrl();
    try {
        const result = await UserFiles.list();
        const files = Array.isArray(result.data) ? result.data : [];
        filePickerFiles.value = files.filter(file => file.fileStatus === 'ACTIVE');
        selectedPickerFileId.value = filePickerFiles.value[0]?.fileId ?? '';
        fileBoxDialog.value?.ShowModal();
        const firstFile = filePickerFiles.value[0];
        if (firstFile) void loadPickerPreview(firstFile);
    } catch {
        showMessage.value?.ShowModal('文件选择', ['文件中转站列表加载失败，请稍后重试'], 'failure');
    } finally {
        isLoadingPickerFiles.value = false;
    }
};
const closeFileBoxDialog = () => {
    fileBoxDialog.value?.Close();
};
const addSelectedFileBoxAttachment = () => {
    const file = selectedPickerFile.value;
    if (!file) return;
    const referenceUrl = UserFiles.downloadUrl(file.fileId);
    if (mailDraft.value.attachment.some(item => item.sendType === 'TRANSFER_LINK' && item.fileData === referenceUrl)) {
        showMessage.value?.ShowModal('文件选择', ['该文件链接已经添加'], 'warning');
        return;
    }
    const attachment: Attachment = {
        fileName: file.fileName,
        fileType: file.fileType || 'application/octet-stream',
        fileSize: file.fileSize,
        fileData: referenceUrl,
        sendType: 'TRANSFER_LINK',
    };
    addAttachmentToDraft(attachment);
    fileBoxDialog.value?.Close();
};

// 联系人列表
const contactSearch = ref('');
const recipientTarget = ref<RecipientTarget>('to');
const addressBookGroups = ref<AddressBookGroup[]>([]);
const addressBookContacts = ref<AddressBookContact[]>([]);
const isContactsLoading = ref(false);
const expandedContactGroupIds = ref<string[]>([]);
const normalizeEmail = (email: string) => email.trim().toLowerCase();
const getContactAvatarUrl = (email: string) => {
    const mailBoxAddress = encodeURIComponent(email.trim());
    return `${baseUrl}/userProfile/mailBoxAvatar?mailBoxAddress=${mailBoxAddress}&v=${mailBoxAddress}`;
};
const getErrorMessage = (error: unknown, fallback: string) => {
    return error instanceof ApiException ? error.message : fallback;
};
const mapAddressBookContact = (contact: AddressBookContact): ContactItem => ({
    id: contact.contactId,
    name: contact.contactName,
    email: contact.email,
});
const buildContactGroups = (groups: AddressBookGroup[], contactList: AddressBookContact[]): ComposeContactGroup[] => {
    const nextGroups = groups
        .map(group => ({
            id: group.groupId,
            name: group.groupName,
            contacts: contactList
                .filter(contact => contact.groupId === group.groupId)
                .map(mapAddressBookContact),
        }))
        .filter(group => group.contacts.length > 0);
    const ungroupedContacts = contactList
        .filter(contact => !contact.groupId)
        .map(mapAddressBookContact);
    return ungroupedContacts.length > 0
        ? [{ id: 'ungrouped', name: '未分组', contacts: ungroupedContacts }, ...nextGroups]
        : nextGroups;
};
const contactGroups = computed(() => buildContactGroups(addressBookGroups.value, addressBookContacts.value));
const contacts = computed(() => contactGroups.value.flatMap(group => group.contacts));
const loadAddressBookContacts = async () => {
    isContactsLoading.value = true;
    try {
        const result = await AddressBookApi.get();
        addressBookGroups.value = result.data.groups;
        addressBookContacts.value = result.data.contacts;
        expandedContactGroupIds.value = buildContactGroups(result.data.groups, result.data.contacts).map(group => group.id);
    } catch (error) {
        showMessage.value?.ShowModal("联系人加载失败", [getErrorMessage(error, "通讯录联系人加载失败，请稍后重试")], "failure");
    } finally {
        isContactsLoading.value = false;
    }
};
const getRecipientList = (target: RecipientTarget) => mailDraft.value[target] ?? [];
const selectedRecipientCount = computed(() => {
    return new Set([...mailDraft.value.to, ...mailDraft.value.cc, ...mailDraft.value.bcc].map(normalizeEmail)).size;
});
const filteredContactGroups = computed(() => {
    const keyword = contactSearch.value.trim().toLowerCase();
    return contactGroups.value
        .map(group => ({
            ...group,
            contacts: group.contacts.filter(contact => {
                return !keyword
                    || group.name.toLowerCase().includes(keyword)
                    || contact.name.toLowerCase().includes(keyword)
                    || contact.email.toLowerCase().includes(keyword);
            })
        }))
        .filter(group => group.contacts.length > 0);
});
const filteredContactTotal = computed(() => {
    return filteredContactGroups.value.reduce((total, group) => total + group.contacts.length, 0);
});
const isContactGroupExpanded = (groupId: string) => {
    return contactSearch.value.trim().length > 0 || expandedContactGroupIds.value.includes(groupId);
};
const toggleContactGroup = (groupId: string) => {
    if (isContactGroupExpanded(groupId)) {
        expandedContactGroupIds.value = expandedContactGroupIds.value.filter(id => id !== groupId);
        return;
    }
    expandedContactGroupIds.value = [...expandedContactGroupIds.value, groupId];
};
const isContactSelected = (email: string) => {
    const normalizedEmail = normalizeEmail(email);
    return [...mailDraft.value.to, ...mailDraft.value.cc, ...mailDraft.value.bcc]
        .some(item => normalizeEmail(item) === normalizedEmail);
};
const addContactToDraft = (contact: ContactItem) => {
    const targetList = getRecipientList(recipientTarget.value);
    if (targetList.some(item => normalizeEmail(item) === normalizeEmail(contact.email))) return;
    mailDraft.value[recipientTarget.value] = [...targetList, contact.email];
};
const clearSelectedRecipients = () => {
    mailDraft.value.to = [];
    mailDraft.value.cc = [];
    mailDraft.value.bcc = [];
    mailDraft.value.recipients = [];
};

// 自适应宽度
const contentRef = ref<HTMLElement | null>(null);
const showContact = ref(true);
let observer: ResizeObserver;

// 发件人设置按钮
const sendMailSetting = ref<HTMLElement | null>(null);
// 发件人设置面板
const sendMailSettingPanel = ref<InstanceType<typeof SafePopover> | null>(null);
// 显示发件人设置面板
const mailBoxes = ref<MailBoxData[]>(); // 邮箱列表
const showSendMailSettingPanel = () => {
    if (mailBoxes.value?.length) {
        sendMailSettingPanel.value?.show(sendMailSetting.value as HTMLElement);
        return;
    }
    if (!userStore.userInfo?.userId) {
        setTimeout(() => {
            showMessage.value?.ShowModal("邮箱列表获取失败", ["邮箱列表获取失败: 用户 ID 为空，是否重试"], "failure", (selectedButtonIndex) => {
                if(selectedButtonIndex === 1) showSendMailSettingPanel();
            });
        }, 300);
        return;
    }
    MailBoxService.getMailBoxList(userStore.userInfo?.userId).then((response: MailBoxData[]) => {
        mailBoxes.value = response;
        sendMailSettingPanel.value?.show(sendMailSetting.value as HTMLElement);
    }).catch((error) => {
        setTimeout(() => {
            showMessage.value?.ShowModal("邮箱列表获取失败", ["邮箱列表获取失败:" + error.message, "是否重试"], "failure", (selectedButtonIndex) => {
                if(selectedButtonIndex === 1) showSendMailSettingPanel();
            });
        }, 300);
    });
}
// 发件邮箱设置
const selectedSendMailBox = (mailBox: MailBoxData) => {
    mailDraft.value.sendMailBoxId = mailBox.mailBoxId;
    mailDraft.value.sendMailBoxView = mailBox.mailBoxView;
}
// 发件人名称设置
const editSendNickname = ref<boolean>(false);
const editSendNicknameValue = ref<string>('');
// 发件人名称输入框
const nicknameEdit = ref<HTMLElement | null>(null);
// 进入名称设置
const openEditSendNickname = () => {
    editSendNicknameValue.value = mailDraft.value.sendNickname;
    editSendNickname.value = true;
    nicknameEdit.value?.focus();
}
// 完成编辑
const MAX_SEND_NICKNAME_LENGTH = 16;
const saveSendNickname = () => {
    const nickname = editSendNicknameValue.value.trim();
    if (!nickname) {
        showMessage.value?.ShowModal("名称设置失败", ["发件人名称不能为空"], "failure");
        editSendNickname.value = false;
        return;
    }
    // 使用 Array.from，避免 emoji / 特殊 Unicode 字符截断异常
    const nicknameChars = Array.from(nickname);
    if (nicknameChars.length > MAX_SEND_NICKNAME_LENGTH) {
        mailDraft.value.sendNickname = nicknameChars.slice(0, MAX_SEND_NICKNAME_LENGTH).join('');
        showMessage.value?.ShowModal("名称设置警告", [`发件人名称最多支持 ${MAX_SEND_NICKNAME_LENGTH} 个字符，已自动截取。`], "warning");
    } else {
        mailDraft.value.sendNickname = nickname;
    }
    editSendNicknameValue.value = '';
    editSendNickname.value = false;
};
// 获取路由中的草稿ID
const getRouteMailDraftId = (): string => {
    const value = route.params.mailDraftId;
    if (Array.isArray(value)) return value[0] ?? '';
    return typeof value === 'string' ? value.trim() : '';
};
const getRouteTemporaryDraftToken = (): string => {
    const value = route.query.tempDraft;
    if (Array.isArray(value)) return value[0] ?? '';
    return typeof value === 'string' ? value.trim() : '';
};

// 设置默认发送邮箱与发件人名称
const applyDefaultSender = () => {
    const nickname = userStore.nickname?.trim() ?? '';
    const defaultMailBoxId = String(userStore.userInfo?.defaultMailBoxId ?? '');
    const defaultMailBoxView = String(userStore.mailBoxView ?? '');
    if (!mailDraft.value.sendNickname?.trim() && nickname) mailDraft.value.sendNickname = nickname;
    if (!mailDraft.value.sendMailBoxId?.trim() && defaultMailBoxId) {
        mailDraft.value.sendMailBoxId = defaultMailBoxId;
        mailDraft.value.sendMailBoxView = defaultMailBoxView;
    }
};
// 初始化草稿
const initializeDraft = async () => {
    const routeDraftId = getRouteMailDraftId();
    const temporaryDraftToken = getRouteTemporaryDraftToken();
    mailDraftId.value = routeDraftId;

    if (!routeDraftId && temporaryDraftToken) {
        const temporaryDraft = MailService.consumeTemporaryDraft(temporaryDraftToken);
        mailDraft.value = temporaryDraft ?? MailService.createDefaultMailDraft();
        applyDefaultSender();
        if (!temporaryDraft) {
            showMessage.value?.ShowModal("邮件写信", ["临时草稿已失效，请重新选择邮件"], "warning");
        }
        return;
    }

    // 没有 mailDraftId，表示新建邮件
    if (!routeDraftId) {
        mailDraft.value = MailService.createDefaultMailDraft();
        applyDefaultSender();
        return;
    }

    try {
        const response = await MailService.getDraft(routeDraftId);
        mailDraft.value = {
            ...MailService.createDefaultMailDraft(),
            ...response
        };
    } catch (error) {
        showMessage.value?.ShowModal("草稿加载失败", ["草稿加载失败，请稍后重试"], "failure");
        mailDraft.value = MailService.createDefaultMailDraft();
    }
    applyDefaultSender();
};

// 监听路由草稿ID变化
watch(
    () => ({
        mailDraftId: getRouteMailDraftId(),
        tempDraft: getRouteTemporaryDraftToken(),
        nickname: userStore.nickname ?? '',
        defaultMailBoxId: String(userStore.userInfo?.defaultMailBoxId ?? '')
    }),
    async (current, previous) => {
        const draftSourceChanged = !previous || current.mailDraftId !== previous.mailDraftId || current.tempDraft !== previous.tempDraft;
        // 草稿来源变化：重新初始化草稿
        if (draftSourceChanged) await initializeDraft();
        else applyDefaultSender(); // 只是用户信息变化：只补默认发件人信息，不重新初始化草稿
    }, { immediate: true }
);

onMounted(() => {
    void loadAddressBookContacts();
    if (!contentRef.value) return;
    observer = new ResizeObserver((entries) => {
        const width = entries[0]? entries[0].contentRect.width : 0;
        showContact.value = width >= 650;
    });
    observer.observe(contentRef.value);
});

onUnmounted(() => {
    observer?.disconnect();
    cleanupPreviewUrl();
});

const buildSendMailData = (): SendMailData | null => {
    const draft = mailDraft.value;
    if (!draft) return null;

    const mailBoxId = Number(draft.sendMailBoxId);
    if (!Number.isFinite(mailBoxId) || mailBoxId <= 0) {
        showMessage.value?.ShowModal("发送邮件", ["请选择发件邮箱"], "failure");
        return null;
    }

    const toAddressList = Array.from(new Set(draft.to.map(item => item.trim()).filter(item => item.length > 0)));
    const ccAddressList = Array.from(new Set(draft.cc.map(item => item.trim()).filter(item => item.length > 0)));
    const bccAddressList = Array.from(new Set(draft.bcc.map(item => item.trim()).filter(item => item.length > 0)));

    if (toAddressList.length === 0) {
        showMessage.value?.ShowModal("发送邮件", ["请填写收件人"], "failure");
        return null;
    }

    const signatureHtml = SignatureService.renderSignature(draft.signatureContent, userStore.account, userStore.nickname, userStore.mailBoxView) || "";
    const forwardContentHtml = MailService.createForwardContentBlock(draft.forwardContent, false);
    const transferAttachmentHtml = MailService.createAttachmentLinkBlock(draft.attachment);
    const bodyData = [
        draft.content || "",
        signatureHtml,
        forwardContentHtml,
        transferAttachmentHtml,
    ].filter(item => item.trim().length > 0).join("<br/>");
    const attachments = draft.attachment.filter(attachment => attachment.sendType === 'MIME_ATTACHMENT');

    return {
        mailBoxId,
        sendName: draft.sendNickname?.trim() || "",
        toAddressList,
        ccAddressList,
        bccAddressList,
        subject: draft.subject?.trim() || "",
        bodyData,
        attachments,
        replySourceMappingId: draft.replySourceMappingId,
    };
};

const buildDraftBodyData = (draft: MailDraft): string => {
    const signatureHtml = draft.signatureId === 'none'
        ? ''
        : SignatureService.renderSignature(draft.signatureContent, userStore.account, userStore.nickname, userStore.mailBoxView) || "";
    const replySourceMappingIdHtml = MailService.createReplySourceMappingIdBlock(draft.replySourceMappingId);
    const forwardContentHtml = MailService.createForwardContentBlock(draft.forwardContent);
    const transferAttachmentHtml = MailService.createAttachmentLinkBlock(draft.attachment);
    return [
        draft.content || "",
        signatureHtml,
        replySourceMappingIdHtml,
        forwardContentHtml,
        transferAttachmentHtml,
    ].filter(item => item.trim().length > 0).join("<br/>");
};

const toPreviewAddress = (address: string) => ({
    name: address,
    address,
});
const buildPreviewMail = (): MailPreviewData => {
    const draft = mailDraft.value;
    const fromAddress = draft.sendMailBoxView || userStore.mailBoxView || '';
    const senderName = draft.sendNickname?.trim() || fromAddress || '未选择发件人';

    return {
        subject: draft.subject?.trim() || '无主题',
        from: {
            name: senderName,
            address: fromAddress,
        },
        tos: Array.from(new Set(draft.to.map(item => item.trim()).filter(Boolean))).map(toPreviewAddress),
        ccs: Array.from(new Set(draft.cc.map(item => item.trim()).filter(Boolean))).map(toPreviewAddress),
        bccs: Array.from(new Set(draft.bcc.map(item => item.trim()).filter(Boolean))).map(toPreviewAddress),
        htmlBody: buildDraftBodyData(draft),
        createTime: new Date().toISOString(),
        attachments: draft.attachment
            .filter(attachment => attachment.sendType === 'MIME_ATTACHMENT')
            .map((attachment, index) => ({
                id: `compose-${index}-${attachment.fileName}`,
                fileName: attachment.fileName,
                fileType: attachment.fileType,
                fileSize: attachment.fileSize,
                isInline: false,
                source: 'LOCAL',
                fileData: attachment.fileData,
            })),
    };
};
const showMailPreview = () => {
    previewMail.value = buildPreviewMail();
    mailPreviewDialog.value?.ShowModal();
};
const closeMailPreview = () => {
    mailPreviewDialog.value?.Close();
};

const buildSaveDraftData = (): SaveDraftData | null => {
    const draft = mailDraft.value;
    if (!draft) return null;

    const mailBoxId = Number(draft.sendMailBoxId);
    if (!Number.isFinite(mailBoxId) || mailBoxId <= 0) {
        showMessage.value?.ShowModal("保存草稿", ["请选择发件邮箱"], "failure");
        return null;
    }

    const bodyData = buildDraftBodyData(draft);
    const attachments = draft.attachment.filter(attachment => attachment.sendType === 'MIME_ATTACHMENT');
    const draftMappingId = Number(mailDraftId.value);

    return {
        draftMappingId: Number.isFinite(draftMappingId) && draftMappingId > 0 ? draftMappingId : null,
        mailBoxId,
        sendName: draft.sendNickname?.trim() || "",
        toAddressList: Array.from(new Set(draft.to.map(item => item.trim()).filter(Boolean))),
        ccAddressList: Array.from(new Set(draft.cc.map(item => item.trim()).filter(Boolean))),
        bccAddressList: Array.from(new Set(draft.bcc.map(item => item.trim()).filter(Boolean))),
        subject: draft.subject?.trim() || "",
        bodyData,
        attachments,
    };
};

const saveDraft = async () => {
    const draftData = buildSaveDraftData();
    if (!draftData) return;

    try {
        const response = await Email.saveDraft(draftData);
        if (response.code === 200 && response.data?.mappingId) {
            mailDraftId.value = String(response.data.mappingId);
            if (getRouteMailDraftId() !== mailDraftId.value) {
                await router.replace({ name: 'composeBox', params: { mailDraftId: mailDraftId.value } });
            }
            showMessage.value?.ShowModal("保存草稿", ["草稿已保存"], "success");
            return;
        }
        showMessage.value?.ShowModal("保存草稿", [response.message || "草稿保存失败"], "failure");
    } catch (error) {
        showMessage.value?.ShowModal("保存草稿", ["草稿保存失败，请检查网络或稍后重试"], "failure");
    }
};

const sendMail = async () => {
    const sendMailData = buildSendMailData();
    if (!sendMailData) return;

    try {
        const response = await Email.sendMail(sendMailData);
        if (response.code === 200) {
            showMessage.value?.ShowModal("发送邮件", [response.data || "邮件已提交发送队列"], "success", (_: number) => {
                // 发送成功后，清除草稿ID并重置草稿内容
                mailDraftId.value = '';
                mailDraft.value = MailService.createDefaultMailDraft();
            });
            return;
        }
        showMessage.value?.ShowModal("发送邮件", [response.message || "邮件发送失败"], "failure");
    } catch (error) {
        showMessage.value?.ShowModal("发送邮件", ["邮件提交失败，请检查网络或稍后重试"], "failure");
    }
};
</script>

<style scoped>
.compose {
    width: 100%;
    height: 100%;
    gap: 10px;
    padding: 12px 15px;
    position: relative;
    z-index: 999;
    top: 0;
    left: 0;
    display: flex;
    flex-direction: column;
    justify-content: center;
    align-items: center;
}

/* 操作列表 */
.compose .operation {
    width: 100%;
    display: flex;
    gap: 8px;
    flex-shrink: 0;
    display: flex;
    justify-content: flex-start;
    align-items: flex-start;
}
.compose .operation .sendButton {
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
    /* 单行文字省略号 */
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
    background-color: var(--button-bg-primary);
}
.compose .operation .sendButton .iconfont {
    font-size: 20px;
    color: var(--button-text-primary);
}
.compose .operation .sendButton:hover {
    background-color: var(--button-bg-primary-hover);
}
.compose .operation .sendButton:active {
    transition: all .1s;
    background-color: var(--button-bg-primary-active);
}
.compose .operation .actions {
    flex: 1;
    min-width: 0;
    height: 100%;
    display: flex;
    align-items: center;
}

.attachmentPanel {
    width: 260px;
    padding: 8px;
    display: flex;
    flex-direction: column;
    gap: 6px;
    background-color: var(--bg-primary);
}
.attachmentOption {
    width: 100%;
    min-height: 56px;
    padding: 8px 10px;
    gap: 10px;
    justify-content: flex-start;
    border-radius: 8px;
    color: var(--text-primary);
    transition: background-color .3s;
}
.attachmentOption:hover {
    background-color: var(--bg-hover-primary);
}
.attachmentOption .optionIcon {
    width: 28px;
    height: 28px;
    flex-shrink: 0;
    display: flex;
    align-items: center;
    justify-content: center;
    color: var(--text-link);
    font-size: 22px;
}
.attachmentOption .optionText {
    min-width: 0;
    display: flex;
    flex-direction: column;
    gap: 3px;
    align-items: flex-start;
}
.attachmentOption .optionTitle,
.attachmentOption .optionDesc {
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.attachmentOption .optionTitle {
    font-size: 14px;
    font-weight: 700;
    color: var(--text-secondary1);
}
.attachmentOption .optionDesc {
    font-size: 12px;
    color: var(--text-tertiary);
}

/* 主面板 */
.compose .content {
    width: 100%;
    flex: 1;
    gap: 1px;
    border-radius: 10px;
    overflow: hidden;
    display: flex;
    justify-content: center;
    align-items: center;
}

/* 输入框 */
.compose .inputBox {
    height: 100%;
    min-width: 0;
    min-height: 0;
    flex: 1;
    display: flex;
    flex-direction: column;
    justify-content: center;
    align-items: center;
    background-color: var(--bg-primary);
}

/* 写信面板 */
.compose .inputBox .write {
    width: 100%;
    min-width: 0;
    min-height: 0;
    flex: 1;
    display: flex;
}

.compose .inputBox .attachmentStrip {
    width: 100%;
    max-height: 92px;
    padding: 6px 10px;
    display: flex;
    flex-wrap: wrap;
    gap: 6px;
    flex-shrink: 0;
    overflow-y: auto;
    border-top: 1px solid var(--bg-hover-primary);
}
.compose .inputBox .attachmentChip {
    max-width: min(360px, 100%);
    min-width: 0;
    min-height: 38px;
    padding: 5px 7px;
    display: flex;
    align-items: center;
    gap: 7px;
    border-radius: 8px;
    background-color: var(--bg-secondary4);
}
.compose .inputBox .attachmentChip img {
    width: 26px;
    height: 26px;
    flex-shrink: 0;
    object-fit: contain;
}
.compose .inputBox .attachmentInfo {
    min-width: 0;
    display: flex;
    flex-direction: column;
    gap: 2px;
}
.compose .inputBox .attachmentName,
.compose .inputBox .attachmentMeta {
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
    text-align: left;
}
.compose .inputBox .attachmentName {
    font-size: 13px;
    font-weight: 700;
    color: var(--text-secondary1);
}
.compose .inputBox .attachmentMeta {
    font-size: 12px;
    color: var(--text-tertiary);
}
.compose .inputBox .attachmentChip .button {
    width: 22px;
    height: 22px;
    padding: 0;
    flex-shrink: 0;
    font-size: 15px;
}

/* 发件人设置 */
.compose .inputBox .sendMailSetting {
    width: 100%;
    padding: 5px 10px;
    font-size: 14px;
    display: flex;
    justify-content: flex-start;
    align-items: center;
}
.compose .inputBox .sendMailSetting .title {
    flex-shrink: 0;
}
.compose .inputBox .sendMailSetting .sendMail {
    min-width: 100px;
    min-width: 300px;
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 5px;
    border-radius: 5px;
    transition: all .3s;
    position: relative;
}
.compose .inputBox .sendMailSetting .sendMail:hover {
    background-color: var(--bg-hover-primary);
}
.compose .inputBox .sendMailSetting .sendMail:active {
    transition: all .1s;
    background-color: var(--bg-active-primary);
}
.compose .inputBox .sendMailSetting .sendMail .nickname {
    max-width: 150px;
    padding-left: 5px;
    font-weight: 700;
    color: var(--text-secondary1);
    /* 单行文本省略 */
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.compose .inputBox .sendMailSetting .sendMail .mailAddress {
    max-width: 200px;
    padding-left: 5px;
    color: var(--text-secondary2);
    /* 单行文本省略 */
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}.compose .inputBox .sendMailSetting .sendMail .mailAddress::after {
    content: ">";
}.compose .inputBox .sendMailSetting .sendMail .mailAddress::before {
    content: "<";
}
.compose .inputBox .sendMailSetting .sendMail .expandIcon {
    padding: 5px;
    font-size: 16px;
    transform: rotate(90deg);
    display: inline-block;
}

/* 发件人设置面板 */
.compose .inputBox .sendMailSetting .sendMail .sendMailSettingPanel {
    width: 100%;
    padding: 10px;
    font-size: 14px;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    background-color: var(--bg-primary);
}
/* 邮箱信息 */
.compose .inputBox .sendMailSetting .sendMail .sendMailSettingPanel .mailBoxInfo {
    width: 100%;
    border-bottom: 1px solid var(--bg-active-primary);
}
.compose .inputBox .sendMailSetting .sendMail .sendMailSettingPanel .mailBoxInfo .mailBox {
    width: 100%;
    height: 100%;
    padding: 12px 5px;
    gap: 10px;
    border-radius: 5px;
    justify-content: flex-start;
}
.compose .inputBox .sendMailSetting .sendMail .sendMailSettingPanel .mailBoxInfo .mailBox .address {
    flex: 1;
    /* 单行文本省略 */
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.compose .inputBox .sendMailSetting .sendMail .sendMailSettingPanel .mailBoxInfo .mailBox .iconfont {
    flex-shrink: 0;
    width: 18px;
    height: 18px;
    font-size: 18px;
}
/* 发件人名称 */
.compose .inputBox .sendMailSetting .sendMail .sendMailSettingPanel .sendNickname {
    width: 100%;
    padding-top: 5px;
    height: 38px;
    display: flex;
    align-items: center;
    justify-content: space-between;
}
.compose .inputBox .sendMailSetting .sendMail .sendMailSettingPanel .sendNickname .title {
    flex-shrink: 0;
}
.compose .inputBox .sendMailSetting .sendMail .sendMailSettingPanel .sendNickname .value {
    flex: 1;
}
.compose .inputBox .sendMailSetting .sendMail .sendMailSettingPanel .sendNickname .btn-text {
    --text-primary: var(--text-link);
}

/* 联系人 */
.compose .contact {
    width: 248px;
    height: 100%;
    flex-shrink: 0;
    min-height: 0;
    padding: 12px 10px;
    display: flex;
    flex-direction: column;
    gap: 10px;
    border-left: 1px solid var(--bg-hover-primary);
    background: var(--bg-primary);
}
.compose .contact .contactHeader {
    min-height: 30px;
    display: flex;
    align-items: center;
    justify-content: space-between;
    flex-shrink: 0;
}
.compose .contact .contactTitleGroup {
    min-width: 0;
    display: flex;
    align-items: center;
    gap: 6px;
    color: var(--text-secondary1);
}
.compose .contact .contactTitleGroup .iconfont {
    width: 24px;
    height: 24px;
    display: flex;
    align-items: center;
    justify-content: center;
    color: var(--text-link);
    font-size: 19px;
    border-radius: 6px;
    background-color: var(--bg-hover-primary);
}
.compose .contact .contactTitle {
    font-size: 15px;
    font-weight: 700;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.compose .contact .contactCount {
    min-width: 38px;
    padding: 2px 6px;
    border-radius: 999px;
    font-size: 12px;
    text-align: center;
    color: var(--text-secondary2);
    background-color: var(--bg-tertiary);
}
.compose .contact .searchInput {
    flex-shrink: 0;
    width: 100%;
    height: 34px;
    display: flex;
    align-items: center;
    justify-content: space-between;
    color: var(--primary-color);
    border: 1px solid var(--border-primary);
    border-radius: 8px;
    position: relative;
    overflow: hidden;
    transition: all .3s;
    background-color: rgba(255, 255, 255, 1);
}
.compose .contact .searchInput::placeholder {
    color: var(--input-primary-text-placeholder);
    font-size: 14px;
}
.compose .contact .searchInput:hover {
    border: 1px solid var(--border-hover);
}
.compose .contact .searchInput:focus-within,
.compose .contact .searchInput.searchClearShown {
    border: 1px solid var(--border-hover);
    background-color: rgba(255, 255, 255, 1);
}
.compose .contact .searchInput input {
    width: 100%;
    height: 100%;
    border: none;
    outline: none;
    padding: 0px 6px 0px 10px;
    font-size: 14px;
    background-color: transparent;
}
.compose .contact .searchInput .button {
    width: 32px;
    height: 32px;
    flex-shrink: 0;
    padding: 0px;
    font-size: 20px;
    --text-primary: var(--text-secondary2);
}
.compose .contact .recipientSwitch {
    width: 100%;
    height: 32px;
    padding: 3px;
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 3px;
    flex-shrink: 0;
    border-radius: 8px;
    background-color: var(--bg-tertiary);
}
.compose .contact .recipientSwitch button {
    min-width: 0;
    padding: 0px;
    border-radius: 6px;
    font-size: 13px;
    color: var(--text-secondary2);
    transition: color .3s, background-color .3s;
}
.compose .contact .recipientSwitch button:hover {
    color: var(--text-link-hover);
}
.compose .contact .recipientSwitch button.selected {
    color: var(--text-link);
    font-weight: 700;
    background-color: var(--bg-primary);
    box-shadow: var(--shadow-hover);
}
.compose .contact .contactTree {
    width: 100%;
    min-height: 0;
    flex: 1;
    display: flex;
    flex-direction: column;
    gap: 8px;
    overflow-y: auto;
    padding-right: 2px;
}
.compose .contact .contactGroupSection {
    width: 100%;
    display: flex;
    flex-direction: column;
    gap: 4px;
}
.compose .contact .contactGroupHeader {
    width: 100%;
    height: 32px;
    padding: 0px 6px;
    justify-content: space-between;
    border-radius: 8px;
    color: var(--text-secondary2);
    background-color: var(--bg-secondary4);
    transition: color .3s, background-color .3s;
}
.compose .contact .contactGroupHeader:hover {
    color: var(--text-link-hover);
    background-color: var(--bg-hover-secondary);
}
.compose .contact .contactGroupHeader .groupTitle {
    min-width: 0;
    display: flex;
    align-items: center;
    gap: 4px;
}
.compose .contact .contactGroupHeader .expandIcon {
    width: 16px;
    height: 16px;
    flex-shrink: 0;
    display: flex;
    align-items: center;
    justify-content: center;
    font-size: 12px;
    transform: rotate(0deg);
    transition: transform .3s;
}
.compose .contact .contactGroupHeader .expandIcon.expanded {
    transform: rotate(90deg);
}
.compose .contact .contactGroupHeader .groupName {
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
    font-size: 13px;
    font-weight: 700;
}
.compose .contact .contactGroupHeader .groupCount {
    min-width: 20px;
    padding: 1px 6px;
    border-radius: 999px;
    font-size: 11px;
    text-align: center;
    color: var(--text-secondary2);
    background-color: var(--bg-primary);
}
.compose .contact .contactList {
    width: 100%;
    display: flex;
    flex-direction: column;
    gap: 5px;
    padding-left: 8px;
}
.compose .contact .contactItem {
    width: 100%;
    min-height: 54px;
    padding: 7px;
    gap: 8px;
    border-radius: 8px;
    justify-content: flex-start;
    color: var(--text-primary);
    border: 1px solid transparent;
    background-color: var(--bg-secondary);
    transition: background-color .3s, border-color .3s, transform .3s;
}
.compose .contact .contactItem:hover {
    border-color: var(--border-hover);
    background-color: var(--bg-hover-secondary);
}
.compose .contact .contactItem:active {
    transform: translateY(1px);
    background-color: var(--bg-active-primary);
}
.compose .contact .contactItem.selected {
    border-color: var(--border-hover);
    background-color: var(--bg-hover-primary);
}
.compose .contact .avatar {
    width: 40px;
    height: 40px;
    flex-shrink: 0;
    border-radius: 50%;
    overflow: hidden;
    background-color: var(--bg-primary);
}
.compose .contact .avatar img {
    width: 100%;
    height: 100%;
    display: block;
    object-fit: cover;
}
.compose .contact .contactInfo {
    min-width: 0;
    flex: 1;
    display: flex;
    flex-direction: column;
    gap: 4px;
    align-items: flex-start;
}
.compose .contact .contactName,
.compose .contact .contactEmail {
    width: 100%;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
    text-align: left;
}
.compose .contact .contactName {
    color: var(--text-secondary1);
    font-size: 14px;
    font-weight: 700;
}
.compose .contact .contactEmail {
    color: var(--text-tertiary);
    font-size: 12px;
}
.compose .contact .contactAction {
    width: 22px;
    flex-shrink: 0;
    display: flex;
    align-items: center;
    justify-content: flex-end;
}
.compose .contact .contactAction .iconfont {
    width: 18px;
    height: 18px;
    display: flex;
    align-items: center;
    justify-content: center;
    color: var(--text-link);
    font-size: 15px;
}
.compose .contact .emptyContact {
    min-height: 150px;
    display: flex;
    flex-direction: column;
    gap: 8px;
    align-items: center;
    justify-content: center;
    color: var(--text-tertiary);
    font-size: 13px;
}
.compose .contact .emptyContact .iconfont {
    font-size: 28px;
    color: var(--text-tertiary);
}
.compose .contact .contactFooter {
    height: 36px;
    padding-top: 8px;
    flex-shrink: 0;
    display: flex;
    align-items: center;
    justify-content: space-between;
    border-top: 1px solid var(--border-primary);
    font-size: 13px;
    color: var(--text-secondary2);
}
.compose .contact .contactFooter button {
    padding: 4px 8px;
    --text-primary: var(--text-link);
}

.filePickerDialog {
    width: min(900px, calc(100vw - 80px));
    height: min(520px, calc(100vh - 180px));
    min-height: 360px;
    display: grid;
    grid-template-columns: 300px minmax(0, 1fr);
    background-color: var(--bg-primary);
    overflow: hidden;
}
.mailPreviewDialog {
    width: min(980px, calc(100vw - 80px));
    height: min(720px, calc(100vh - 160px));
    max-height: calc(100vh - 160px);
    min-height: 320px;
    display: flex;
    background-color: var(--bg-primary);
    overflow: hidden;
}
.filePickerSidebar {
    width: 300px;
    min-width: 0;
    min-height: 0;
    border-right: 1px solid var(--border-primary);
    display: flex;
    flex-direction: column;
}
.filePickerSearch {
    position: relative;
    padding: 10px;
    flex-shrink: 0;
    box-sizing: border-box;
}
.filePickerSearch input {
    width: 100%;
    min-width: 0;
    height: 34px;
    padding: 0 34px 0 10px;
    border: 1px solid var(--border-primary);
    border-radius: 8px;
    outline: none;
    box-sizing: border-box;
}
.filePickerSearch .iconfont {
    position: absolute;
    right: 22px;
    top: 50%;
    transform: translateY(-50%);
    color: var(--text-tertiary);
    pointer-events: none;
}
.filePickerList {
    min-height: 0;
    flex: 1;
    overflow-y: auto;
    padding: 0 8px 8px;
    display: flex;
    flex-direction: column;
    gap: 6px;
}
.filePickerItem {
    width: 100%;
    min-height: 68px;
    padding: 8px;
    display: flex;
    align-items: center;
    gap: 8px;
    border-radius: 8px;
    background-color: var(--bg-secondary4);
    box-sizing: border-box;
}
.filePickerItem.selected {
    background-color: var(--bg-hover-primary);
}
.filePickerIcon {
    width: 32px;
    height: 32px;
    flex-shrink: 0;
}
.filePickerIcon img {
    width: 100%;
    height: 100%;
    object-fit: contain;
}
.filePickerInfo {
    min-width: 0;
    display: flex;
    flex-direction: column;
    gap: 4px;
}
.filePickerName,
.filePickerMeta {
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
    text-align: left;
}
.filePickerName {
    font-size: 13px;
    font-weight: 700;
    color: var(--text-secondary1);
}
.filePickerMeta {
    font-size: 12px;
    color: var(--text-tertiary);
}
.filePickerEmpty {
    min-height: 120px;
    display: flex;
    align-items: center;
    justify-content: center;
    color: var(--text-tertiary);
    font-size: 13px;
}
.filePickerPreview {
    min-width: 0;
    min-height: 0;
    display: flex;
    flex-direction: column;
    padding: 10px;
    gap: 10px;
}
.filePickerPreviewHeader {
    display: flex;
    justify-content: space-between;
    gap: 12px;
    color: var(--text-secondary1);
    font-size: 13px;
    font-weight: 700;
}
.filePickerPreviewBody {
    min-height: 0;
    flex: 1;
    border-radius: 8px;
    overflow: hidden;
    background-color: var(--bg-secondary4);
    display: flex;
    align-items: center;
    justify-content: center;
}
.filePickerPreviewBody img,
.filePickerPreviewBody iframe {
    width: 100%;
    height: 100%;
    object-fit: contain;
    border: none;
}
.filePickerPreviewFallback img {
    width: 56px;
    height: 56px;
    object-fit: contain;
}
.filePickerPreviewFallback {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    gap: 10px;
    color: var(--text-tertiary);
    font-size: 13px;
}
.filePickerPreviewFallback .iconfont {
    font-size: 40px;
}

@media (max-height: 620px) {
    .filePickerDialog {
        height: calc(100vh - 150px);
        min-height: 300px;
    }
    .mailPreviewDialog {
        height: calc(100vh - 140px);
        min-height: 280px;
    }
    .filePickerItem {
        min-height: 58px;
    }
}

@media (max-width: 760px) {
    .filePickerDialog {
        width: calc(100vw - 40px);
        grid-template-columns: 240px minmax(0, 1fr);
    }
    .mailPreviewDialog {
        width: calc(100vw - 40px);
        height: calc(100vh - 140px);
        max-height: calc(100vh - 140px);
    }
    .filePickerSidebar {
        width: 240px;
    }
}
</style>
