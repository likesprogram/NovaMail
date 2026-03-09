<template>
    <div class="emailList">
        <div class="listTitle">
            <div class="title">
                <!-- 状态栏 -->
                <div class="status">
                    <input type="checkbox" v-model="selectAll">
                </div>
                <!-- 标题栏 -->
                <div class="titleText">{{ title }}</div>
                <!-- 筛选 -->
                <div class="filter">
                    <span class="filterIcon iconfont icon-likesprogramfilter"></span>
                </div>
            </div>
            <div class="actions">
                <ActionBar :actionItems="actionItems.filter(item => hasAction(item.action))"></ActionBar>
            </div>
            <!-- 统计 -->
            <div class="statistics">
                共 {{ 100 }} 封
            </div>
        </div>
        <div class="listBody">
            <div v-for="emailGroup in emailList" class="listGroup">
                <div class="listGroupHeader">
                    <div class="groupName">{{ emailGroup.dateLabel }}</div>
                    <div class="groupCount">({{ emailGroup.count }} 封)</div>
                </div>
                <router-link v-for="email in emailGroup.emails" class="emailItem" :to="{ name: mailBox + 'Read', params: { id: email.id } }" :key="email.id">
                    <div class="sender">
                        <div class="status">
                            <input type="checkbox" v-model="selectAll" @click.stop>
                            <!-- 未读 -->
                            <span v-if="!email.isRead" class="statusIcon iconfont icon-likesprogramyoujian" style="color: #f5a623; font-size: 20px;"></span>
                            <!-- 已读 -->
                            <span v-else-if="email.isRead" class="statusIcon iconfont icon-likesprogramyoujian2"></span>
                            <!-- 发送中 -->
                            <span v-else class="statusIcon iconfont icon-likesprogramfasong"></span>
                            <!-- 发送成功 -->
                            <!-- <span class="statusIcon iconfont icon-likesprogramyoujianfasongchenggong"></span> -->
                            <!-- 发送失败 -->
                            <!-- <span class="statusIcon iconfont icon-likesprogramyoujianfasongshibai"></span> -->
                            <!-- 草稿 -->
                            <!-- <span class="statusIcon iconfont icon-likesprogramxiexin"></span> -->
                        </div>
                        <span class="senderName">
                            <span class="senderNameText">{{ email.sender }}</span>
                        </span>
                    </div>
                    <div class="title">
                        <span class="titleText">{{ email.title }}</span>
                        <span class="subTitleText">{{ email.subtitle }}</span>
                    </div>
                    <div class="time">{{ formatEmailTime(email.time ? email.time : "1999-1-1 00:00:00") }}</div>
                    <div class="starred" :class="{ 'active': email.isStarred }" @click.stop="toggleStarred(email); $event.preventDefault();">
                        <span v-if="email.isStarred" class="iconfont icon-likesprograma-shoucang2"></span>
                        <span v-else class="iconfont icon-likesprograma-shoucang"></span>
                    </div>
                </router-link>
            </div>
            <!-- <div class="listGroupFooter">
                <div class="more">加载更多</div>
            </div> -->
        </div>
    </div>
</template>

<script lang="ts">
import { defineComponent, ref } from 'vue';
import { EmailActions } from '@typings/emailList/EmailActions';
import ActionBar, { type ActionItem } from '@components/ActionBar.vue';
export default defineComponent({
    name: 'EmailList',
});
interface EmailItem {
    id: number;                // 唯一 ID
    sender: string;            // 发件人名称或邮箱
    senderAvatar?: string;     // 发件人头像 URL（可选）
    recipients?: string[];     // 收件人列表，可选，用于显示或转发
    title?: string;            // 邮件标题
    subtitle?: string;         // 邮件副标题，用于列表预览
    time?: string;             // 邮件发送时间（ISO 字符串）
    content?: string;          // 邮件正文内容 
    attachments?: string[];    // 附件列表，存储文件名或 URL
    folder?: string;           // 所属文件夹
    isRead?: boolean;          // 是否已读
    isStarred?: boolean;       // 是否标星
    isSpam?: boolean;          // 是否被标记为垃圾邮件
    isAd?: boolean;            // 是否广告邮件
    hasAttachments?: boolean;  // 是否包含附件
    replied?: boolean;         // 是否已回复
}


interface EmailGroup {
    dateLabel: string;
    count?: number;
    emails: EmailItem[];
}
</script>

<script setup lang="ts">
const actionItems: ActionItem[] = [
    { icon: "icon-likesprogramtianxie", title: "再次编辑", action: EmailActions.Edit },
    { icon: "icon-likesprograma-zuojiantoufanhuishangyibu", title: "撤回", action: EmailActions.Revoke },
    { icon: "icon-likesprogramhuishouzhan", title: "删除", action: EmailActions.Delete },
    { icon: "icon-likesprograma-shangyibu1", title: "回复全部", action: EmailActions.ReplyAll },
    { icon: "icon-likesprogramxiayibu", title: "转发", action: EmailActions.Forward },
    { icon: "icon-likesprogramtingyong", title: "这不是垃圾邮件", action: EmailActions.NotSpam },
    { icon: "icon-likesprogramtingyong", title: "这不是广告邮件", action: EmailActions.NotAd },
    { icon: "icon-likesprogramyoujian2", title: "全部已读", action: EmailActions.MarkRead },
    { icon: "icon-likesprograma-shoucang", title: "标记为", action: EmailActions.Mark },
    { icon: "icon-likesprogramArrows_Arrows_Right_Left", title: "移动到", action: EmailActions.MoveTo },
]

const props = withDefaults(defineProps<{
    title: string;
    mailBox?: string;
    actionsMask: EmailActions;
}>(), {
    title: "邮件列表",
    mailBox: "",
    actionsMask: EmailActions.All,
});

// 邮件列表
const emailList = ref<EmailGroup[]>([
  {
    dateLabel: '今天',
    count: 2,
    emails: [
      { id: 1, sender: '网易邮件中心', title: '网易邮箱账号修复结果通知', subtitle: '尊敬的用户：您的邮箱 m**8@163.com 已通过验证，请在24小时内点击下方按钮重置密码或安全手机（过期需重新申请）。', time: '2025-10-3 20:11' },
      { id: 2, sender: 'Steam', title: '将手机号码添加至您的 Steam 帐户', subtitle: '13574378538，您好！我们收到了将手机号码（尾号为 38）添加至您 Steam 帐户的请求。如果这是您本人，请点击下方按钮以确认。', time: '2025-10-3 19:54' },
    ]
  },
  {
    dateLabel: '昨天',
    count: 2,
    emails: [
      { id: 3, sender: 'Steam Team', title: 'Steam 上有新的登录操作', subtitle: 'Steam 上有新的登录操作来自您的帐户“13574378538”登录地点：Changsha, Hunan, CN授权方式：您邮件中的 Steam 令牌验证码如果以上详情看起来很熟悉，您可以安心地忽略此邮件。否则，请立即重置 Steam 的密码。', time: '2025-10-2 19:53' },
      { id: 4, sender: 'PostMaster', title: '来自qq.com的退信', subtitle: '很抱歉您发送的邮件被退回，以下是该邮件的相关信息：被退回邮件	主 题：测试时间：2025-09-16 22:40:17', time: '2025-10-2 03:43' },
    ]
  },
  {
    dateLabel: '周三',
    count: 2,
    emails: [
      { id: 5, sender: 'service', title: '【建设银行】-2025年09月02日：个人信用报告 “个人手机银行” 自助查询结果', subtitle: '尊敬的龙兵寅：您好！附件是您在 2025年09月02日11时49分45秒 通过 “建设银行-个人手机银行” 申请查询的个人信用报告，请查收。基于对您个人隐私及信息安全的保护，附件已加密加压。解压密码将通过手机短信发送至您预留的手机号码，该解压密码也可通过“建设银行-个人手机银行”APP中的“个人信用报告”列表查看', time: '2025-10-1 21:07' },
      { id: 6, sender: '蓝桥杯组委会', title: '【蓝桥杯】获奖选手免考申领IITC证书，助力秋招冲刺！', subtitle: '同学你好！原“工业和信息化岗位能力评价证书”现已完成升级，更名为《IITC工信人才岗位能力评价证书》（简称IITC证书）。蓝桥杯获奖选手可免考申领该证书——现在申请，国庆前即可拿到，助力金九银十秋招冲刺！证书优势：', time: '2025-10-1 16:00', isStarred: true },
    ]
  },
  {
    dateLabel: '更早',
    count: 50,
    emails: [
      { id: 7, sender: 'GitHub', title: '[GitHub] Please verify your device', subtitle: 'Hey likesprogram!A sign in attempt requires further verification because we did not recognize your device. To complete the sign in, enter the verification code on the unrecognized device.Device: Safari on iOS iPhone', time: '2025-2-24 21:22' },
      { id: 8, sender: '湖南信息学院软件卓越人才班', title: '【卓越班报名平台】 邮箱验证码', subtitle: '验证码：jBrt，邮箱验证码5分钟内有效【湘信卓越班】', time: '2024-9-10 17:52' },
      { id: 7, sender: 'GitHub', title: '[GitHub] Please verify your device', subtitle: 'Hey likesprogram!A sign in attempt requires further verification because we did not recognize your device. To complete the sign in, enter the verification code on the unrecognized device.Device: Safari on iOS iPhone', time: '2024-5-24 21:22' },
      { id: 7, sender: 'GitHub', title: '[GitHub] Please verify your device', subtitle: 'Hey likesprogram!A sign in attempt requires further verification because we did not recognize your device. To complete the sign in, enter the verification code on the unrecognized device.Device: Safari on iOS iPhone', isRead: true, time: '2024-4-30 21:22' },
      { id: 7, sender: 'GitHub', title: '[GitHub] Please verify your device', subtitle: 'Hey likesprogram!A sign in attempt requires further verification because we did not recognize your device. To complete the sign in, enter the verification code on the unrecognized device.Device: Safari on iOS iPhone', isRead: true, time: '2024-4-29 21:22' },
      { id: 7, sender: 'GitHub', title: '[GitHub] Please verify your device', subtitle: 'Hey likesprogram!A sign in attempt requires further verification because we did not recognize your device. To complete the sign in, enter the verification code on the unrecognized device.Device: Safari on iOS iPhone', isRead: true, time: '2024-4-28 21:22' },
      { id: 7, sender: 'GitHub', title: '[GitHub] Please verify your device', subtitle: 'Hey likesprogram!A sign in attempt requires further verification because we did not recognize your device. To complete the sign in, enter the verification code on the unrecognized device.Device: Safari on iOS iPhone', time: '2024-4-27 21:22' },
      { id: 7, sender: 'GitHub', title: '[GitHub] Please verify your device', subtitle: 'Hey likesprogram!A sign in attempt requires further verification because we did not recognize your device. To complete the sign in, enter the verification code on the unrecognized device.Device: Safari on iOS iPhone', time: '2024-4-26 21:22' },
      { id: 7, sender: 'GitHub', title: '[GitHub] Please verify your device', subtitle: 'Hey likesprogram!A sign in attempt requires further verification because we did not recognize your device. To complete the sign in, enter the verification code on the unrecognized device.Device: Safari on iOS iPhone', time: '2024-4-25 21:22' },
      { id: 7, sender: 'GitHub', title: '[GitHub] Please verify your device', subtitle: 'Hey likesprogram!A sign in attempt requires further verification because we did not recognize your device. To complete the sign in, enter the verification code on the unrecognized device.Device: Safari on iOS iPhone', time: '2024-4-24 21:22' },
      { id: 7, sender: 'GitHub', title: '[GitHub] Please verify your device', subtitle: 'Hey likesprogram!A sign in attempt requires further verification because we did not recognize your device. To complete the sign in, enter the verification code on the unrecognized device.Device: Safari on iOS iPhone', time: '2024-3-29 21:22' },
      { id: 7, sender: 'GitHub', title: '[GitHub] Please verify your device', subtitle: 'Hey likesprogram!A sign in attempt requires further verification because we did not recognize your device. To complete the sign in, enter the verification code on the unrecognized device.Device: Safari on iOS iPhone', time: '2024-3-28 21:22' },
      { id: 7, sender: 'GitHub', title: '[GitHub] Please verify your device', subtitle: 'Hey likesprogram!A sign in attempt requires further verification because we did not recognize your device. To complete the sign in, enter the verification code on the unrecognized device.Device: Safari on iOS iPhone', time: '2024-3-27 21:22' },
      { id: 7, sender: 'GitHub', title: '[GitHub] Please verify your device', subtitle: 'Hey likesprogram!A sign in attempt requires further verification because we did not recognize your device. To complete the sign in, enter the verification code on the unrecognized device.Device: Safari on iOS iPhone', time: '2024-3-26 21:22' },
      { id: 7, sender: 'GitHub', title: '[GitHub] Please verify your device', subtitle: 'Hey likesprogram!A sign in attempt requires further verification because we did not recognize your device. To complete the sign in, enter the verification code on the unrecognized device.Device: Safari on iOS iPhone', time: '2024-3-25 21:22' },
      { id: 7, sender: 'GitHub', title: '[GitHub] Please verify your device', subtitle: 'Hey likesprogram!A sign in attempt requires further verification because we did not recognize your device. To complete the sign in, enter the verification code on the unrecognized device.Device: Safari on iOS iPhone', time: '2024-3-24 21:22' },
      { id: 7, sender: 'GitHub', title: '[GitHub] Please verify your device', subtitle: 'Hey likesprogram!A sign in attempt requires further verification because we did not recognize your device. To complete the sign in, enter the verification code on the unrecognized device.Device: Safari on iOS iPhone', time: '2024-3-9 21:22' },
      { id: 7, sender: 'GitHub', title: '[GitHub] Please verify your device', subtitle: 'Hey likesprogram!A sign in attempt requires further verification because we did not recognize your device. To complete the sign in, enter the verification code on the unrecognized device.Device: Safari on iOS iPhone', time: '2024-3-8 21:22' },
      { id: 7, sender: 'GitHub', title: '[GitHub] Please verify your device', subtitle: 'Hey likesprogram!A sign in attempt requires further verification because we did not recognize your device. To complete the sign in, enter the verification code on the unrecognized device.Device: Safari on iOS iPhone', time: '2024-3-7 21:22' },
      { id: 7, sender: 'GitHub', title: '[GitHub] Please verify your device', subtitle: 'Hey likesprogram!A sign in attempt requires further verification because we did not recognize your device. To complete the sign in, enter the verification code on the unrecognized device.Device: Safari on iOS iPhone', time: '2024-3-6 21:22' },
      { id: 7, sender: 'GitHub', title: '[GitHub] Please verify your device', subtitle: 'Hey likesprogram!A sign in attempt requires further verification because we did not recognize your device. To complete the sign in, enter the verification code on the unrecognized device.Device: Safari on iOS iPhone', time: '2024-3-5 21:22' },
      { id: 7, sender: 'GitHub', title: '[GitHub] Please verify your device', subtitle: 'Hey likesprogram!A sign in attempt requires further verification because we did not recognize your device. To complete the sign in, enter the verification code on the unrecognized device.Device: Safari on iOS iPhone', time: '2024-3-4 21:22' },
      { id: 7, sender: 'GitHub', title: '[GitHub] Please verify your device', subtitle: 'Hey likesprogram!A sign in attempt requires further verification because we did not recognize your device. To complete the sign in, enter the verification code on the unrecognized device.Device: Safari on iOS iPhone', time: '2024-3-3 21:22' },
      { id: 7, sender: 'GitHub', title: '[GitHub] Please verify your device', subtitle: 'Hey likesprogram!A sign in attempt requires further verification because we did not recognize your device. To complete the sign in, enter the verification code on the unrecognized device.Device: Safari on iOS iPhone', time: '2024-3-2 21:22' },
      { id: 7, sender: 'GitHub', title: '[GitHub] Please verify your device', subtitle: 'Hey likesprogram!A sign in attempt requires further verification because we did not recognize your device. To complete the sign in, enter the verification code on the unrecognized device.Device: Safari on iOS iPhone', time: '2024-2-26 21:22' },
      { id: 7, sender: 'GitHub', title: '[GitHub] Please verify your device', subtitle: 'Hey likesprogram!A sign in attempt requires further verification because we did not recognize your device. To complete the sign in, enter the verification code on the unrecognized device.Device: Safari on iOS iPhone', time: '2024-2-25 21:22' },
      { id: 7, sender: 'GitHub', title: '[GitHub] Please verify your device', subtitle: 'Hey likesprogram!A sign in attempt requires further verification because we did not recognize your device. To complete the sign in, enter the verification code on the unrecognized device.Device: Safari on iOS iPhone', time: '2024-2-24 21:22' },
    ]
  }
]);

// 全选状态
const selectAll = ref(false);

// 检查是否包含某个操作
const hasAction = (action: EmailActions) => {
  return (props.actionsMask & action) !== 0;
};

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

// 加入星标
const toggleStarred = (email: EmailItem) => {
    email.isStarred = !email.isStarred;
}
</script>

<style scoped>
.emailList {
    width: 100%;
    height: 100%;
    min-height: 0;
    position: relative;
    top: 0;
    left: 0;
    display: flex;
    flex-direction: column;
    --title-width: 274px;
    overflow: hidden;
    overflow-y: auto;
    --color: var(--text-primary);
    color: var(--color);
}
.emailList .listTitle {
    width: 100%;
    height: 50px;
    flex-shrink: 0;
    display: flex;
    justify-content: center;
    align-items: center;
    padding: 0 20px;
}
.emailList .listTitle .title {
    width: var(--title-width);
    display: flex;
    justify-content: flex-start;
    align-items: center;
    /* color: var(--text-secondary1); */
}
.emailList .listTitle .title .titleText { 
    font-size: 16px;
    font-weight: 700;
    padding: 0px 8px;
    /* 单行文字省略号 */
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.emailList .listTitle .title .filter { 
    height: 100%;
}
.emailList .listTitle .title .filter .filterIcon { 
    height: 100%;
    font-size: 20px;
    font-weight: 300;
}
.emailList .listTitle .actions {
    flex: 1;
    min-width: 0; 
    height: 100%;
    padding: 0px 10px;
    display: flex;
    align-items: center;
}

.emailList .listTitle .statistics {
    display: flex;
    justify-content: flex-end;
    align-items: center;
    gap: 10px;
    font-size: 12px;
    color: var(--text-secondary2);
    /* 单行文字省略号 */
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}

/* 列表内容 */
.emailList .listBody { 
    flex: 1;
    min-height: 0;
    width: 100%;
    display: flex;
    flex-direction: column;
    overflow: hidden;
    overflow-y: auto;
}
/* 列表分组 */
.emailList .listBody .listGroup { 
    display: flex;
    flex-direction: column;
    width: 100%;
    padding: 0px 8px;
}
/* 列表分组标题 */
.emailList .listBody .listGroup .listGroupHeader { 
    display: flex;
    align-items: center;
    height: 20px;
    font-size: 13px;
    padding-left: 10px;
    padding-bottom: 5px;
    gap: 5px;
}
.emailList .listBody .listGroup .listGroupHeader .groupName { 
    font-weight: 700;
}
.emailList .listBody .listGroup .listGroupHeader .groupCount { 
    color: var(--text-tertiary);
}.emailList .listBody .listGroup:not(:first-child) {
    padding-top: 15px;
}
/* 列表项 */
.emailList .listBody .listGroup .emailItem { 
    display: flex;
    align-items: center;
    width: 100%;
    height: 40px;
    padding: 0 12px;
    border-radius: 5px;
    color: var(--color);
    cursor: pointer;
    transition: all .3s;
    justify-content: flex-start;
    position: relative;
}
.emailList .listBody .listGroup .emailItem::after { 
    content: '';
    display: block;
    width: calc(100% - 10px);
    height: 1px;
    background-color: var(--bg-active-primary);
    position: absolute;
    top: 100%;
    left: 50%;
    transform: translateX(-50%);
}
.emailList .listBody .listGroup .emailItem:hover { 
    background-color: var(--bg-hover-primary);
}
.emailList .listBody .listGroup .emailItem:active:not(:has(.starred:active)) { 
    transition: all .1s;
    background-color: var(--bg-active-primary);
}
.emailList .listBody .listGroup .emailItem .sender { 
    display: flex;
    align-items: center;
    justify-content: flex-start;
    width: var(--title-width);
    height: 100%;
    flex-shrink: 0;
}
.emailList .listBody .listGroup .emailItem .sender .status { 
    height: 100%;
    display: flex;
    align-items: center;
    justify-content: center;
    flex-shrink: 0;
}
.emailList .listBody .listGroup .emailItem .sender .status .statusIcon { 
    height: 100%;
    width: 20px;
    height: 20px;
    font-size: 22px;
    font-weight: 300;
    color: var(--text-tertiary);
    display: flex;
    align-items: center;
    justify-content: center;
    margin: 0px 10px;
}
.emailList .listBody .listGroup .emailItem .sender .senderName { 
    flex: 1;
    min-width: 0;
    height: 100%;
    display: flex;
    align-items: center;
    justify-content: flex-start;
    font-size: 14px;
    padding-right: 8px;
}
.emailList .listBody .listGroup .emailItem .sender .senderName .senderNameText { 
    width: 100%;
    /* 单行文字省略号 */
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.emailList .listBody .listGroup .emailItem .title { 
    flex: 1;
    height: 100%;
    min-width: 0;
    display: flex;
    align-items: center;
    justify-content: flex-start;
    font-size: 14px;
}
.emailList .listBody .listGroup .emailItem .title .titleText { 
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
    padding-right: 8px;
}
.emailList .listBody .listGroup .emailItem .title .subTitleText { 
    color: var(--text-tertiary);
    flex: 1;                /* 占据剩余空间 */
    min-width: 0;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.emailList .listBody .listGroup .emailItem .time { 
    font-size: 14px;
    padding: 0px 15px;
    flex-shrink: 0;
}
.emailList .listBody .listGroup .emailItem .starred { 
    width: 30px;
    height: 30px;
    flex-shrink: 0;
    font-size: 20px;
    display: flex;
    justify-content: center;
    align-items: center;
    border-radius: 5px;
    cursor: pointer;
    transition: all .3s;
}
.emailList .listBody .listGroup .emailItem .starred:hover {
    background-color: var(--bg-hover-primary);
}
.emailList .listBody .listGroup .emailItem .starred:active { 
    transition: all .1s;
    background-color: var(--bg-active-primary);
}
.emailList .listBody .listGroup .emailItem .starred.active { 
    color: var(--color-warning);
    font-size: 21px;
}
</style>
