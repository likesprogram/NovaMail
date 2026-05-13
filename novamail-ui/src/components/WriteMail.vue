<template>
    <div class="writeMail">
        <!-- 分别发送 -->
        <div class="inputGroup" v-if="sendSeparately">
            <span class="groupTitle">分别发送</span>
            <RecipientInput v-model="recipients" />
            <span class="button btn-text" @click="sendSeparately = false">
                取消分别发送
            </span>
        </div>
        <div v-else ref="recipientAreaRef" class="recipientArea">
            <!-- 收件人 -->
            <div class="inputGroup">
                <span class="groupTitle">收件人</span>
                <RecipientInput v-model="recipientTos" />
                <span v-if="!shouldShowCc" class="button btn-text" @click="showCc = true">
                    抄送
                </span>
                <span v-if="!shouldShowBcc" class="button btn-text" @click="showBcc = true">
                    密送
                </span>
                <span class="button btn-text" @click="sendSeparately = true">
                    分别发送
                </span>
            </div>
            <!-- 抄送 -->
            <div class="inputGroup" v-if="shouldShowCc">
                <span class="groupTitle">抄送</span>
                <RecipientInput v-model="recipientCcs" />
            </div>
            <!-- 密送 -->
            <div class="inputGroup" v-if="shouldShowBcc">
                <span class="groupTitle">密送</span>
                <RecipientInput v-model="recipientBccs" />
            </div>
        </div>
        <!-- 主题 -->
        <div class="inputGroup">
            <span class="groupTitle">主  题</span>
            <input type="text" v-model="subject" />
        </div>

        <!-- 富文本框操作按钮 -->
        <div class="editorToolbar">
            <div class="editorToolbarLeft">
                <!-- 撤销 -->
                <button class="toolbarButton" :class="{ 'btn-disabled': !canUndo }" @click.stop="undoEditor">
                    <span class="iconfont icon-likesprograma-shangyibu1"></span>
                </button>
                <!-- 重做 -->
                <button class="toolbarButton" :class="{ 'btn-disabled': !canRedo }" @click="redoEditor">
                    <span class="iconfont icon-likesprogramxiayibu"></span>
                </button>
                <span class="line"></span>
                <!-- 图片 -->
                <button class="toolbarButton" ref="imageButton" @click="showImagePanel">
                    <span class="iconfont icon-likesprogramtupianzujian"></span>
                    <span class="text">图片</span>
                    <!-- 发件人设置面板 -->
                    <SafePopover ref="imagePanel" :NoBoxShadow="['all']" :Placement="'Bottom,Left'">
                        <div class="imagePanel">
                            <div class="imagePanelGroup">
                                <input type="text" placeholder="请输入图片URL" v-model="imagePanelUrl">
                                <button class="btn-primary" @click="insertImage(imagePanelUrl)">
                                    插入
                                </button>
                            </div>
                            <div class="imagePanelGroup">
                                <button class="btn-primary">
                                    从文件中转站选择
                                </button>
                                <button class="btn-primary" @click="openImage">
                                    上传本地图片
                                </button>
                            </div>
                        </div>
                    </SafePopover>
                </button>
                <!-- 表情 -->
                <button class="toolbarButton" ref="emojiPanelButton" @click="showEmojiPanel">
                    <span class="iconfont icon-likesprogrambiaoqing"></span>
                    <span class="text">表情</span>
                    <!-- 发件人设置面板 -->
                    <SafePopover ref="emojiPanel" :NoBoxShadow="['all']">
                        <div class="emojiPanel">
                            <div class="groupTitle">
                                <div v-for="(emojiGroup, index) in emojiData.emojiGroups"
                                    :class="{ 'title': true, 'selectedTitle': index === emojiData.selectIndex }"
                                    @click="selectedEmojiGroup(index)">
                                    {{ emojiGroup.GroupTitle }}
                                </div>
                            </div>
                            <div class="emojiList">
                                <span @click.stop="insertEmoji(emoji.Emoji)" v-for="emoji in emojiData.emojiGroups[emojiData.selectIndex]?.EmojiList" class="emoji" :title="emoji.EmojiTitle">
                                    {{ emoji.Emoji }}
                                </span>
                            </div>
                        </div>
                    </SafePopover>
                </button>
            </div>
            <!-- 签名 -->
            <button ref="signatureSettingButton" class="toolbarButton signatureButton" @click="showSignatureSettingPanel">
                <span class="iconfont icon-likesprogramedit"></span>
                <span class="text">签名</span>
                <span class="iconfont select icon-likesprogramxiangyouxiayiye"></span>
                <!-- 发件人设置面板 -->
                <SafePopover ref="signatureSettingPanel" :NoBoxShadow="['all']" :Placement="'Bottom,Right'">
                    <div class="signaturePanel">
                        <div class="signatureInfo">
                            <div class="button btn-text signature" @click="selectedSignature('none')">
                                <span class="title">不使用签名</span>
                                <span class="iconfont icon-likesprogramzhengque" v-if="signatureId === 'none'"></span>
                            </div>
                        </div>
                        <div class="signatureInfo" v-for="signature in signatures">
                            <div class="button btn-text signature" @click="selectedSignature(signature.signatureId)">
                                <span class="title">{{ signature.signatureName }}</span>
                                <span class="iconfont icon-likesprogramzhengque" v-if="signature.signatureId == signatureId"></span>
                            </div>
                        </div>
                    </div>
                </SafePopover>
            </button>
        </div>
        <div class="inputContent" @click="focusEditor">
            <!-- 富文本框 -->
            <ckeditor :editor="editor" class="mailEditor" v-model="content" :config="editorConfig" @ready="onEditorReady" />
            <div v-if="signatureContent && signatureContent.length > 0" class="signature" @click.stop v-html="SignatureService.renderSignature(
                signatureContent, userStore.account, userStore.nickname, userStore.mailBoxView
            )"></div>
        </div>

        <!-- 消息提示 -->
        <ShowMessage ref="showMessage"/>
    </div>
</template>

<script lang="ts">
import { computed, defineComponent, onBeforeUnmount, onMounted, ref, shallowRef } from 'vue'
import CKEditor from '@ckeditor/ckeditor5-vue';
import ClassicEditor from '@ckeditor/ckeditor5-build-classic';
import RecipientInput  from '@components/RecipientInput.vue';
import { SignatureService } from '@service/SignatureService'
import { UserStore } from '@store/UserStore';
import SafePopover from '@components/SafePopover.vue';
import EmojiListData from '@components/staticData/EmojiList.json';
import ShowMessage from '@components/messageDialog/ShowMessage.vue';
import { Signature } from '@api/users/Signature';
import { ImageProcessing } from '@utils/ImageProcessing';
import { MailService, type MailDraft, type MailExtraData } from '@service/MailService';

export default defineComponent({
    name: 'WriteMail',
});
</script>
<script setup lang="ts">
const props = withDefaults(defineProps<{
    modelValue: MailDraft;
}>(), {
    modelValue: MailService.createDefaultMailDraft
});
const emit = defineEmits<{
    (e: 'update:modelValue', value: MailDraft): void;
}>();
const updateDraft = <K extends keyof MailDraft>(key: K, value: MailDraft[K]) => {
    emit('update:modelValue', {...props.modelValue, [key]: value});
};
const uniqueRecipients = (list: string[]) => {
    return Array.from(
        new Set(list.map(item => item.trim()).filter(item => item.length > 0))
    );
};
const recipients = computed<string[]>({
    get: () => {
        return uniqueRecipients([
            ...props.modelValue.to,
            ...props.modelValue.cc,
            ...props.modelValue.bcc
        ]);
    },
    set: value => {
        const nextRecipients = uniqueRecipients(value);
        const nextSet = new Set(nextRecipients);
        const oldTo = props.modelValue.to;
        const oldCc = props.modelValue.cc;
        const oldBcc = props.modelValue.bcc;
        const oldAllSet = new Set([
            ...oldTo,
            ...oldCc,
            ...oldBcc
        ]);
        // 原来属于 TO 的，仍然保留在 TO；如果被删除，则同步删除
        const nextTo = oldTo.filter(email => nextSet.has(email));
        // 原来属于 CC 的，仍然保留在 CC；如果被删除，则同步删除
        const nextCc = oldCc.filter(email => nextSet.has(email));
        // 原来属于 BCC 的，仍然保留在 BCC；如果被删除，则同步删除
        const nextBcc = oldBcc.filter(email => nextSet.has(email));
        // 新增的收件人默认加入 TO
        for (const email of nextRecipients) {
            if (!oldAllSet.has(email)) nextTo.push(email);
        }
        emit('update:modelValue', {
            ...props.modelValue,
            to: uniqueRecipients(nextTo),
            cc: uniqueRecipients(nextCc),
            bcc: uniqueRecipients(nextBcc)
        });
    }
});
const recipientTos = computed({
    get: () => props.modelValue.to,
    set: value => updateDraft('to', value)
});
const recipientCcs = computed({
    get: () => props.modelValue.cc,
    set: value => updateDraft('cc', value)
});
const recipientBccs = computed({
    get: () => props.modelValue.bcc,
    set: value => updateDraft('bcc', value)
});
const sendSeparately = computed({
    get: () => props.modelValue.sendSeparately,
    set: value => updateDraft('sendSeparately', value)
});
const subject = computed({
    get: () => props.modelValue.subject,
    set: value => updateDraft('subject', value)
});
const content = computed({
    get: () => props.modelValue.content,
    set: value => updateDraft('content', value)
});
const signatureId = computed({
    get: () => props.modelValue.signatureId,
    set: value => updateDraft('signatureId', value)
});
const signatureContent = computed({
    get: () => props.modelValue.signatureContent,
    set: value => updateDraft('signatureContent', value)
});
const recipientAreaRef = ref<HTMLElement | null>(null);
const showCc = ref(false);
const showBcc = ref(false);
const shouldShowCc = computed(() => {
    return showCc.value || recipientCcs.value.length > 0;
});
const shouldShowBcc = computed(() => {
    return showBcc.value || recipientBccs.value.length > 0;
});
const closeEmptyRecipientFields = () => {
    if (recipientCcs.value.length <= 0) {
        showCc.value = false;
    }

    if (recipientBccs.value.length <= 0) {
        showBcc.value = false;
    }
};
const handleDocumentMouseDown = (event: MouseEvent) => {
    const target = event.target as Node;
    // 点击仍在收件人区域内部，不关闭
    if (recipientAreaRef.value?.contains(target)) return;
    // 点击外部时，只隐藏没有内容的抄送/密送
    closeEmptyRecipientFields();
};

// 对话框实例
const showMessage = ref<InstanceType<typeof ShowMessage>>();

// 正文编辑框
const ckeditor = CKEditor.component;
const editor = ClassicEditor;
const editorConfig = { toolbar: [], placeholder:"输入正文" };
const editorInstance = shallowRef<any | null>(null);
const canUndo = ref(false); // 撤销
const canRedo = ref(false); // 重做
const onEditorReady = (instance: any) => {
    editorInstance.value = instance;
    const undoCommand = instance.commands.get('undo');
    const redoCommand = instance.commands.get('redo');
    if (undoCommand) {
        canUndo.value = undoCommand.isEnabled;
        undoCommand.on('change:isEnabled', () => {
            canUndo.value = undoCommand.isEnabled;
        });
    }
    if (redoCommand) {
        canRedo.value = redoCommand.isEnabled;
        redoCommand.on('change:isEnabled', () => {
            canRedo.value = redoCommand.isEnabled;
        });
    }
}
const focusEditor = () => {
    // 让 CKEditor 获取焦点
    editorInstance.value?.editing.view.focus();
}
const undoEditor = () => {
    const editor = editorInstance.value
    if (!editor) return
    const undoCommand = editor.commands.get('undo')
    if (!undoCommand?.isEnabled) return
    editor.execute('undo')
    editor.editing.view.focus()
}
const redoEditor = () => {
    const editor = editorInstance.value
    if (!editor) return
    const redoCommand = editor.commands.get('redo')
    if (!redoCommand?.isEnabled) return
    editor.execute('redo')
    editor.editing.view.focus()
}

// 用户
const userStore = UserStore();

// 组件挂载
onMounted(async () => {
    document.addEventListener('mousedown', handleDocumentMouseDown);
    // 获取签名列表
    await getSignatures();
    // 获取签名内容
    if (signatureId.value && signatureId.value !== "none") getSignatureContent(signatureId.value);
});
// 组件卸载
onBeforeUnmount(() => {
    document.removeEventListener('mousedown', handleDocumentMouseDown);
});

// 图片路径
const imagePanelUrl = ref<string>('');
// 插入图片按钮
const imageButton = ref<HTMLElement | null>(null);
// 图片插入面板
const imagePanel = ref<InstanceType<typeof SafePopover> | null>(null);
const showImagePanel = () => {
    imagePanel.value?.show(imageButton.value as HTMLElement);
}
// 从本地打开图片
const openImage = async () => {
    try {
        const imageProcessingTemp = new ImageProcessing();
        await imageProcessingTemp.SetImage();
        const imageBase64 = String(await imageProcessingTemp.GetBase64() ?? '').trim();
        if (!imageBase64) {
            showMessage.value?.ShowModal("图片插入失败", ["未读取到有效的图片数据"], "failure");
            return;
        }

        // 后续发送时会替换为 CID，这里应该按 MIME inline 图片估算
        const mailExtraData: MailExtraData = { type: 'image', url: imageBase64, asMimeInline: true };
        const estimateMailSize: number = MailService.estimateMailSize(props.modelValue, mailExtraData);
        const maxEstimateMailSize: number = MailService.getMaxMailSize();

        // 超出最大限制：不允许插入
        if (estimateMailSize > maxEstimateMailSize) {
            showMessage.value?.ShowModal("图片插入失败", ["邮件大小已经超出限制，请将嵌入式的图片", "或文件改用文件中转站链接发送"], "failure");
            return;
        }

        // 先插入图片，避免弹窗导致 CKEditor 光标丢失
        insertImage(imageBase64);

        // 达到 80%：只警告，不阻止插入
        if (estimateMailSize >= maxEstimateMailSize * 0.8) {
            setTimeout(() => {
                showMessage.value?.ShowModal("邮件大小警告", ["邮件大小即将超出限制，建议将嵌入式的", "图片或文件改用文件中转站链接发送"], "warning");
            }, 0);
        }
    } catch (error) {
        showMessage.value?.ShowModal("图片插入失败", ["图片读取失败，请重新选择图片"], "failure");
    }
};
// 校验图片地址：只允许 http、https、base64 图片
const normalizeImageUrl = (url: string): string | null => {
    const value = url.trim();
    if (!value) return null;
    // http / https 图片地址
    if (/^https?:\/\/.+/i.test(value)) return value;
    // base64 图片
    if (/^data:image\/(png|jpeg|jpg|gif|webp|bmp|svg\+xml);base64,/i.test(value)) return value;
    return null;
};
// 插入图片
const insertImage = (url: string) => {
    imagePanel.value?.hide(); // 关闭面板
    imagePanelUrl.value = ''; // 清空输入框
    const editor = editorInstance.value;
    if (!editor) return;
    const imageUrl = normalizeImageUrl(url);
    if (!imageUrl) {
        showMessage.value?.ShowModal("图片插入错误", ["非法的图片地址"], "failure");
        return;
    }
    editor.editing.view.focus();
    // 优先使用 CKEditor 5 的 insertImage 命令
    if (editor.commands.get("insertImage")) {
        editor.execute("insertImage", { source: imageUrl });
        editor.editing.view.focus();
        return;
    }
    // 降级方案：手动插入 imageBlock
    editor.model.change((writer: any) => {
        const imageElement = writer.createElement("imageBlock", { src: imageUrl });
        editor.model.insertContent(imageElement, editor.model.document.selection);
        writer.setSelection(imageElement, "after");
    });
    editor.editing.view.focus();
};

// EmoJi 数据
const emojiData = ref<{
    selectIndex: number;
    emojiGroups: Array<{
        GroupTitle: string;
        EmojiList: Array<{
            EmojiTitle: string;
            Emoji: string;
        }>;
    }>;
}>({
    selectIndex: 0,
    emojiGroups: []
});
// 表情面板按钮
const emojiPanelButton = ref<HTMLElement | null>(null);
// 表情面板
const emojiPanel = ref<InstanceType<typeof SafePopover> | null>(null);
// 显示表情面板
const showEmojiPanel = () => {
    // 加载 Emoji 数据
    if (emojiData.value.emojiGroups.length <= 0) emojiData.value.emojiGroups = EmojiListData;
    emojiPanel.value?.show(emojiPanelButton.value as HTMLElement);
}
// 选择分组
const selectedEmojiGroup = (index: number) => {
    if (index < 0 || index >= emojiData.value.emojiGroups.length) return;
    emojiData.value.selectIndex = index;
}
// 插入表情
const insertEmoji = (emoji: string) => {
    const editor = editorInstance.value;
    if (!editor) return;
    editor.model.change((writer: any) => {
        const selection = editor.model.document.selection;
        // 创建表情文本节点
        const emojiText = writer.createText(emoji);
        // 插入到当前选区/光标位置
        const insertedRange = editor.model.insertContent(emojiText, selection);
        // 插入后把光标移动到表情后面
        if (insertedRange) writer.setSelection(insertedRange.end);
    });
    // 关闭 表情 面板
    emojiPanel.value?.hide();
    // 插入完成后重新聚焦编辑器
    editor.editing.view.focus();
}

// 签名列表
const signatures = ref<Signature[]>();
// 获取签名列表
const getSignatures = async () => {
    try {
        const response = await SignatureService.getSignatures();
        signatures.value = response.data;
        if (!signatureId.value) {
            const defaultSignature = signatures.value.find(signature => signature.default);
            if (defaultSignature) signatureId.value = defaultSignature.signatureId;
            else signatureId.value = "none";
        }
    } catch (error) {
        setTimeout(() => {
            showMessage.value?.ShowModal("获取签名列表失败", ["获取签名列表失败，是否重试"], "failure", (selectedButtonIndex) => {
                if(selectedButtonIndex === 1) getSignatures();
            });
        }, 300);
    }
};
// 获取签名内容
const getSignatureContent = async (_signatureId: string) => {
    if (_signatureId == "none") {
        signatureContent.value = "";
        return;
    }
    try {
        const renderedContent = await SignatureService.getSignatureContent(_signatureId);
        signatureContent.value = renderedContent.data;
    } catch (error) {
        setTimeout(() => {
            showMessage.value?.ShowModal("获取签名内容失败", ["获取签名内容失败，是否重试"], "failure", (selectedButtonIndex) => {
                if(selectedButtonIndex === 1) getSignatureContent(_signatureId);
            });
        }, 300);
    }
}
// 签名设置按钮
const signatureSettingButton = ref<HTMLElement | null>(null);
// 签名设置面板
const signatureSettingPanel = ref<InstanceType<typeof SafePopover> | null>(null);
// 显示签名设置面板
const showSignatureSettingPanel = () => {
    signatureSettingPanel.value?.show(signatureSettingButton.value as HTMLElement);
}
// 选择 签名
const selectedSignature = (_signatureId: string) => {
    signatureId.value = _signatureId;
    getSignatureContent(_signatureId);
}

</script>

<style scoped>
.writeMail {
    width: 100%;
    height: 100%;
    min-height: 0;
    display: flex;
    flex-direction: column;
    gap: 5px;
}

/* 收件人与主题 */
.writeMail .inputGroup {
    margin: 0px 10px;
    display: flex;
    align-items: center;
    gap: 10px;
    flex-shrink: 0;
    font-size: 14px;
    padding: 3px 0px;
    border-bottom: 1px solid var(--border-primary);
}.writeMail .inputGroup:hover {
    border-bottom: 1px solid var(--border-hover);
}.writeMail .inputGroup:focus-within {
    border-bottom: 1px solid var(--input-primary-border);
}.writeMail .inputGroup:focus-within .groupTitle {
    color: var(--text-link);
}
.writeMail .inputGroup .groupTitle {
    width: 60px;
    text-align: justify;
    text-align-last: justify;
    flex-shrink: 0;
    margin-right: 10px;
}
.writeMail .inputGroup input {
    flex: 1;
    min-width: 0;
    border: none;
}
.writeMail .inputGroup .button {
    flex-shrink: 0;
}

.writeMail .recipientArea {
    width: 100%;
    flex-shrink: 0;
    display: flex;
    flex-direction: column;
    gap: 5px;
}

/* 富文本框工具栏 */
.writeMail .editorToolbar {
    width: 100%;
    padding: 0px 10px;
    margin-top: 10px;
    display: flex;
    justify-content: space-between;
    align-items: center;
    flex-shrink: 0;
    font-size: 14px;
    position: relative;
    z-index: 999;
}
/* 左侧按钮组 */
.writeMail .editorToolbar .editorToolbarLeft {
    display: flex;
    align-items: center;
    gap: 6px;
}
.writeMail .editorToolbar .toolbarButton {
    margin: 0px;
    gap: 5px;
    padding: 5px;
    --button-bg-disabled: var(--bg-transparent);
    transition: all .3s;
    position: relative;
}
.writeMail .editorToolbar .toolbarButton:hover {
    background-color: var(--bg-hover-primary);
}
.writeMail .editorToolbar .toolbarButton:active {
    transition: all .1s;
    background-color: var(--bg-active-primary);
}
.writeMail .editorToolbar .toolbarButton .iconfont {
    font-size: 20px;
}.writeMail .editorToolbar .toolbarButton .iconfont.select {
    font-size: 12px;
    transform: rotate(90deg);
}
.writeMail .editorToolbar .editorToolbarLeft .toolbarButton .text {
    padding-right: 2px;
}
/* 分割线 */
.writeMail .editorToolbar .editorToolbarLeft .line {
    width: 1px;
    height: 20px;
    margin: 0px 5px;
    background-color: var(--border-disabled);
}

/* 图片插入面板 */
.writeMail .editorToolbar .imagePanel {
    width: 270px;
    gap: 10px;
    padding: 10px;
    display: flex;
    cursor: default;
    background-color: var(--bg-primary);
    flex-direction: column;
}
.writeMail .editorToolbar .imagePanel .imagePanelGroup {
    height: 30px;
    flex-shrink: 0;
    gap: 10px;
    display: flex;
    justify-content: space-between;
}
.writeMail .editorToolbar .imagePanel .imagePanelGroup input {
    flex: 1;
}

/* 表情面板 */
.writeMail .editorToolbar .emojiPanel {
    width: 300px;
    font-size: 14px;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    cursor: default;
    background-color: var(--bg-primary);
}
/* 标签分组标题 */
.writeMail .editorToolbar .emojiPanel .groupTitle {
    width: 100%;
    height: 40px;
    padding: 10px 0px;
    position: relative;
    flex-shrink: 0;
    display: flex;
    align-items: center;
    justify-content: space-between;
    overflow: hidden;
}
.writeMail .editorToolbar .emojiPanel .groupTitle .title {
    width: 100%;
    border-right: 1px solid var(--border-disabled);
    display: flex;
    cursor: pointer;
    align-items: center;
    justify-content: center;
    transition: all .3s;
}.writeMail .editorToolbar .emojiPanel .groupTitle .title:last-child {
    border-right: none;
}.writeMail .editorToolbar .emojiPanel .groupTitle .title:hover {
    color: var(--text-link-hover);
}.writeMail .editorToolbar .emojiPanel .groupTitle .title:active,
.writeMail .editorToolbar .emojiPanel .groupTitle .title.selectedTitle {
    transition: all .1s;
    color: var(--text-link-active);
}
.writeMail .editorToolbar .emojiPanel .emojiList {
    flex: 1;
    width: 100%;
    max-height: 200px;
    gap: 8px;
    padding: 0px 10px;
    padding-bottom: 10px;
    display: flex;
    flex-wrap: wrap;
    overflow-y: auto;
}
.writeMail .editorToolbar .emojiPanel .emojiList .emoji {
    width: 23px;
    height: 23px;
    font-size: 18px;
    border-radius: 5px;
    float: left;
    display: flex;
    align-items: center;
    justify-content: center;
    transition: all .3s;
    position: relative;
}
.writeMail .editorToolbar .emojiPanel .emojiList .emoji:hover {
    font-size: 22px;
    background-color: rgba(255, 255, 255, 1);
}
.writeMail .editorToolbar .emojiPanel .emojiList .emoji:active {
    transition: all .1s;
    font-size: 20px;
}

/* 签名选择面板 */
.writeMail .editorToolbar .signaturePanel {
    width: 250px;
    padding: 10px;
    font-size: 14px;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    background-color: var(--bg-primary);
}
/* 邮箱信息 */
.writeMail .editorToolbar .signaturePanel .signatureInfo {
    width: 100%;
    border-bottom: 1px solid var(--bg-active-primary);
}.writeMail .editorToolbar .signaturePanel .signatureInfo:last-child {
    border-bottom: none;
}
.writeMail .editorToolbar .signaturePanel .signatureInfo .signature {
    width: 100%;
    height: 100%;
    padding: 12px 5px;
    gap: 10px;
    border-radius: 5px;
    justify-content: flex-start;
}
.writeMail .editorToolbar .signaturePanel .signatureInfo .signature .title {
    flex: 1;
    /* 单行文本省略 */
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.writeMail .editorToolbar .signaturePanel .signatureInfo .signature .iconfont {
    flex-shrink: 0;
    width: 18px;
    height: 18px;
    font-size: 18px;
}

/* 输入框内容 */
.writeMail .inputContent {
    flex: 1;
    width: 100%;
    min-width: 0;
    min-height: 0;
    display: flex;
    flex-direction: column;
    overflow: auto;
    cursor: text;
    position: relative;
    z-index: 998;
}
.writeMail .inputContent .mailEditor {
    width: 100%;
    min-width: 0;
    flex-shrink: 0;
}
.writeMail .inputContent .signature {
    align-self: flex-start;
    width: max-content;
    max-width: none;
    flex-shrink: 0;
    padding: 0px 10px;
    box-sizing: border-box;
}

/* 富文本框 */
.writeMail > div:first-of-type {
    flex-shrink: 0;
}
.writeMail :deep(.ck-editor) {
    border: none !important;
    width: 100%;
    min-width: 0;
    display: flex;
    flex-direction: column;
}
/* 顶部区域 */
.writeMail :deep(.ck-editor__top),
.writeMail :deep(.ck-sticky-panel),
.writeMail :deep(.ck-sticky-panel__content),
.writeMail :deep(.ck-toolbar),
.writeMail :deep(.ck-toolbar_grouping) {
    border: none !important;
    box-shadow: none !important;
}
/* 有些版本会给 sticky panel content 加 border-bottom */
.writeMail :deep(.ck-sticky-panel__content) {
    border: 0 !important;
    border-bottom: none !important;
}
/* 工具栏内部 */
.writeMail :deep(.ck-toolbar__items) {
    border: none !important;
    box-shadow: none !important;
}
/* 主编辑区域 */
.writeMail :deep(.ck-editor__main) {
    border: none !important;
    width: 100%;
    min-width: 0;
    display: block;
}
.writeMail :deep(.ck-editor__editable) {
    border: none !important;
    box-shadow: none !important;
    outline: none !important;
    width: 100%;
    min-width: 0;
    min-height: 50px;
    overflow: visible;
    padding: 0px 10px;
    font-size: 14px;
    line-height: 1.6;
    color: var(--text-primary);
    background: var(--bg-primary);

    box-sizing: border-box;
}
</style>
