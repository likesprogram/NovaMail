<template>
    <div class="systemSetting">
        <div class="settingItem">
            <div class="settingContent alignItemsStart">
                <div class="settingTitle">域名</div>
                <div class="settingBox">
                    <div class="header">
                        <div class="title">系统域名管理</div>
                        <button class="btn-text" @click="openDomainCreateDialog">
                            <span class="iconfont icon-likesprogramwangzhi"></span>
                            添加域名
                        </button>
                    </div>
                    <div class="settings">
                        <div v-for="domain in domains" :key="domain.domainNameId" class="settingRow">
                            <div class="rowMain">
                                <div class="rowTitle">
                                    <span>{{ domain.domainName }}</span>
                                    <span v-if="domain.isDefault" class="tag primary">默认</span>
                                    <span class="tag" :class="domain.userId ? 'private' : 'success'">{{ domain.userId ? '用户域名' : '公共域名' }}</span>
                                    <span v-if="domain.hasCertificate" class="tag success">TLS</span>
                                    <span v-if="domain.hasDKIM" class="tag success">DKIM</span>
                                </div>
                                <div class="rowDescription">
                                    <span>{{ domain.userId ? `用户ID ${domain.userId}` : '所有用户可用' }}</span>
                                    <span>{{ formatDateTime(domain.updateTime) }}</span>
                                </div>
                            </div>
                            <div class="operation">
                                <button class="btn-primary" :class="{ 'btn-disabled': domain.isDefault || domain.userId }" @click="setDefaultDomain(domain)">
                                    <span class="iconfont icon-likesprogramzhengque"></span>
                                    设为默认
                                </button>
                                <button class="btn-primary" @click="openDomainEditDialog(domain)">
                                    <span class="iconfont icon-likesprogrambianji"></span>
                                    修改
                                </button>
                                <button class="btn-danger" :class="{ 'btn-disabled': domain.isDefault }" @click="removeDomain(domain)">
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
                <div class="settingTitle">头像</div>
                <div class="settingBox avatarBox">
                    <div class="header">
                        <div class="title">默认头像管理</div>
                        <button class="btn-text" @click="openAvatarDialog">
                            <span class="iconfont icon-likesprogramshangchuan"></span>
                            修改默认头像
                        </button>
                    </div>
                    <div class="settings avatarSettings">
                        <div class="avatarPreview">
                            <img :src="defaultAvatarPreviewUrl" alt="default avatar">
                        </div>
                        <div class="settingInfo">
                            <div class="rowTitle">
                                <span>{{ defaultAvatar.settingName }}</span>
                                <span class="tag primary">{{ defaultAvatar.settingType }}</span>
                            </div>
                            <div class="rowDescription">
                                <span>用于无头像用户、邮箱头像缺省场景</span>
                                <span>{{ formatDateTime(defaultAvatar.updateTime) }}</span>
                            </div>
                        </div>
                        <div class="operation">
                            <button class="btn-primary" @click="openAvatarDialog">
                                <span class="iconfont icon-likesprogrambianji"></span>
                                修改
                            </button>
                            <button class="btn-warning" @click="resetDefaultAvatar">
                                <span class="iconfont icon-likesprogramgenxin"></span>
                                恢复默认
                            </button>
                        </div>
                    </div>
                </div>
            </div>
        </div>

        <div class="settingItem">
            <div class="settingContent alignItemsStart">
                <div class="settingTitle">资料卡</div>
                <div class="settingBox profileBox">
                    <div class="header">
                        <div class="title">默认资料卡样式管理</div>
                        <button class="btn-text" @click="openDefaultSettingDialog(false)">
                            <span class="iconfont icon-likesprogrambianji"></span>
                            编辑样式
                        </button>
                    </div>
                    <div class="settings defaultSettingSettings">
                        <div class="settingInfo">
                            <div class="rowTitle">
                                <span>{{ defaultProfileSetting.settingName }}</span>
                                <span class="tag primary">{{ defaultProfileSetting.settingType }}</span>
                            </div>
                            <div class="rowDescription">
                                <span>作为新用户默认资料卡/默认签名模板</span>
                                <span>{{ formatDateTime(defaultProfileSetting.updateTime) }}</span>
                            </div>
                        </div>
                        <div class="operation">
                            <button class="btn-primary" @click="openDefaultSettingDialog(true)">
                                <span class="iconfont icon-likesprogrammimakejian"></span>
                                预览
                            </button>
                            <button class="btn-primary" @click="openDefaultSettingDialog(false)">
                                <span class="iconfont icon-likesprogrambianji"></span>
                                编辑
                            </button>
                            <button class="btn-warning" @click="resetDefaultSetting">
                                <span class="iconfont icon-likesprogramgenxin"></span>
                                恢复默认
                            </button>
                        </div>
                    </div>
                </div>
            </div>
        </div>

        <div class="settingItem">
            <div class="settingContent alignItemsStart">
                <div class="settingTitle">客户端</div>
                <div class="settingBox clientBox">
                    <div class="header">
                        <div class="title">客户端下载配置</div>
                        <button class="btn-text" @click="openClientDialog">
                            <span class="iconfont icon-likesprogramshangchuan"></span>
                            上传客户端
                        </button>
                    </div>
                    <div class="settings clientSettings">
                        <div class="settingInfo">
                            <div class="rowTitle">
                                <span>{{ clientDownloadInfo?.fileName || '暂未上传客户端' }}</span>
                                <span v-if="clientDownloadInfo?.version" class="tag primary">{{ clientDownloadInfo.version }}</span>
                                <span v-if="clientDownloadInfo" class="tag success">{{ formatFileSize(clientDownloadInfo.fileSize) }}</span>
                            </div>
                            <div class="rowDescription">
                                <span>{{ clientDownloadInfo?.description || '上传后 Sidebar 的“下载客户端”会自动指向最新安装包' }}</span>
                                <span>{{ clientDownloadInfo ? formatDateTime(clientDownloadInfo.uploadTime) : '' }}</span>
                            </div>
                        </div>
                        <div class="operation">
                            <button class="btn-primary" :class="{ 'btn-disabled': !clientDownloadInfo }" @click="downloadClientPackage">
                                <span class="iconfont icon-likesprogramxiazai"></span>
                                下载
                            </button>
                            <button class="btn-primary" @click="openClientDialog">
                                <span class="iconfont icon-likesprogramshangchuan"></span>
                                上传
                            </button>
                        </div>
                    </div>
                </div>
            </div>
        </div>

        <MessageDialog ref="domainDialog" :width="'640px'">
            <template v-slot:title>
                <span>{{ domainDialogMode === 'create' ? '新增系统域名' : '修改系统域名' }}</span>
            </template>
            <template v-slot:content>
                <div class="dialogContent systemSettingDialog domainDialogContent">
                    <div class="inputGroup">
                        <label for="domainName">域名</label>
                        <input id="domainName" v-model="domainForm.domainName" type="text" placeholder="例如 novamail.com">
                    </div>
                    <div class="inputGroup">
                        <label for="domainUserId">所属用户ID</label>
                        <input id="domainUserId" v-model="domainForm.userId" type="text" placeholder="留空表示公共域名">
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
                    <div class="hintText">DKIM 密钥由系统自动生成，需要在域名服务商处添加 TXT 记录；DNS 验证会检查 DKIM、SPF、IMAP、SMTP 和 MX 解析。</div>
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
                </div>
            </template>
            <template v-slot:footer>
                <div class="dialogFooterButton failure" @click="domainDialog?.Close">
                    <span class="textBox">
                        <span class="iconfont icon-likesprogramcuowu"></span>
                        <span class="text">取消</span>
                    </span>
                    <span class="line"></span>
                </div>
                <div class="dialogFooterButton success" @click="saveDomain">
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
                <div class="dialogContent systemSettingDialog tlsDialogContent">
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

        <MessageDialog ref="avatarDialog" :width="'460px'">
            <template v-slot:title>
                <span>修改默认头像</span>
            </template>
            <template v-slot:content>
                <div class="dialogContent systemSettingDialog">
                    <div class="avatarEditor">
                        <div class="avatarPreview large">
                            <img :src="avatarDialogPreviewUrl" alt="default avatar preview">
                        </div>
                        <button class="btn-primary" @click="chooseAvatarImage">
                            <span class="iconfont icon-likesprogramshangchuan"></span>
                            选择图片
                        </button>
                    </div>
                    <div class="inputGroup">
                        <label for="avatarValue">头像值</label>
                        <textarea id="avatarValue" v-model="avatarForm.settingValue" placeholder="支持 data:image base64、URL 或对象存储路径"></textarea>
                    </div>
                    <div class="hintText">选择图片会上传到 MinIO；直接填写 URL 或 data:image 时会写入系统配置。页面展示统一走后端默认头像接口。</div>
                </div>
            </template>
            <template v-slot:footer>
                <div class="dialogFooterButton failure" @click="avatarDialog?.Close">
                    <span class="textBox">
                        <span class="iconfont icon-likesprogramcuowu"></span>
                        <span class="text">取消</span>
                    </span>
                    <span class="line"></span>
                </div>
                <div class="dialogFooterButton success" @click="saveDefaultAvatar">
                    <span class="textBox">
                        <span class="iconfont icon-likesprogramzhengque"></span>
                        <span class="text">完成</span>
                    </span>
                </div>
            </template>
        </MessageDialog>

        <MessageDialog ref="defaultSettingDialog" :width="'720px'">
            <template v-slot:title>
                <span>{{ defaultSettingPreview ? '预览默认资料卡样式' : '编辑默认资料卡样式' }}</span>
            </template>
            <template v-slot:content>
                <div class="dialogContent defaultSettingDialog">
                    <template v-if="defaultSettingPreview">
                        <div class="signaturePreviewContent" v-html="renderedDefaultSetting"></div>
                    </template>
                    <template v-else>
                        <div class="signatureContent">
                            <span class="title">
                                <span class="text">HTML 模板：</span>
                                <span class="subtitle">支持基础 HTML 标签，并可使用以下占位符。</span>
                            </span>
                            <div class="prompt">
                                <div class="prompts">
                                    <div class="promptItem">
                                        <span class="label" v-pre>{{ profileCardUrl }}</span>
                                        <span class="description">个人资料卡链接</span>
                                    </div>
                                    <div class="promptItem">
                                        <span class="label" v-pre>{{ avatarUrl }}</span>
                                        <span class="description">用户头像地址</span>
                                    </div>
                                    <div class="promptItem">
                                        <span class="label" v-pre>{{ nickname }}</span>
                                        <span class="description">用户昵称</span>
                                    </div>
                                    <div class="promptItem">
                                        <span class="label" v-pre>{{ email }}</span>
                                        <span class="description">默认邮箱地址</span>
                                    </div>
                                </div>
                            </div>
                            <textarea v-model="profileForm.settingValue"></textarea>
                        </div>
                    </template>
                </div>
            </template>
            <template v-slot:footer>
                <div class="dialogFooterButton failure" @click="defaultSettingDialog?.Close">
                    <span class="textBox">
                        <span class="iconfont icon-likesprogramcuowu"></span>
                        <span class="text">取消</span>
                    </span>
                    <span class="line"></span>
                </div>
                <div v-if="defaultSettingPreview" class="dialogFooterButton success" @click="defaultSettingPreview = false">
                    <span class="textBox">
                        <span class="iconfont icon-likesprogrambianji"></span>
                        <span class="text">编辑</span>
                    </span>
                    <span class="line"></span>
                </div>
                <div v-else class="dialogFooterButton success" @click="defaultSettingPreview = true">
                    <span class="textBox">
                        <span class="iconfont icon-likesprogrammimakejian"></span>
                        <span class="text">预览</span>
                    </span>
                    <span class="line"></span>
                </div>
                <div class="dialogFooterButton success" @click="saveDefaultSetting">
                    <span class="textBox">
                        <span class="iconfont icon-likesprogramzhengque"></span>
                        <span class="text">完成</span>
                    </span>
                </div>
            </template>
        </MessageDialog>

        <MessageDialog ref="clientDialog" :width="'520px'">
            <template v-slot:title>
                <span>上传客户端安装包</span>
            </template>
            <template v-slot:content>
                <div class="dialogContent systemSettingDialog">
                    <div class="clientFilePicker">
                        <button class="btn-primary" @click="chooseClientPackage">
                            <span class="iconfont icon-likesprogramshangchuan"></span>
                            选择安装包
                        </button>
                        <div class="clientFileName">{{ clientForm.file?.name || '未选择文件' }}</div>
                    </div>
                    <div class="inputGroup">
                        <label for="clientVersion">版本号</label>
                        <input id="clientVersion" v-model="clientForm.version" type="text" placeholder="例如 1.0.0">
                    </div>
                    <div class="inputGroup">
                        <label for="clientDescription">说明</label>
                        <textarea id="clientDescription" v-model="clientForm.description" placeholder="例如 Windows x64 安装包"></textarea>
                    </div>
                    <div v-if="clientUploadProgress > 0 && clientUploadProgress < 100" class="uploadProgress">
                        <span :style="{ width: `${clientUploadProgress}%` }"></span>
                    </div>
                    <div class="hintText">客户端本体会保存到 MinIO，系统配置只保存文件名、版本、大小和对象路径。</div>
                </div>
            </template>
            <template v-slot:footer>
                <div class="dialogFooterButton failure" @click="clientDialog?.Close">
                    <span class="textBox">
                        <span class="iconfont icon-likesprogramcuowu"></span>
                        <span class="text">取消</span>
                    </span>
                    <span class="line"></span>
                </div>
                <div class="dialogFooterButton success" @click="uploadClientPackage">
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
import { computed, onMounted, ref, watch } from 'vue';
import MessageDialog from '@components/messageDialog/MessageDialog.vue';
import ShowMessage from '@components/messageDialog/ShowMessage.vue';
import '@components/messageDialog/MessageDialog.css';
import { formatDateTime } from '@utils/Utils';
import { FileProcessing } from '@utils/FileProcessing';
import { SignatureService } from '@service/SignatureService';
import { UserStore } from '@store/UserStore';
import { SystemSettingService } from '@service/SystemSettingService';
import { baseUrl } from '@utils/Request';
import type { DomainDkimConfigData, DomainDnsVerificationData, DomainNameData } from '@api/email/MailBox';
import type { SystemClientDownloadInfo, SystemSettingData } from '@api/SystemSetting';

export default {
    name: 'SystemSetting',
}
</script>

<script setup lang="ts">
type DialogMode = 'create' | 'edit';

const defaultAvatarBase64 = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAADwAAAA8CAYAAAA6/NlyAAAAAXNSR0IArs4c6QAAApJJREFUaEPtmc9PE0EUx787u93WUqUaWmsgxgMHDh4w4UqiiRwI/vhbFQ0HTTTxaiIHDxw4EANpbYlWKKXtdmfM2GwC2GaH7bx13M5eO52+z/vMezOzdbbrA4EZehwLnHHb1nDGBcMatoYzlgG7pDMm9C8ca9gazlgG7JLOmNB/27TkPfR+kWExz/4EctTn+NblcFLMcmpLeqnAsFp2x6LttkMc9ngq2KkAPygyPLw1Hjai/HoS4qBLD50K8LNaTsnem0agNG6aQeTAKyUXy6VRzcY9+x2OvU4YN2yqz8mBt2o55aYkm9pbYsukwBLgueJyjrRtNwLlBCVRbYGTZG3Sd2bOsEzETNWwBH4072LxhlqXPjrn+PLrP+/SXAAv7qntw6/rARjxOZO0aUW1vTrvYinG8uE5xy6xXRlPKsDyhzaqHvIT9PW5wLvmUGe/nDhXasCyY6/f8VD2L6/Z9kDg048h6d57kT414OhHZU1X8iPoVl+Q1+xV1aTAt30H8o50rPiP7ILvQPbon4rjk9SAduAnFQ9z7vhWu38WYu/08hVw5SbD8tz4q+NZKPChpbe2tQEXGPC0Gr/9DDmw0xxdAzerOXgKW/T7ZgBd7we0Aa+VXdQkNcHT6HF8bus5kGgDVr3kJ82HrpcDWoBDAbxUPE0lBX5VDzChNVxrSi3AMpDNu/H1e63Irgze+R5AJnbaRwuwDGKmlnSU9ccLHkqe3tN/Zyjw8Vjf1qTN8MWlJleez4ByzkHNZ6gUHBRjCrAbCrR6Ao0BRzsQGHCQHDdJgKetM8rvW2DK7JowtzVsggXKGKxhyuyaMLc1bIIFyhisYcrsmjC3NWyCBcoYrGHK7JowtzVsggXKGKxhyuyaMPdvK7NxwJf6+eYAAAAASUVORK5CYII=';
const defaultProfileCardHtml = `<xm-signature contenteditable="false" style="display:inline-block;line-height:1.43;">
    <div style="display:block;width:150px;height:1px;margin:32px 0 10px;background:#e6e8eb;"></div>
    <a href="{{profileCardUrl}}" target="_blank" style="text-decoration:none;font-family:-apple-system,BlinkMacSystemFont,PingFang SC,Microsoft YaHei">
        <table cellspacing="0" cellpadding="0" style="table-layout:fixed;padding-right:20px">
            <tr valign="top">
                <td style="width:40px;min-width:40px;padding-top:10px">
                    <div style="width:38px;height:38px;border-radius:50%;box-shadow:0 0 10px rgba(127,152,178,0.14)">
                        <img src="{{avatarUrl}}" style="width:100%;height:100%;border-radius:50%;">
                    </div>
                </td>
                <td style="padding:10px 0 8px 10px">
                    <div style="font-size:14px;color:#333;font-weight:500">
                        {{nickname}}
                    </div>
                    <div style="font-size:12px;color:#999">
                        {{email}}
                    </div>
                </td>
            </tr>
        </table>
    </a>
</xm-signature>`;

const userStore = UserStore();
const showMessage = ref<InstanceType<typeof ShowMessage>>();
const domainDialog = ref<InstanceType<typeof MessageDialog>>();
const tlsDialog = ref<InstanceType<typeof MessageDialog>>();
const avatarDialog = ref<InstanceType<typeof MessageDialog>>();
const defaultSettingDialog = ref<InstanceType<typeof MessageDialog>>();
const clientDialog = ref<InstanceType<typeof MessageDialog>>();

const domains = ref<DomainNameData[]>([]);

const defaultAvatar = ref<SystemSettingData>({
    settingId: '1',
    settingName: 'DefaultAvatar',
    settingValue: defaultAvatarBase64,
    settingType: 'base64:image',
    createTime: '2026-05-01 09:00:00',
    updateTime: '2026-05-21 10:30:00',
});

const defaultProfileSetting = ref<SystemSettingData>({
    settingId: '2',
    settingName: 'DefaultSetting',
    settingValue: defaultProfileCardHtml,
    settingType: 'text:html',
    createTime: '2026-05-01 09:00:00',
    updateTime: '2026-05-21 10:30:00',
});

const domainDialogMode = ref<DialogMode>('create');
const editingDomainId = ref('');
const domainForm = ref({
    domainName: '',
    userId: '',
    hasCertificate: false,
    hasDKIM: false,
    isDefault: false,
});
const dkimConfig = ref<DomainDkimConfigData | null>(null);
const dnsVerification = ref<DomainDnsVerificationData | null>(null);

const tlsForm = ref({
    domainNameId: '',
    privateKey: '',
    certificate: '',
    caChain: '',
});

const avatarForm = ref({
    settingValue: '',
    file: null as File | null,
    previewUrl: '',
});

const profileForm = ref({
    settingValue: '',
});
const defaultSettingPreview = ref(false);
const clientDownloadInfo = ref<SystemClientDownloadInfo | null>(null);
const clientUploadProgress = ref(0);
const clientForm = ref<{
    file: File | null;
    version: string;
    description: string;
}>({
    file: null,
    version: '',
    description: '',
});
const defaultAvatarVersion = ref(Date.now());

const tlsDomainName = computed(() => domains.value.find(item => item.domainNameId === tlsForm.value.domainNameId)?.domainName || domainForm.value.domainName || '当前域名');
const defaultAvatarPreviewUrl = computed(() => `${baseUrl}/userProfile/defaultAvatar?v=${defaultAvatarVersion.value}`);
const avatarDialogPreviewUrl = computed(() => {
    if (avatarForm.value.previewUrl) return avatarForm.value.previewUrl;
    const value = avatarForm.value.settingValue.trim();
    if (value.startsWith('data:image') || value.startsWith('http://') || value.startsWith('https://')) return value;
    return defaultAvatarPreviewUrl.value;
});

watch(() => domainForm.value.userId, value => {
    if (value.trim()) domainForm.value.isDefault = false;
});

const renderedDefaultSetting = computed(() => {
    return SignatureService.renderSignature(
        profileForm.value.settingValue,
        userStore.account || 'demo',
        userStore.nickname || 'NovaMail 用户',
        userStore.mailBoxView || 'demo@lby.email',
    );
});

const getErrorMessage = (error: unknown, fallback: string) => {
    return error instanceof Error && error.message ? error.message : fallback;
};

const formatFileSize = (size?: number | null): string => {
    if (!size || size <= 0) return '0 B';
    const units = ['B', 'KB', 'MB', 'GB', 'TB'];
    let value = size;
    let index = 0;
    while (value >= 1024 && index < units.length - 1) {
        value /= 1024;
        index++;
    }
    return `${value.toFixed(index === 0 ? 0 : 2)} ${units[index]}`;
};

const loadDomains = async () => {
    try {
        domains.value = await SystemSettingService.getDomainList();
    } catch (error) {
        domains.value = [];
        showMessage.value?.ShowModal('域名列表加载失败', [getErrorMessage(error, '请稍后重试')], 'failure');
    }
};

const loadSystemSettings = async () => {
    try {
        defaultAvatar.value = await SystemSettingService.getDefaultAvatar();
        defaultAvatarVersion.value = Date.now();
    } catch (error) {
        showMessage.value?.ShowModal('默认头像加载失败', [getErrorMessage(error, '已使用本地默认值')], 'warning');
    }

    try {
        defaultProfileSetting.value = await SystemSettingService.getDefaultSetting();
    } catch (error) {
        showMessage.value?.ShowModal('默认资料卡加载失败', [getErrorMessage(error, '已使用本地默认值')], 'warning');
    }

    try {
        clientDownloadInfo.value = await SystemSettingService.getClientDownloadInfo();
    } catch (error) {
        clientDownloadInfo.value = null;
    }
};

const openDomainCreateDialog = () => {
    domainDialogMode.value = 'create';
    editingDomainId.value = '';
    domainForm.value = {
        domainName: '',
        userId: '',
        hasCertificate: false,
        hasDKIM: false,
        isDefault: domains.value.length === 0,
    };
    dkimConfig.value = null;
    dnsVerification.value = null;
    domainDialog.value?.ShowModal();
};

const openDomainEditDialog = (domain: DomainNameData) => {
    domainDialogMode.value = 'edit';
    editingDomainId.value = domain.domainNameId;
    domainForm.value = {
        domainName: domain.domainName,
        userId: domain.userId ?? '',
        hasCertificate: domain.hasCertificate,
        hasDKIM: domain.hasDKIM,
        isDefault: domain.isDefault,
    };
    dkimConfig.value = null;
    dnsVerification.value = null;
    domainDialog.value?.ShowModal();
};

const saveDomain = async () => {
    const domainName = domainForm.value.domainName.trim().toLowerCase();
    const userId = domainForm.value.userId.trim();
    const domainRegex = /^(?!-)(?:[a-zA-Z0-9-]{1,63}\.)+[a-zA-Z]{2,}$/;

    if (!domainRegex.test(domainName)) {
        showMessage.value?.ShowModal('信息不完整', ['请输入正确的域名，例如 example.com'], 'warning');
        return;
    }
    if (userId && !/^\d+$/.test(userId)) {
        showMessage.value?.ShowModal('信息不完整', ['所属用户ID只能填写数字，留空表示公共域名'], 'warning');
        return;
    }
    const duplicated = domains.value.some(domain => domain.domainName === domainName && domain.domainNameId !== editingDomainId.value);
    if (duplicated) {
        showMessage.value?.ShowModal('域名已存在', ['当前域名已经存在'], 'warning');
        return;
    }
    if (userId && domainForm.value.isDefault) {
        showMessage.value?.ShowModal('无法设置默认', ['用户私有域名不能设为系统默认域名'], 'warning');
        return;
    }
    const currentDomain = domains.value.find(domain => domain.domainNameId === editingDomainId.value);
    if (userId && currentDomain?.isDefault) {
        showMessage.value?.ShowModal('无法修改归属', ['系统默认域名不能改为用户私有域名，请先设置其他公共域名为默认'], 'warning');
        return;
    }

    const payload = {
        domainName,
        userId: userId || null,
        hasCertificate: domainForm.value.hasCertificate,
        hasDKIM: domainForm.value.hasDKIM,
        isDefault: domainForm.value.isDefault,
    };

    try {
        if (domainDialogMode.value === 'create') {
            const createdDomain = await SystemSettingService.createDomain(payload);
            await loadDomains();
            if (createdDomain?.domainNameId) {
                editingDomainId.value = String(createdDomain.domainNameId);
                domainDialogMode.value = 'edit';
                domainForm.value.hasDKIM = Boolean(createdDomain.hasDKIM);
                if (payload.hasDKIM) {
                    await showDkimConfig(String(createdDomain.domainNameId), false);
                    showMessage.value?.ShowModal('DKIM 已生成', ['请复制弹窗中的 DKIM TXT 记录，并到域名服务商处配置解析。'], 'success');
                    return;
                }
            }
        } else {
            await SystemSettingService.updateDomain({
                domainNameId: editingDomainId.value,
                ...payload,
            });
            await loadDomains();
        }
    } catch (error) {
        showMessage.value?.ShowModal(domainDialogMode.value === 'create' ? '域名创建失败' : '域名修改失败', [getErrorMessage(error, '请稍后重试')], 'failure');
        return;
    }

    domainDialog.value?.Close();
};

const setDefaultDomain = async (domain: DomainNameData) => {
    if (domain.isDefault) return;
    if (domain.userId) {
        showMessage.value?.ShowModal('无法设置默认', ['用户私有域名不能设为系统默认域名'], 'warning');
        return;
    }
    try {
        await SystemSettingService.setDefaultDomain(domain.domainNameId);
        await loadDomains();
    } catch (error) {
        showMessage.value?.ShowModal('默认域名设置失败', [getErrorMessage(error, '请稍后重试')], 'failure');
    }
};

const removeDomain = (domain: DomainNameData) => {
    if (domain.isDefault) {
        showMessage.value?.ShowModal('无法移除', ['默认域名不能移除，请先设置其他默认域名'], 'warning');
        return;
    }
    showMessage.value?.ShowModal('确认移除', [`确定要移除域名 ${domain.domainName} 吗？`], 'warning', async selectedButtonIndex => {
        if (selectedButtonIndex !== 1) return;
        try {
            await SystemSettingService.deleteDomain(domain.domainNameId);
            await loadDomains();
        } catch (error) {
            showMessage.value?.ShowModal('域名移除失败', [getErrorMessage(error, '请稍后重试')], 'failure');
        }
    });
};

const configureTls = () => {
    const targetDomainId = editingDomainId.value;
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
        await SystemSettingService.configureDomainTls({
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
        : editingDomainId.value;
    if (!targetDomainId) {
        showMessage.value?.ShowModal('请先保存域名', ['保存域名后系统会自动生成 DKIM 解析记录'], 'warning');
        return;
    }

    try {
        dkimConfig.value = await SystemSettingService.getDomainDkimConfig(targetDomainId);
        domainForm.value.hasDKIM = true;
        await loadDomains();
        if (showSuccessMessage) {
            showMessage.value?.ShowModal('DKIM 解析记录', ['已生成 DKIM TXT 记录，请复制后到域名服务商处配置解析。'], 'success');
        }
    } catch (error) {
        showMessage.value?.ShowModal('DKIM 记录获取失败', [getErrorMessage(error, '请稍后重试')], 'failure');
    }
};

const verifyDomainDns = async () => {
    const targetDomainId = editingDomainId.value;
    if (!targetDomainId) {
        showMessage.value?.ShowModal('请先保存域名', ['保存域名后即可验证 DNS 解析'], 'warning');
        return;
    }

    try {
        dnsVerification.value = await SystemSettingService.verifyDomainDns(targetDomainId);
        showMessage.value?.ShowModal(
            dnsVerification.value.passed ? 'DNS 验证通过' : 'DNS 验证未通过',
            [dnsVerification.value.passed ? '所有必要解析均已配置正确。' : '仍有解析未生效，请查看弹窗中的验证结果。'],
            dnsVerification.value.passed ? 'success' : 'warning'
        );
    } catch (error) {
        showMessage.value?.ShowModal('DNS 验证失败', [getErrorMessage(error, '请稍后重试')], 'failure');
    }
};

const openAvatarDialog = () => {
    if (avatarForm.value.previewUrl) URL.revokeObjectURL(avatarForm.value.previewUrl);
    avatarForm.value = {
        settingValue: defaultAvatar.value.settingValue,
        file: null,
        previewUrl: '',
    };
    avatarDialog.value?.ShowModal();
};

const chooseAvatarImage = async () => {
    try {
        const file = await FileProcessing.openFiles('image/*', false) as File;
        if (avatarForm.value.previewUrl) URL.revokeObjectURL(avatarForm.value.previewUrl);
        avatarForm.value.file = file;
        avatarForm.value.previewUrl = URL.createObjectURL(file);
    } catch (error) {
        return;
    }
};

const saveDefaultAvatar = async () => {
    const settingValue = avatarForm.value.settingValue.trim();
    if (!avatarForm.value.file && !settingValue) {
        showMessage.value?.ShowModal('信息不完整', ['默认头像不能为空'], 'warning');
        return;
    }
    try {
        if (avatarForm.value.file) {
            defaultAvatar.value = await SystemSettingService.uploadDefaultAvatar(avatarForm.value.file);
        } else {
            defaultAvatar.value = await SystemSettingService.setDefaultAvatar(settingValue);
        }
        defaultAvatarVersion.value = Date.now();
        if (avatarForm.value.previewUrl) URL.revokeObjectURL(avatarForm.value.previewUrl);
        avatarDialog.value?.Close();
    } catch (error) {
        showMessage.value?.ShowModal('默认头像保存失败', [getErrorMessage(error, '请稍后重试')], 'failure');
    }
};

const resetDefaultAvatar = () => {
    showMessage.value?.ShowModal('确认恢复', ['确定要恢复系统默认头像吗？'], 'warning', async selectedButtonIndex => {
        if (selectedButtonIndex !== 1) return;
        try {
            defaultAvatar.value = await SystemSettingService.setDefaultAvatar(defaultAvatarBase64);
            defaultAvatarVersion.value = Date.now();
        } catch (error) {
            showMessage.value?.ShowModal('默认头像恢复失败', [getErrorMessage(error, '请稍后重试')], 'failure');
        }
    });
};

const openDefaultSettingDialog = (preview: boolean) => {
    profileForm.value = {
        settingValue: defaultProfileSetting.value.settingValue,
    };
    defaultSettingPreview.value = preview;
    defaultSettingDialog.value?.ShowModal();
};

const saveDefaultSetting = async () => {
    const settingValue = profileForm.value.settingValue.trim();
    if (!settingValue) {
        showMessage.value?.ShowModal('信息不完整', ['默认资料卡样式不能为空'], 'warning');
        return;
    }
    try {
        defaultProfileSetting.value = await SystemSettingService.setDefaultSetting(settingValue);
        defaultSettingDialog.value?.Close();
    } catch (error) {
        showMessage.value?.ShowModal('默认资料卡保存失败', [getErrorMessage(error, '请稍后重试')], 'failure');
    }
};

const resetDefaultSetting = () => {
    showMessage.value?.ShowModal('确认恢复', ['确定要恢复系统默认资料卡样式吗？'], 'warning', async selectedButtonIndex => {
        if (selectedButtonIndex !== 1) return;
        try {
            defaultProfileSetting.value = await SystemSettingService.setDefaultSetting(defaultProfileCardHtml);
        } catch (error) {
            showMessage.value?.ShowModal('默认资料卡恢复失败', [getErrorMessage(error, '请稍后重试')], 'failure');
        }
    });
};

const openClientDialog = () => {
    clientForm.value = {
        file: null,
        version: clientDownloadInfo.value?.version || '',
        description: clientDownloadInfo.value?.description || '',
    };
    clientUploadProgress.value = 0;
    clientDialog.value?.ShowModal();
};

const chooseClientPackage = async () => {
    try {
        const file = await FileProcessing.openFiles('*', false) as File;
        clientForm.value.file = file;
    } catch (error) {
        return;
    }
};

const uploadClientPackage = async () => {
    if (!clientForm.value.file) {
        showMessage.value?.ShowModal('信息不完整', ['请选择客户端安装包文件'], 'warning');
        return;
    }
    try {
        clientUploadProgress.value = 1;
        clientDownloadInfo.value = await SystemSettingService.uploadClientPackage(
            clientForm.value.file,
            {
                version: clientForm.value.version.trim(),
                description: clientForm.value.description.trim(),
            },
            progress => {
                clientUploadProgress.value = progress;
            }
        );
        clientUploadProgress.value = 100;
        clientDialog.value?.Close();
        showMessage.value?.ShowModal('客户端上传成功', ['Sidebar 下载入口已指向最新客户端'], 'success');
    } catch (error) {
        showMessage.value?.ShowModal('客户端上传失败', [getErrorMessage(error, '请稍后重试')], 'failure');
    }
};

const downloadClientPackage = async () => {
    if (!clientDownloadInfo.value) {
        showMessage.value?.ShowModal('暂未配置客户端', ['请先上传客户端安装包'], 'warning');
        return;
    }
    try {
        await SystemSettingService.downloadClient(clientDownloadInfo.value.fileName);
    } catch (error) {
        showMessage.value?.ShowModal('客户端下载失败', [getErrorMessage(error, '请稍后重试')], 'failure');
    }
};

onMounted(async () => {
    await loadDomains();
    await loadSystemSettings();
});
</script>

<style src="@views/settings/SettingItem.css" scoped></style>

<style scoped>
.systemSetting {
    width: 100%;
    height: 100%;
    padding: 0px 10px;
    position: relative;
    z-index: 999;
    top: 0;
    left: 0;
    overflow-y: auto;
}

.systemSetting .settingItem .settingContent .settingBox {
    max-width: 100%;
    margin-left: 0px;
    font-size: 14px;
}

.systemSetting .settingItem .settingContent .settingBox .settings {
    max-height: 280px;
}

.settingRow,
.avatarSettings,
.defaultSettingSettings,
.clientSettings {
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
.rowMain,
.settingInfo {
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
.tag.success {
    color: var(--color-success);
    background-color: rgba(82, 196, 26, .12);
}
.tag.private {
    color: var(--color-warning);
    background-color: rgba(250, 173, 20, .12);
}
.tag.primary {
    color: var(--text-link);
    background-color: var(--bg-hover-primary);
}

.systemSetting .operation {
    gap: 6px;
}
.systemSetting .operation button {
    height: 30px;
    padding: 4px 8px;
    font-size: 13px;
    white-space: nowrap;
}
.systemSetting .operation button .iconfont {
    font-size: 16px;
}

.avatarSettings,
.defaultSettingSettings,
.clientSettings {
    border-bottom: none;
}
.avatarPreview {
    width: 48px;
    height: 48px;
    flex-shrink: 0;
    border-radius: 50%;
    overflow: hidden;
    background-color: var(--bg-primary);
    box-shadow: var(--shadow-card);
}
.avatarPreview.large {
    width: 72px;
    height: 72px;
}
.avatarPreview img {
    width: 100%;
    height: 100%;
    display: block;
    object-fit: cover;
}

.systemSettingDialog {
    width: 100%;
    padding: 12px 14px;
    box-sizing: border-box;
    display: flex;
    flex-direction: column;
    gap: 12px;
}
.systemSettingDialog .inputGroup {
    width: 100%;
    min-width: 0;
    display: flex;
    flex-direction: column;
    gap: 6px;
    color: var(--text-secondary2);
    font-size: 13px;
}
.systemSettingDialog input,
.systemSettingDialog textarea {
    width: 100%;
    padding: 8px 10px;
    box-sizing: border-box;
    border-radius: 7px;
    border: 1px solid var(--border-primary);
    color: var(--text-primary);
    background-color: var(--bg-primary);
    outline: none;
    cursor: text;
    transition: border-color .3s;
}
.systemSettingDialog input {
    height: 36px;
}
.systemSettingDialog textarea {
    height: 110px;
    resize: none;
    line-height: 1.5;
}
.tlsDialogContent textarea {
    min-height: 96px;
    resize: vertical;
    font-family: Consolas, 'Courier New', monospace;
    line-height: 1.45;
}
.systemSettingDialog input:hover,
.systemSettingDialog input:focus,
.systemSettingDialog textarea:hover,
.systemSettingDialog textarea:focus {
    border-color: var(--border-hover);
}
.domainDialogContent {
    max-height: calc(100vh - 150px);
    overflow-y: auto;
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
.optionGrid {
    width: 100%;
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 8px;
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
.avatarEditor {
    display: flex;
    align-items: center;
    gap: 14px;
}
.clientFilePicker {
    min-width: 0;
    display: flex;
    align-items: center;
    gap: 12px;
}
.clientFileName {
    min-width: 0;
    flex: 1;
    color: var(--text-secondary2);
    font-size: 13px;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.uploadProgress {
    width: 100%;
    height: 6px;
    border-radius: 999px;
    overflow: hidden;
    background-color: var(--bg-secondary4);
}
.uploadProgress span {
    height: 100%;
    display: block;
    border-radius: inherit;
    background-color: var(--text-link);
    transition: width .2s ease;
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

.defaultSettingDialog {
    width: 100%;
    min-height: 360px;
    box-sizing: border-box;
    color: var(--text-primary);
}
.signatureContent {
    padding: 10px 14px;
    display: flex;
    flex-direction: column;
    gap: 10px;
}
.signatureContent .title {
    display: flex;
    flex-direction: column;
    gap: 3px;
}
.signatureContent .title .text {
    color: var(--text-secondary1);
    font-size: 15px;
    font-weight: 700;
}
.signatureContent .title .subtitle {
    color: var(--text-tertiary);
    font-size: 12px;
}
.prompt {
    padding: 8px 10px;
    border-radius: 8px;
    background-color: var(--bg-secondary4);
}
.prompts {
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 6px;
}
.promptItem {
    min-width: 0;
    display: flex;
    flex-direction: column;
    gap: 2px;
}
.promptItem .label {
    color: var(--text-link);
    font-size: 12px;
    font-family: Consolas, monospace;
}
.promptItem .description {
    color: var(--text-tertiary);
    font-size: 12px;
}
.signatureContent textarea {
    width: 100%;
    height: 210px;
    padding: 10px;
    box-sizing: border-box;
    border-radius: 8px;
    border: 1px solid var(--border-primary);
    resize: none;
    outline: none;
    color: var(--text-primary);
    background-color: var(--bg-primary);
    font-family: Consolas, monospace;
    line-height: 1.5;
}
.signatureContent textarea:hover,
.signatureContent textarea:focus {
    border-color: var(--border-hover);
}
.signaturePreviewContent {
    min-height: 330px;
    padding: 18px;
    box-sizing: border-box;
    overflow: auto;
    background-color: var(--bg-primary);
}
</style>
