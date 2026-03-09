<template>
   <MessageDialog ref="messageDialog" @closeEvent="() => {
        if(messageDialogData.messageCloseEvent) {
            try {
                messageDialogData.messageCloseEvent(messageDialogData.selectedButtonIndex);
            } catch (error) {
                console.error('执行消息对话框关闭事件时出错：' + error);
            }
        }
    }">
       <template v-slot:title>
           <span>{{ messageDialogData.title }}</span>
       </template>
       <template v-slot:content>
           <div class="dialogContent">
               <div class="messageBox" :class="messageDialogData.type">
                   <div class="message" v-for="message in messageDialogData.message">{{ message }}</div>
               </div>
           </div>
       </template>
       <template v-slot:footer>
           <div v-for="(button, index) in messageDialogData.buttons" class="dialogFooterButton" :class="button.type" @click="ButtonClick(index, button)">
                <span class="textBox">
                    <span class="iconfont" :class="button.icon" v-if="button.icon && button.iconPosition === 'left'"></span>
                    <span class="text">{{ button.text }}</span>
                    <span class="iconfont" :class="button.icon" v-if="button.icon && button.iconPosition === 'right'"></span>
                </span>
                <span class="line" v-if="index < (messageDialogData.buttons?.length ?? 0) - 1"></span>
           </div>
       </template>
   </MessageDialog>
</template>

<script lang="ts">
import { defineComponent,ref } from 'vue';
import MessageDialog from '@components/messageDialog/MessageDialog.vue';
import type { MessageButton } from '@components/messageDialog/MessageDialog.vue';
import '@components/messageDialog/MessageDialog.css'
export default defineComponent({
    name: 'ShowMessage',
});
</script>

<script setup lang="ts">
    // 消息对话框
    const messageDialog = ref<typeof MessageDialog>();
    const messageDialogData = ref<{ 
        title: string, 
        message: string[], 
        type: 'primary' | 'success' | 'warning' | 'failure',
        buttons?: MessageButton[],
        selectedButtonIndex: number,
        messageCloseEvent?: ((selectedButtonIndex: number) => void),
    }>({
        title: '',
        message: [''],
        type: 'success',
        selectedButtonIndex: -1,
        buttons: [],
        messageCloseEvent: undefined
    });

    // 初始化数据
    const InitMessageDialogData = (title: string, message: string[], type: 'primary' | 'success' | 'warning' | 'failure', messageCloseEvent?: ((selectedButtonIndex: number) => void), buttons?: MessageButton[]) => {
        messageDialogData.value.title = title;
        messageDialogData.value.message = message;
        messageDialogData.value.type = type;
        messageDialogData.value.selectedButtonIndex = -1;
        if((buttons?.length ?? 0) <= 0) {
            messageDialogData.value.buttons = [{
                    text: '取消',
                    type: 'failure',
                    autoClose: true
                }, {
                    text: '确定',
                    type: 'success',
                    autoClose: true
            }];
        } else {
            messageDialogData.value.buttons = buttons;
        }
        
        messageDialogData.value.messageCloseEvent = messageCloseEvent;
    }

    // 按钮点击事件
    const ButtonClick = (index: number, button: MessageButton) => {
        messageDialogData.value.selectedButtonIndex = index;
        if(button.buttonEvent) button.buttonEvent();
        if(button.autoClose === true) {
            Close();
        }
    }

    // 显示对话框
    const Show = (title: string, message: string[], type: 'primary' | 'success' | 'warning' | 'failure', messageCloseEvent?: ((selectedButtonIndex: number) => void), buttons?: MessageButton[]) => {
        InitMessageDialogData(title, message, type, messageCloseEvent, buttons);
        messageDialog.value?.Show();
    }

    // 以模态形式显示对话框
    const ShowModal = (title: string, message: string[], type: 'primary' | 'success' | 'warning' | 'failure', messageCloseEvent?: ((selectedButtonIndex: number) => void), buttons?: MessageButton[]) => {
        InitMessageDialogData(title, message, type, messageCloseEvent, buttons);
        messageDialog.value?.ShowModal();
    }

    // 关闭对话框
    const Close = () => {
        messageDialog.value?.Close();
    }

    // 向父组件公开方法
    defineExpose({
        Show,
        ShowModal,
        Close
    });
</script>
