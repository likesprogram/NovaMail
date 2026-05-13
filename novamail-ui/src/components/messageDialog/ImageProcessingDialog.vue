<template>
    <div class="imageProcessingDialog">
        <!-- 图片编辑对话框 -->
        <MessageDialog ref="imageDialog">
            <template v-slot:title>
                <span>封面编辑</span>
            </template>
            <template v-slot:content>
                <div class="imageEditor" :style="imageDialogStyle">
                    <div class="editorButton">
                        <!-- 效果预览 -->
                         <div class="imagePreview">
                            <div class="previewText">效果预览</div>
                            <!-- 预览图片 -->
                            <div class="image">
                                <img :src="imageBase64" ref="previewImage" style="transform: translate(0, 0);" alt="Processed Image" />
                            </div>
                         </div>
                        <!-- 重置 -->
                         <div class="reset button" @click="resetImage">
                            <span class="iconfont icon-likesprogramsync"></span>
                            重置
                         </div>
                        <!-- 重新选择 -->
                         <div class="resetSelect button" @click="showimageDialog">
                            <span class="iconfont icon-likesprogramcarouselitem"></span>
                            重新选择图片
                         </div>
                    </div>
                    <div class="editorBox">
                        <div class="imageBox">
                            <div class="select" ref="imageSelect" @mousedown="dragx($event)">
                                <img :src="imageBase64" style="transform: translate(0, 0);" alt="Processed Image" />
                            </div>
                            <div class="zoom" ref="selectZoom">
                                <div class="zoomrLeft">
                                    <div class="top" @mousedown="resize($event, 'LeftTop')"></div>
                                    <div class="bottom" @mousedown="resize($event, 'LeftBottom')"></div>
                                </div>
                                <div class="zoomrRight">
                                    <div class="top" @mousedown="resize($event, 'RightTop')"></div>
                                    <div class="bottom" @mousedown="resize($event, 'RightBottom')"></div>
                                </div>
                            </div>
                        </div>
                        <img :src="imageBase64" :style="imageEditorBoxImageStyle" ref="imageEditorBoxImage" alt="Processed Image" />
                    </div>
                </div>
            </template>
            <template v-slot:footer>
                <div class="dialogFooterButton failure" @click="closeimageDialog">
                    <span class="textBox">
                        <span class="iconfont icon-likesprogramcuowu"></span>
                        <span class="text">取消</span>
                    </span>
                    <span class="line"></span>
                </div>
                <div class="dialogFooterButton success" @click="cropImage">
                    <span class="textBox">
                        <span class="iconfont icon-likesprogramzhengque"></span>
                        <span class="text">完成</span>
                    </span>
                </div>
            </template>
        </MessageDialog>
    </div>
</template>

<script lang="ts">
import { computed, defineComponent, nextTick, ref } from 'vue';
import { ImageProcessing } from '@utils/ImageProcessing';  // 导入 ImageProcessing 类
import MessageDialog from '@components/messageDialog/MessageDialog.vue';
import '@components/messageDialog/MessageDialog.css'

export default defineComponent({
    name: 'ImageProcessingDialog',
});
</script>

<script setup lang="ts">
// 定义props
const props = withDefaults(defineProps<{
    width?: number,
    height?: number,
    previeWidth?: number,
    previeHeight?: number,
    previeRadius?: string,
}>(), {
    width: 60,
    height: 60,
    previeWidth: 60,
    previeHeight: 60,
    previeRadius: '0px'
});

// 定义事件
const emit = defineEmits<{
    // 删除事件
    (e: 'returnImage', image: string): void;
}>();

// 图片编辑对话框
const imageDialog = ref<typeof MessageDialog>();
const imageEditorSize = ref({
    width: 900,
    height: 450
});

const imageDialogStyle = computed(() => {
    return `
        --editorBox-width: ${imageEditorSize.value.width}px;
        --editorBox-height: ${imageEditorSize.value.height}px;
        --selectWidth: ${props.width}px;
        --selectHeight: ${props.height}px;
        --previeWidth: ${props.previeWidth}px;
        --previeHeight: ${props.previeHeight}px;
        --previeRadius: ${props.previeRadius};
        --selectZoomWidth: ${props.width + 20}px;
        --selectZoomHeight: ${props.height + 20}px;
    `;
});

const imageProcessing = ref(new ImageProcessing());
const imageBase64 = ref('');

// 缩放比例
const Resize = ref(1);
// 预览图片
const previewImage = ref<HTMLElement | null>(null);

// 正在编辑的图片
const imageEditorBoxImage = ref<HTMLElement | null>(null);
// 正在编辑的图片位置
const imageEditorBoxImageStyle = ref(`
    top: 0px;
    left: 0px;
`);
// 可拖拽的位置
const imageEditorBoxSelect = ref({
    minWidth: 0,
    minHeight: 0,
    maxWidth: 0,
    maxHeight: 0
});

// 拖拽时的选择框
const imageSelect = ref<HTMLElement | null>(null);
// 选择大小  监听 props 变化并进行处理
const imageSelectMinSize = computed(() => {
    return { width: props.width, height: props.height,
        previeWidth: props.previeWidth / props.width, previeHeight: props.previeHeight / props.height
    };
});

// 拖拽时的选择框的缩放框
const selectZoom = ref<HTMLElement | null>(null);
// 关闭对话框
const closeimageDialog = () => {
    imageDialog.value?.Close();
};

// 打开图片
const openImage = async () => {
    let imageProcessingTemp = new ImageProcessing();
    await imageProcessingTemp.SetImage();
    imageProcessing.value = imageProcessingTemp;
};

// 初始化图片
const initImage = async () => {
    // 初始化缩放比例
    let resize = 1;
    
    // 如果大于编辑框
    if(imageProcessing.value.transform.width > imageEditorSize.value.width || imageProcessing.value.transform.height > imageEditorSize.value.height)
        resize = Math.min(imageEditorSize.value.width / imageProcessing.value.transform.width, imageEditorSize.value.height / imageProcessing.value.transform.height);
    
    // 如果小于最小大小
    if(imageProcessing.value.transform.width < imageSelectMinSize.value.width || imageProcessing.value.transform.height < imageSelectMinSize.value.height)
        resize = Math.max(imageSelectMinSize.value.width / imageProcessing.value.transform.width, imageSelectMinSize.value.height / imageProcessing.value.transform.height);
    
    // 设置缩放
    await imageProcessing.value.SetResizeRotate(resize, imageProcessing.value.transform.rotate);
    // 更新图片的 base64 数据
    imageBase64.value = await imageProcessing.value.GetBase64();
};

// 初始化大小信息
const initSize = () => {
    // 设置缩放比例
    Resize.value = 1;
    // 等待 DOM 更新完成
    nextTick(() => {
        if (imageEditorBoxImage.value) {
            // 获取元素的位置信息和尺寸
            const width = imageEditorBoxImage.value.offsetWidth;
            const height = imageEditorBoxImage.value.offsetHeight;
            // 设置选择框的最小尺寸
            const minWidth = (imageEditorSize.value.width / 2) - (width / 2);
            const minHeight = (imageEditorSize.value.height / 2) - (height / 2);
            // 设置选择框的最大尺寸
            const maxWidth = minWidth + width;
            const maxHeight = minHeight + height;
            
            // 设置正在编辑的图片位置
            imageEditorBoxImageStyle.value = `
                top: ${minHeight}px;
                left: ${minWidth}px;
            `;
            
            // 设置大小位置限制
            imageEditorBoxSelect.value = { minWidth, minHeight, maxWidth, maxHeight };
            
            // 移动选择框到中心
            setSelectMov(
                ((maxWidth - minWidth) / 2 + minWidth) - imageSelectMinSize.value.width / 2,
                (maxHeight - minHeight) / 2 + minHeight - imageSelectMinSize.value.height / 2
            );
            // 设置选择框大小
            setSelectSize(imageSelectMinSize.value.width, imageSelectMinSize.value.height);
        }
    });
};

// 当文件选择时，读取文件并设置到 ImageProcessing 类
const showimageDialog = async () => {
    try {
        // 打开图片
        await openImage();
        // 初始化图片
        await initImage();
        // 调用 showModal 方法打开对话框
        imageDialog.value?.ShowModal();
        // 初始化大小信息
        initSize();
    } catch (error) { }
};

// 设置拖动框位置
const setSelectMov = (left: number, top: number) => {
    const img = imageSelect.value?.querySelector('img'); // 获取父元素中的 img 子元素
    if (!imageSelect.value || !selectZoom.value || !previewImage.value || !img) return;
    // 移动 img 元素，相对父元素
    img.style.left = `-${left - imageEditorBoxSelect.value.minWidth}px`;
    img.style.top = `-${top - imageEditorBoxSelect.value.minHeight}px`;
    
    //移动当前元素
    imageSelect.value.style.left = `${left}px`;
    imageSelect.value.style.top = `${top}px`;

    // 移动放大镜
    selectZoom.value.style.left = `${left - 10}px`;
    selectZoom.value.style.top = `${top - 10}px`;

    // 移动预览图片
    previewImage.value.style.left = `-${(left - imageEditorBoxSelect.value.minWidth) * imageSelectMinSize.value.previeWidth * Resize.value}px`;
    previewImage.value.style.top = `-${(top - imageEditorBoxSelect.value.minHeight) * imageSelectMinSize.value.previeHeight * Resize.value}px`;
};

// 元素拖动事件
const dragx = (el: MouseEvent) => {
    if (!imageSelect.value) return;

    // 鼠标按下，计算当前元素距离可视区的距离
    let disX = el.clientX - imageSelect.value.offsetLeft;
    let disY = el.clientY - imageSelect.value.offsetTop;
        
    document.onmousemove = function (e) {
        if (!imageSelect.value) return;
        // 通过事件委托，计算移动的距离
        let left = e.clientX - disX;
        let top = e.clientY - disY;
        // 获取元素大小
        let width = imageSelect.value.offsetWidth;
        let height = imageSelect.value.offsetHeight;
        if (left < imageEditorBoxSelect.value.minWidth) { // 如果小于最小宽度，则不改变位置
            left = imageEditorBoxSelect.value.minWidth;
        } else if (left > imageEditorBoxSelect.value.maxWidth - width) { // 如果大于最大宽度，则不改变位置
            left =  imageEditorBoxSelect.value.maxWidth - width;
        }
        // 同理处理y轴
        if (top < imageEditorBoxSelect.value.minHeight) {
            top = imageEditorBoxSelect.value.minHeight;
        } else if (top > imageEditorBoxSelect.value.maxHeight - height) {
            top = imageEditorBoxSelect.value.maxHeight - height;
        }
        // 设置拖动位置
        setSelectMov(left, top);
    };
    document.onmouseup = function () {
        // 鼠标松开时，移除事件
        document.onmousemove = null;
        document.onmouseup = null;
    };
    // 禁用默认拖拽行为
    document.ondragstart = function (ev) {
        ev.preventDefault();
    };
    document.ondragend = function (ev) {
        ev.preventDefault();
    };
    return false;
};

// 设置缩放
const setSelectSize = (width: number, height: number) => {
    if (!imageSelect.value || !selectZoom.value || !previewImage.value || !imageEditorBoxImage.value) return;
    // 缩放选择器
    imageSelect.value.style.width = `${width}px`;
    imageSelect.value.style.height = `${height}px`;
    // 缩放放大镜
    selectZoom.value.style.width = `${width + 20}px`;
    selectZoom.value.style.height = `${height + 20}px`;
    
    // 更新缩放比例
    Resize.value = imageSelectMinSize.value.width / width;
    // 更新预览图片尺寸
    previewImage.value.style.width = `${imageEditorBoxImage.value.offsetWidth * imageSelectMinSize.value.previeWidth * Resize.value }px;`;
    previewImage.value.style.height = `${imageEditorBoxImage.value.offsetHeight * imageSelectMinSize.value.previeHeight * Resize.value }px`;
};

// 缩放功能实现
const resize = (event: MouseEvent, direction : string) => {
    if (!imageSelect.value) return;
    // 记录鼠标起始位置
    const startX = event.clientX;
    const startY = event.clientY;
    // 记录元素起始宽高
    const startWidth = imageSelect.value.offsetWidth;
    const startHeight = imageSelect.value.offsetHeight;
    // 记录元素起始位置
    const startLeft = imageSelect.value.offsetLeft;
    const startTop = imageSelect.value.offsetTop;
    document.onmousemove = function (e) {
        // 计算鼠标移动的偏移量
        let deltaX = e.clientX - startX;
        let deltaY = e.clientY - startY;
        // 目标尺寸比例
        let ratio = imageSelectMinSize.value.width / imageSelectMinSize.value.height;
        // 初始化新宽高和新位置
        let newWidth = startWidth;
        let newHeight = startHeight;
        let newLeft = startLeft;
        let newTop = startTop;
        if (direction.includes('Right')) {
            if (direction.includes('Bottom')) // 右下角调整
            {
                // 判断缩放尺寸
                let delta = Math.max(Math.abs(e.clientX - startX), Math.abs(e.clientY - startY));
                // 将 放大时的 delta 调整为正数，缩小时的 delta 为负数
                if( (Math.abs(e.clientX - startX) > Math.abs(e.clientY - startY) && deltaX < 0) ||
                    (Math.abs(e.clientX - startX) < Math.abs(e.clientY - startY) && deltaY < 0)
                ) delta = -delta;
                // 限制大小
                if(startWidth + delta < imageSelectMinSize.value.width) delta = imageSelectMinSize.value.width - startWidth;
                if(startHeight + (delta / ratio) < imageSelectMinSize.value.height) delta = (imageSelectMinSize.value.height - startHeight) * ratio;
                
                // 限制边界
                if(newLeft + startWidth + delta > imageEditorBoxSelect.value.maxWidth) 
                    delta = imageEditorBoxSelect.value.maxWidth - startWidth - newLeft;
                if(newTop + startHeight + (delta / ratio) > imageEditorBoxSelect.value.maxHeight)
                    delta = (imageEditorBoxSelect.value.maxHeight - startHeight - newTop) * ratio;
                // 更新宽高
                newWidth = startWidth + delta;
                newHeight = startHeight + (delta / ratio);
            } else if (direction.includes('Top')) // 右上角调整
            {
                // 判断缩放尺寸
                let delta = Math.max(Math.abs(e.clientX - startX), Math.abs(e.clientY - startY));
                // 将 放大时的 delta 调整为正数，缩小时的 delta 为负数
                if( (Math.abs(e.clientX - startX) > Math.abs(e.clientY - startY) && deltaX < 0) ||
                    (Math.abs(e.clientX - startX) < Math.abs(e.clientY - startY) && deltaY > 0) // deltaY > 0 因为放大调整的时候，deltaY 的值是负的
                ) delta = -delta;
                // 限制大小
                if(startWidth + delta < imageSelectMinSize.value.width) delta = imageSelectMinSize.value.width - startWidth;
                if(startHeight + (delta / ratio) < imageSelectMinSize.value.height) delta = (imageSelectMinSize.value.height - startHeight) * ratio;
                // 限制边界
                if(newLeft + startWidth + delta > imageEditorBoxSelect.value.maxWidth) 
                    delta = imageEditorBoxSelect.value.maxWidth - startWidth - newLeft;
                if(startTop - (delta / ratio) < imageEditorBoxSelect.value.minHeight)
                    delta = (startTop - imageEditorBoxSelect.value.minHeight) * ratio;
                // 更新宽高
                newWidth = startWidth + delta;
                newHeight = startHeight + (delta / ratio);
                // 更新位置，保证其余边位置不变
                newTop = startTop - (delta / ratio);
            }
        } else if (direction.includes('Left')) {
            if (direction.includes('Bottom')) // 左下角调整
            {
                // 判断缩放尺寸
                let delta = Math.max(Math.abs(e.clientX - startX), Math.abs(e.clientY - startY));
                if( (Math.abs(e.clientX - startX) > Math.abs(e.clientY - startY) && deltaX > 0) || // deltaX > 0 因为放大调整的时候，deltaX 的值是负的
                    (Math.abs(e.clientX - startX) < Math.abs(e.clientY - startY) && deltaY < 0)
                ) delta = -delta;
                // 限制大小
                if(startWidth + delta < imageSelectMinSize.value.width) delta = imageSelectMinSize.value.width - startWidth;
                if(startHeight + (delta / ratio) < imageSelectMinSize.value.height) delta = (imageSelectMinSize.value.height - startHeight) * ratio;
                // 限制边界
                if(newLeft - delta < imageEditorBoxSelect.value.minWidth)
                    delta = startLeft - imageEditorBoxSelect.value.minWidth;
                if(newTop + startHeight + (delta / ratio) > imageEditorBoxSelect.value.maxHeight)
                    delta = (imageEditorBoxSelect.value.maxHeight - startHeight - newTop) * ratio;
                // 更新宽高
                newWidth = startWidth + delta;
                newHeight = startHeight + (delta / ratio);
                // 更新位置，保证其余边位置不变
                newLeft = startLeft - delta;
            } else if (direction.includes('Top')) // 左上角调整
            {
                // 判断缩放尺寸
                let delta = Math.max(Math.abs(e.clientX - startX), Math.abs(e.clientY - startY));
                // 将 放大时的 delta 调整为正数，缩小时的 delta 为负数
                if( (Math.abs(e.clientX - startX) > Math.abs(e.clientY - startY) && deltaX > 0) || // deltaX > 0 因为放大调整的时候，deltaX 的值是负的
                    (Math.abs(e.clientX - startX) < Math.abs(e.clientY - startY) && deltaY > 0) // deltaY > 0 因为放大调整的时候，deltaY 的值是负的
                ) delta = -delta;
                // 限制大小
                if(startWidth + delta < imageSelectMinSize.value.width) delta = imageSelectMinSize.value.width - startWidth;
                if(startHeight + (delta / ratio) < imageSelectMinSize.value.height) delta = (imageSelectMinSize.value.height - startHeight) * ratio;
                // 限制边界
                if(newLeft - delta < imageEditorBoxSelect.value.minWidth)
                    delta = startLeft - imageEditorBoxSelect.value.minWidth;
                if(startTop - (delta / ratio) < imageEditorBoxSelect.value.minHeight)
                    delta = (startTop - imageEditorBoxSelect.value.minHeight) * ratio;
                // 更新宽高
                newWidth = startWidth + delta;
                newHeight = startHeight + (delta / ratio);
                // 更新位置，保证其余边位置不变
                newLeft = startLeft - delta;
                newTop = startTop - (delta / ratio);
            }
        }
        // 更新元素样式
        setSelectSize(newWidth, newHeight);
        setSelectMov(newLeft, newTop);
    };
    document.onmouseup = function () {
        // 鼠标松开时，移除事件
        document.onmousemove = null;
        document.onmouseup = null;
    };
    // 禁用默认拖拽行为
    document.ondragstart = function (ev) {
        ev.preventDefault();
    };
    document.ondragend = function (ev) {
        ev.preventDefault();
    };
    return false;
};

// 重置
const resetImage = async () => {
    await imageProcessing.value.Restore();
    // 初始化图片
    await initImage();
    // 初始化大小信息
    initSize();
};

// 确认裁剪图片
const cropImage = async () => {
    if (!imageSelect.value) return;
    let startLeft = imageSelect.value.offsetLeft - imageEditorBoxSelect.value.minWidth;
    let startTop = imageSelect.value.offsetTop - imageEditorBoxSelect.value.minHeight;
    const startWidth = imageSelect.value.offsetWidth;
    const startHeight = imageSelect.value.offsetHeight;
    
    // 裁剪
    await imageProcessing.value.Cropping(startLeft, startTop, startLeft + startWidth, startTop + startHeight);
    imageBase64.value = await imageProcessing.value.GetBase64();
    // 缩放倍数
    await imageProcessing.value.SetResizeRotate(Resize.value, 0);
    let returnImage = await imageProcessing.value.GetBase64() as string;
    
    // 触发关闭事件
    emit('returnImage', returnImage);
    closeimageDialog();
};

// 公开函数
const Show = () => {
    showimageDialog();
};

const Close = () => {
    closeimageDialog();
};

defineExpose({
    Show, Close
});
</script>

<style scoped>
/* 图片编辑 */
.imageEditor {
    overflow: hidden;
}
/* 按钮列表 */
.imageEditor .editorButton {
    padding: 0px 20px;
    width: 100%;
    margin-bottom: 10px;
    font-size: 16px;
    display: flex;
    justify-content: space-between;
    align-items: center;
}

/* 效果预览 */
.imageEditor .editorButton .imagePreview {
    display: flex;
    justify-content: center;
    align-items: center;
}
.imageEditor .editorButton .imagePreview .previewText, .imageEditor .editorButton .rotate .rotateText {
    padding-right: 10px;
}
.imageEditor .editorButton .imagePreview .image {
    width: var(--previeWidth);
    height: var(--previeHeight);
    border-radius: var(--previeRadius);
    border: 2px solid rgba(65, 185, 255, 1);
    position: relative;
    overflow: hidden;
}
.imageEditor .editorButton .imagePreview .image img {
    position: absolute;
}

/* 旋转按钮 */
.imageEditor .editorButton .rotate {
    display: flex;
    justify-content: center;
    align-items: center;
}
.imageEditor .editorButton .rotate .button {
    width: 40px;
    height: 40px;
    font-size: 28px;
    border-radius: 50%;
    background-color: rgba(65, 185, 255, 1);
    color: rgba(255, 255, 255, 1);
    margin-right: 10px;
    display: flex;
    justify-content: center;
    align-items: center;
    cursor: pointer;
    transition: all .3s;
}

.imageEditor .editorButton .reset, .imageEditor .editorButton .resetSelect {
    padding: 5px 15px;
    border-radius: 5px;
    transition: all .3s;
    background-color: rgba(65, 185, 255, 1);
    color: rgba(255, 255, 255, 1);
    display: flex;
    justify-content: center;
    align-items: center;
}

/* 按钮样式 */
.imageEditor .editorButton .button .iconfont {
    padding-right: 5px;
    font-size: 24px;
}
.imageEditor .editorButton .button:hover {
    background-color: rgba(65, 185, 255, .8);
}
.imageEditor .editorButton .button:active {
    background-color: rgba(65, 185, 255, .9);
    transition: all .1s;
}

/* 编辑内容 */
.imageEditor .editorBox {
    width: var(--editorBox-width);
    height: var(--editorBox-height);
    margin: 0px 10px;
    overflow: hidden;
    position: relative;
    /* 背景色设置成图片空像素样式 */
    background-image: 
        linear-gradient(45deg, rgba(200, 200, 200, .5) 25%, transparent 25%, transparent 75%, rgba(200, 200, 200, .5) 75%, rgba(200, 200, 200, .5)),
        linear-gradient(45deg, rgba(200, 200, 200, .5) 25%, transparent 25%, transparent 75%, rgba(200, 200, 200, .5) 75%, rgba(200, 200, 200, .5));
    background-size: 18px 18px;
    background-position: 0 0, 9px 9px;
}
.imageEditor .editorBox img {
    position: absolute;
    /* max-width: var(--editorBox-width);
    max-height: var(--editorBox-height); */
    border: none;
    /* 内边距 */
    padding: 0px;
    margin: 0px;
    /* 内边框 */
    border-width: 0px;
    outline: none;
}

/* 图片编辑选框 */
.imageEditor .editorBox .imageBox {
    width: 100%;
    height: 100%;
    background: rgba(100, 100, 100, 0.7); /* 半透明效果 */
    position: relative;
    z-index: 2;
}
.imageEditor .editorBox .imageBox .select {
    position: absolute; /* 保留普通流布局 */
    width: var(--selectWidth);
    height: var(--selectHeight);
    overflow: hidden;
    background: transparent;
    border: 2px solid rgba(0, 162, 255, 1); /* 确保透明区域不被覆盖 */
    z-index: 10; /* 确保位于模糊区域之上 */
    cursor: move;
}
.imageEditor .editorBox .imageBox .zoom {
    position: absolute; /* 保留普通流布局 */
    width: var(--selectZoomWidth);
    height: var(--selectZoomHeight);
    overflow: hidden;
    background: transparent;
    z-index: 8; /* 确保位于模糊区域之上 */
}
/* 缩放控制点 */
.imageEditor .editorBox .imageBox .zoom .zoomrLeft, .imageEditor .editorBox .imageBox .zoom .zoomrRight {
    position: absolute;
    width: 100%;
    height: 100%;
}
.imageEditor .editorBox .imageBox .zoom .zoomrLeft .top,
.imageEditor .editorBox .imageBox .zoom .zoomrLeft .bottom,
.imageEditor .editorBox .imageBox .zoom .zoomrRight .top,
.imageEditor .editorBox .imageBox .zoom .zoomrRight .bottom {
    content: "";
    display: inline-block;
    width: 12px;
    height: 12px;
    position: absolute;
    z-index: 12;
    border: 2px solid rgba(0, 162, 255, 1); /* 确保透明区域不被覆盖 */
}
.imageEditor .editorBox .imageBox .zoom .zoomrLeft .top
{
    top: 0px;
    left: 0px;
    cursor: nwse-resize;
}
.imageEditor .editorBox .imageBox .zoom .zoomrLeft .bottom
{
    bottom: 0px;
    left: 0px;
    cursor: nesw-resize;
}
.imageEditor .editorBox .imageBox .zoom .zoomrRight .top
{
    top: 0px;
    right: 0px;
    cursor: nesw-resize;
}
.imageEditor .editorBox .imageBox .zoom .zoomrRight .bottom
{
    bottom: 0px;
    right: 0px;
    cursor: nwse-resize;
}

.imageEditor .editorBox .imageBox .select::after {
    content: "";
    display: inline-block;
    width: 100%;
    height: 100%;
    position: absolute;
    background: rgba(255, 255, 255, 0);
}
</style>