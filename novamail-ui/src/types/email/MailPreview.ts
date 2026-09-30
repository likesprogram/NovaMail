export interface MailPreviewAddress {
    name: string;
    address: string;
}

export type MailPreviewAttachmentSource = 'REMOTE' | 'LOCAL';

export interface MailPreviewAttachment {
    id?: string | number | null;
    fileName: string;
    fileType: string;
    fileSize: number;
    isInline?: boolean;
    source: MailPreviewAttachmentSource;
    fileData?: string;
    attachmentId?: number | null;
    partIndex?: number;
    sourceType?: string;
    mappingId?: number;
}

export interface MailPreviewData {
    subject: string;
    from: MailPreviewAddress;
    tos: MailPreviewAddress[];
    ccs?: MailPreviewAddress[];
    bccs?: MailPreviewAddress[];
    htmlBody?: string;
    textBody?: string;
    preview?: string;
    createTime?: string;
    attachments?: MailPreviewAttachment[];
}
