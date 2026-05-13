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
                共 {{ emailCont }} 封
            </div>
        </div>
        <div class="listBody">
            <div v-for="emailGroup in emailList" class="listGroup">
                <div class="listGroupHeader">
                    <div class="groupName">{{ emailGroup.dateLabel }}</div>
                    <div class="groupCount">({{ emailGroup.count }} 封)</div>
                </div>
                <router-link v-for="email in emailGroup.emails" class="emailItem" :to="{ name: mailBox + 'Read', params: { id: email.mappingId } }" :key="email.mappingId">
                    <div class="sender">
                        <div class="status">
                            <input type="checkbox" v-model="email.select" @click.stop>
                            <!-- 未读 -->
                            <span v-if="!email.read" class="statusIcon iconfont icon-likesprogramyoujian" style="color: #f5a623; font-size: 20px;"></span>
                            <!-- 已读 -->
                            <span v-else-if="email.read" class="statusIcon iconfont icon-likesprogramyoujian2"></span>
                            <!-- 发送中 -->
                            <span v-else class="statusIcon iconfont icon-likesprogramfasong"></span>

                        </div>
                        <span class="senderName">
                            <span class="senderNameText">{{ email.fromName ? email.fromName : email.fromMail  }}</span>
                        </span>
                    </div>
                    <div class="title">
                        <span class="titleText">{{ email.subject }}</span>
                        <span class="subTitleText">{{ email.preview }}</span>
                    </div>
                    <div class="time">{{ formatEmailTime(email.createTime ? email.createTime : "1999-1-1 00:00:00") }}</div>
                    <div class="starred" :class="{ 'active': email.starred }" @click.stop="toggleStarred(email); $event.preventDefault();">
                        <span v-if="email.starred" class="iconfont icon-likesprograma-shoucang2"></span>
                        <span v-else class="iconfont icon-likesprograma-shoucang"></span>
                    </div>
                </router-link>
            </div>
                
            <div class="noData" v-if="!emailList || emailList.length === 0">暂无邮件</div>
        </div>

        <!-- 消息提示 -->
        <ShowMessage ref="showMessage"/>
    </div>
</template>

<script lang="ts">
import { defineComponent, onMounted, ref } from 'vue';
import { EmailActions } from '@typings/emailList/EmailActions';
import ActionBar, { type ActionItem } from '@components/ActionBar.vue';
import { Email, type MailBoxFolder , type MailBoxMailMapping } from '@api/email/Email';
import ShowMessage from '@components/messageDialog/ShowMessage.vue';
export default defineComponent({
    name: 'EmailList',
});

interface EmailGroup {
    dateLabel: string;
    count?: number;
    emails: MailBoxMailMapping[];
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
    mailBoxFolder?: MailBoxFolder;
    actionsMask: EmailActions;
}>(), {
    title: "邮件列表",
    mailBox: "",
    mailBoxFolder: () =>{
        return {
            folderId: '',       // 文件夹ID
            floderName: '',     // 文件夹名称
            noReadCont: 0,     // 未读邮件数量
        }
    },
    actionsMask: EmailActions.All,
});

// 邮件列表
const emailList = ref<EmailGroup[]>([ ]);
const emailCont = ref<number>(0);

// 对话框实例
const showMessage = ref<InstanceType<typeof ShowMessage>>();

// 全选状态
const selectAll = ref(false);

const parseLocalDateTime = (value: string): Date => {
    // 兼容后端 LocalDateTime: 2026-05-11T22:30:00
    return new Date(value);
};

const isSameDate = (a: Date, b: Date): boolean => {
    return a.getFullYear() === b.getFullYear() && a.getMonth() === b.getMonth() && a.getDate() === b.getDate();
};

const getDateStart = (date: Date): Date => {
    return new Date(date.getFullYear(), date.getMonth(), date.getDate());
};

const getWeekdayLabel = (date: Date): string => {
    const weekMap: Record<number, string> = {
        0: '周日', 1: '周一', 2: '周二', 3: '周三',
        4: '周四', 5: '周五', 6: '周六',
    };
    return weekMap[date.getDay()] ?? '未知';
};

const getMailDateLabel = (createTime: string): string => {
    const mailDate = parseLocalDateTime(createTime);
    const today = getDateStart(new Date());
    const yesterday = new Date(today);
    yesterday.setDate(today.getDate() - 1);

    if (isSameDate(mailDate, today)) return '今天';
    if (isSameDate(mailDate, yesterday)) return '昨天';
    const mailDateStart = getDateStart(mailDate);
    const diffDays = Math.floor((today.getTime() - mailDateStart.getTime()) / (24 * 60 * 60 * 1000));

    // 最近 7 天显示周几
    if (diffDays >= 2 && diffDays < 7) return getWeekdayLabel(mailDate);
    return '更早';
};

const buildEmailGroups = (mails: MailBoxMailMapping[]): EmailGroup[] => {
    const groupMap = new Map<string, MailBoxMailMapping[]>();
    for (const mail of mails) {
        const label = getMailDateLabel(mail.createTime);
        if (!groupMap.has(label)) groupMap.set(label, []);
        groupMap.get(label)!.push(mail);
    }
    const fixedOrder = ['今天', '昨天', '周一', '周二', '周三', '周四', '周五', '周六', '周日', '更早', '未知'];
    return fixedOrder .filter(label => groupMap.has(label)).map(label => {
        const emails = groupMap.get(label) ?? [];
        return { dateLabel: label, count: emails.length, emails};
    });
};

// 获取邮件列表
const getEmailList = async (mailBoxFolder: MailBoxFolder) => {
    if (!mailBoxFolder || !mailBoxFolder.folderId) return;
    try {
        const response = await Email.getEmailList(mailBoxFolder);
        const mails = response.data ?? [];
        emailCont.value = mails.length;
        emailList.value = buildEmailGroups(mails);
        // 切换文件夹后重置全选状态
        selectAll.value = false;
    } catch (error) {
        setTimeout(() => {
            showMessage.value?.ShowModal("邮件列表获取失败", ["邮件列表获取失败，是否重试"], "failure", (selectedButtonIndex) => {
                if(selectedButtonIndex === 1) getEmailList(mailBoxFolder);
            });
        }, 300);
    }
};

// 组件挂载
onMounted(() => {
    // 获取邮件列表
    getEmailList(props.mailBoxFolder);
});

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
const toggleStarred = (email: MailBoxMailMapping) => {
    email.starred = !email.starred;
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
    color: var(--text-secondary1);
}
.emailList .listTitle .title .status {
    height: 100%;
    display: flex;
    align-items: center;
    justify-content: center;
    flex-shrink: 0;
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
.emailList .noData {
    width: 100%;
    height: 100%;
    font-family: 'artisticFonts';
    font-size: 48px;
    color: var(--text-tertiary);
    display: flex;
    justify-content: center;
    align-items: center;
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
