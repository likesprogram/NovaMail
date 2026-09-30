import { Email, type MailAttachment, type MailDetailResponse } from '@api/email/Email';
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
    forwardContent: string;
    replySourceMappingId: number | null;
    attachment: Attachment[];
    signatureId: string;
    signatureContent: string;
    sendNickname: string;
    sendMailBoxId: string;
    sendMailBoxView: string;
}

export interface ForwardDraftResult {
    draft: MailDraft;
    skippedAttachments: string[];
}

export interface ReplyDraftOptions {
    replyAll?: boolean;
    currentUserAddresses?: string[];
}

const temporaryDrafts = new Map<string, MailDraft>();
const FORWARD_CONTENT_START = '<!--NOVAMAIL_FORWARD_CONTENT_START-->';
const FORWARD_CONTENT_END = '<!--NOVAMAIL_FORWARD_CONTENT_END-->';
const REPLY_SOURCE_MAPPING_ID_PATTERN = /<!--NOVAMAIL_REPLY_SOURCE_MAPPING_ID:(\d+)-->/;

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
const formatAttachmentSize = (bytes: number): string => {
    const safeBytes = Number(bytes || 0);
    if (safeBytes < 1024) return `${safeBytes} B`;
    if (safeBytes < 1024 * 1024) return `${(safeBytes / 1024).toFixed(2)} KB`;
    return `${(safeBytes / 1024 / 1024).toFixed(2)} MB`;
};
const formatAddress = (address?: { name?: string; address?: string }): string => {
    if (!address?.address) return '';
    const name = address.name?.trim();
    return name && name !== address.address ? `${name} <${address.address}>` : address.address;
};
const normalizeMailAddress = (address?: string): string => (address ?? '').trim();
const normalizeMailAddressKey = (address?: string): string => normalizeMailAddress(address).toLowerCase();
const toAddressList = (addresses?: Array<{ address?: string }>): string[] => {
    return (addresses ?? []).map(address => normalizeMailAddress(address.address)).filter(Boolean);
};
const uniqueAddressList = (addresses: string[], excluded: Iterable<string> = []): string[] => {
    const seen = new Set(Array.from(excluded).map(normalizeMailAddressKey).filter(Boolean));
    const result: string[] = [];
    for (const address of addresses) {
        const value = normalizeMailAddress(address);
        const key = normalizeMailAddressKey(value);
        if (!key || seen.has(key)) continue;
        seen.add(key);
        result.push(value);
    }
    return result;
};
const formatAddressList = (addresses?: Array<{ name?: string; address?: string }>): string => {
    const result = (addresses ?? []).map(formatAddress).filter(Boolean);
    return result.length > 0 ? result.join(', ') : '未填写';
};
const formatForwardTime = (value?: string): string => {
    if (!value) return '';
    const date = new Date(value);
    if (Number.isNaN(date.getTime())) return value;
    return date.toLocaleString('zh-CN', { hour12: false });
};
const normalizeForwardSubject = (subject: string, count: number): string => {
    if (count > 1) return `转发: ${count} 封邮件`;
    const value = subject.trim() || '无主题';
    return /^(转发|fwd?|fw)\s*[:：]/i.test(value) ? value : `转发: ${value}`;
};
const normalizeReplySubject = (subject: string): string => {
    const value = subject.trim() || '无主题';
    return /^(回复|re)\s*[:：]/i.test(value) ? value : `回复: ${value}`;
};
const renderTextBody = (text: string): string => {
    return `<div>${escapeHtml(text).replace(/\r?\n/g, '<br/>')}</div>`;
};
const getForwardBody = (mail: MailDetailResponse): string => {
    if (mail.htmlBody?.trim()) return MailService.renderMail(mail.htmlBody);
    if (mail.textBody?.trim()) return renderTextBody(mail.textBody);
    if (mail.preview?.trim()) return renderTextBody(mail.preview);
    return '<div>原邮件内容为空</div>';
};
const isVisibleForwardAttachment = (attachment: MailAttachment): boolean => !attachment.isInline;
const buildForwardAttachmentSummary = (mail: MailDetailResponse): string => {
    const attachments = (mail.attachments ?? []).filter(isVisibleForwardAttachment);
    if (attachments.length === 0) return '';
    const names = attachments.map(attachment => escapeHtml(attachment.fileName)).join(', ');
    return `<div style="margin-top:4px;"><strong>附件:</strong> ${names}</div>`;
};
const buildForwardBlock = (mail: MailDetailResponse): string => {
    const subject = escapeHtml(mail.subject?.trim() || '无主题');
    const from = escapeHtml(formatAddress(mail.from));
    const to = escapeHtml(formatAddressList(mail.tos));
    const cc = mail.ccs?.length ? `<div style="margin-top:4px;">抄送人：${escapeHtml(formatAddressList(mail.ccs))}</div>` : '';
    const time = escapeHtml(formatForwardTime(mail.createTime));
    const attachmentSummary = buildForwardAttachmentSummary(mail);
    const body = getForwardBody(mail);

    return `<div class="novamailForwardBlock" style="margin-top:22px;padding-top:14px;border-top:1px solid #e6ebf2;color:#17233d;font-family:Arial,'Microsoft YaHei',sans-serif;"><div style="margin:0 0 8px;color:#8a96a8;font-size:13px;line-height:20px;">原始邮件</div><div style="padding:10px 12px;border-radius:6px;background:#f3f5f8;color:#1f2937;font-size:13px;line-height:21px;"><div>发件人：${from}</div>${time ? `<div>发送时间：${time}</div>` : ''}<div>收件人：${to}</div>${cc}<div>主题：${subject}</div>${attachmentSummary}</div><div style="margin-top:22px;font-size:14px;line-height:1.7;color:#111827;">${body}</div></div>`;
};
const createReplyRecipients = (mail: MailDetailResponse, options?: ReplyDraftOptions): Pick<MailDraft, 'to' | 'cc'> => {
    const ownAddressKeys = new Set((options?.currentUserAddresses ?? []).map(normalizeMailAddressKey).filter(Boolean));
    const fromAddress = normalizeMailAddress(mail.from?.address);
    const fromAddressKey = normalizeMailAddressKey(fromAddress);
    const fromIsOwnAddress = !!fromAddressKey && ownAddressKeys.has(fromAddressKey);

    if (!options?.replyAll) {
        if (fromAddress && !fromIsOwnAddress) return { to: [fromAddress], cc: [] };
        return {
            to: uniqueAddressList(toAddressList(mail.tos), ownAddressKeys),
            cc: [],
        };
    }

    const to = fromIsOwnAddress
        ? uniqueAddressList(toAddressList(mail.tos), ownAddressKeys)
        : uniqueAddressList([fromAddress], ownAddressKeys);

    const ccExcluded = new Set([
        ...Array.from(ownAddressKeys),
        ...to.map(normalizeMailAddressKey),
    ]);
    const cc = uniqueAddressList([...toAddressList(mail.tos), ...toAddressList(mail.ccs)], ccExcluded);

    return { to, cc };
};
const blobToDataUrl = async (blob: Blob): Promise<string> => {
    return await new Promise((resolve, reject) => {
        const reader = new FileReader();
        reader.onload = () => resolve(String(reader.result || ''));
        reader.onerror = () => reject(reader.error);
        reader.readAsDataURL(blob);
    });
};
const toForwardAttachment = async (attachment: MailAttachment): Promise<Attachment> => {
    const blob = await Email.attachmentBlob(attachment);
    return {
        fileName: attachment.fileName,
        fileType: attachment.fileType || blob.type || 'application/octet-stream',
        fileSize: attachment.fileSize || blob.size,
        fileData: await blobToDataUrl(blob),
        sendType: 'MIME_ATTACHMENT',
    };
};
const addForwardAttachments = async (draft: MailDraft, mails: MailDetailResponse[]): Promise<string[]> => {
    const skippedAttachments: string[] = [];
    for (const mail of mails) {
        const attachments = (mail.attachments ?? []).filter(isVisibleForwardAttachment);
        for (const attachment of attachments) {
            try {
                const forwardAttachment = await toForwardAttachment(attachment);
                const estimateMailSize = MailService.estimateMailSize(draft, { type: 'attachment', attachment: forwardAttachment });
                if (estimateMailSize > MailService.getMaxMailSize()) {
                    skippedAttachments.push(attachment.fileName);
                    continue;
                }
                draft.attachment = [...draft.attachment, forwardAttachment];
            } catch {
                skippedAttachments.push(attachment.fileName);
            }
        }
    }
    return skippedAttachments;
};
const createTemporaryDraftToken = (): string => {
    if (globalThis.crypto?.randomUUID) return globalThis.crypto.randomUUID();
    return `${Date.now()}-${Math.random().toString(36).slice(2)}`;
};
const splitForwardContent = (bodyData?: string): Pick<MailDraft, 'content' | 'forwardContent' | 'replySourceMappingId'> => {
    const html = bodyData ?? '';
    const replySourceMappingId = Number(html.match(REPLY_SOURCE_MAPPING_ID_PATTERN)?.[1] ?? NaN);
    const htmlWithoutReplyMarker = html.replace(REPLY_SOURCE_MAPPING_ID_PATTERN, '');
    const startIndex = htmlWithoutReplyMarker.indexOf(FORWARD_CONTENT_START);
    const endIndex = htmlWithoutReplyMarker.indexOf(FORWARD_CONTENT_END);
    if (startIndex < 0 || endIndex < startIndex) {
        return {
            content: htmlWithoutReplyMarker,
            forwardContent: '',
            replySourceMappingId: Number.isFinite(replySourceMappingId) ? replySourceMappingId : null,
        };
    }

    const before = htmlWithoutReplyMarker.slice(0, startIndex);
    const forwardContent = htmlWithoutReplyMarker.slice(startIndex + FORWARD_CONTENT_START.length, endIndex);
    const after = htmlWithoutReplyMarker.slice(endIndex + FORWARD_CONTENT_END.length);
    const content = [before, after].filter(item => item.trim().length > 0).join('<br/>');
    return {
        content,
        forwardContent,
        replySourceMappingId: Number.isFinite(replySourceMappingId) ? replySourceMappingId : null,
    };
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
    getDraft: async (draftId: string): Promise<MailDraft> => {
        const mappingId = Number(draftId);
        if (!Number.isFinite(mappingId) || mappingId <= 0) throw new Error('草稿ID无效');
        const response = await Email.getDraft(mappingId);
        const draft = response.data;
        const { content, forwardContent, replySourceMappingId } = splitForwardContent(draft.bodyData);
        return {
            recipients: [],
            to: draft.toAddressList ?? [],
            cc: draft.ccAddressList ?? [],
            bcc: draft.bccAddressList ?? [],
            sendSeparately: false,
            subject: draft.subject ?? '',
            content,
            forwardContent,
            replySourceMappingId,
            attachment: draft.attachments ?? [],
            signatureId: 'none',
            signatureContent: '',
            sendNickname: draft.sendName ?? '',
            sendMailBoxId: String(draft.mailBoxId ?? ''),
            sendMailBoxView: draft.sendMailBoxView ?? ''
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
        forwardContent: '',
        replySourceMappingId: null,
        attachment: [],
        signatureId: '',
        signatureContent: '',
        sendNickname: '',
        sendMailBoxId: '',
        sendMailBoxView: ''
    }),
    createForwardDraft: async (mails: MailDetailResponse | MailDetailResponse[]): Promise<ForwardDraftResult> => {
        const mailList = (Array.isArray(mails) ? mails : [mails]).filter(Boolean);
        const draft = MailService.createDefaultMailDraft();
        draft.subject = normalizeForwardSubject(mailList[0]?.subject ?? '', mailList.length);
        draft.forwardContent = mailList.map(buildForwardBlock).join('<br/>');
        const skippedAttachments = await addForwardAttachments(draft, mailList);
        return { draft, skippedAttachments };
    },
    createReplyDraft: (mail: MailDetailResponse, options?: ReplyDraftOptions): MailDraft => {
        const draft = MailService.createDefaultMailDraft();
        const recipients = createReplyRecipients(mail, options);
        draft.to = recipients.to;
        draft.cc = recipients.cc;
        draft.subject = normalizeReplySubject(mail.subject ?? '');
        draft.forwardContent = buildForwardBlock(mail);
        draft.replySourceMappingId = Number.isFinite(mail.mappingId) ? mail.mappingId : null;
        return draft;
    },
    storeTemporaryDraft: (draft: MailDraft): string => {
        const token = createTemporaryDraftToken();
        temporaryDrafts.set(token, draft);
        return token;
    },
    consumeTemporaryDraft: (token: string): MailDraft | null => {
        const draft = temporaryDrafts.get(token) ?? null;
        temporaryDrafts.delete(token);
        return draft;
    },
    createAttachmentLink: (attachment: Attachment): string => {
        return MailService.createAttachmentLinkBlock([attachment]);
    },
    createForwardContentBlock: (forwardContent?: string, includeDraftMarkers = true): string => {
        const html = forwardContent?.trim() ?? '';
        if (!html) return '';
        if (!includeDraftMarkers) return html;
        return `${FORWARD_CONTENT_START}${html}${FORWARD_CONTENT_END}`;
    },
    createReplySourceMappingIdBlock: (replySourceMappingId?: number | null): string => {
        if (!replySourceMappingId || !Number.isFinite(replySourceMappingId) || replySourceMappingId <= 0) return '';
        return `<!--NOVAMAIL_REPLY_SOURCE_MAPPING_ID:${Math.trunc(replySourceMappingId)}-->`;
    },
    createAttachmentLinkBlock: (attachments: Attachment[]): string => {
        const transferAttachments = attachments.filter(attachment => attachment.sendType === 'TRANSFER_LINK' && attachment.fileData?.trim());
        if (transferAttachments.length === 0) return '';

        const rows = transferAttachments.map((attachment, index) => {
            const url = attachment.fileData.trim();
            const fileName = escapeHtml(attachment.fileName || url);
            const fileType = escapeHtml(attachment.fileType || '文件');
            const fileSize = escapeHtml(formatAttachmentSize(attachment.fileSize));
            const borderTop = index === 0 ? '' : 'border-top:1px solid #e7eef8;';

            return `<tr><td style="${borderTop}padding:10px 0;"><table role="presentation" cellpadding="0" cellspacing="0" style="width:100%;border-collapse:collapse;"><tr><td style="width:46px;vertical-align:top;"><span style="display:inline-block;min-width:34px;padding:5px 6px;border-radius:6px;background:#e7f4ff;color:#1f9eea;font-size:12px;font-weight:700;text-align:center;line-height:16px;">附件</span></td><td style="vertical-align:top;padding-left:10px;"><a href="${escapeHtml(url)}" target="_blank" rel="noopener noreferrer" style="display:inline-block;color:#168ddb;font-size:14px;font-weight:700;text-decoration:none;line-height:20px;word-break:break-all;">${fileName}</a><div style="margin-top:3px;color:#7d8794;font-size:12px;line-height:18px;">${fileSize} · ${fileType} · 文件中转站链接</div></td></tr></table></td></tr>`;
        }).join('');

        return `<div style="margin-top:16px;padding:12px 14px;border:1px solid #dce8f7;border-radius:8px;background:#f8fbff;font-family:Arial,'Microsoft YaHei',sans-serif;color:#253040;"><div style="margin-bottom:2px;color:#253040;font-size:14px;font-weight:700;line-height:22px;">附件链接</div><table role="presentation" cellpadding="0" cellspacing="0" style="width:100%;border-collapse:collapse;">${rows}</table></div>`;
    },
    // 邮件大小估计
    estimateMailSize: (draft: MailDraft, newData?: MailExtraData | MailExtraData[]): number => {
        const signatureContent = draft.signatureContent ?? '';
        const mailContent = draft.content ?? '';
        const replySourceMappingId = MailService.createReplySourceMappingIdBlock(draft.replySourceMappingId);
        const forwardContent = MailService.createForwardContentBlock(draft.forwardContent);
        const transferAttachmentLinks = MailService.createAttachmentLinkBlock(draft.attachment);

        const allRecipients = draft.sendSeparately ? draft.recipients : [
            ...draft.to,
            ...draft.cc,
            ...draft.bcc
        ];

        const recipientHeader = allRecipients.join(',');

        const htmlContent = [mailContent, signatureContent, replySourceMappingId, forwardContent, transferAttachmentLinks].join('');

        const cleanHtml = MailService.renderMail(htmlContent);

        // 邮件头、MIME 边界、Content-Type 等基础开销预估
        const baseMimeHeaderSize = 2048;

        const headerSize = getUtf8Size(draft.subject) + getUtf8Size(recipientHeader);

        const bodySize = getUtf8Size(cleanHtml);

        const attachmentSize = draft.attachment.reduce((total, attachment) => {
            if (attachment.sendType === 'TRANSFER_LINK') return total;
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
