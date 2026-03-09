<template>
    <div class="messageDialogBox" ref="messageDialog">
        <div class="messageDialogBackground"  @click="Close" v-if="showModal"></div>
        <div ref="dialog" class="messageDialog" tabindex="-1">
            <header>
                <div class="title" ref="title">
                    <slot name="title">
                        <span>标题</span>
                    </slot>
                </div>
                <span tabindex="0" class="closeButton iconfont icon-likesprogramcuowu" @click="Close"></span>
            </header>
            <main>
                <slot name="content">
                    <div>
                        内容区域
                    </div>
                </slot>
            </main>
            <footer>
                <slot name="footer">
                    <button @click="Show">show显示</button>
                    <button @click="ShowModal">showModal显示</button>
                    <button @click="Close">close关闭</button>
                </slot>
            </footer>
        </div>
    </div>
</template>

<script lang="ts">
import { defineComponent, onMounted, nextTick, ref } from 'vue';
import { Panel } from '@/utils/panelToggle/panelToggle';

export default defineComponent({
    name: 'MessageDialog',
});

export type MessageButton = {
    text: string;
    type: 'primary' | 'failure' | 'warning' | 'success';
    icon?: string; // 按钮图标类名
    iconPosition?: 'left' | 'right'; // 图标位置，默认值为 'left'
    autoClose?: boolean; // 点击按钮后是否自动关闭对话框，默认值为 true
    buttonEvent?: () => void; // 按钮点击事件
};
</script>

<script setup lang="ts">
const show = ref(false); // 是否显示对话框
const showModal = ref(false); // 是否Modal对话框
const messageDialog = ref<HTMLElement | undefined>(undefined);
const title = ref<HTMLElement | undefined>(undefined);
const dialog = ref<HTMLElement | undefined>(undefined);

let panel: Panel | null = null;
onMounted(() => {
    // 将容器添加到 body 中
    if (messageDialog.value) document.body.appendChild(messageDialog.value);
    if (dialog.value) {
        panel = new Panel(dialog.value, messageDialog.value, {
            animation: 'fade',
            duration: 200,
            dragHandle: title.value,
        });
    }
});

// 定义事件
const emit = defineEmits<{
    // 删除事件
    (e: 'closeEvent'): void;
}>();

const Show = () => {
    show.value = true;          // 显示对话框
    showModal.value = false;    // 关闭 Modal 模式
    getFocus(); // 获取焦点
    
    // 移除事件监听
    document.removeEventListener('keydown', handleKeydown);

    // 显示对话框
    panel?.show({center: true});
};
const ShowModal = () => {
    show.value = true;          // 显示对话框
    showModal.value = true;     // 开启 Modal 模式
    getFocus(); // 获取焦点
    
    // 监听键盘事件，确保焦点管理
    document.addEventListener('keydown', handleKeydown);

    // 显示对话框
    panel?.show({center: true});
};
const Close = () => {
    show.value = false;         // 关闭对话框
    showModal.value = false;    // 关闭 Modal 模式

    // 移除事件监听
    document.removeEventListener('keydown', handleKeydown);
    
    // 触发关闭事件
    emit('closeEvent');

    // 隐藏对话框
    panel?.hide();
};
// 获取焦点
const getFocus = () => {
    // 等待DOM更新后再添加事件监听
    nextTick(() => {
        // 获取对话框内的第一个可聚焦元素
        if (!dialog.value) return;
        const focusableElements = getFocusableElements(dialog.value); // 获取所有可聚焦元素
        // 如果存在，将焦点设置到该元素上
        if (focusableElements) {
            focusableElements[0]?.focus();
        }
    });
}
// 捕获 Tab 键的事件，确保焦点循环在对话框内
const handleKeydown = (event: KeyboardEvent) => {
    if (event.key !== 'Tab') return; // 只处理 Tab 键
    
    if (!dialog.value) return;
    const focusableElements = getFocusableElements(dialog.value); // 获取所有可聚焦元素
    
    if (focusableElements.length === 0) return; // 如果没有可聚焦元素，直接返回
    
    const firstElement = focusableElements[0];
    const lastElement = focusableElements[focusableElements.length - 1];
    
    // 如果是 Shift + Tab（向前循环），且当前焦点在第一个元素时
    if (event.shiftKey && document.activeElement === firstElement) {
        event.preventDefault(); // 阻止默认行为
        lastElement!.focus(); // 将焦点移到最后一个元素
    }
    // 普通 Tab（向后循环），且当前焦点在最后一个元素时
    else if (!event.shiftKey && document.activeElement === lastElement) {
        event.preventDefault(); // 阻止默认行为
        firstElement!.focus(); // 将焦点移到第一个元素
    }
};
// 获取模态框中所有可聚焦的元素
const getFocusableElements = (container: HTMLElement): HTMLElement[] => {
    if (!container) return []; // 如果容器不存在，返回空数组
    return Array.from(
        container.querySelectorAll(
            'a, button, input, textarea, select, [tabindex]:not([tabindex="-1"])'
        )
    );
};
// 向父组件公开方法
defineExpose({
    Show,
    ShowModal,
    Close,
});
</script>

<style scoped>
.messageDialogBox {
    z-index: 9999999999;
}

.messageDialogBackground {
    z-index: 1;
    width: 100%;
    height: 100%;
    left: 0px;
    top: 0px;
    position: absolute;
    background-color: rgba(200, 200, 200, .2);
    backdrop-filter: blur(.5rem);
        -webkit-backdrop-filter: blur(.5rem);
        -moz-backdrop-filter: blur(.5rem);
        -ms-backdrop-filter: blur(.5rem);
}

.messageDialog {
    z-index: 2;
    min-width: 200px;
    min-height: 220px;
    border-radius: 8px;
    box-shadow: 0 4px 10px rgba(0, 0, 0, 0.3);
    background: #fff;
    overflow: hidden;
    /* 自动开启纵向滚动条 */
    overflow-y: auto;
    margin: 10px;
    display: flex;
    flex-direction: column;

}

.messageDialog:focus {
    outline: none;
    border: none;
}

/* 标题栏 */
.messageDialog header {
    max-width: 100%;
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 10px;
    padding-left: 15px;
    position: relative;
}
.messageDialog header .title {
    font-size: 20px;
    width: calc(100% - 50px);
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}

/* 关闭按钮 */
.messageDialog header .closeButton {
    font-size: 24px;
    padding: 3px 8px;
    border-radius: 5px;
    cursor: pointer;
    transition: .3s all;
}
.messageDialog header .closeButton:focus {
    border: none;
    outline: none;
}
.messageDialog header .closeButton:hover {
    font-size: 24px;
    padding: 3px 8px;
    border-radius: 5px;
    background-color: rgba(255, 0, 0, 0.3);
}
.messageDialog header .closeButton:active {
    color: rgba(240, 240, 240, 1);
    transition: .1s all;
}

.messageDialog main {
    display: flex;
    justify-content: center;
    align-items: center;
    width: 100%;
    height: 100%;
    /* min-height: 100px; */
    position: relative;
}

.messageDialog footer {
    display: flex;
    justify-content: space-between;
    align-items: center;
    flex-shrink: 0;
    width: 100%;
    height: 60px;
    position: relative;
}
</style>