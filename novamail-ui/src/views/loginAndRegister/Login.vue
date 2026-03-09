<template>
    <div class="login">
        <div class="title">
            <span>登</span>
            <span>录</span>
        </div>
        <form action="" method="post" @submit.prevent="LoginSubmit">
            <div class="inputGroup">
                <label for="NovaMailAccount">账号</label>
                <input type="text" name="NovaMailAccount" id="NovaMailAccount" class="inputGroupInput" placeholder="请输入账号">
            </div>
            <div class="inputGroup">
                <label for="NovaMailPassword">密码</label>
                <input type="password" name="NovaMailPassword" id="NovaMailPassword" class="inputGroupInput" placeholder="请输入密码">
            </div>
            <div class="inputGroup">
                <label for="NovaMailCode">验证码</label>
                <span class="inputGroupBetween inputGroupInput">
                    <input type="text" name="NovaMailCode" id="NovaMailCode" class="inputGroupInput" placeholder="请输入验证码">
                    <img class="input mailCode" @click="getImageCode" :src="imageCode!.code" alt="验证码"></img>
                </span>
            </div>
            <div class="inputGroup inputGroupBetween inputGroupDocument">
                <span class="inputGroupBetween">
                    <input type="checkbox" id="NovaMailAutoLogin" name="NovaMailAutoLogin"></input>
                    <label for="NovaMailAutoLogin">自动登录</label>
                </span>
                <span class="inputGroupBetween">
                    <input type="checkbox" id="NovaMailRemember" name="NovaMailRemember" checked>
                    <label for="NovaMailRemember">记住密码</label>
                </span>
                <router-link class="inputGroupBetween" to="/">
                    忘记密码
                </router-link>
            </div>
            <div class="inputGroup submitButton">
                <button class="btn-primary inputGroupInput button" type="submit">
                    <span>登</span>
                    <span>录</span>
                </button>
            </div>
            <div class="inputGroup inputGroupDocument">
                <span class="inputGroupBetween">
                    <input type="checkbox" id="NovaMailAgree" name="NovaMailAgree"></input>
                    <label class="agree" for="NovaMailAgree">
                        阅读并同意
                        <a href="">《NovaMail用户协议》</a>与
                        <a href="">《NovaMail隐私政策》</a>
                    </label>
                </span>
            </div>
        </form>
    </div>
    <ShowMessage ref="showMessage"/>
</template>

<script lang="ts">
import { onMounted, ref } from 'vue';
import { VerificationCode } from '@api/VerificationCode';
import type { ImageCode } from '@api/VerificationCode';
import ShowMessage from '@/components/messageDialog/ShowMessage.vue';
export default {
    name: 'App',
}
</script>

<script setup lang="ts">
const imageCode = ref<ImageCode>({
    code: '',
    sessionId: ''
});

const showMessage = ref<InstanceType<typeof ShowMessage>>();

const getImageCode = async () => {
    try {
        const request = await VerificationCode.getImageCode();
        if (request.code == 200) {
            imageCode.value = request.data;
        } else {
            showMessage.value?.ShowModal("验证码获取失败", ["验证码获取失败：" + request.message, "是否重新获取"], "failure", (selectedButtonIndex) => {
                if(selectedButtonIndex === 1) getImageCode();
            });
        }
    } catch (error) {
        if (error instanceof Error) {
            showMessage.value?.ShowModal("验证码获取失败", ["验证码获取失败：" + error.message, "是否重新获取"], "failure", (selectedButtonIndex) => {
                if(selectedButtonIndex === 1) getImageCode();
            });
        } else {
            showMessage.value?.ShowModal("验证码获取失败", ["验证码获取失败：未知错误！", "是否重新获取"], "failure", (selectedButtonIndex) => {
                if(selectedButtonIndex === 1) getImageCode();
            });
        }
    }
};

// 组件挂载后获取验证码
onMounted(() => {
    getImageCode();
});

const LoginSubmit = () => {
    console.log("提交登录表单");
};

</script>

<style scoped>
.login {
    height: 100%;
    min-width: 0px;
    min-height: 0px;
    display: flex;
    flex-direction: column;
    justify-content: flex-start;
    align-items: center;
    --gap: 20px;
    gap: var(--gap);
}
.title {
    width: 100%;
    display: flex;
    justify-content: center;
    align-items: center;
    font-family: "artisticFonts";
    font-size: 32px;
    color: var(--text-link);
    gap: var(--gap);
}

/* 表单 */
form {
    width: 350px;
    display: flex;
    flex-direction: column;
    justify-content: flex-start;
    align-items: flex-start;
    gap: var(--gap);
}
.inputGroup { 
    width: 100%;
    height: 40px;
    min-height: 40px;
    display: flex;
    justify-content: center;
    align-items: center;
}
.inputGroup label {
    flex-shrink: 0;
    min-width: 80px;
    font-size: 16px;
    cursor: pointer;
}
.inputGroup input { 
    min-width: 0px;
    font-size: 16px;
}
.inputGroup .inputGroupInput { 
    flex: 1;
    display: flex;
    justify-content: flex-start;
    align-items: center;
    font-size: 16px;
    gap: 10px;
}
.inputGroupBetween { 
    height: 100%;
    min-width: 0px;
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 5px;
}
.inputGroupBetween label { 
    font-size: 15px;
}
.mailCode { 
    height: 30px;
    padding: 0px;
    overflow: hidden;
}
.inputGroup.inputGroupDocument {
    height: 30px;
    min-height: 30px;
    display: flex;
}
.inputGroup.submitButton { 
    height: auto;
    padding-bottom: 20px;
}
.inputGroup .button {
    width: calc(100% - 80px);
    max-width: calc(100% - 80px);
    height: 35px;
    display: flex;
    justify-content: center;
    align-items: center;
    font-size: 16px;
    gap: var(--gap);
}
.inputGroup .agree { 
    flex: 1;
    font-size: 15px;
    padding-left: 5px;
    /* 强制换行 */
    white-space: pre-wrap;
    word-wrap: break-word;
    word-break: break-all;
    text-align: left;
}
</style>