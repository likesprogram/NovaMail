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
        <div class="content" ref="contentRef">
            <!-- 输入框 -->
            <div class="inputBox">
                <!-- 写邮件组件 -->
                <div class="write">
                    <WriteMail v-model="mailDraft"></WriteMail>
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
            <div class="contact" v-if="showContact"></div>
        </div>
        
        <!-- 消息提示 -->
        <ShowMessage ref="showMessage"/>
    </div>
</template>

<script lang="ts">
import { ref, onMounted, onUnmounted, watch } from 'vue';
import ActionBar, { type ActionItem } from '@components/ActionBar.vue';
import { EmailActions } from '@typings/emailList/EmailActions';
import { UserStore } from '@store/UserStore';
import SafePopover from '@components/SafePopover.vue';
import WriteMail from '@components/WriteMail.vue';
import ShowMessage from '@components/messageDialog/ShowMessage.vue';
import { MailService, type MailDraft } from '@service/MailService';
import { useRoute } from 'vue-router';
import type { MailBoxData } from '@api/email/MailBox';
import { MailBoxService } from '@service/MailBoxService';
import { Email, type SendMailData } from '@api/email/Email';
import { SignatureService } from '@service/SignatureService'

export default {
    name: 'Compose',
}
</script>

<script setup lang="ts">
const actionItems: ActionItem[] = [
    { icon: "icon-likesprogramtianxie", title: "预览", action: EmailActions.Edit },
    { icon: "icon-likesprogrambaocun", title: "保存草稿", action: EmailActions.Edit },
    { icon: "icon-likesprogramclip", title: "附件", action: EmailActions.Revoke },
];

// 路由
const route = useRoute();

// 用户
const userStore = UserStore();

// 对话框实例
const showMessage = ref<InstanceType<typeof ShowMessage>>();

// 邮件草稿
const mailDraft = ref<MailDraft>(MailService.createDefaultMailDraft());
// 当前草稿ID
const mailDraftId = ref<string>('');

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
    mailDraftId.value = routeDraftId;

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
        nickname: userStore.nickname ?? '',
        defaultMailBoxId: String(userStore.userInfo?.defaultMailBoxId ?? '')
    }),
    async (current, previous) => {
        const draftIdChanged = !previous || current.mailDraftId !== previous.mailDraftId;
        // 草稿ID变化：重新初始化草稿
        if (draftIdChanged) await initializeDraft();
        else applyDefaultSender(); // 只是用户信息变化：只补默认发件人信息，不重新初始化草稿
    }, { immediate: true }
);

onMounted(() => {
    if (!contentRef.value) return;
    observer = new ResizeObserver((entries) => {
        const width = entries[0]? entries[0].contentRect.width : 0;
        showContact.value = width >= 650;
    });
    observer.observe(contentRef.value);
});

onUnmounted(() => {
    observer?.disconnect();
});

const buildSendMailData = (): SendMailData | null => {
    const draft = mailDraft.value;
    if (!draft) return null;

    const mailBoxId = Number(draft.sendMailBoxId);
    if (!Number.isFinite(mailBoxId) || mailBoxId <= 0) {
        showMessage.value?.ShowModal("发送邮件", ["请选择发件邮箱"], "failure");
        return null;
    }

    const toAddressList = Array.from(new Set([...draft.to, ...draft.cc, ...draft.bcc].map(item => item.trim()).filter(item => item.length > 0)));

    if (toAddressList.length === 0) {
        showMessage.value?.ShowModal("发送邮件", ["请填写收件人"], "failure");
        return null;
    }

    const bodyData = [draft.content || "", SignatureService.renderSignature(draft.signatureContent, userStore.account, userStore.nickname, userStore.mailBoxView) || "",].filter(item => item.trim().length > 0).join("<br/>");

    return { mailBoxId, sendName: draft.sendNickname?.trim() || "", toAddressList, subject: draft.subject?.trim() || "", bodyData,};
};

const sendMail = async () => {
    const sendMailData = buildSendMailData();
    if (!sendMailData) return;

    try {
        const response = await Email.sendMail(sendMailData);
        if (response.code === 200) {
            showMessage.value?.ShowModal("发送邮件", [response.data || "邮件已提交发送队列"], "success");
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
    width: 200px;
    height: 100%;
    flex-shrink: 0;
    background-color: var(--bg-primary);
    background-image: url("/contactlist.png");
    background-position: 2px 0px;
    background-repeat: no-repeat;
    background-size: 100% auto;
}
</style>
