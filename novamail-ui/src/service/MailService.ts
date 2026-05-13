import { ValidateHtml } from '@utils/Utils';
export interface Attachment {
    fileName: string;
    fileType: string;
    fileSize: number;
    fileData: string;
    sendType: 'MIME_ATTACHMENT' | 'TRANSFER_LINK';
}

export interface MailDraft {
    recipients: string[];
    to: string[];
    cc: string[];
    bcc: string[];
    sendSeparately: boolean;
    subject: string;
    content: string;
    attachment: Attachment[];
    signatureId: string;
    signatureContent: string;
    sendNickname: string;
    sendMailBoxId: string;
    sendMailBoxView: string;
}

// 计算数据大小
const getUtf8Size = (text?: string): number => {
    if (!text) return 0;
    return new Blob([text]).size;
};
// 判断是否是 Http 地址
const isHttpUrl = (value: string): boolean => {
    return /^https?:\/\/.+/i.test(value.trim());
};
// 判断是否是 Base64 地址
const isBase64DataUrl = (value: string): boolean => {
    return /^data:[^;]+;base64,/i.test(value.trim());
};
// 计算 Base64 大小
const getBase64RawSize = (base64OrDataUrl: string): number => {
    if (!base64OrDataUrl) return 0;
    const base64 = base64OrDataUrl.includes(',') ? base64OrDataUrl.split(',')[1] : base64OrDataUrl;
    if (!base64) return 0;
    const pureBase64 = base64.replace(/\s/g, '');
    const padding = pureBase64.endsWith('==') ? 2 : pureBase64.endsWith('=') ? 1 : 0;
    return Math.max(0, Math.floor(pureBase64.length * 3 / 4) - padding);
};
// 计算 MimeBase64 大小
const estimateMimeBase64Size = (rawSize: number): number => {
    const base64Size = Math.ceil(rawSize / 3) * 4;
    // MIME Base64 通常每 76 个字符换行一次，CRLF 为 2 字节
    const lineBreakSize = Math.ceil(base64Size / 76) * 2;
    return base64Size + lineBreakSize;
};
// 转换 HTML 字符
const escapeHtml = (value: string): string => {
    return value.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;').replace(/'/g, '&#39;');
};
// 计算图片大小
const estimateImageSize = (url: string, asMimeInline: boolean = false): number => {
    const value = url.trim();
    if (!value) return 0;
    // http / https 图片，正文里只是一个 img 标签
    if (isHttpUrl(value)) {
        const imageHtml = `<img src="${escapeHtml(value)}" />`;
        return getUtf8Size(imageHtml);
    }
    // base64 图片
    if (isBase64DataUrl(value)) {
        // 最终只是把 data:image/base64 放进 HTML
        if (!asMimeInline) {
            const imageHtml = `<img src="${value}" />`;
            return getUtf8Size(imageHtml);
        }
        // 最终会把图片转成 MIME 内嵌附件 CID
        const rawSize = getBase64RawSize(value);
        const mimeHeaderSize = 512;
        const cidImageTagSize = getUtf8Size(`<img src="cid:image" />`);
        return estimateMimeBase64Size(rawSize) + mimeHeaderSize + cidImageTagSize;
    }

    return 0;
};

// 预测附件大小
const estimateAttachmentSize = (attachment: Attachment): number => {
    const fileData = attachment.fileData?.trim() ?? '';
    if (!fileData) return 0;
    // 作为真正 MIME 附件，写入 multipart/mixed，体积按 base64 后大小计算
    if (attachment.sendType === 'MIME_ATTACHMENT') {
        const rawSize = getBase64RawSize(fileData);
        return estimateMimeBase64Size(rawSize) + 512;
    }
    // 文件中转站链接：只作为正文链接，不作为 MIME 附件，体积只计算 URL 文本大小
    if (attachment.sendType === 'TRANSFER_LINK') return getUtf8Size(MailService.createAttachmentLink(attachment));
    // 普通 base64
    const rawSize = getBase64RawSize(fileData);
    return estimateMimeBase64Size(rawSize) + 512;
};

// 新增数据类型
export type MailExtraData = { type: 'html'; html: string } | { type: 'image'; url: string; asMimeInline?: boolean } | { type: 'attachment'; attachment: Attachment };
// 预测 新增 内容大小
const estimateExtraDataSize = (newData?: MailExtraData | MailExtraData[]): number => {
    if (!newData) return 0;
    const list = Array.isArray(newData) ? newData : [newData];
    return list.reduce((total, item) => {
        if (item.type === 'html') return total + getUtf8Size(MailService.renderMail(item.html));
        if (item.type === 'image') return total + estimateImageSize(item.url, item.asMimeInline);
        if (item.type === 'attachment') return total + estimateAttachmentSize(item.attachment);
        return total;
    }, 0);
};

export const MailService = {
    // 获取邮件最大限制
    getMaxMailSize: ():number => {
        return 10485760;
    },
    getDraft: (draftId: string): MailDraft => {
        return {
            recipients: [],
            to: [],
            cc: [],
            bcc: [],
            sendSeparately: false,
            subject: '',
            content: '',
            attachment: [],
            signatureId: '',
            signatureContent: '',
            sendNickname: '',
            sendMailBoxId: '',
            sendMailBoxView: ''
        }
    },
    // 初始化草稿
    createDefaultMailDraft: (): MailDraft => ({
        recipients: [],
        to: [],
        cc: [],
        bcc: [],
        sendSeparately: false,
        subject: '',
        content: '',
        attachment: [],
        signatureId: '',
        signatureContent: '',
        sendNickname: '',
        sendMailBoxId: '',
        sendMailBoxView: ''
    }),
    createAttachmentLink: (attachment: Attachment): string => {
        const url = attachment.fileData.trim();
        return `<p>附件：<a href="${escapeHtml(url)}">${escapeHtml(attachment.fileName || url)}</a></p>`;
    },
    // 邮件大小估计
    estimateMailSize: (draft: MailDraft, newData?: MailExtraData | MailExtraData[]): number => {
        const signatureContent = draft.signatureContent ?? '';
        const mailContent = draft.content ?? '';

        const allRecipients = draft.sendSeparately ? draft.recipients : [
            ...draft.to,
            ...draft.cc,
            ...draft.bcc
        ];

        const recipientHeader = allRecipients.join(',');

        const htmlContent = [mailContent, signatureContent, newData].join('');

        const cleanHtml = MailService.renderMail(htmlContent);

        // 邮件头、MIME 边界、Content-Type 等基础开销预估
        const baseMimeHeaderSize = 2048;

        const headerSize = getUtf8Size(draft.subject) + getUtf8Size(recipientHeader);

        const bodySize = getUtf8Size(cleanHtml);

        const attachmentSize = draft.attachment.reduce((total, attachment) => {
            return total + estimateAttachmentSize(attachment);
        }, 0);
        
        const extraSize = estimateExtraDataSize(newData);

        return baseMimeHeaderSize + headerSize + bodySize + attachmentSize + extraSize;
    },
    // 格式化邮件大小
    formatMailSize: (bytes: number): string => {
        if (bytes < 1024) return `${bytes} B`;
        if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(2)} KB`;
        return `${(bytes / 1024 / 1024).toFixed(2)} MB`;
    },
    // 渲染邮件内容
    renderMail: (html: string): string => {
        const { valid, clean } = ValidateHtml(html);
        if (!valid) console.warn('邮件内容包含不安全的元素或属性，已自动清理');
        return clean;
    }
}
