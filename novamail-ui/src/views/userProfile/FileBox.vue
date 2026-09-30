<template>
    <div class="fileBox">
        <div class="fileHeader">
            <div class="titleGroup">
                <span class="iconfont icon-likesprogramwenzhang"></span>
                <div class="titleText">
                    <span class="title">文件中转站</span>
                    <span class="summary">{{ isLoadingFiles ? '加载中' : `${displayableFiles.length} 个文件 · ${formatFileSize(displayableFileSize)}` }}</span>
                </div>
            </div>
            <div class="headerActions">
                <button class="button btn-primary" :class="{ 'btn-disabled': isUploading }" @click="openUploadForm">
                    <span class="iconfont icon-likesprogramshangchuan"></span>
                    <span>上传文件</span>
                </button>
            </div>
        </div>

        <div class="fileContent">
            <aside class="sidePanel">
                <div class="searchInput" :class="{ searchClearShown: searchKeyword.trim().length > 0 }">
                    <input v-model="searchKeyword" type="text" placeholder="搜索文件、路径或说明">
                    <button v-if="searchKeyword.trim().length > 0" class="button btn-text iconfont icon-likesprogramcancel"
                        title="清空搜索" @click="searchKeyword = ''"></button>
                    <div v-else class="button btn-text iconfont icon-likesprogramchaxun"></div>
                </div>

                <div class="pathSection">
                    <span class="sectionTitle">文件路径</span>
                    <button v-for="path in pathFilters" :key="path.path" class="pathItem"
                        :class="{ selected: selectedPath === path.path }"
                        @click="selectPath(path.path)">
                        <span class="pathName">
                            <span class="iconfont icon-likesprogramwenzhang"></span>
                            <span>{{ path.name }}</span>
                        </span>
                        <span class="pathCount">{{ path.count }}</span>
                    </button>
                </div>
            </aside>

            <main class="filePanel">
                <div class="actionBar">
                    <div class="listTitle">
                        <span>{{ listTitle }}</span>
                        <span class="listCount">{{ visibleRows.length }}</span>
                    </div>
                    <div class="listTools">
                        <select v-model="sortMode" title="排序方式">
                            <option value="updateTime">最近更新</option>
                            <option value="uploadTime">上传时间</option>
                            <option value="fileName">文件名</option>
                            <option value="fileSize">文件大小</option>
                        </select>
                        <button class="button btn-text" :class="{ 'btn-disabled': !selectedFile }" @click="previewSelectedFile">
                            <span class="iconfont icon-likesprogrammimakejian"></span>
                            <span>预览</span>
                        </button>
                        <button class="button btn-text" :class="{ 'btn-disabled': !selectedFile }" @click="openFileEdit">
                            <span class="iconfont icon-likesprogrambianji"></span>
                            <span>编辑</span>
                        </button>
                        <button class="button btn-text" :class="{ 'btn-disabled': !selectedFile }" @click="downloadSelectedFile">
                            <span class="iconfont icon-likesprogramxiazai"></span>
                            <span>下载</span>
                        </button>
                        <button class="button btn-text danger" :class="{ 'btn-disabled': !selectedFile }" @click="deleteSelectedFile">
                            <span class="iconfont icon-likesprogramhuishouzhan"></span>
                            <span>删除</span>
                        </button>
                    </div>
                </div>

                <div class="fileTable">
                    <div class="tableHeader">
                        <span>名称</span>
                        <span>大小</span>
                        <span>版本</span>
                        <span>更新时间</span>
                    </div>

                    <button v-for="row in visibleRows" :key="row.id" class="fileRow"
                        :class="{ selected: row.fileId === selectedFileId }"
                        @click="selectRow(row)"
                        @dblclick="previewRow(row)">
                        <span class="fileNameCell">
                            <span class="fileIcon">
                                <img :src="row.iconUrl" alt="file icon">
                            </span>
                            <span class="nameInfo">
                                <span class="fileName">{{ row.name }}</span>
                                <span class="filePath">{{ row.pathLabel }}</span>
                            </span>
                        </span>
                        <span class="fileSize">{{ row.sizeLabel }}</span>
                        <span>{{ row.versionLabel }}</span>
                        <span>{{ row.timeLabel }}</span>
                    </button>

                    <div v-if="visibleRows.length === 0" class="emptyFiles">
                        <span class="iconfont icon-likesprogramchaxun"></span>
                        <span>没有匹配的文件</span>
                    </div>
                </div>
            </main>

            <aside class="detailPanel">
                <template v-if="detailMode === 'view' && selectedFile">
                    <div class="detailHero">
                        <span class="detailFileIcon">
                            <img :src="getFileIconUrl(selectedFile.fileName)" alt="file icon">
                        </span>
                        <span class="detailName">{{ selectedFile.fileName }}</span>
                        <span class="detailPath">{{ selectedFile.filePath }}</span>
                    </div>

                    <div class="detailList">
                        <div class="detailItem">
                            <span class="label">大小</span>
                            <span class="value">{{ formatFileSize(selectedFile.fileSize) }}</span>
                        </div>
                        <div class="detailItem">
                            <span class="label">类型</span>
                            <span class="value">{{ selectedFile.fileType || '未知类型' }}</span>
                        </div>
                        <div class="detailItem">
                            <span class="label">版本</span>
                            <span class="value">v{{ selectedFile.version }}</span>
                        </div>
                        <div class="detailItem">
                            <span class="label">文件哈希</span>
                            <span class="value hash">{{ selectedFile.fileHash }}</span>
                        </div>
                        <div class="detailItem">
                            <span class="label">说明</span>
                            <span class="value description">{{ selectedFile.description || '未填写' }}</span>
                        </div>
                    </div>

                    <div class="authorizationPanel">
                        <div class="authorizationHeader">
                            <span>附件授权</span>
                            <span>{{ selectedFileAuthorizations.length }}</span>
                        </div>
                        <div v-if="isLoadingAuthorizations" class="authorizationEmpty">加载中</div>
                        <div v-else-if="selectedFileAuthorizations.length === 0" class="authorizationEmpty">暂无邮件附件引用</div>
                        <div v-else class="authorizationList">
                            <div v-for="item in selectedFileAuthorizations" :key="item.attachmentId" class="authorizationItem">
                                <div class="authorizationInfo">
                                    <span class="authorizationSubject">{{ item.mailSubject || '无主题邮件' }}</span>
                                    <span class="authorizationMeta">{{ item.senderEmail || item.senderName || '未知来源' }}</span>
                                </div>
                                <button class="button btn-text danger" @click="revokeAuthorization(item.attachmentId)">
                                    <span>撤销</span>
                                </button>
                            </div>
                        </div>
                    </div>
                </template>

                <template v-else-if="detailMode === 'uploadForm'">
                    <div class="formHeader">
                        <span>上传文件</span>
                        <button class="button btn-text iconfont icon-likesprogramcancel" title="取消" @click="closeForm"></button>
                    </div>
                    <div class="formBody">
                        <button class="uploadPicker" :class="{ 'btn-disabled': isUploading }" @click="chooseUploadFiles">
                            <span class="iconfont icon-likesprogramshangchuan"></span>
                            <span>{{ uploadFiles.length ? `已选择 ${uploadFiles.length} 个文件` : '选择本地文件' }}</span>
                        </button>
                        <div v-if="uploadFiles.length > 0" class="uploadPreview">
                            <span v-for="file in uploadFiles" :key="file.name" class="uploadFileName">
                                {{ file.name }} · {{ formatFileSize(file.size) }}
                            </span>
                        </div>
                        <label class="formField">
                            <span>文件路径</span>
                            <input v-model="uploadForm.filePath" type="text" placeholder="例如 /毕业设计">
                        </label>
                        <label class="formField">
                            <span>说明</span>
                            <textarea v-model="uploadForm.description" placeholder="可选"></textarea>
                        </label>
                    </div>
                    <div class="formActions">
                        <button class="button btn-text" :class="{ 'btn-disabled': isUploading }" @click="closeForm">取消</button>
                        <button class="button btn-primary" :class="{ 'btn-disabled': isUploading }" @click="saveUploadFiles">
                            {{ isUploading ? `上传中 ${uploadProgress}%` : '保存' }}
                        </button>
                    </div>
                </template>

                <template v-else-if="detailMode === 'fileForm'">
                    <div class="formHeader">
                        <span>编辑文件</span>
                        <button class="button btn-text iconfont icon-likesprogramcancel" title="取消" @click="closeForm"></button>
                    </div>
                    <div class="formBody">
                        <label class="formField">
                            <span>文件名</span>
                            <input v-model="fileForm.fileName" type="text" placeholder="请输入文件名">
                        </label>
                        <label class="formField">
                            <span>说明</span>
                            <textarea v-model="fileForm.description" placeholder="可选"></textarea>
                        </label>
                    </div>
                    <div class="formActions">
                        <button class="button btn-text" @click="closeForm">取消</button>
                        <button class="button btn-primary" @click="saveFileEdit">保存</button>
                    </div>
                </template>

                <div v-else class="emptyDetail">
                    <span class="iconfont icon-likesprogramwenzhang"></span>
                    <span>选择文件查看详情</span>
                </div>
            </aside>
        </div>

        <MessageDialog ref="previewDialog" @closeEvent="closePreview">
            <template #title>
                <span>{{ previewFile?.fileName || '文件预览' }}</span>
            </template>
            <template #content>
                <div class="previewDialog">
                    <div class="previewHeader">
                        <span>{{ previewFile?.fileType || '未知类型' }}</span>
                        <span>{{ previewFile ? formatFileSize(previewFile.fileSize) : '' }}</span>
                    </div>
                    <div class="previewBody">
                        <div v-if="isPreviewLoading" class="previewFallback">
                            <span>预览加载中</span>
                        </div>
                        <img v-else-if="previewFile && isImageFile(previewFile) && previewUrl" :src="previewUrl" alt="file preview"
                            @error="handlePreviewError">
                        <iframe v-else-if="previewFile && (isPdfFile(previewFile) || isTextLikeFile(previewFile)) && previewUrl"
                            :src="previewUrl" title="file preview"></iframe>
                        <div v-else class="previewFallback">
                            <img v-if="previewFile" :src="getFileIconUrl(previewFile.fileName)" alt="file icon">
                            <span>{{ isPreviewFailed ? '预览加载失败' : '此文件暂不支持直接预览' }}</span>
                        </div>
                    </div>
                </div>
            </template>
            <template #footer>
                <div class="dialogFooterButton failure" @click="closePreviewDialog">
                    <span class="textBox">
                        <span class="iconfont icon-likesprogramcuowu"></span>
                        <span class="text">关闭</span>
                    </span>
                    <span class="line"></span>
                </div>
                <div class="dialogFooterButton success" :class="{ disabled: !previewFile }" @click="downloadPreviewFile">
                    <span class="textBox">
                        <span class="iconfont icon-likesprogramxiazai"></span>
                        <span class="text">下载</span>
                    </span>
                </div>
            </template>
        </MessageDialog>

        <ShowMessage ref="showMessage"/>
    </div>
</template>

<script lang="ts">
export default {
    name: 'FileBox',
}
</script>

<script setup lang="ts">
import { computed, onMounted, onUnmounted, ref, watch } from 'vue';
import ShowMessage from '@components/messageDialog/ShowMessage.vue';
import MessageDialog from '@components/messageDialog/MessageDialog.vue';
import '@components/messageDialog/MessageDialog.css';
import { FileProcessing } from '@utils/FileProcessing';
import { UserFiles } from '@api/users/UserFiles';
import type { FileStatus, UserFile, UserFileAttachmentAuthorization } from '@api/users/UserFiles';

type DetailMode = 'view' | 'uploadForm' | 'fileForm';

interface FileRow {
    id: string;
    fileId: string;
    name: string;
    path: string;
    pathLabel: string;
    iconUrl: string;
    sizeLabel: string;
    versionLabel: string;
    timeLabel: string;
}

const ROOT_PATH = '/';
const showMessage = ref<InstanceType<typeof ShowMessage>>();
const previewDialog = ref<InstanceType<typeof MessageDialog> | null>(null);

const files = ref<UserFile[]>([]);

const selectedPath = ref(ROOT_PATH);
const selectedFileId = ref('');
const searchKeyword = ref('');
const sortMode = ref<'updateTime' | 'uploadTime' | 'fileName' | 'fileSize'>('updateTime');
const detailMode = ref<DetailMode>('view');
const isLoadingFiles = ref(false);
const isUploading = ref(false);
const uploadProgress = ref(0);
const uploadFiles = ref<File[]>([]);
const uploadForm = ref({
    filePath: ROOT_PATH,
    description: '',
});
const fileForm = ref({
    fileName: '',
    description: '',
});
const selectedFileAuthorizations = ref<UserFileAttachmentAuthorization[]>([]);
const isLoadingAuthorizations = ref(false);
const previewFile = ref<UserFile | null>(null);
const previewUrl = ref('');
const isPreviewLoading = ref(false);
const isPreviewFailed = ref(false);

const isDisplayableFile = (file: UserFile) => file.fileStatus === 'ACTIVE' || file.fileStatus === 'UPLOADING';
const displayableFiles = computed(() => files.value.filter(isDisplayableFile));
const displayableFileSize = computed(() => displayableFiles.value.reduce((total, file) => total + file.fileSize, 0));

const normalizeFilePath = (path: string) => {
    const value = path.trim().replace(/\\/g, '/').replace(/\/+/g, '/');
    if (!value || value === ROOT_PATH) return ROOT_PATH;
    return value.startsWith(ROOT_PATH) ? value.replace(/\/$/, '') : `${ROOT_PATH}${value}`.replace(/\/$/, '');
};

const getPathName = (path: string) => {
    if (path === ROOT_PATH) return '全部文件';
    return path.split('/').filter(Boolean).pop() ?? '全部文件';
};

const pathFilters = computed(() => {
    const countMap = new Map<string, number>();
    displayableFiles.value.forEach(file => {
        const path = normalizeFilePath(file.filePath);
        countMap.set(path, (countMap.get(path) ?? 0) + 1);
    });

    const paths = Array.from(countMap.entries())
        .sort(([firstPath], [secondPath]) => firstPath.localeCompare(secondPath, 'zh-Hans-CN'))
        .map(([path, count]) => ({
            path,
            name: getPathName(path),
            count,
        }));

    return [
        { path: ROOT_PATH, name: '全部文件', count: displayableFiles.value.length },
        ...paths.filter(item => item.path !== ROOT_PATH),
    ];
});

const listTitle = computed(() => {
    if (searchKeyword.value.trim()) return '搜索结果';
    return getPathName(selectedPath.value);
});


const matchesKeyword = (file: UserFile, keyword: string) => {
    return !keyword
        || file.fileName.toLowerCase().includes(keyword)
        || file.fileType?.toLowerCase().includes(keyword)
        || file.filePath.toLowerCase().includes(keyword)
        || file.fileHash.toLowerCase().includes(keyword)
        || file.description.toLowerCase().includes(keyword);
};

const sortedFiles = (items: UserFile[]) => {
    return [...items].sort((a, b) => {
        if (sortMode.value === 'fileName') return a.fileName.localeCompare(b.fileName, 'zh-Hans-CN');
        if (sortMode.value === 'fileSize') return b.fileSize - a.fileSize;
        const key = sortMode.value;
        return new Date(b[key]).getTime() - new Date(a[key]).getTime();
    });
};

const visibleRows = computed<FileRow[]>(() => {
    const keyword = searchKeyword.value.trim().toLowerCase();
    return sortedFiles(displayableFiles.value.filter(file => {
        const pathMatched = keyword || selectedPath.value === ROOT_PATH
            ? true
            : normalizeFilePath(file.filePath) === selectedPath.value;
        return pathMatched && matchesKeyword(file, keyword);
    })).map(file => ({
        id: `file-${file.fileId}`,
        fileId: file.fileId,
        name: file.fileName,
        path: file.filePath,
        pathLabel: file.filePath,
        iconUrl: getFileIconUrl(file.fileName),
        sizeLabel: formatFileSize(file.fileSize),
        versionLabel: `v${file.version}`,
        timeLabel: formatDate(file.updateTime),
    }));
});

const selectedFile = computed(() => displayableFiles.value.find(file => file.fileId === selectedFileId.value) ?? null);

const isImageFile = (file: UserFile) => /^(image\/)/i.test(file.fileType || '');
const isPdfFile = (file: UserFile) => (file.fileType || '').toLowerCase() === 'application/pdf';
const isTextLikeFile = (file: UserFile) => {
    const type = (file.fileType || '').toLowerCase();
    return type.startsWith('text/') || ['application/json', 'application/xml', 'application/javascript'].includes(type);
};
const canInlinePreview = (file: UserFile) => isImageFile(file) || isPdfFile(file) || isTextLikeFile(file);
const cleanupPreviewUrl = () => {
    if (previewUrl.value) {
        URL.revokeObjectURL(previewUrl.value);
        previewUrl.value = '';
    }
};
const handlePreviewError = () => {
    isPreviewFailed.value = true;
    cleanupPreviewUrl();
};
const openPreview = async (file: UserFile) => {
    cleanupPreviewUrl();
    previewFile.value = file;
    isPreviewFailed.value = false;
    isPreviewLoading.value = canInlinePreview(file);
    previewDialog.value?.ShowModal();

    if (!canInlinePreview(file)) {
        isPreviewLoading.value = false;
        return;
    }

    try {
        const blob = await UserFiles.previewBlob(file.fileId);
        if (previewFile.value?.fileId !== file.fileId) return;
        previewUrl.value = URL.createObjectURL(blob);
    } catch (error) {
        if (previewFile.value?.fileId === file.fileId) isPreviewFailed.value = true;
    } finally {
        if (previewFile.value?.fileId === file.fileId) isPreviewLoading.value = false;
    }
};
const closePreview = () => {
    cleanupPreviewUrl();
    previewFile.value = null;
    isPreviewLoading.value = false;
    isPreviewFailed.value = false;
};
const closePreviewDialog = () => {
    previewDialog.value?.Close();
};
const previewSelectedFile = () => {
    if (!selectedFile.value) return;
    void openPreview(selectedFile.value);
};
const previewRow = (row: FileRow) => {
    const file = displayableFiles.value.find(item => item.fileId === row.fileId);
    if (!file) return;
    selectedFileId.value = row.fileId;
    detailMode.value = 'view';
    void openPreview(file);
};
const downloadPreviewFile = async () => {
    if (!previewFile.value) return;
    try {
        await UserFiles.download(previewFile.value);
    } catch (error) {
        showMessage.value?.ShowModal('下载文件', ['文件下载失败，请稍后重试'], 'failure');
    }
};

const loadAttachmentAuthorizations = async (fileId: string) => {
    if (!fileId) {
        selectedFileAuthorizations.value = [];
        return;
    }
    isLoadingAuthorizations.value = true;
    try {
        const result = await UserFiles.listAttachmentAuthorizations(fileId);
        if (selectedFileId.value === fileId) selectedFileAuthorizations.value = result.data;
    } catch (error) {
        if (selectedFileId.value === fileId) selectedFileAuthorizations.value = [];
    } finally {
        if (selectedFileId.value === fileId) isLoadingAuthorizations.value = false;
    }
};

const loadFiles = async (preferredFileId?: string) => {
    isLoadingFiles.value = true;
    try {
        const result = await UserFiles.list();
        files.value = result.data;
        selectedFileId.value = preferredFileId && result.data.some(file => file.fileId === preferredFileId)
            ? preferredFileId
            : visibleRows.value[0]?.fileId ?? '';
    } catch (error) {
        showMessage.value?.ShowModal('文件加载', ['文件列表获取失败，请稍后重试'], 'failure');
    } finally {
        isLoadingFiles.value = false;
    }
};

const revokeAuthorization = async (attachmentId: string) => {
    if (!selectedFile.value) return;
    try {
        await UserFiles.revokeAttachmentAuthorization(selectedFile.value.fileId, attachmentId);
        selectedFileAuthorizations.value = selectedFileAuthorizations.value.filter(item => item.attachmentId !== attachmentId);
        showMessage.value?.ShowModal('撤销授权', ['附件下载授权已撤销'], 'success');
    } catch (error) {
        showMessage.value?.ShowModal('撤销授权', ['附件下载授权撤销失败，请稍后重试'], 'failure');
    }
};

const selectPath = (path: string) => {
    selectedPath.value = normalizeFilePath(path);
    detailMode.value = 'view';
    selectedFileId.value = visibleRows.value[0]?.fileId ?? '';
};

const selectRow = (row: FileRow) => {
    selectedFileId.value = row.fileId;
    detailMode.value = 'view';
};

const formatFileSize = (bytes: number) => {
    return FileProcessing.getFileSize(bytes);
};

const formatDate = (rawTime: string) => {
    const date = new Date(rawTime);
    if (Number.isNaN(date.getTime())) return rawTime;

    const now = new Date();
    const pad = (value: number) => value.toString().padStart(2, '0');
    const timeStr = `${pad(date.getHours())}:${pad(date.getMinutes())}`;
    const today = new Date(now.getFullYear(), now.getMonth(), now.getDate());
    const targetDay = new Date(date.getFullYear(), date.getMonth(), date.getDate());
    const diffMs = today.getTime() - targetDay.getTime();
    const diffDays = Math.floor(diffMs / (1000 * 60 * 60 * 24));

    if (diffDays === 0) return `今天 ${timeStr}`;
    if (diffDays === 1) return `昨天 ${timeStr}`;
    if (diffDays < 7 && diffDays > 1) {
        const weekdays = ['周日', '周一', '周二', '周三', '周四', '周五', '周六'];
        return `${weekdays[date.getDay()]} ${timeStr}`;
    }
    if (date.getFullYear() === now.getFullYear()) {
        return `${pad(date.getMonth() + 1)}月${pad(date.getDate())}日`;
    }
    return `${date.getFullYear()}年${pad(date.getMonth() + 1)}月${pad(date.getDate())}日`;
};

const getFileExtension = (fileName: string) => {
    const index = fileName.lastIndexOf('.');
    if (index < 0) return '';
    return fileName.slice(index + 1).toLowerCase();
};

const fileIconUrlMap = import.meta.glob('@assets/icons/fileIcons/*.png', {
    eager: true,
    import: 'default',
}) as Record<string, string>;

const resolveFileIconUrl = (iconName: string) => {
    const iconPath = Object.keys(fileIconUrlMap).find(path => path.endsWith(`/fileIcons/${iconName}.png`));
    if (iconPath) return fileIconUrlMap[iconPath] ?? '';
    const miscIconPath = Object.keys(fileIconUrlMap).find(path => path.endsWith('/fileIcons/Misc.png'));
    return miscIconPath ? fileIconUrlMap[miscIconPath] ?? '' : '';
};

const getFileIconName = (fileName: string) => {
    const extension = getFileExtension(fileName);
    if (!extension) return 'Misc';
    if (['pdf'].includes(extension)) return 'PDF';
    if (['doc', 'docx'].includes(extension)) return 'Word';
    if (['ppt', 'pptx'].includes(extension)) return 'PPT';
    if (['xls', 'xlsx'].includes(extension)) return 'Excel';
    if (['mdb', 'accdb'].includes(extension)) return 'Access';
    if (['txt', 'md', 'log'].includes(extension)) return 'Text';
    if (['zip', '7z', 'gz', 'tar'].includes(extension)) return 'ZIP_54';
    if (['rar'].includes(extension)) return 'RAR';
    if (['jpg', 'jpeg', 'png', 'gif', 'bmp', 'webp', 'svg'].includes(extension)) return 'Picture';
    if (['mp4', 'avi', 'mov', 'wmv', 'flv', 'mkv'].includes(extension)) return 'Video';
    if (['mp3', 'wav', 'aac', 'ogg', 'flac'].includes(extension)) return 'Music';
    if (['html', 'htm'].includes(extension)) return 'Web';
    if (['js', 'ts', 'css', 'json', 'xml', 'cpp', 'c', 'java', 'py', 'go', 'rs'].includes(extension)) return 'Code';
    if (['sql'].includes(extension)) return 'SQL';
    if (['exe', 'msi'].includes(extension)) return 'EXE';
    if (['ai'].includes(extension)) return 'AI';
    if (['psd'].includes(extension)) return 'PS';
    if (['eps'].includes(extension)) return 'EPS';
    if (['cad', 'dwg', 'dxf'].includes(extension)) return 'CAD';
    if (['ttf', 'otf', 'woff', 'woff2'].includes(extension)) return 'Fonts';
    if (['xmind'].includes(extension)) return 'Xmind';
    if (['vsd', 'vsdx'].includes(extension)) return 'Visio';
    if (['apk'].includes(extension)) return 'Android';
    if (['torrent'].includes(extension)) return 'BT';
    return 'Misc';
};

const getFileIconUrl = (fileName: string) => {
    return resolveFileIconUrl(getFileIconName(fileName));
};

const openUploadForm = () => {
    if (isUploading.value) return;
    uploadFiles.value = [];
    uploadForm.value = {
        filePath: selectedPath.value,
        description: '',
    };
    detailMode.value = 'uploadForm';
};

const chooseUploadFiles = async () => {
    if (isUploading.value) return;
    try {
        const result = await FileProcessing.openFiles('*', true);
        uploadFiles.value = result instanceof FileList ? Array.from(result) : [result];
    } catch (error) {
        return;
    }
};

const saveUploadFiles = async () => {
    if (isUploading.value) return;
    if (uploadFiles.value.length === 0) {
        showMessage.value?.ShowModal('上传文件', ['请先选择需要上传的文件'], 'failure');
        return;
    }
    const uploadPath = normalizeFilePath(uploadForm.value.filePath);
    isUploading.value = true;
    uploadProgress.value = 0;
    try {
        const uploadedFiles: UserFile[] = [];
        for (let index = 0; index < uploadFiles.value.length; index++) {
            const file = uploadFiles.value[index];
            if (!file) continue;
            const result = await UserFiles.upload(file, {
                filePath: uploadPath,
                description: uploadForm.value.description.trim(),
            }, progress => {
                uploadProgress.value = Math.round(((index + progress / 100) / uploadFiles.value.length) * 100);
            });
            uploadedFiles.push(result.data);
        }
        files.value = [...uploadedFiles, ...files.value];
        selectedPath.value = uploadPath;
        selectedFileId.value = uploadedFiles[0]?.fileId ?? '';
        isUploading.value = false;
        closeForm();
        showMessage.value?.ShowModal('上传文件', [`成功上传 ${uploadedFiles.length} 个文件`], 'success');
    } catch (error) {
        await loadFiles();
        showMessage.value?.ShowModal('上传文件', ['文件上传失败，请稍后重试'], 'failure');
    } finally {
        isUploading.value = false;
        uploadProgress.value = 0;
    }
};

const openFileEdit = () => {
    if (!selectedFile.value) return;
    fileForm.value = {
        fileName: selectedFile.value.fileName,
        description: selectedFile.value.description,
    };
    detailMode.value = 'fileForm';
};

const saveFileEdit = async () => {
    if (!selectedFile.value) return;
    const fileName = fileForm.value.fileName.trim();
    if (!fileName) {
        showMessage.value?.ShowModal('编辑文件', ['文件名不能为空'], 'failure');
        return;
    }
    try {
        const result = await UserFiles.update({
            fileId: selectedFile.value.fileId,
            fileName,
            description: fileForm.value.description.trim(),
        });
        files.value = files.value.map(file => file.fileId === result.data.fileId ? result.data : file);
        selectedFileId.value = result.data.fileId;
        closeForm();
        showMessage.value?.ShowModal('编辑文件', ['文件信息已更新'], 'success');
    } catch (error) {
        showMessage.value?.ShowModal('编辑文件', ['文件更新失败，请稍后重试'], 'failure');
    }
};

const executeDeleteSelectedFile = async () => {
    if (!selectedFile.value) return;
    try {
        await UserFiles.delete(selectedFile.value.fileId);
        updateSelectedFileStatus('DELETED');
        showMessage.value?.ShowModal('删除文件', ['文件已删除'], 'success');
    } catch (error) {
        showMessage.value?.ShowModal('删除文件', ['文件删除失败，请稍后重试'], 'failure');
    }
};
const deleteSelectedFile = () => {
    if (!selectedFile.value) return;
    const fileName = selectedFile.value.fileName;
    showMessage.value?.ShowModal(
        '删除文件',
        [`确定删除「${fileName}」吗？`, '删除后文件会从文件中转站移除。'],
        'warning',
        selectedButtonIndex => {
            if (selectedButtonIndex === 1) void executeDeleteSelectedFile();
        },
        [
            { text: '取消', type: 'failure', autoClose: true },
            { text: '删除', type: 'warning', icon: 'icon-likesprogramhuishouzhan', iconPosition: 'left', autoClose: true },
        ],
    );
};

const updateSelectedFileStatus = (status: FileStatus) => {
    const targetFileId = selectedFile.value?.fileId;
    if (!targetFileId) return;
    files.value = files.value.map(file => {
        if (file.fileId !== targetFileId) return file;
        return { ...file, fileStatus: status, updateTime: new Date().toISOString() };
    });
    selectedFileId.value = visibleRows.value[0]?.fileId ?? '';
};

const downloadSelectedFile = async () => {
    if (!selectedFile.value) return;
    try {
        await UserFiles.download(selectedFile.value);
    } catch (error) {
        showMessage.value?.ShowModal('下载文件', ['文件下载失败，请稍后重试'], 'failure');
    }
};

const closeForm = () => {
    if (isUploading.value) return;
    detailMode.value = 'view';
    uploadFiles.value = [];
};

onMounted(() => {
    void loadFiles();
});

onUnmounted(() => {
    cleanupPreviewUrl();
});

watch(selectedFileId, fileId => {
    if (detailMode.value !== 'view') {
        selectedFileAuthorizations.value = [];
        return;
    }
    void loadAttachmentAuthorizations(fileId);
});

watch(detailMode, mode => {
    if (mode === 'view' && selectedFileId.value) {
        void loadAttachmentAuthorizations(selectedFileId.value);
        return;
    }
    selectedFileAuthorizations.value = [];
});
</script>

<style scoped>
.fileBox {
    width: 100%;
    height: 100%;
    min-height: 0;
    padding: 14px;
    position: relative;
    z-index: 999;
    top: 0;
    left: 0;
    display: flex;
    flex-direction: column;
    gap: 12px;
}

.fileHeader {
    width: 100%;
    min-height: 46px;
    flex-shrink: 0;
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
}
.fileHeader .titleGroup {
    min-width: 0;
    display: flex;
    align-items: center;
    gap: 10px;
}
.fileHeader .titleGroup > .iconfont {
    width: 36px;
    height: 36px;
    flex-shrink: 0;
    display: flex;
    align-items: center;
    justify-content: center;
    border-radius: 8px;
    color: var(--text-link);
    font-size: 23px;
    background-color: var(--bg-hover-primary);
}
.fileHeader .titleText {
    min-width: 0;
    display: flex;
    flex-direction: column;
    gap: 3px;
}
.fileHeader .title {
    color: var(--text-secondary1);
    font-size: 18px;
    font-weight: 700;
}
.fileHeader .summary {
    color: var(--text-tertiary);
    font-size: 13px;
}
.fileHeader .headerActions {
    flex-shrink: 0;
    display: flex;
    align-items: center;
    gap: 8px;
}
.fileHeader .headerActions .button {
    height: 34px;
    font-size: 14px;
}
.fileHeader .headerActions .iconfont {
    font-size: 18px;
}

.fileContent {
    width: 100%;
    min-height: 0;
    flex: 1;
    display: grid;
    grid-template-columns: 230px minmax(460px, 1fr) 320px;
    gap: 1px;
    overflow: hidden;
    border-radius: 10px;
    background-color: var(--bg-hover-primary);
}

.sidePanel,
.filePanel,
.detailPanel {
    min-height: 0;
    background-color: var(--bg-primary);
}

.sidePanel {
    padding: 12px;
    display: flex;
    flex-direction: column;
    gap: 12px;
}
.searchInput {
    width: 100%;
    height: 34px;
    flex-shrink: 0;
    display: flex;
    align-items: center;
    justify-content: space-between;
    color: var(--primary-color);
    border: 1px solid var(--border-primary);
    border-radius: 8px;
    overflow: hidden;
    transition: border-color .3s;
    background-color: var(--bg-primary);
}
.searchInput:hover,
.searchInput:focus-within,
.searchInput.searchClearShown {
    border-color: var(--border-hover);
}
.searchInput input {
    width: 100%;
    height: 100%;
    border: none;
    outline: none;
    padding: 0px 6px 0px 10px;
    box-sizing: border-box;
    font-size: 14px;
    background-color: transparent;
    cursor: text;
}
.searchInput .button {
    width: 32px;
    height: 32px;
    flex-shrink: 0;
    padding: 0px;
    font-size: 20px;
    --text-primary: var(--text-secondary2);
}
.pathSection {
    width: 100%;
    min-height: 0;
    display: flex;
    flex-direction: column;
    gap: 6px;
}
.pathSection {
    flex: 1;
    overflow-y: auto;
}
.sectionTitle {
    padding: 0px 4px;
    color: var(--text-tertiary);
    font-size: 12px;
}
.pathItem {
    width: 100%;
    min-height: 34px;
    padding: 0px 8px;
    flex-shrink: 0;
    justify-content: space-between;
    border-radius: 8px;
    color: var(--text-secondary2);
    transition: all .3s;
}
.pathItem:hover {
    color: var(--text-link-hover);
    background-color: var(--bg-hover-secondary);
}
.pathItem.selected {
    color: var(--text-link);
    background-color: var(--bg-hover-primary);
}
.pathName {
    min-width: 0;
    display: flex;
    align-items: center;
    gap: 7px;
}
.pathName span:last-child {
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.pathName .iconfont {
    width: 18px;
    flex-shrink: 0;
    font-size: 17px;
}
.pathCount {
    min-width: 22px;
    padding: 1px 6px;
    border-radius: 999px;
    text-align: center;
    font-size: 12px;
    background-color: var(--bg-tertiary);
}

.filePanel {
    min-width: 0;
    display: flex;
    flex-direction: column;
}
.actionBar {
    height: 48px;
    padding: 0px 14px;
    flex-shrink: 0;
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
    border-bottom: 1px solid var(--bg-hover-primary);
}
.listTitle {
    min-width: 0;
    display: flex;
    align-items: center;
    gap: 8px;
    color: var(--text-secondary1);
    font-size: 16px;
    font-weight: 700;
}
.listTitle > span:first-child {
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.listCount {
    min-width: 24px;
    padding: 1px 7px;
    border-radius: 999px;
    color: var(--text-secondary2);
    background-color: var(--bg-tertiary);
    font-size: 12px;
    text-align: center;
}
.listTools {
    flex-shrink: 0;
    display: flex;
    align-items: center;
    gap: 6px;
}
.listTools .button {
    height: 30px;
    padding: 4px 8px;
    font-size: 13px;
}
.listTools select {
    width: 108px;
    height: 30px;
    padding: 0px 9px;
    box-sizing: border-box;
    border-radius: 8px;
    border: 1px solid var(--border-primary);
    color: var(--text-secondary2);
    background-color: var(--bg-primary);
    font-size: 13px;
    outline: none;
    cursor: pointer;
}
.listTools select:hover,
.listTools select:focus {
    border-color: var(--border-hover);
}
.listTools .danger {
    --text-primary: var(--color-error);
}
.fileTable {
    min-height: 0;
    flex: 1;
    padding: 10px;
    display: flex;
    flex-direction: column;
    gap: 5px;
    overflow-y: auto;
}
.tableHeader,
.fileRow {
    width: 100%;
    display: grid;
    grid-template-columns: minmax(220px, 1fr) 92px 58px 132px;
    align-items: center;
    gap: 8px;
}
.tableHeader {
    height: 30px;
    padding: 0px 10px;
    color: var(--text-tertiary);
    font-size: 12px;
}
.fileRow {
    min-height: 58px;
    padding: 8px 10px;
    justify-content: stretch;
    border-radius: 8px;
    border: 1px solid transparent;
    color: var(--text-secondary2);
    background-color: var(--bg-secondary);
    transition: background-color .3s, border-color .3s, transform .3s;
}
.fileRow:hover {
    border-color: var(--border-hover);
    background-color: var(--bg-hover-secondary);
}
.fileRow:active {
    transform: translateY(1px);
    background-color: var(--bg-active-primary);
}
.fileRow.selected {
    border-color: var(--border-hover);
    background-color: var(--bg-hover-primary);
}
.fileNameCell {
    min-width: 0;
    display: flex;
    align-items: center;
    gap: 10px;
}
.fileIcon {
    width: 34px;
    height: 34px;
    flex-shrink: 0;
    display: flex;
    align-items: center;
    justify-content: center;
}
.fileIcon img {
    width: 32px;
    height: 32px;
    display: block;
    object-fit: contain;
}
.nameInfo {
    min-width: 0;
    display: flex;
    flex-direction: column;
    gap: 4px;
    align-items: flex-start;
}
.fileName,
.filePath {
    max-width: 100%;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
    text-align: left;
}
.fileName {
    color: var(--text-secondary1);
    font-size: 14px;
    font-weight: 700;
}
.filePath {
    color: var(--text-tertiary);
    font-size: 12px;
}
.fileSize {
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.tableHeader > span:last-child,
.fileRow > span:last-child {
    text-align: right;
    white-space: nowrap;
}
.detailPanel {
    min-width: 0;
    padding: 16px;
    display: flex;
    flex-direction: column;
}
.detailHero {
    padding: 12px 0px 18px;
    flex-shrink: 0;
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 8px;
    border-bottom: 1px solid var(--border-primary);
}
.detailFileIcon {
    width: 72px;
    height: 72px;
    flex-shrink: 0;
    display: flex;
    align-items: center;
    justify-content: center;
}
.detailFileIcon img {
    width: 64px;
    height: 64px;
    display: block;
    object-fit: contain;
}
.detailName {
    max-width: 100%;
    color: var(--text-secondary1);
    font-size: 17px;
    font-weight: 700;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.detailPath {
    max-width: 100%;
    color: var(--text-tertiary);
    font-size: 12px;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.detailList {
    min-height: 0;
    padding-top: 12px;
    display: flex;
    flex-direction: column;
    gap: 10px;
    overflow-y: auto;
}
.detailItem {
    display: flex;
    flex-direction: column;
    gap: 5px;
}
.detailItem .label {
    color: var(--text-tertiary);
    font-size: 12px;
}
.detailItem .value {
    min-height: 26px;
    padding: 6px 8px;
    border-radius: 8px;
    color: var(--text-secondary1);
    background-color: var(--bg-secondary4);
    font-size: 14px;
    line-height: 1.5;
    word-break: break-word;
}
.detailItem .hash {
    font-size: 12px;
    font-family: Consolas, monospace;
}
.detailItem .description {
    min-height: 64px;
}
.authorizationPanel {
    margin-top: 12px;
    padding-top: 12px;
    border-top: 1px solid var(--border-primary);
    display: flex;
    flex-direction: column;
    gap: 8px;
}
.authorizationHeader {
    display: flex;
    align-items: center;
    justify-content: space-between;
    color: var(--text-secondary1);
    font-size: 13px;
    font-weight: 700;
}
.authorizationHeader span:last-child {
    min-width: 24px;
    padding: 1px 7px;
    border-radius: 999px;
    color: var(--text-secondary2);
    background-color: var(--bg-tertiary);
    text-align: center;
    font-size: 12px;
}
.authorizationList {
    display: flex;
    flex-direction: column;
    gap: 6px;
}
.authorizationItem {
    min-height: 48px;
    padding: 7px 8px;
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 8px;
    border-radius: 8px;
    background-color: var(--bg-secondary4);
}
.authorizationInfo {
    min-width: 0;
    display: flex;
    flex-direction: column;
    gap: 4px;
}
.authorizationSubject,
.authorizationMeta {
    max-width: 100%;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.authorizationSubject {
    color: var(--text-secondary1);
    font-size: 13px;
    font-weight: 700;
}
.authorizationMeta,
.authorizationEmpty {
    color: var(--text-tertiary);
    font-size: 12px;
}
.authorizationItem .button {
    height: 28px;
    flex-shrink: 0;
    padding: 4px 8px;
    --text-primary: var(--color-error);
}
.authorizationEmpty {
    min-height: 40px;
    display: flex;
    align-items: center;
    justify-content: center;
    border-radius: 8px;
    background-color: var(--bg-secondary4);
}
.previewDialog {
    width: min(900px, calc(100vw - 80px));
    height: min(620px, calc(100vh - 180px));
    min-height: 360px;
    padding: 0px 14px 14px;
    display: flex;
    flex-direction: column;
    gap: 10px;
    overflow: hidden;
    background-color: var(--bg-primary);
}
.previewHeader {
    height: 32px;
    flex-shrink: 0;
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
    color: var(--text-secondary2);
    font-size: 13px;
}
.previewHeader span {
    min-width: 0;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.previewBody {
    min-height: 0;
    flex: 1;
    border-radius: 8px;
    overflow: hidden;
    background-color: var(--bg-secondary4);
    display: flex;
    align-items: center;
    justify-content: center;
}
.previewBody > img,
.previewBody > iframe {
    width: 100%;
    height: 100%;
    border: none;
    object-fit: contain;
}
.previewFallback {
    width: 100%;
    height: 100%;
    padding: 24px;
    display: flex;
    align-items: center;
    justify-content: center;
    flex-direction: column;
    gap: 12px;
    color: var(--text-tertiary);
    font-size: 14px;
}
.previewFallback img {
    width: 72px;
    height: 72px;
    object-fit: contain;
}
.formHeader {
    height: 38px;
    flex-shrink: 0;
    display: flex;
    align-items: center;
    justify-content: space-between;
    color: var(--text-secondary1);
    font-size: 16px;
    font-weight: 700;
    border-bottom: 1px solid var(--border-primary);
}
.formHeader .button {
    width: 30px;
    height: 30px;
    padding: 0px;
    font-size: 18px;
}
.formBody {
    width: 100%;
    min-height: 0;
    padding-top: 12px;
    display: flex;
    flex-direction: column;
    gap: 10px;
    overflow-y: auto;
}
.formField {
    width: 100%;
    min-width: 0;
    display: flex;
    flex-direction: column;
    gap: 5px;
    color: var(--text-secondary2);
    font-size: 13px;
}
.formField input,
.formField select,
.formField textarea,
.readonlyValue {
    width: 100%;
    height: 42px;
    min-height: 42px;
    padding: 6px 9px;
    box-sizing: border-box;
    border-radius: 8px;
    border: 1px solid var(--border-primary);
    background-color: var(--bg-primary);
    color: var(--text-primary);
    font-size: 14px;
    outline: none;
    transition: border-color .3s;
}
.readonlyValue {
    display: flex;
    align-items: center;
    color: var(--text-secondary2);
    background-color: var(--bg-secondary4);
}
.formField input,
.formField textarea {
    cursor: text;
}
.formField select {
    appearance: none;
    cursor: pointer;
}
.formField .selectBox {
    width: 100%;
    min-width: 0;
    position: relative;
    display: flex;
    align-items: center;
}
.formField .selectBox select {
    padding-right: 34px;
}
.formField .selectBox .iconfont {
    width: 30px;
    height: 100%;
    position: absolute;
    right: 3px;
    top: 0px;
    display: flex;
    align-items: center;
    justify-content: center;
    color: var(--text-secondary2);
    font-size: 12px;
    transform: rotate(90deg);
    pointer-events: none;
}
.formField textarea {
    height: 104px;
    min-height: 104px;
    resize: none;
    line-height: 1.5;
}
.formField input:hover,
.formField select:hover,
.formField textarea:hover,
.formField input:focus,
.formField select:focus,
.formField textarea:focus {
    border-color: var(--border-hover);
}
.uploadPicker {
    width: 100%;
    min-height: 88px;
    flex-shrink: 0;
    display: flex;
    flex-direction: column;
    gap: 8px;
    align-items: center;
    justify-content: center;
    border-radius: 8px;
    border: 1px dashed var(--border-primary);
    color: var(--text-secondary2);
    background-color: var(--bg-secondary4);
    transition: all .3s;
}
.uploadPicker:hover {
    color: var(--text-link-hover);
    border-color: var(--border-hover);
    background-color: var(--bg-hover-secondary);
}
.uploadPicker .iconfont {
    font-size: 28px;
}
.uploadPreview {
    max-height: 92px;
    display: flex;
    flex-direction: column;
    gap: 5px;
    overflow-y: auto;
}
.uploadFileName {
    min-height: 26px;
    padding: 5px 8px;
    border-radius: 7px;
    color: var(--text-secondary2);
    background-color: var(--bg-secondary4);
    font-size: 12px;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.formActions {
    margin-top: auto;
    padding-top: 14px;
    display: flex;
    justify-content: flex-end;
    gap: 8px;
}
.formActions .button {
    min-width: 68px;
    height: 34px;
}
.emptyFiles,
.emptyDetail {
    min-height: 220px;
    flex: 1;
    display: flex;
    flex-direction: column;
    gap: 10px;
    align-items: center;
    justify-content: center;
    color: var(--text-tertiary);
    font-size: 14px;
}
.emptyFiles .iconfont,
.emptyDetail .iconfont {
    color: var(--text-tertiary);
    font-size: 36px;
}
</style>
