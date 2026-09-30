<template>
    <div class="mailSetting">
        <div class="settingItem">
            <div class="settingContent alignItemsStart">
                <div class="settingTitle">邮箱</div>
                <div class="settingBox">
                    <div class="header">
                        <div class="title">邮箱管理</div>
                        <button class="btn-text" @click="openCreateDialog('mailbox')">
                            <span class="iconfont icon-likesprogramtianjiahaoyou"></span>
                            注册新邮箱
                        </button>
                    </div>
                    <div class="settings">
                        <div v-for="mailbox in mailboxes" :key="mailbox.id" class="settingRow">
                            <div class="rowMain">
                                <div class="rowTitle">
                                    <span>{{ mailbox.address }}</span>
                                    <span v-if="mailbox.isDefault" class="tag primary">默认</span>
                                    <span class="tag" :class="mailbox.status">{{ getStatusText(mailbox.status) }}</span>
                                </div>
                                <div class="rowDescription">
                                    <span>{{ mailbox.remark || '未设置备注' }}</span>
                                    <span>{{ formatDateTime(mailbox.updateTime) }}</span>
                                </div>
                            </div>
                            <div class="operation">
                                <button class="btn-primary" :class="{ 'btn-disabled': mailbox.isDefault || mailbox.status === 'disabled' }"
                                    @click="setDefaultMailbox(mailbox)">
                                    <span class="iconfont icon-likesprogramzhengque"></span>
                                    设为默认
                                </button>
                                <button class="btn-primary" @click="openEditDialog('mailbox', mailbox)">
                                    <span class="iconfont icon-likesprogrambianji"></span>
                                    修改
                                </button>
                                <button :class="mailbox.status === 'enabled' ? 'btn-warning' : 'btn-primary'"
                                    @click="toggleMailboxStatus(mailbox)">
                                    <span class="iconfont" :class="mailbox.status === 'enabled' ? 'icon-likesprogramtingyong' : 'icon-likesprogramqiyong'"></span>
                                    {{ mailbox.status === 'enabled' ? '停用' : '启用' }}
                                </button>
                                <button class="btn-danger" :class="{ 'btn-disabled': mailbox.isDefault }" @click="removeMailbox(mailbox)">
                                    <span class="iconfont icon-likesprogramhuishouzhan"></span>
                                    移除
                                </button>
                            </div>
                        </div>
                        <div class="noData" v-if="mailboxes.length === 0">暂无邮箱</div>
                    </div>
                </div>
            </div>
        </div>

        <div class="settingItem">
            <div class="settingContent alignItemsStart">
                <div class="settingTitle">域名</div>
                <div class="settingBox">
                    <div class="header">
                        <div class="title">个性化域名管理</div>
                        <button class="btn-text" @click="openCreateDialog('domain')">
                            <span class="iconfont icon-likesprogramwangzhi"></span>
                            添加域名
                        </button>
                    </div>
                    <div class="settings">
                        <div v-for="domain in domains" :key="domain.id" class="settingRow">
                            <div class="rowMain">
                                <div class="rowTitle">
                                    <span>{{ domain.domainName }}</span>
                                    <span v-if="domain.isDefault" class="tag primary">默认</span>
                                    <span class="tag" :class="domain.editable ? 'owner' : 'readonly'">{{ getDomainPermissionText(domain) }}</span>
                                    <span v-if="domain.hasCertificate" class="tag success">TLS</span>
                                    <span v-if="domain.hasDKIM" class="tag success">DKIM</span>
                                </div>
                                <div class="rowDescription">
                                    <span>{{ domain.description || '未设置说明' }}</span>
                                    <span>{{ formatDateTime(domain.updateTime) }}</span>
                                </div>
                            </div>
                            <div class="operation">
                                <button class="btn-primary" :class="{ 'btn-disabled': !domain.editable }" @click="openEditDialog('domain', domain)">
                                    <span class="iconfont icon-likesprogrambianji"></span>
                                    修改
                                </button>
                                <button class="btn-danger" :class="{ 'btn-disabled': !domain.editable || domain.isDefault }" @click="removeDomain(domain)">
                                    <span class="iconfont icon-likesprogramhuishouzhan"></span>
                                    移除
                                </button>
                            </div>
                        </div>
                        <div class="noData" v-if="domains.length === 0">暂无域名</div>
                    </div>
                </div>
            </div>
        </div>

        <div class="settingItem">
            <div class="settingContent alignItemsStart">
                <div class="settingTitle">权限</div>
                <div class="settingBox">
                    <div class="header">
                        <div class="title">账号授权管理</div>
                        <button class="btn-text" @click="openCreateDialog('authCode')">
                            <span class="iconfont icon-likesprogramplus"></span>
                            生成授权码
                        </button>
                    </div>
                    <div class="settings">
                        <div v-for="authCode in authCodes" :key="authCode.id" class="settingRow">
                            <div class="rowMain">
                                <div class="rowTitle">
                                    <span>{{ authCode.name }}</span>
                                    <span class="tag" :class="authCode.status">{{ getStatusText(authCode.status) }}</span>
                                </div>
                                <div class="rowDescription">
                                    <span>{{ getAuthCodeDescription(authCode) }}</span>
                                    <span>{{ formatExpireTime(authCode.expireTime) }}</span>
                                </div>
                            </div>
                            <div class="operation">
                                <button class="btn-primary" @click="openEditDialog('authCode', authCode)">
                                    <span class="iconfont icon-likesprogrambianji"></span>
                                    修改
                                </button>
                                <button class="btn-primary" @click="copyAuthCode(authCode)">
                                    <span class="iconfont icon-likesprogramcopy"></span>
                                    复制
                                </button>
                                <button :class="authCode.status === 'enabled' ? 'btn-warning' : 'btn-primary'" @click="toggleAuthCodeStatus(authCode)">
                                    <span class="iconfont" :class="authCode.status === 'enabled' ? 'icon-likesprogramtingyong' : 'icon-likesprogramqiyong'"></span>
                                    {{ authCode.status === 'enabled' ? '停用' : '启用' }}
                                </button>
                                <button class="btn-danger" @click="removeAuthCode(authCode)">
                                    <span class="iconfont icon-likesprogramhuishouzhan"></span>
                                    移除
                                </button>
                            </div>
                        </div>
                        <div class="noData" v-if="authCodes.length === 0">暂无授权码</div>
                    </div>
                </div>
            </div>
        </div>

        <MessageDialog ref="editDialog" :width="dialogWidth">
            <template v-slot:title>
                <span>{{ dialogTitle }}</span>
            </template>
            <template v-slot:content>
                <div class="dialogContent mailSettingDialog" :class="{ domainDialogContent: dialogType === 'domain' }">
                    <template v-if="dialogType === 'mailbox'">
                        <div class="inputGroup">
                            <label for="mailboxLocalPart">{{ dialogMode === 'edit' ? '邮箱账号' : '邮箱名称' }}</label>
                            <div v-if="dialogMode === 'edit'" class="readonlyField">
                                {{ editingMailbox?.address || buildAddress(mailboxForm.localPart, mailboxForm.domainId) }}
                            </div>
                            <div v-else class="addressInput">
                                <input id="mailboxLocalPart" v-model="mailboxForm.localPart" type="text" placeholder="请输入邮箱名称">
                                <span>@</span>
                                <select v-model="mailboxForm.domainId">
                                    <option v-for="domain in enabledDomains" :key="domain.id" :value="domain.id">
                                        {{ domain.domainName }}
                                    </option>
                                </select>
                            </div>
                            <div v-if="dialogMode === 'edit'" class="hintText">邮箱账号创建后不可修改。</div>
                        </div>
                        <div class="inputGroup">
                            <label for="mailboxRemark">备注</label>
                            <input id="mailboxRemark" v-model="mailboxForm.remark" type="text" placeholder="例如：工作邮箱">
                        </div>
                    </template>

                    <template v-else-if="dialogType === 'domain'">
                        <div class="inputGroup">
                            <label for="domainName">域名</label>
                            <input id="domainName" v-model="domainForm.domainName" type="text" placeholder="例如 example.com">
                        </div>
                        <div class="domainConfigGrid">
                            <button type="button" class="configActionButton" :class="{ active: domainForm.hasCertificate }" @click="configureTls">
                                <span class="configDot"></span>
                                <span>{{ domainForm.hasCertificate ? 'TLS 已配置' : '配置 TLS' }}</span>
                            </button>
                            <button type="button" class="configActionButton" :class="{ active: domainForm.hasDKIM }" @click="showDkimConfig()">
                                <span class="configDot"></span>
                                <span>{{ domainForm.hasDKIM ? '查看 DKIM 解析' : '生成 DKIM 解析' }}</span>
                            </button>
                            <button type="button" class="configActionButton" :class="{ active: dnsVerification?.passed }" @click="verifyDomainDns">
                                <span class="configDot"></span>
                                <span>{{ dnsVerification?.passed ? 'DNS 已验证' : 'DNS 验证' }}</span>
                            </button>
                        </div>
                        <div class="hintText">DKIM 密钥由系统自动生成，需要在域名服务商处手动添加 TXT 解析记录；DNS 验证会检查 DKIM、SPF、IMAP、SMTP 和 MX 解析。</div>
                        <div v-if="dkimConfig" class="dkimRecordBox">
                            <div class="recordLine">
                                <span>类型</span>
                                <strong>{{ dkimConfig.recordType }}</strong>
                            </div>
                            <div class="recordLine">
                                <span>主机记录</span>
                                <button type="button" @click="copyText(dkimConfig.recordName, '主机记录已复制')">{{ dkimConfig.recordName }}</button>
                            </div>
                            <div class="recordLine">
                                <span>记录值</span>
                                <button type="button" @click="copyText(dkimConfig.recordValue, 'DKIM 记录值已复制')">{{ dkimConfig.recordValue }}</button>
                            </div>
                        </div>
                        <div v-if="dnsVerification" class="dnsVerifyBox">
                            <div class="dnsVerifyHeader" :class="{ success: dnsVerification.passed, failure: !dnsVerification.passed }">
                                <span>{{ dnsVerification.passed ? 'DNS 验证通过' : 'DNS 验证未通过' }}</span>
                                <button type="button" @click="verifyDomainDns">重新验证</button>
                            </div>
                            <div v-for="record in dnsVerification.records" :key="record.key" class="dnsRecordItem" :class="{ success: record.passed, failure: !record.passed }">
                                <div class="dnsRecordTitle">
                                    <span>{{ record.label }}</span>
                                    <strong>{{ record.passed ? '通过' : '未通过' }}</strong>
                                </div>
                                <div class="dnsRecordMeta">
                                    <span>{{ record.host }}</span>
                                    <span>{{ record.recordType }}</span>
                                </div>
                                <div class="recordLine">
                                    <span>期望值</span>
                                    <button type="button" @click="copyText(record.expectedValue, '期望记录已复制')">{{ record.expectedValue }}</button>
                                </div>
                                <div class="recordLine">
                                    <span>实际值</span>
                                    <strong>{{ record.actualValue }}</strong>
                                </div>
                                <div class="hintText">{{ record.message }}</div>
                            </div>
                        </div>
                    </template>

                    <template v-else>
                        <div class="inputGroup">
                            <label for="authCodeName">名称</label>
                            <input id="authCodeName" v-model="authCodeForm.name" type="text" placeholder="例如：Outlook 客户端">
                        </div>
                        <div class="inputGroup">
                            <label for="authCodeExpireTime">过期时间</label>
                            <input id="authCodeExpireTime" v-model="authCodeForm.expireTime" type="datetime-local">
                        </div>
                    </template>
                </div>
            </template>
            <template v-slot:footer>
                <div class="dialogFooterButton failure" @click="editDialog?.Close">
                    <span class="textBox">
                        <span class="iconfont icon-likesprogramcuowu"></span>
                        <span class="text">取消</span>
                    </span>
                    <span class="line"></span>
                </div>
                <div class="dialogFooterButton success" @click="saveDialog">
                    <span class="textBox">
                        <span class="iconfont icon-likesprogramzhengque"></span>
                        <span class="text">完成</span>
                    </span>
                </div>
            </template>
        </MessageDialog>

        <MessageDialog ref="tlsDialog" :width="'720px'">
            <template v-slot:title>
                <span>配置 TLS</span>
            </template>
            <template v-slot:content>
                <div class="dialogContent mailSettingDialog tlsDialogContent">
                    <div class="inputGroup">
                        <label for="tlsDomainName">域名</label>
                        <div id="tlsDomainName" class="readonlyField">{{ tlsDomainName }}</div>
                    </div>
                    <div class="inputGroup">
                        <label for="tlsPrivateKey">TLS 私钥 PEM</label>
                        <textarea id="tlsPrivateKey" v-model="tlsForm.privateKey" placeholder="-----BEGIN PRIVATE KEY-----"></textarea>
                    </div>
                    <div class="inputGroup">
                        <label for="tlsCertificate">TLS 证书 PEM</label>
                        <textarea id="tlsCertificate" v-model="tlsForm.certificate" placeholder="-----BEGIN CERTIFICATE-----"></textarea>
                    </div>
                    <div class="inputGroup">
                        <label for="tlsCaChain">中间证书链 PEM</label>
                        <textarea id="tlsCaChain" v-model="tlsForm.caChain" placeholder="可选"></textarea>
                    </div>
                </div>
            </template>
            <template v-slot:footer>
                <div class="dialogFooterButton failure" @click="tlsDialog?.Close">
                    <span class="textBox">
                        <span class="iconfont icon-likesprogramcuowu"></span>
                        <span class="text">取消</span>
                    </span>
                    <span class="line"></span>
                </div>
                <div class="dialogFooterButton success" @click="saveTlsConfig">
                    <span class="textBox">
                        <span class="iconfont icon-likesprogramzhengque"></span>
                        <span class="text">完成</span>
                    </span>
                </div>
            </template>
        </MessageDialog>

        <ShowMessage ref="showMessage"/>
    </div>
</template>

<script lang="ts">
import { computed, onMounted, ref } from 'vue';
import MessageDialog from '@components/messageDialog/MessageDialog.vue';
import ShowMessage from '@components/messageDialog/ShowMessage.vue';
import '@components/messageDialog/MessageDialog.css';
import { formatDateTime } from '@utils/Utils';
import { UserStore } from '@store/UserStore';
import { MailBoxService } from '@service/MailBoxService';
import { userService } from '@service/UserService';
import type { DomainDkimConfigData, DomainDnsVerificationData, DomainNameData, EmailAuthorizationData, MailBoxData } from '@api/email/MailBox';

export default {
    name: 'MailSetting',
}
</script>

<script setup lang="ts">
type ManageStatus = 'enabled' | 'disabled';
type DialogType = 'mailbox' | 'domain' | 'authCode';
type DialogMode = 'create' | 'edit';

interface MailboxItem {
    id: string;
    domainId: string;
    localPart: string;
    address: string;
    remark: string;
    status: ManageStatus;
    isDefault: boolean;
    createTime: string;
    updateTime: string;
}

interface DomainItem {
    id: string;
    domainName: string;
    ownerUserId: string | null;
    permission: string;
    editable: boolean;
    description: string;
    status: ManageStatus;
    hasCertificate: boolean;
    hasDKIM: boolean;
    isDefault: boolean;
    createTime: string;
    updateTime: string;
}

interface AuthCodeItem {
    id: string;
    name: string;
    account: string;
    code: string;
    status: ManageStatus;
    expireTime: string;
    lastLoginTime: string;
    loginCount: number;
    createTime: string;
    updateTime: string;
}

const userStore = UserStore();
const showMessage = ref<InstanceType<typeof ShowMessage>>();
const editDialog = ref<InstanceType<typeof MessageDialog>>();
const tlsDialog = ref<InstanceType<typeof MessageDialog>>();
const MAILBOX_REMARK_STORAGE_PREFIX = 'novamail:mailboxRemarks:';

const domains = ref<DomainItem[]>([]);

const mailboxes = ref<MailboxItem[]>([]);

const authCodes = ref<AuthCodeItem[]>([]);

const dialogType = ref<DialogType>('mailbox');
const dialogMode = ref<DialogMode>('create');
const editingId = ref('');

const mailboxForm = ref({
    localPart: '',
    domainId: '',
    remark: '',
});

const domainForm = ref({
    domainName: '',
    hasCertificate: false,
    hasDKIM: false,
});

const dkimConfig = ref<DomainDkimConfigData | null>(null);
const dnsVerification = ref<DomainDnsVerificationData | null>(null);

const tlsForm = ref({
    domainNameId: '',
    privateKey: '',
    certificate: '',
    caChain: '',
});

const authCodeForm = ref({
    name: '',
    expireTime: '',
});

const enabledDomains = computed(() => domains.value);
const editingMailbox = computed(() => mailboxes.value.find(item => item.id === editingId.value));
const tlsDomainName = computed(() => domains.value.find(item => item.id === tlsForm.value.domainNameId)?.domainName || domainForm.value.domainName || '当前域名');
const dialogWidth = computed(() => dialogType.value === 'domain' ? '640px' : '460px');

const dialogTitle = computed(() => {
    const action = dialogMode.value === 'create' ? '新增' : '修改';
    const typeText: Record<DialogType, string> = {
        mailbox: '邮箱',
        domain: '域名',
        authCode: '授权码',
    };
    return `${action}${typeText[dialogType.value]}`;
});

const getStatusText = (status: ManageStatus) => status === 'enabled' ? '启用' : '停用';

const getErrorMessage = (error: unknown, fallback: string) => {
    return error instanceof Error && error.message ? error.message : fallback;
};

const getDomainPermissionText = (domain: DomainItem) => {
    if (domain.permission === 'OWNER') return '我的域名';
    if (domain.permission === 'ADMIN') return '授权管理';
    if (domain.permission === 'USE') return '授权使用';
    if (domain.permission === 'PUBLIC') return '公共可用';
    return domain.editable ? '我的域名' : '可使用';
};

const getMailboxRemarkStorageKey = () => `${MAILBOX_REMARK_STORAGE_PREFIX}${userStore.userInfo?.userId || 'default'}`;

const getMailboxRemarks = () => {
    try {
        const raw = localStorage.getItem(getMailboxRemarkStorageKey());
        if (!raw) return {} as Record<string, string>;
        const parsed = JSON.parse(raw);
        return parsed && typeof parsed === 'object' ? parsed as Record<string, string> : {};
    } catch (error) {
        return {} as Record<string, string>;
    }
};

const saveMailboxRemark = (mailboxId: string, remark: string) => {
    const remarks = getMailboxRemarks();
    if (remark) remarks[mailboxId] = remark;
    else delete remarks[mailboxId];
    localStorage.setItem(getMailboxRemarkStorageKey(), JSON.stringify(remarks));
};

const getNow = () => new Date().toISOString();

const parseAddress = (address: string) => {
    const [localPart = '', domainName = ''] = address.split('@');
    return { localPart, domainName };
};

const buildAddress = (localPart: string, domainId: string) => {
    const domain = domains.value.find(item => item.id === domainId);
    return domain ? `${localPart}@${domain.domainName}` : localPart;
};

const normalizeDomain = (domain: DomainNameData): DomainItem => {
    const permission = domain.permission || (domain.editable ? 'OWNER' : 'USE');
    return {
        id: String(domain.domainNameId),
        domainName: domain.domainName,
        ownerUserId: domain.userId == null ? null : String(domain.userId),
        permission,
        editable: Boolean(domain.editable),
        description: getDomainPermissionText({
            id: String(domain.domainNameId),
            domainName: domain.domainName,
            ownerUserId: domain.userId == null ? null : String(domain.userId),
            permission,
            editable: Boolean(domain.editable),
            description: '',
            status: 'enabled',
            hasCertificate: Boolean(domain.hasCertificate),
            hasDKIM: Boolean(domain.hasDKIM),
            isDefault: Boolean(domain.isDefault),
            createTime: domain.updateTime,
            updateTime: domain.updateTime,
        }),
        status: 'enabled',
        hasCertificate: Boolean(domain.hasCertificate),
        hasDKIM: Boolean(domain.hasDKIM),
        isDefault: Boolean(domain.isDefault),
        createTime: domain.updateTime,
        updateTime: domain.updateTime,
    };
};

const normalizeMailBox = (mailbox: MailBoxData, defaultMailBoxId = userStore.userInfo?.defaultMailBoxId): MailboxItem => {
    const rawMailbox = mailbox as MailBoxData & { MailBoxView?: string; isOpen?: boolean | number };
    const address = rawMailbox.mailBoxView || rawMailbox.MailBoxView || '';
    const parsed = parseAddress(address);
    const domain = domains.value.find(item => item.id === String(mailbox.domainNameId))
        || domains.value.find(item => item.domainName === parsed.domainName);
    const isOpen = Number(rawMailbox.isOpen);

    return {
        id: String(mailbox.mailBoxId),
        domainId: domain?.id || String(mailbox.domainNameId || ''),
        localPart: mailbox.localPart || parsed.localPart,
        address: address || buildAddress(mailbox.localPart, String(mailbox.domainNameId || '')),
        remark: getMailboxRemarks()[String(mailbox.mailBoxId)] || '',
        status: isOpen === 0 ? 'disabled' : 'enabled',
        isDefault: String(mailbox.mailBoxId) === String(defaultMailBoxId ?? ''),
        createTime: mailbox.createTime,
        updateTime: mailbox.updateTime,
    };
};

const normalizeAuthCode = (authorization: EmailAuthorizationData): AuthCodeItem => {
    return {
        id: String(authorization.emailAuthorizationId),
        name: authorization.clientInfo || 'SMTP/IMAP 客户端',
        account: userStore.account || '当前账号',
        code: authorization.authCode || '',
        status: Number(authorization.isActivated) === 1 ? 'enabled' : 'disabled',
        expireTime: authorization.expireTime || '',
        lastLoginTime: authorization.lastLoginTime || '',
        loginCount: Number(authorization.loginCount || 0),
        createTime: authorization.createTime,
        updateTime: authorization.updateTime,
    };
};

const loadMailboxes = async () => {
    const currentUser = userStore.userInfo ?? await userService.getUserInfo();
    if (!currentUser?.userId) {
        mailboxes.value = [];
        return;
    }
    try {
        const response = await MailBoxService.getMailBoxList(currentUser.userId);
        mailboxes.value = response.map(mailbox => normalizeMailBox(mailbox, currentUser.defaultMailBoxId));
    } catch (error) {
        mailboxes.value = [];
        return;
    }
};

const loadDomains = async () => {
    try {
        const response = await MailBoxService.getAvailableDomainList();
        domains.value = response.map(normalizeDomain);
    } catch (error) {
        domains.value = [];
        return;
    }
};

const loadAuthCodes = async () => {
    try {
        const response = await MailBoxService.getEmailAuthorizationList();
        authCodes.value = response.map(normalizeAuthCode);
    } catch (error) {
        authCodes.value = [];
        return;
    }
};

const toDatetimeLocalValue = (value: string | null | undefined) => {
    if (!value) return '';
    const date = new Date(value);
    if (Number.isNaN(date.getTime())) return value.slice(0, 16);
    const pad = (num: number) => String(num).padStart(2, '0');
    return `${date.getFullYear()}-${pad(date.getMonth() + 1)}-${pad(date.getDate())}T${pad(date.getHours())}:${pad(date.getMinutes())}`;
};

const toApiExpireTime = (value: string) => {
    return value ? value : null;
};

const openCreateDialog = (type: DialogType) => {
    dialogType.value = type;
    dialogMode.value = 'create';
    editingId.value = '';

    if (type === 'mailbox') {
        if (enabledDomains.value.length === 0) {
            showMessage.value?.ShowModal('暂无可用域名', ['请先添加自己的域名，或联系域名拥有者授权使用域名'], 'warning');
            return;
        }
        mailboxForm.value = {
            localPart: '',
            domainId: enabledDomains.value[0]?.id || '',
            remark: '',
        };
    } else if (type === 'domain') {
        domainForm.value = {
            domainName: '',
            hasCertificate: false,
            hasDKIM: false,
        };
        dkimConfig.value = null;
        dnsVerification.value = null;
    } else {
        authCodeForm.value = {
            name: '',
            expireTime: '',
        };
    }

    editDialog.value?.ShowModal();
};

const openEditDialog = (type: DialogType, item: MailboxItem | DomainItem | AuthCodeItem) => {
    if (type === 'domain' && !(item as DomainItem).editable) {
        showMessage.value?.ShowModal('无权修改', ['只能编辑自己拥有的域名'], 'warning');
        return;
    }

    dialogType.value = type;
    dialogMode.value = 'edit';
    editingId.value = item.id;

    if (type === 'mailbox') {
        const mailbox = item as MailboxItem;
        mailboxForm.value = {
            localPart: mailbox.localPart,
            domainId: mailbox.domainId,
            remark: mailbox.remark,
        };
    } else if (type === 'domain') {
        const domain = item as DomainItem;
        domainForm.value = {
            domainName: domain.domainName,
            hasCertificate: domain.hasCertificate,
            hasDKIM: domain.hasDKIM,
        };
        dkimConfig.value = null;
        dnsVerification.value = null;
    } else {
        const authCode = item as AuthCodeItem;
        authCodeForm.value = {
            name: authCode.name,
            expireTime: toDatetimeLocalValue(authCode.expireTime),
        };
    }

    editDialog.value?.ShowModal();
};

const saveDialog = async () => {
    if (dialogType.value === 'mailbox') await saveMailbox();
    else if (dialogType.value === 'domain') await saveDomain();
    else await saveAuthCode();
};

const saveMailbox = async () => {
    const localPart = mailboxForm.value.localPart.trim();
    const domainId = mailboxForm.value.domainId;
    const localPartRegex = /^[A-Za-z0-9]{2,64}$/;

    if (dialogMode.value === 'create') {
        if (!localPartRegex.test(localPart)) {
            showMessage.value?.ShowModal('信息不完整', ['邮箱名称需为 2-64 位字母或数字'], 'warning');
            return;
        }
        if (!domainId) {
            showMessage.value?.ShowModal('信息不完整', ['请先选择可用域名'], 'warning');
            return;
        }
    }
    const address = buildAddress(localPart, domainId);
    if (dialogMode.value === 'create') {
        const duplicated = mailboxes.value.some(item => item.address === address);
        if (duplicated) {
            showMessage.value?.ShowModal('邮箱已存在', ['当前邮箱地址已经存在，请换一个名称或域名'], 'warning');
            return;
        }
    }

    const remark = mailboxForm.value.remark.trim();

    if (dialogMode.value === 'create') {
        try {
            await MailBoxService.createMailBox(localPart, domainId);
            await loadMailboxes();
            const createdMailbox = mailboxes.value.find(item => item.address === address);
            if (createdMailbox) saveMailboxRemark(createdMailbox.id, remark);
            await loadMailboxes();
            showMessage.value?.ShowModal('邮箱创建成功', [`${address} 已添加到邮箱列表`], 'success');
        } catch (error) {
            showMessage.value?.ShowModal('邮箱创建失败', [getErrorMessage(error, '请稍后重试')], 'failure');
            return;
        }
    } else {
        const currentMailbox = editingMailbox.value;
        if (!currentMailbox) {
            showMessage.value?.ShowModal('邮箱不存在', ['当前邮箱不存在或已被移除'], 'warning');
            return;
        }
        saveMailboxRemark(currentMailbox.id, remark);
        mailboxes.value = mailboxes.value.map(item => item.id === currentMailbox.id ? { ...item, remark, updateTime: getNow() } : item);
        showMessage.value?.ShowModal('邮箱修改成功', ['备注已更新'], 'success');
    }

    editDialog.value?.Close();
};

const saveDomain = async () => {
    const domainName = domainForm.value.domainName.trim().toLowerCase();
    const domainRegex = /^(?!-)(?:[a-zA-Z0-9-]{1,63}\.)+[a-zA-Z]{2,}$/;

    if (!domainRegex.test(domainName)) {
        showMessage.value?.ShowModal('信息不完整', ['请输入正确的域名，例如 example.com'], 'warning');
        return;
    }
    const duplicated = domains.value.some(item => item.domainName === domainName && item.id !== editingId.value);
    if (duplicated) {
        showMessage.value?.ShowModal('域名已存在', ['当前域名已经在列表中'], 'warning');
        return;
    }

    try {
        if (dialogMode.value === 'create') {
            const createdDomain = await MailBoxService.createDomain({ domainName });
            if (createdDomain?.domainNameId) {
                await loadDomains();
                editingId.value = String(createdDomain.domainNameId);
                dialogMode.value = 'edit';
                domainForm.value.hasDKIM = true;
                await showDkimConfig(String(createdDomain.domainNameId), false);
                showMessage.value?.ShowModal('DKIM 已生成', ['请复制弹窗中的 DKIM TXT 记录，并到域名服务商处配置解析。'], 'success');
                return;
            }
        } else {
            const domain = domains.value.find(item => item.id === editingId.value);
            if (!domain?.editable) {
                showMessage.value?.ShowModal('无权修改', ['只能编辑自己拥有的域名'], 'warning');
                return;
            }
            await MailBoxService.updateDomain({
                domainNameId: editingId.value,
                domainName,
            });
        }
        await loadDomains();
        await loadMailboxes();
    } catch (error) {
        showMessage.value?.ShowModal(dialogMode.value === 'create' ? '域名创建失败' : '域名修改失败', [getErrorMessage(error, '请稍后重试')], 'failure');
        return;
    }

    editDialog.value?.Close();
};

const configureTls = () => {
    const targetDomainId = editingId.value;
    if (!targetDomainId) {
        showMessage.value?.ShowModal('请先保存域名', ['保存域名后即可写入 TLS 证书配置'], 'warning');
        return;
    }
    tlsForm.value = {
        domainNameId: targetDomainId,
        privateKey: '',
        certificate: '',
        caChain: '',
    };
    tlsDialog.value?.ShowModal();
};

const saveTlsConfig = async () => {
    const privateKey = tlsForm.value.privateKey.trim();
    const certificate = tlsForm.value.certificate.trim();
    const caChain = tlsForm.value.caChain.trim();
    if (!privateKey || !certificate) {
        showMessage.value?.ShowModal('信息不完整', ['请填写 TLS 私钥和证书 PEM'], 'warning');
        return;
    }
    try {
        await MailBoxService.configureDomainTls({
            domainNameId: tlsForm.value.domainNameId,
            privateKey,
            certificate,
            caChain: caChain || null,
        });
        domainForm.value.hasCertificate = true;
        await loadDomains();
        tlsDialog.value?.Close();
        showMessage.value?.ShowModal('TLS 配置成功', ['证书已写入数据库，SMTP/IMAP 会自动读取。'], 'success');
    } catch (error) {
        showMessage.value?.ShowModal('TLS 配置失败', [getErrorMessage(error, '请稍后重试')], 'failure');
    }
};

const copyText = async (text: string, successMessage: string) => {
    try {
        await navigator.clipboard.writeText(text);
        showMessage.value?.ShowModal('复制成功', [successMessage], 'success');
    } catch (error) {
        showMessage.value?.ShowModal('复制失败', ['当前浏览器不允许访问剪贴板，请手动复制内容'], 'failure');
    }
};

const showDkimConfig = async (domainId?: string, showSuccessMessage = true) => {
    const targetDomainId = typeof domainId === 'string'
        ? domainId
        : editingId.value;
    if (!targetDomainId) {
        showMessage.value?.ShowModal('请先保存域名', ['保存域名后系统会自动生成 DKIM 解析记录'], 'warning');
        return;
    }

    try {
        dkimConfig.value = await MailBoxService.getDomainDkimConfig(targetDomainId);
        domainForm.value.hasDKIM = true;
        if (showSuccessMessage) {
            showMessage.value?.ShowModal('DKIM 解析记录', ['已生成 DKIM TXT 记录，请复制后到域名服务商处配置解析。'], 'success');
        }
    } catch (error) {
        showMessage.value?.ShowModal('DKIM 记录获取失败', [getErrorMessage(error, '请稍后重试')], 'failure');
    }
};

const verifyDomainDns = async () => {
    const targetDomainId = editingId.value;
    if (!targetDomainId) {
        showMessage.value?.ShowModal('请先保存域名', ['保存域名后即可验证 DNS 解析'], 'warning');
        return;
    }

    try {
        dnsVerification.value = await MailBoxService.verifyDomainDns(targetDomainId);
        showMessage.value?.ShowModal(
            dnsVerification.value.passed ? 'DNS 验证通过' : 'DNS 验证未通过',
            [dnsVerification.value.passed ? '所有必要解析均已配置正确。' : '仍有解析未生效，请查看弹窗中的验证结果。'],
            dnsVerification.value.passed ? 'success' : 'warning'
        );
    } catch (error) {
        showMessage.value?.ShowModal('DNS 验证失败', [getErrorMessage(error, '请稍后重试')], 'failure');
    }
};

const saveAuthCode = async () => {
    const name = authCodeForm.value.name.trim();

    if (!name) {
        showMessage.value?.ShowModal('信息不完整', ['请输入授权码名称'], 'warning');
        return;
    }

    try {
        if (dialogMode.value === 'create') {
            const authorization = await MailBoxService.createEmailAuthorization({
                clientInfo: name,
                expireTime: toApiExpireTime(authCodeForm.value.expireTime),
                isActivated: 1,
            });
            await loadAuthCodes();
            const code = authorization.authCode;
            editDialog.value?.Close();
            showMessage.value?.ShowModal('授权码已生成', ['请妥善保存授权码：', code], 'success');
            return;
        }
        await MailBoxService.updateEmailAuthorization({
            emailAuthorizationId: editingId.value,
            clientInfo: name,
            expireTime: toApiExpireTime(authCodeForm.value.expireTime),
            isActivated: authCodes.value.find(item => item.id === editingId.value)?.status === 'disabled' ? 0 : 1,
        });
        await loadAuthCodes();
        editDialog.value?.Close();
    } catch (error) {
        showMessage.value?.ShowModal(dialogMode.value === 'create' ? '授权码创建失败' : '授权码修改失败', [getErrorMessage(error, '请稍后重试')], 'failure');
        return;
    }
};

const setDefaultMailbox = async (mailbox: MailboxItem) => {
    if (mailbox.isDefault || mailbox.status === 'disabled') return;
    try {
        await MailBoxService.setDefaultMailBox(mailbox.id);
        await userService.refreshUser();
        mailboxes.value = mailboxes.value.map(item => ({ ...item, isDefault: item.id === mailbox.id }));
    } catch (error) {
        showMessage.value?.ShowModal('默认邮箱设置失败', [getErrorMessage(error, '请稍后重试')], 'failure');
    }
};

const toggleMailboxStatus = async (mailbox: MailboxItem) => {
    if (mailbox.isDefault && mailbox.status === 'enabled') {
        showMessage.value?.ShowModal('无法停用', ['默认邮箱不能直接停用，请先设置其他默认邮箱'], 'warning');
        return;
    }
    try {
        await MailBoxService.setOpenMailBox(mailbox.id, mailbox.status === 'enabled' ? 0 : 1);
        await loadMailboxes();
    } catch (error) {
        showMessage.value?.ShowModal('邮箱状态更新失败', [getErrorMessage(error, '请稍后重试')], 'failure');
    }
};

const toggleAuthCodeStatus = async (authCode: AuthCodeItem) => {
    try {
        await MailBoxService.setEmailAuthorizationActivated(authCode.id, authCode.status === 'enabled' ? 0 : 1);
        await loadAuthCodes();
    } catch (error) {
        showMessage.value?.ShowModal('授权码状态更新失败', [getErrorMessage(error, '请稍后重试')], 'failure');
    }
};

const removeMailbox = (mailbox: MailboxItem) => {
    if (mailbox.isDefault) {
        showMessage.value?.ShowModal('无法移除', ['默认邮箱不能移除，请先设置其他默认邮箱'], 'warning');
        return;
    }
    showMessage.value?.ShowModal('确认移除', [`确定要移除邮箱 ${mailbox.address} 吗？`], 'warning', async selectedButtonIndex => {
        if (selectedButtonIndex !== 1) return;
        try {
            await MailBoxService.removeMailBox(mailbox.id);
            await loadMailboxes();
        } catch (error) {
            showMessage.value?.ShowModal('邮箱移除失败', [getErrorMessage(error, '请稍后重试')], 'failure');
        }
    });
};

const removeDomain = (domain: DomainItem) => {
    if (!domain.editable) {
        showMessage.value?.ShowModal('无权移除', ['只能移除自己拥有的域名'], 'warning');
        return;
    }
    if (domain.isDefault) {
        showMessage.value?.ShowModal('无法移除', ['默认域名不能移除，请先设置其他默认域名'], 'warning');
        return;
    }
    const relatedMailboxes = mailboxes.value.filter(mailbox => mailbox.domainId === domain.id);
    if (relatedMailboxes.length > 0) {
        showMessage.value?.ShowModal('无法移除', ['该域名下仍有邮箱，请先移除相关邮箱'], 'warning');
        return;
    }
    showMessage.value?.ShowModal('确认移除', [`确定要移除域名 ${domain.domainName} 吗？`], 'warning', async selectedButtonIndex => {
        if (selectedButtonIndex !== 1) return;
        try {
            await MailBoxService.deleteDomain(domain.id);
            await loadDomains();
        } catch (error) {
            showMessage.value?.ShowModal('域名移除失败', [getErrorMessage(error, '请稍后重试')], 'failure');
        }
    });
};

const removeAuthCode = (authCode: AuthCodeItem) => {
    showMessage.value?.ShowModal('确认移除', [`确定要移除授权码 ${authCode.name} 吗？`], 'warning', async selectedButtonIndex => {
        if (selectedButtonIndex !== 1) return;
        try {
            await MailBoxService.deleteEmailAuthorization(authCode.id);
            await loadAuthCodes();
        } catch (error) {
            showMessage.value?.ShowModal('授权码移除失败', [getErrorMessage(error, '请稍后重试')], 'failure');
        }
    });
};

const copyAuthCode = async (authCode: AuthCodeItem) => {
    try {
        await navigator.clipboard.writeText(authCode.code);
        showMessage.value?.ShowModal('复制成功', ['授权码已复制到剪贴板'], 'success');
    } catch (error) {
        showMessage.value?.ShowModal('复制失败', ['当前浏览器不允许访问剪贴板，请手动复制授权码'], 'failure');
    }
};

const formatExpireTime = (expireTime: string) => {
    return expireTime ? `有效至 ${formatDateTime(expireTime)}` : '长期有效';
};

const getAuthCodeDescription = (authCode: AuthCodeItem) => {
    const loginText = authCode.lastLoginTime ? `最近登录 ${formatDateTime(authCode.lastLoginTime)}` : '尚未登录';
    return `${authCode.account} · SMTP/IMAP · ${loginText}`;
};

onMounted(async () => {
    if (!userStore.userInfo) {
        await userService.getUserInfo();
    }
    await loadDomains();
    await loadMailboxes();
    await loadAuthCodes();
});
</script>

<style src="@views/settings/SettingItem.css" scoped></style>

<style scoped>
.mailSetting {
    width: 100%;
    height: 100%;
    padding: 0px 10px;
    position: relative;
    z-index: 999;
    top: 0;
    left: 0;
    overflow-y: auto;
}

.mailSetting .settingItem .settingContent .settingBox {
    max-width: 100%;
    margin-left: 0px;
    font-size: 14px;
}

.mailSetting .settingItem .settingContent .settingBox .settings {
    max-height: 260px;
}

.settingRow {
    width: 100%;
    min-height: 58px;
    padding: 8px 0px;
    display: flex;
    justify-content: space-between;
    align-items: center;
    gap: 12px;
    border-bottom: 1px solid var(--bg-active-primary);
}
.settingRow:last-child {
    border-bottom: none;
}
.rowMain {
    min-width: 0;
    flex: 1;
    display: flex;
    flex-direction: column;
    gap: 5px;
}
.rowTitle {
    min-width: 0;
    display: flex;
    align-items: center;
    gap: 6px;
    color: var(--text-secondary1);
    font-size: 14px;
    font-weight: 700;
}
.rowTitle > span:first-child {
    min-width: 0;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.rowDescription {
    min-width: 0;
    display: flex;
    align-items: center;
    gap: 12px;
    color: var(--text-tertiary);
    font-size: 12px;
}
.rowDescription span {
    min-width: 0;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.rowDescription span:last-child {
    flex-shrink: 0;
}
.tag {
    height: 20px;
    padding: 0px 7px;
    flex-shrink: 0;
    display: inline-flex;
    align-items: center;
    border-radius: 999px;
    color: var(--text-secondary2);
    background-color: var(--bg-tertiary);
    font-size: 12px;
    font-weight: 400;
}
.tag.enabled,
.tag.success {
    color: var(--color-success);
    background-color: rgba(82, 196, 26, .12);
}
.tag.disabled {
    color: var(--text-tertiary);
    background-color: var(--bg-secondary4);
}
.tag.primary {
    color: var(--text-link);
    background-color: var(--bg-hover-primary);
}
.tag.owner {
    color: var(--text-link);
    background-color: var(--bg-hover-primary);
}
.tag.readonly {
    color: var(--text-tertiary);
    background-color: var(--bg-secondary4);
}

.mailSetting .operation {
    gap: 6px;
}
.mailSetting .operation button {
    height: 30px;
    padding: 4px 8px;
    font-size: 13px;
    white-space: nowrap;
}
.mailSetting .operation button .iconfont {
    font-size: 16px;
}

.mailSettingDialog {
    width: 100%;
    padding: 12px 14px;
    box-sizing: border-box;
    display: flex;
    flex-direction: column;
    gap: 12px;
}
.domainDialogContent {
    max-height: calc(100vh - 150px);
    overflow-y: auto;
}
.mailSettingDialog .inputGroup {
    width: 100%;
    min-width: 0;
    display: flex;
    flex-direction: column;
    gap: 6px;
    color: var(--text-secondary2);
    font-size: 13px;
}
.mailSettingDialog label {
    cursor: default;
}
.mailSettingDialog input,
.mailSettingDialog select,
.mailSettingDialog textarea {
    width: 100%;
    padding: 0px 10px;
    box-sizing: border-box;
    border-radius: 7px;
    border: 1px solid var(--border-primary);
    color: var(--text-primary);
    background-color: var(--bg-primary);
    outline: none;
    transition: border-color .3s;
}
.mailSettingDialog input,
.mailSettingDialog select {
    height: 36px;
}
.mailSettingDialog textarea {
    min-height: 96px;
    padding-top: 8px;
    padding-bottom: 8px;
    resize: vertical;
    font-family: Consolas, 'Courier New', monospace;
    line-height: 1.45;
}
.mailSettingDialog input {
    cursor: text;
}
.mailSettingDialog textarea {
    cursor: text;
}
.mailSettingDialog select {
    cursor: pointer;
}
.mailSettingDialog input:hover,
.mailSettingDialog input:focus,
.mailSettingDialog textarea:hover,
.mailSettingDialog textarea:focus,
.mailSettingDialog select:hover,
.mailSettingDialog select:focus {
    border-color: var(--border-hover);
}
.tlsDialogContent {
    max-height: 62vh;
    overflow-y: auto;
}
.addressInput {
    width: 100%;
    min-width: 0;
    display: grid;
    grid-template-columns: minmax(0, 1fr) 18px minmax(120px, .8fr);
    align-items: center;
    gap: 6px;
}
.addressInput > span {
    color: var(--text-tertiary);
    text-align: center;
}
.readonlyField {
    width: 100%;
    min-height: 36px;
    padding: 8px 10px;
    box-sizing: border-box;
    display: flex;
    align-items: center;
    border-radius: 7px;
    border: 1px solid var(--border-primary);
    color: var(--text-secondary2);
    background-color: var(--bg-secondary4);
    word-break: break-all;
}
.hintText {
    color: var(--text-tertiary);
    font-size: 12px;
    line-height: 1.5;
}
.domainConfigGrid {
    width: 100%;
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 8px;
}
.configActionButton {
    min-width: 0;
    min-height: 36px;
    padding: 0px 10px;
    box-sizing: border-box;
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 8px;
    border: none;
    border-radius: 7px;
    color: var(--text-secondary2);
    background-color: var(--bg-secondary4);
    font-size: 13px;
    cursor: pointer;
}
.configActionButton:hover {
    color: var(--text-link);
    background-color: var(--bg-hover-primary);
}
.configActionButton.active {
    color: var(--color-success);
    background-color: rgba(82, 196, 26, .12);
}
.configActionButton span:last-child {
    min-width: 0;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.configDot {
    width: 14px;
    height: 14px;
    flex-shrink: 0;
    border-radius: 50%;
    border: 1px solid var(--border-primary);
    background-color: var(--bg-primary);
}
.configActionButton.active .configDot {
    border-color: var(--color-success);
    background-color: var(--color-success);
}
.dkimRecordBox {
    width: 100%;
    padding: 10px;
    box-sizing: border-box;
    display: flex;
    flex-direction: column;
    gap: 8px;
    border-radius: 7px;
    background-color: var(--bg-secondary4);
}
.recordLine {
    min-width: 0;
    display: grid;
    grid-template-columns: 64px minmax(0, 1fr);
    gap: 8px;
    align-items: start;
    color: var(--text-secondary2);
    font-size: 12px;
}
.recordLine > span {
    color: var(--text-tertiary);
}
.recordLine strong {
    min-width: 0;
    font-weight: 500;
}
.recordLine button {
    min-width: 0;
    padding: 0px;
    border: none;
    color: var(--text-link);
    background: transparent;
    text-align: left;
    word-break: break-all;
    line-height: 1.5;
    cursor: pointer;
}
.dnsVerifyBox {
    width: 100%;
    display: flex;
    flex-direction: column;
    gap: 8px;
}
.dnsVerifyHeader {
    min-height: 36px;
    padding: 8px 10px;
    box-sizing: border-box;
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 10px;
    border-radius: 7px;
    background-color: var(--bg-secondary4);
    color: var(--text-secondary2);
    font-size: 13px;
}
.dnsVerifyHeader.success {
    color: var(--color-success);
    background-color: rgba(82, 196, 26, .12);
}
.dnsVerifyHeader.failure {
    color: var(--color-warning);
    background-color: rgba(250, 173, 20, .12);
}
.dnsVerifyHeader button {
    flex-shrink: 0;
    border: none;
    color: var(--text-link);
    background: transparent;
    cursor: pointer;
}
.dnsRecordItem {
    width: 100%;
    padding: 10px;
    box-sizing: border-box;
    display: flex;
    flex-direction: column;
    gap: 7px;
    border-radius: 7px;
    border: 1px solid var(--border-primary);
    background-color: var(--bg-primary);
}
.dnsRecordItem.success {
    border-color: rgba(82, 196, 26, .35);
}
.dnsRecordItem.failure {
    border-color: rgba(250, 173, 20, .45);
}
.dnsRecordTitle,
.dnsRecordMeta {
    min-width: 0;
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 10px;
}
.dnsRecordTitle span {
    color: var(--text-secondary1);
    font-weight: 700;
}
.dnsRecordTitle strong {
    flex-shrink: 0;
    color: var(--text-tertiary);
    font-size: 12px;
    font-weight: 500;
}
.dnsRecordItem.success .dnsRecordTitle strong {
    color: var(--color-success);
}
.dnsRecordItem.failure .dnsRecordTitle strong {
    color: var(--color-warning);
}
.dnsRecordMeta {
    color: var(--text-tertiary);
    font-size: 12px;
}
.dnsRecordMeta span {
    min-width: 0;
    word-break: break-all;
}
.dnsRecordItem .recordLine {
    grid-template-columns: 54px minmax(0, 1fr);
}
.dnsRecordItem .recordLine strong {
    word-break: break-all;
    line-height: 1.5;
}
.checkLine {
    min-width: 0;
    min-height: 32px;
    padding: 0px 8px;
    box-sizing: border-box;
    display: flex;
    align-items: center;
    gap: 8px;
    border-radius: 7px;
    color: var(--text-secondary2);
    background-color: var(--bg-secondary4);
    font-size: 13px;
    cursor: pointer;
}
.checkLine input {
    width: 14px;
    height: 14px;
    min-width: 14px;
    padding: 0px;
    cursor: pointer;
}
.checkLine span {
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
</style>
