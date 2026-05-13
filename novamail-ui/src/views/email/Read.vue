<template>
    <div class="read">
        <!-- 操作列表 -->
        <div class="operation">
            <div class="sendButton" @click="goBack">
                <span class="iconfont icon-likesprograma-zuojiantoufanhuishangyibu"></span>
                <span class="actionTitle">返回</span>
            </div>
            <div class="actions">
                <ActionBar :actionItems="actionItems"></ActionBar>
            </div>
        </div>

        <!-- 邮件信息 -->
        <div class="mailInfo">
            <div class="subject">
                <span class="subjectText">{{ mail?.subject }}</span>
                <button class="btn btn-text">
                    <span class="iconfont icon-likesprograma-shoucang"></span>
                </button>
            </div>
            <div class="senderInfo">
                <div class="avatar">
                    <img :src="fromAvatar" alt="avatar">
                </div>
                <div class="sender">
                    <div class="mailAddressbox">
                        <span class="mailAddress">
                            <span class="name">{{ mail?.from.name }}</span>
                            <span class="address">{{ mail?.from.address }}</span>
                        </span>
                    </div>
                    <div class="mailAddressbox">
                        <span class="title">收件人</span>
                        <span v-for="mailAddress in mail?.tos" class="mailAddress">
                            <span class="name">{{ mailAddress.name }}</span>
                            <span class="address">{{ mailAddress.address }}</span>
                        </span>
                    </div>
                </div>
                <div class="otherInformation">
                    <div class="createTime">{{ formatEmailTime(mail?.createTime ? mail?.createTime : "1999-1-1 00:00:00") }}</div>
                </div>
            </div>
            <div class="line">
                <span></span>
            </div>
        </div>

        <!-- 邮件内容 -->
        <div class="content" v-if="mail?.htmlBody" v-html="Email.renderMailDetail(mail.htmlBody)">
        </div>
        <div class="content" v-else-if="mail?.textBody" v-text="mail?.textBody">
        </div>
        <div class="content" v-else-if="mail?.preview" v-text="mail?.preview">
        </div>
        <div class="content renderError" v-else>
            <span class="iconfont icon-likesprogramjiazaizhong"></span>
            邮件内容渲染中
        </div>

        <!-- 消息提示 -->
        <ShowMessage ref="showMessage"/>
    </div>
</template>

<script lang="ts">
import { Email, type MailDetailResponse } from '@api/email/Email';
import ActionBar, { type ActionItem } from '@components/ActionBar.vue';
import { EmailActions } from '@typings/emailList/EmailActions';
import { computed, onMounted, ref } from 'vue';
import ShowMessage from '@components/messageDialog/ShowMessage.vue';
import { useRoute, useRouter } from 'vue-router';
import { baseUrl } from '@utils/Request';

export default {
    name: 'Read',
}
</script>

<script setup lang="ts">
const router = useRouter();
const route = useRoute();

const actionItems: ActionItem[] = [
    { icon: "icon-likesprogramhuishouzhan", title: "删除", action: EmailActions.Edit },
    { icon: "icon-likesprograma-shangyibu1", title: "回复", action: EmailActions.Revoke },
    { icon: "icon-likesprogramxiayibu", title: "转发", action: EmailActions.Revoke },
    { icon: "icon-likesprograma-shoucang", title: "标记为", action: EmailActions.Revoke },
    { icon: "icon-likesprogramArrows_Arrows_Right_Left", title: "移动到", action: EmailActions.Revoke },
];

// 对话框实例
const showMessage = ref<InstanceType<typeof ShowMessage>>();
    
// 返回
const goBack = () => {
    if (window.history.length > 1) {
        router.back();
    } else {
        router.push('/inbox');
    }
};

// 接收 mappingId
const mappingId = computed(() => {
    const id = route.params.id;
    if (Array.isArray(id)) return Number(id[0]);
    return Number(id);
});

// 获取邮件内容
const mail = ref<MailDetailResponse | null>(null);
const fromAvatar = ref<string>(`${baseUrl}/userProfile/mailBoxAvatar?mailBoxAddress=test@test.test`);
const getMailDetail = async () => {
    if (!Number.isFinite(mappingId.value) || mappingId.value <= 0) return;

    try {
        const response = await Email.getMailDetail(mappingId.value);
        mail.value = response.data;
        fromAvatar.value = `${baseUrl}/userProfile/mailBoxAvatar?mailBoxAddress=${mail?.value?.from.address}&v=${mail?.value?.from.address}`;
    } catch (error) {
        setTimeout(() => {
            showMessage.value?.ShowModal("获取邮件内容", ["获取邮件内容失败，是否重试"], "failure", (selectedButtonIndex) => {
                if(selectedButtonIndex === 1) getMailDetail();
            });
        }, 300);
    }
};

onMounted(() => {
    getMailDetail();
});

// 格式化邮件时间显示
const formatEmailTime = (rawTime: string): string => {
    const date = new Date(rawTime);
    const now = new Date();

    const pad = (n: number) => n.toString().padStart(2, "0");
    const timeStr = `${pad(date.getHours())}:${pad(date.getMinutes())}`;

    const today = new Date(now.getFullYear(), now.getMonth(), now.getDate());
    const targetDay = new Date(date.getFullYear(), date.getMonth(), date.getDate());
    const diffMs = today.getTime() - targetDay.getTime();
    const diffDays = Math.floor(diffMs / (1000 * 60 * 60 * 24));

    if (diffDays === 0) {
        return `今天 ${timeStr}`;
    } else if (diffDays === 1) {
        return `昨天 ${timeStr}`;
    } else if (diffDays < 7 && diffDays > 1) {
        const weekdays = ["周日","周一","周二","周三","周四","周五","周六"];
        return `${weekdays[date.getDay()]} ${timeStr}`;
    } else if (date.getFullYear() === now.getFullYear()) {
        return `${pad(date.getMonth() + 1)}月${pad(date.getDate())}日`;
    } else {
        return `${date.getFullYear()}年${pad(date.getMonth() + 1)}月${pad(date.getDate())}日`;
    }
}
</script>

<style scoped>
.read {
    width: 100%;
    height: 100%;
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
.read .operation {
    width: 100%;
    display: flex;
    gap: 8px;
    padding: 12px 15px;
    padding-bottom: 0px;
    flex-shrink: 0;
    display: flex;
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
    /* 单行文字省略号 */
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

/* 邮件信息 */
.read .mailInfo {
    width: 100%;
    padding: 0px 15px;
}
.read .mailInfo .subject {
    width: 100%;
    gap: 10px;
    padding: 15px 0px;
    padding-bottom: 10px;
    display: flex;
    justify-content: flex-start;
    align-items: center;
}.read .mailInfo .subject .subjectText {
    font-size: 16px;
    font-weight: 700;
    color: var(--text-secondary1);
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}.read .mailInfo .subject .btn {
    font-size: 20px;
    width: 30px;
    height: 30px;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}.read .mailInfo .subject .btn:hover {
    background-color: var(--bg-secondary3);
}.read .mailInfo .subject .btn:active {
    background-color: var(--bg-secondary4);
}
.read .mailInfo .senderInfo {
    width: 100%;
    min-height: 0;
    gap: 10px;
    min-width: 0;
    display: flex;
    justify-content: flex-start;
    align-items: center;
}.read .mailInfo .senderInfo .avatar {
    width: 40px;
    height: 40px;
    border-radius: 50%;
    overflow: hidden;
    flex-shrink: 0;
    background-color: var(--bg-primary);
}.read .mailInfo .senderInfo .avatar img {
    width: 100%;
    height: 100%;
}
.read .mailInfo .sender {
    height: 100%;
    flex: 1;
    min-width: 0;
    display: flex;
    font-size: 14px;
    gap: 3px;
    flex-direction: column;
    justify-content: center;
    align-items: center;
}.read .mailInfo .sender .mailAddressbox {
    width: 100%;
    flex-shrink: 0;
    gap: 5px;
    display: flex;
    justify-content: flex-start;
    align-items: center;
    /* 单行文本省略 */
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}.read .mailInfo .sender .mailAddressbox .mailAddress {
    gap: 5px;
    display: flex;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}.read .mailInfo .sender .mailAddressbox .title {
    flex-shrink: 0;
    font-size: 14px;
}.read .mailInfo .sender .mailAddressbox .name {
    min-width: 20px;
    font-weight: 700;
    color: var(--text-secondary1);
    /* 单行文本省略 */
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}.read .mailInfo .sender .mailAddressbox .address {
    color: var(--text-secondary2);
    /* 单行文本省略 */
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}.read .mailInfo .sender .mailAddressbox .address::after {
    content: ">";
}.read .mailInfo .sender .mailAddressbox .address::before {
    content: "<";
}
.read .mailInfo .otherInformation {
    width: 200px;
    align-self: stretch;
    height: auto;
    flex-shrink: 0;
    display: flex;
    justify-content: center;
    align-items: flex-end;
}.read .mailInfo .otherInformation .createTime {
    width: 200px;
    font-size: 14px;
    flex-shrink: 0;
    gap: 5px;
    display: flex;
    justify-content: flex-end;
    align-items: center;
    /* 单行文本省略 */
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.read .line {
    width: 100%;
    height: 1px;
    padding: 0px 15px;
    margin-top: 20px;
    display: flex;
    justify-content: center;
    align-items: center;
}.read .line span {
    width: 100%;
    height: 1px;
    background-color: var(--bg-secondary3);
}

/* 主面板 */
.read .content {
    width: 100%;
    flex: 1;
    padding: 0px 20px;
    overflow: hidden;
    overflow-x: auto;
    overflow-y: auto;
}
.read .content.renderError {
    width: 100%;
    height: 100%;
    font-family: 'artisticFonts';
    font-size: 48px;
    color: var(--text-tertiary);
    display: flex;
    flex-direction: column;
    justify-content: center;
    align-items: center;
}.read .content.renderError .iconfont {
    font-size: 100px;
}
</style>
