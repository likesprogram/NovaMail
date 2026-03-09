<template>
  <div ref="contentRef" class="safePopover">
    <slot></slot>
  </div>
</template>

<script lang="ts">
import { ref, onMounted, onBeforeUnmount, defineComponent } from 'vue';
import { Panel } from '@/utils/panelToggle/panelToggle';
export default defineComponent({
    name: 'SafePopover',
});
</script>

<script setup lang="ts">
const contentRef = ref<HTMLElement | null>(null);
let panel: Panel | null = null;
let safeElements: HTMLElement[] = [];
let panelElement: HTMLElement | null = null;
let parentEl: HTMLElement | null = null;
let resizeHandler: (() => void) | null = null;

const props = withDefaults(defineProps<{
    NoBoxShadow?: string[],
    BorderRadius?: number,
    Width?: boolean | string,
    Height?: boolean | string,
}>(), {
    NoBoxShadow: () => [],
    BorderRadius: 5,
    Width: false,
    Height: false,
});

/**
 * 显示 Popover
 * @param parent 触发源（如按钮）
 * @param safeEls 额外安全元素（点击这些不会关闭）
 */
const show = (parent: HTMLElement, ...safeEls: HTMLElement[]) => {
    if (!contentRef.value) return;

    parentEl = parent;
    safeElements = [parent, ...safeEls];

    // 若尚未初始化 panel，则创建
    if (!panel) {
        const element = panelElement ?? contentRef.value;
        panel = new Panel(element, parent, {
            animation: 'slide',
            duration: 300,
        });
    }

    // 更新一次尺寸
    updateSize();

    // 注册 window resize 事件（每次显示时）
    if (!resizeHandler) {
        resizeHandler = () => updateSize();
        window.addEventListener('resize', resizeHandler);
    }

    // 相对 parent 自动定位
    panel.show({ top: '100%', left: '50%', transform: 'translateX(-50%)' });

    // 防止透明度动画残留
    contentRef.value.style.display = 'block';
};

/** 更新宽高逻辑 */
const updateSize = () => {
    if (!contentRef.value || !parentEl) return;

    const parentRect = parentEl.getBoundingClientRect();

    // === Width ===
    if (props.Width === true) {
        contentRef.value.style.width = `${parentRect.width}px`;
    } else if (typeof props.Width === 'string') {
        if (props.Width.endsWith('%')) {
            const percent = parseFloat(props.Width);
            contentRef.value.style.width = `${(parentRect.width * percent) / 100}px`;
        } else if (props.Width.endsWith('px')) {
            contentRef.value.style.width = props.Width;
        }
    } else {
        contentRef.value.style.width = '';
    }

    // === Height ===
    if (props.Height === true) {
        contentRef.value.style.height = `${parentRect.height}px`;
    } else if (typeof props.Height === 'string') {
        if (props.Height.endsWith('%')) {
            const percent = parseFloat(props.Height);
            contentRef.value.style.height = `${(parentRect.height * percent) / 100}px`;
        } else if (props.Height.endsWith('px')) {
            contentRef.value.style.height = props.Height;
        }
    } else {
        contentRef.value.style.height = '';
    }
};

/** 隐藏 */
const hide = () => {
    panel?.hide();
    safeElements = [];

    // 移除 resize 监听（仅在隐藏时）
    if (resizeHandler) {
        window.removeEventListener('resize', resizeHandler);
        resizeHandler = null;
    }
};

/** 点击外部关闭 */
const handleClickOutside = (event: MouseEvent) => {
    const clicked = event.target as HTMLElement;
    if (!contentRef.value) return;

    // 点击自己或安全区域不关闭
    if (contentRef.value.contains(clicked)) return;
    for (const el of safeElements) {
        if (el.contains(clicked)) return;
    }

    hide();
};

onMounted(() => {
    document.addEventListener('click', handleClickOutside);

    if (contentRef.value) {
        contentRef.value.style.setProperty('--popover-radius', `${props.BorderRadius}px`);
    }

    // === 初始化 NoBoxShadow 包装层 ===
    if (props.NoBoxShadow && props.NoBoxShadow.length > 0) {
        panelElement = document.createElement('div');
        panelElement.style.overflow = 'hidden';
        panelElement.style.position = 'relative';
        panelElement.style.display = 'block';
        panelElement.style.padding = '5px';
        if (contentRef.value) panelElement.appendChild(contentRef.value);

        for (const el of props.NoBoxShadow) {
            if (el === 'left') {
                panelElement.style.paddingLeft = '0px';
                contentRef.value?.style.setProperty('--popover-radius', `0px ${props.BorderRadius}px ${props.BorderRadius}px 0px`);
            } else if (el === 'right') {
                panelElement.style.paddingRight = '0px';
                contentRef.value?.style.setProperty('--popover-radius', `${props.BorderRadius}px 0px 0px ${props.BorderRadius}px`);
            } else if (el === 'top') {
                panelElement.style.paddingTop = '0px';
                contentRef.value?.style.setProperty('--popover-radius', `0px 0px ${props.BorderRadius}px ${props.BorderRadius}px`);
            } else if (el === 'bottom') {
                panelElement.style.paddingBottom = '0px';
                contentRef.value?.style.setProperty('--popover-radius', `${props.BorderRadius}px ${props.BorderRadius}px 0px 0px`);
            } else if (el === 'all') {
                contentRef.value?.style.setProperty('--popover-radius', `${props.BorderRadius}px`);
            }
        }
    }
});

onBeforeUnmount(() => {
    document.removeEventListener('click', handleClickOutside);
    hide(); // hide 中已负责移除 resize
});

defineExpose({ show, hide });
</script>

<style scoped>
.safePopover {
    position: relative;
    border-radius: var(--popover-radius);
    box-shadow: var(--shadow-card);
    display: none;
    overflow: hidden;
}
</style>
