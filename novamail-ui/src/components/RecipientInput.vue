<template>
    <div class="recipientInput" @click="focusInput">
        <!-- 已完成的邮箱块 -->
        <template v-for="(email, index) in innerValue" :key="email + index">
            <!-- 普通块状态：只显示 @ 前面的内容 -->
            <span v-if="editingIndex !== index" class="recipientTag" :title="email" @click.stop="startEdit(index)">
                {{ getEmailName(email) }}
                <button class="removeButton" type="button" @click.stop="removeEmail(index)">
                    <span class="iconfont icon-likesprogramcuowu"></span>
                </button>
            </span>

            <!-- 编辑状态：显示完整邮箱 -->
            <input v-else ref="editInputRef" v-model="editingText" class="recipientEditInput" :style="editInputStyle" type="text"
                @click.stop="() => {}" @keydown="handleEditKeydown" @blur="finishEdit"
            />
        </template>

        <!-- 继续输入邮箱 -->
        <input ref="inputRef" v-model="inputText" class="recipientTextInput" type="text"
            @input="handleInput" @keydown="handleKeydown" @paste="handlePaste" @blur="handleBlur"
        />
    </div>
</template>

<script setup lang="ts">
import { computed, nextTick, ref, watch } from 'vue';

const props = defineProps<{
    modelValue: string[];
}>();

const emit = defineEmits<{
    'update:modelValue': [value: string[]];
}>();
const editInputStyle = computed(() => {
    const length = editingText.value.length || 1;

    return {
        width: `${Math.min(Math.max(length + 3, 6), 40)}ch`
    };
});
const inputRef = ref<HTMLInputElement | null>(null);
const editInputRef = ref<HTMLInputElement[] | null>(null);

const inputText = ref('');
const editingText = ref('');
const editingIndex = ref<number | null>(null);

const innerValue = ref<string[]>([...props.modelValue]);

watch(
    () => props.modelValue,
    (value) => {
        innerValue.value = [...value];
    }
);

const updateValue = () => {
    emit('update:modelValue', [...innerValue.value]);
};

const focusInput = () => {
    inputRef.value?.focus();
};

const getEmailName = (email: string) => {
    const index = email.indexOf('@');
    if (index === -1) return email;
    return email.substring(0, index);
};

const normalizeEmail = (email: string) => {
    return email.trim();
};

const isValidEmail = (email: string) => {
    return /^[^\s@;；]+@[^\s@;；]+\.[^\s@;；]+$/.test(email);
};

const addEmail = (email: string) => {
    const normalized = normalizeEmail(email);
    if (!normalized) return;

    // 邮箱格式校验
    if (!isValidEmail(normalized)) {
        inputText.value = normalized;
        return;
    }

    const exists = innerValue.value.some(
        item => item.toLowerCase() === normalized.toLowerCase()
    );

    if (!exists) {
        innerValue.value.push(normalized);
        updateValue();
    }
};

const parseEmails = (text: string, commitLast = false) => {
    const parts = text.split(/[;；]/);

    const completedParts = commitLast ? parts : parts.slice(0, -1);
    const lastPart = commitLast ? '' : (parts[parts.length - 1] ?? '');

    completedParts.forEach(item => {
        addEmail(item);
    });

    inputText.value = lastPart.trim();
};

const handleInput = () => {
    if (/[;；]/.test(inputText.value)) {
        parseEmails(inputText.value);
    }
};

const handleKeydown = (event: KeyboardEvent) => {
    if (event.key === 'Enter') {
        event.preventDefault();
        if (inputText.value.trim()) {
            addEmail(inputText.value);
            inputText.value = '';
        }
        return;
    }

    // 英文分号
    if (event.key === ';') {
        event.preventDefault();
        if (inputText.value.trim()) {
            addEmail(inputText.value);
            inputText.value = '';
        }
        return;
    }

    // 输入框为空时，按退格键，把最后一个邮箱恢复成可编辑文本
    if (event.key === 'Backspace' && !inputText.value && innerValue.value.length > 0) {
        const lastEmail = innerValue.value.pop()!;
        inputText.value = lastEmail;
        updateValue();

        nextTick(() => {
            inputRef.value?.focus();
        });
    }
};

const handlePaste = (event: ClipboardEvent) => {
    const text = event.clipboardData?.getData('text');
    if (!text) return;

    if (/[;；\n\r,\s]/.test(text)) {
        event.preventDefault();

        const emails = text
            .split(/[;；\n\r,\s]+/)
            .map(item => item.trim())
            .filter(Boolean);

        emails.forEach(email => addEmail(email));
        inputText.value = '';
    }
};

const handleBlur = () => {
    // 失去焦点时，如果输入框里还有完整邮箱，也自动转换成块
    if (inputText.value.trim() && isValidEmail(inputText.value.trim())) {
        addEmail(inputText.value);
        inputText.value = '';
    }
};

const removeEmail = (index: number) => {
    innerValue.value.splice(index, 1);
    updateValue();

    nextTick(() => {
        inputRef.value?.focus();
    });
};

const startEdit = (index: number) => {
    editingIndex.value = index;
    editingText.value = innerValue.value[index] ?? '';

    nextTick(() => {
        const input = editInputRef.value?.[0];
        input?.focus();
    });
};

const finishEdit = () => {
    if (editingIndex.value === null) return;

    const value = normalizeEmail(editingText.value);

    if (value && isValidEmail(value)) {
        innerValue.value[editingIndex.value] = value;
        updateValue();
    }

    editingIndex.value = null;
    editingText.value = '';

    nextTick(() => {
        inputRef.value?.focus();
    });
};

const handleEditKeydown = (event: KeyboardEvent) => {
    if (event.key === 'Enter' || event.key === ';') {
        event.preventDefault();
        finishEdit();
    }

    if (event.key === 'Escape') {
        editingIndex.value = null;
        editingText.value = '';
        nextTick(() => {
            inputRef.value?.focus();
        });
    }
};
</script>

<style scoped>
.recipientInput {
    width: 100%;
    height: 28px;
    padding: 0px 10px;
    display: flex;
    align-items: center;
    flex-wrap: wrap;
    gap: 6px;
    border: none;
    cursor: text;
    box-sizing: border-box;
}

.recipientTag {
    max-width: 180px;
    height: 100%;
    padding: 0 8px;
    display: inline-flex;
    align-items: center;
    gap: 6px;
    border-radius: 5px;
    color: var(--text-primary);
    font-size: 14px;
    cursor: pointer;
    user-select: none;
    background-color: var( --bg-secondary3);
    transition: all .3s;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.recipientTag:hover {
    background-color: var(--bg-hover-primary);
}.recipientTag:active {
    background-color: var(--bg-active-primary);
}

.removeButton {
    border: none;
    outline: none;
    background: transparent;
    color: var(--text-secondary, #999);
    cursor: pointer;
    font-size: 14px;
    line-height: 1;
    padding: 0;
}

.removeButton:hover {
    color: #f56c6c;
}

.recipientTextInput {
    min-width: 160px;
    flex: 1;
    height: 100%;
    padding: 0px;
    border: none;
    outline: none;
    background: transparent;
    font-size: 14px;
    color: var(--text-primary, #333);
}

.recipientEditInput {
    flex: none;
    min-width: 6ch;
    max-width: 40ch;
    height: 100%;
    padding: 0 6px;
    border: none;
    outline: none;
    border-radius: 4px;
    background: transparent;
    font-size: 14px;
    color: var(--text-primary, #333);
    box-sizing: border-box;
}
</style>