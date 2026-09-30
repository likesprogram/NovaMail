<template>
    <div class="actionBar" ref="barRef">
        <!-- 可见的 actionItems -->
        <div v-for="(actionItem, index) in actionItems"
            :key="actionItem.id ? actionItem.id  : actionItem.title + '-' + index"
            class="actionItem" :style="visibleItems.includes(actionItem) ? '' : 'visibility: hidden'"
            @click="handleClick(actionItem, $event)">
            <span class="actionIcon iconfont" :class="actionItem.icon"></span>
            <span class="actionTitle">{{ actionItem.title }}</span>
        </div>

        <!-- 更多按钮 -->
        <div ref="moreBtnRef" class="actionItem moreButton" @click="showMore = !showMore">
            <span class="actionIcon iconfont icon-likesprogramgengduo1"></span>
            <span class="actionTitle">更多</span>
            <!-- 下拉菜单 -->
            <div v-if="showMore" class="moreMenu">
              <div
                v-for="(actionItem, index) in hiddenItems"
                :key="actionItem.id ? actionItem.id : actionItem.title + '-' + index"
                class="moreItem"
                @click="handleClick(actionItem, $event)"
              >
                <span class="actionIcon iconfont" :class="actionItem.icon"></span>
                <span class="actionTitle">{{ actionItem.title }}</span>
              </div>
            </div>
        </div>
    </div>
</template>
<script lang="ts">
import { defineComponent, nextTick, onBeforeUnmount, onMounted, ref, watch } from 'vue';
export default defineComponent({
    name: 'ActionBar',
});
// 定义 ActionItem 接口
export interface ActionItem {
    id?: number | string, // 可选的唯一标识符
    icon: string,
    title: string,
    onClick?: (target?: HTMLElement) => void | Promise<void>, // 点击事件
    [key: string]: any   // 允许多余字段
}
</script>

<script setup lang="ts">
// 定义属性
const props = withDefaults(defineProps<{
    actionItems: ActionItem[];
}>(), {
    actionItems: () => [],
});

// 点击事件
const handleClick = (actionItem: ActionItem, event: MouseEvent) => {
    event.stopPropagation();
    if (actionItem.onClick) actionItem.onClick(event.currentTarget as HTMLElement);
    showMore.value = false;
}

// 响应式计算, 布局
const barRef = ref<HTMLDivElement>();
const moreBtnRef = ref<HTMLDivElement>();
const visibleItems = ref<ActionItem[]>([]);
const hiddenItems = ref<ActionItem[]>([]);
const showMore = ref(false);
const moreWidth = ref(0);
// 计算哪些按钮显示，哪些放入“更多”
const updateVisible = () => {
    if (!barRef.value || !moreBtnRef.value) return;

    const parent = barRef.value.parentElement;
    let containerWidth = barRef.value.clientWidth; // fallback

    if (parent) {
        const style = window.getComputedStyle(parent);
        const paddingLeft = parseFloat(style.paddingLeft) || 0;
        const paddingRight = parseFloat(style.paddingRight) || 0;
        containerWidth = parent.clientWidth - paddingLeft - paddingRight;
    }

    const children = Array.from(barRef.value.children) as HTMLDivElement[];

    let used = 0;
    const tempVisible: ActionItem[] = [];
    const tempHidden: ActionItem[] = [];
    let hiddenStarted = false;

    const style = window.getComputedStyle(barRef.value!);
    const gap = parseFloat(style.gap) || parseFloat(style.columnGap) || parseFloat(style.rowGap) || 0;
    const itemWidths = props.actionItems.map((_item, index) => {
        const el = children[index];
        return el ? el.offsetWidth : 0;
    });
    const totalItemsWidth = itemWidths.reduce((total, width) => total + width, 0)
        + Math.max(props.actionItems.length - 1, 0) * gap;

    if (totalItemsWidth <= containerWidth) {
        visibleItems.value = [...props.actionItems];
        hiddenItems.value = [];
        showMore.value = false;
        moreBtnRef.value!.style.visibility = 'hidden';
        return;
    }

    props.actionItems.forEach((item, idx) => {
        const itemWidth = itemWidths[idx];
        if (!itemWidth) return;

        const nextUsed = tempVisible.length > 0 ? used + gap + itemWidth : itemWidth;
        const remainingItems = props.actionItems.length - idx - 1;
        const reserve = remainingItems > 0 ? moreWidth.value + gap : 0;

        if (hiddenStarted || nextUsed + reserve > containerWidth) {
            tempHidden.push(item);
            hiddenStarted = true;
        } else {
            tempVisible.push(item);
            used = nextUsed;
        }
    });
    visibleItems.value = tempVisible;
    hiddenItems.value = tempHidden;
    moreBtnRef.value!.style.visibility = hiddenItems.value.length > 0 ? 'visible' : 'hidden';
    moreBtnRef.value!.style.left = hiddenItems.value.length > 0
        ? `${tempVisible.length > 0 ? used + gap : 0}px`
        : '0px';
    if (hiddenItems.value.length === 0) showMore.value = false;
};

let observer: ResizeObserver;
// 初始化 + 监听窗口变化
onMounted(() => {
    observer = new ResizeObserver(() => {
        moreWidth.value = moreBtnRef.value?.offsetWidth || 0;
        updateVisible();
    });
    if (barRef.value) observer.observe(barRef.value);
    nextTick(() => {
        moreWidth.value = moreBtnRef.value?.offsetWidth || 0;
        updateVisible();
    });
});
onBeforeUnmount(() => {
    if (observer && barRef.value) observer.unobserve(barRef.value);
});
// 响应 props 改变
watch(() => props.actionItems, () => nextTick(updateVisible), { deep: true });
</script>

<style scoped>
.actionBar {
    width: 100%;
    height: 100%;
    display: flex;
    min-width: 0;
    justify-content: flex-start;
    align-items: center;
    gap: 8px;
    position: relative;
}
.actionBar .actionItem {
    cursor: pointer;
    display: flex;
    justify-content: center;
    align-items: center;
    gap: 4px;
    padding: 4px 7px;
    border-radius: 5px;
    font-size: 14px;
    flex-shrink: 0;
    color: var(--text-primary);
    transition: color 0.3s, background-color 0.3s, border-color 0.3s;
    border: 1px solid var(--border-primary);
    /* 单行文字省略号 */
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.actionBar .actionItem .actionIcon {
    font-size: 20px;
    color: var(--text-primary);
}
.actionBar .actionItem.moreButton {
    position: absolute;
    visibility: hidden;
}
.actionBar .actionItem:hover {
    background-color: var(--bg-hover-primary);
}
.actionBar .actionItem:active {
    transition: all .1s;
    background-color: var(--bg-active-primary);
}
</style>
