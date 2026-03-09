<template>
  <div class="mail-compose-container">
    <h2 class="title">写邮件</h2>

    <!-- 收件人 -->
    <div class="form-item">
      <label for="toList">收件人：</label>
      <input
        v-model="toList"
        id="toList"
        type="text"
        placeholder="输入收件人邮箱地址"
        class="input-box"
        @keyup.enter="addRecipient"
      />
      <div class="tags">
        <span v-for="(email, index) in recipients" :key="index" class="tag">
          {{ email }} <button @click="removeRecipient(index)">x</button>
        </span>
      </div>
    </div>

    <!-- 主题 -->
    <div class="form-item">
      <label for="subject">主题：</label>
      <input
        v-model="subject"
        id="subject"
        type="text"
        placeholder="请输入邮件主题"
        class="input-box"
      />
    </div>

    <!-- 富文本正文 -->
    <div class="form-item">
      <label for="content">正文：</label>
      <div id="editor"></div>
    </div>

    <!-- 附件 -->
    <div class="form-item">
      <label for="attachments">附件：</label>
      <input
        type="file"
        id="attachments"
        multiple
        @change="handleFileChange"
        class="input-box"
      />
      <div class="attachments">
        <span v-for="(file, index) in attachments" :key="index" class="attachment-item">
          {{ file.name }} <button @click="removeAttachment(index)">x</button>
        </span>
      </div>
    </div>

    <!-- 操作按钮 -->
    <div class="form-item">
      <button @click="sendMail" class="send-button">发送</button>
      <button @click="saveDraft" class="draft-button">存草稿</button>
    </div>
  </div>
</template>

<script lang="ts">
import { defineComponent, getCurrentInstance, onMounted, ref } from 'vue'

export default defineComponent({
  name: 'WriteMail',
  setup() {
    // 数据绑定
    const toList = ref('')
    const subject = ref('')
    const content = ref('')
    const recipients = ref<string[]>([])
    const attachments = ref<File[]>([])
    // 初始化 TinyMCE 编辑器
    onMounted(() => {
      // 挂载到全局（TypeScript
      (window as any).tinymce.init({
        selector: '#editor',
        license_key: 'gpl', 
  language: 'zh_CN',                      // ✅ 设置语言
  // language_url: '/src/assets/tinymce/langs/zh_CN.js',  // ✅ 离线路径
        height: 400,
        // menubar: false,
        // plugins: 'link image table code lists',
        menubar: false,
  plugins: [
    'image', 'table', 'link', 'lists', 'code', 'fullscreen', 'preview', 'wordcount'
  ],
  toolbar:
    'undo redo | link image localimage table | bold italic underline forecolor backcolor | ' +
    'alignleft aligncenter alignright alignjustify | bullist numlist outdent indent | code fullscreen',
        // toolbar: 'undo redo | bold italic underline | alignleft aligncenter alignright | bullist numlist outdent indent | code',
        external_plugins: {}, // ✅ 防止它尝试自动加载 licensekeymanager
        statusbar: false,    // ✅ 关闭底部状态栏
        branding: false,     // ✅ 去掉 “Powered by Tiny” 字样
        setup(editor: any) {
        editor.ui.registry.addButton('localimage', {
          text: '本地图片',
          onAction: openLocalImage, // 这里用箭头函数绑定外层作用域,
        });
      },
      });
    })

     const openLocalImage = () => {
        console.log("选择本地图片");
      };

    // 初始化 TinyMCE 编辑器配置
    const editorInit = {
      height: 400,
      menubar: true,
      plugins: 'lists link image code table',
      toolbar: 'undo redo | bold italic underline | bullist numlist | link image | code | table',
    }

    // 添加收件人到列表
    const addRecipient = () => {
      if (toList.value.trim() && isValidEmail(toList.value)) {
        recipients.value.push(toList.value.trim())
        toList.value = ''
      } else {
        alert('请输入有效的邮箱地址')
      }
    }

    // 删除收件人
    const removeRecipient = (index: number) => {
      recipients.value.splice(index, 1)
    }

    // 文件上传处理
    const handleFileChange = (event: Event) => {
      const target = event.target as HTMLInputElement
      if (target.files) {
        attachments.value.push(...Array.from(target.files))
      }
    }

    // 删除附件
    const removeAttachment = (index: number) => {
      attachments.value.splice(index, 1)
    }

    // 邮件发送
    const sendMail = () => {
      console.log('发送邮件:', {
        to: recipients.value,
        subject: subject.value,
        html: content.value,
        attachments: attachments.value
      })
      alert('邮件发送模拟完成')
    }

    // 存草稿
    const saveDraft = () => {
      console.log('保存草稿:', {
        to: recipients.value,
        subject: subject.value,
        html: content.value,
        attachments: attachments.value
      })
      alert('草稿已保存')
    }

    // 邮箱格式验证
    const isValidEmail = (email: string): boolean => {
      const emailPattern = /^[^\s@]+@[^\s@]+\.[^\s@]+$/
      return emailPattern.test(email)
    }

    return {
      toList,
      subject,
      content,
      recipients,
      attachments,
      editorInit,
      addRecipient,
      removeRecipient,
      handleFileChange,
      removeAttachment,
      sendMail,
      saveDraft
    }
  }
})
</script>

<style scoped>
.mail-compose-container {
  max-width: 800px;
  margin: 0 auto;
  padding: 20px;
  background-color: #fff;
  box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
}

h2.title {
  text-align: center;
  font-size: 1.5rem;
  margin-bottom: 20px;
}

.form-item {
  margin-bottom: 15px;
}

.input-box {
  width: 100%;
  padding: 10px;
  border-radius: 4px;
  border: 1px solid #ccc;
}

.tags,
.attachments {
  margin-top: 10px;
}

.tag,
.attachment-item {
  display: inline-block;
  margin-right: 10px;
  background-color: #f0f0f0;
  padding: 5px 10px;
  border-radius: 20px;
}

button {
  padding: 10px 20px;
  border: none;
  cursor: pointer;
  border-radius: 4px;
}

.send-button {
  background-color: #4CAF50;
  color: white;
}

.draft-button {
  background-color: #f1c40f;
  color: white;
  margin-left: 10px;
}

button:hover {
  opacity: 0.8;
}
</style>
