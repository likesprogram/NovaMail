<template>
    <div class="register">
        <div class="title">
            <span>注</span>
            <span>册</span>
        </div>
        <form @submit.prevent="registerUser">
            <div class="inputGroup">
                <label for="NovaMailAccount">手机号</label>
                <input type="text" v-model="registerInfo.account" name="NovaMailAccount" id="NovaMailAccount" class="inputGroupInput" placeholder="请输入手机号">
            </div>
            <div class="inputGroup">
                <label for="NovaMailPassword">密码</label>
                <input type="password" v-model="registerInfo.password" name="NovaMailPassword" id="NovaMailPassword" class="inputGroupInput" placeholder="请输入密码">
            </div>
            <div class="inputGroup">
                <label for="NovaMailConfirmPassword">确认密码</label>
                <input type="password" v-model="confirmPassword" name="NovaMailConfirmPassword" id="NovaMailConfirmPassword" class="inputGroupInput" placeholder="请输入密码">
            </div>
            <div class="inputGroup">
                <label for="NovaMailCode">验证码</label>
                <span class="inputGroupBetween inputGroupInput">
                    <input type="text" v-model="registerInfo.smsCode.code" name="NovaMailCode" id="NovaMailCode" class="inputGroupInput" placeholder="请输入验证码">
                    <button class="btn-primary inputGroupInput getCode" :class="{ 'btn-countdown': countdown > 0 }" type="button"
                    @click="ShowSMSBox">{{ countdown > 0 ? `${countdown}s 后重试` : '获取验证码' }}</button>
                </span>
            </div>
            <div class="inputGroup inputGroupDocument">
                <span class="inputGroupBetween">
                    <input type="checkbox" v-model="agreeProtocol" id="NovaMailAgree" name="NovaMailAgree"></input>
                    <label class="agree" for="NovaMailAgree">
                        阅读并同意
                        <a href="">《NovaMail用户协议》</a>与
                        <a href="">《NovaMail隐私政策》</a>
                    </label>
                </span>
            </div>
            <div class="inputGroup">
                <button class="btn-primary inputGroupInput button" type="submit">
                    <span>注</span>
                    <span>册</span>
                </button>
            </div>
        </form>
        <MessageDialog ref="messageDialog">
            <template v-slot:title>
                <span>获取验证码</span>
            </template>
            <template v-slot:content>
                <div class="dialogContent">
                    <div class="messageBox">
                        <span class="messageText">短信验证码将会发送至<span class="phone">{{ registerInfo.smsCode.phoneNumber }}</span></span>
                        <div class="inputGroup message">
                            <label for="NovaMailCode">图片验证码</label>
                            <span class="inputGroupBetween inputGroupInput">
                                <input type="text" v-model="registerInfo.smsCode.imageCode.code" class="inputGroupInput" placeholder="请输入验证码">
                                <img class="input mailCode" @click="getImageCode" :src="imageCode!.code" alt="验证码"></img>
                            </span>
                        </div>
                    </div>
                </div>
            </template>
            <template v-slot:footer>
                <div class="dialogFooterButton">
                     <span class="textBox">
                         <span class="text">取消</span>
                     </span>
                     <span class="line"></span>
                </div>
                <div class="dialogFooterButton" @click="getSmsCode">
                     <span class="textBox">
                         <span class="text">确定</span>
                     </span>
                     <span class="line"></span>
                </div>
            </template>
        </MessageDialog>
        <ShowMessage ref="showMessage"/>
    </div>
</template>

<script lang="ts">
import { ref, onMounted, onUnmounted } from 'vue';
import MessageDialog from '@components/messageDialog/MessageDialog.vue';
import '@components/messageDialog/MessageDialog.css'
import { VerificationCode } from '@api/VerificationCode';
import type { ImageCode } from '@api/VerificationCode';
import ShowMessage from '@components/messageDialog/ShowMessage.vue';
import { Authentication, type RegisterData } from '@api/users/Authentication';
import { useRouter } from 'vue-router';
import { ApiException } from '@utils/Request';

// 短信验证码倒计时持久化键
const SMS_TIMER_KEY = 'sms_code_expire_time';

export default {
    name: 'Register',
}
</script>

<script setup lang="ts">
// 路由
const router = useRouter();

// 获取验证码对话框
const messageDialog = ref<typeof MessageDialog>();

// 图片验证码
const imageCode = ref<ImageCode>({
    code: '',
    sessionId: ''
});

// 注册信息
const registerInfo = ref<RegisterData>({
    account: '',        // 账号
    password: '',       // 密码
    smsCode: {          // 短信验证码
        phoneNumber: '',
        code: '',
        imageCode: {
            code: '',
            sessionId: ''
        }
    }
});
// 确认密码
const confirmPassword = ref<string>('');
// 是否同意协议
const agreeProtocol = ref<boolean>(false);

// 显示获取短信验证码对话框
const ShowSMSBox = () => {
    if (!ValidateInfo()) return;
    getImageCode();
    messageDialog.value?.ShowModal();
}

// 验证信息完整性
const ValidateInfo = (): boolean => {
    // 验证信息是否填写完整
    if (!registerInfo.value.account) {
        showMessage.value?.ShowModal("信息不完整", ["请输入手机号！"], "warning");
        return false;
    }
    // 验证手机号是否合法
    const phoneRegex = /^1[3-9]\d{9}$/;
    if (!phoneRegex.test(registerInfo.value.account)) {
        showMessage.value?.ShowModal("信息不完整", ["请输入合法的手机号！"], "warning");
        return false;
    }

    registerInfo.value.smsCode.phoneNumber = registerInfo.value.account;

    if (!registerInfo.value.password) {
        showMessage.value?.ShowModal("信息不完整", ["请输入密码！"], "warning");
        return false;
    }
    // 密码长度至少8位，且包含大小写字母、数字和特殊字符
    const passwordRegex = /^(?=.*[a-z])(?=.*[A-Z])(?=.*\d)(?=.*[!@#$%^&*()_+\-=\[\]{};':"\\|,.<>/?]).+$/;
    if (registerInfo.value.password.length < 8 || !passwordRegex.test(registerInfo.value.password)) {
        showMessage.value?.ShowModal("信息不完整", ["密码长度不得少于8位，且包含","大小写字母、数字和特殊字符！"], "warning");
        return false;
    }
    if (registerInfo.value.password !== confirmPassword.value) {
        showMessage.value?.ShowModal("信息不完整", ["两次输入的密码不一致！"], "warning");
        return false;
    }

    return true;
};

// 消息对话框
const showMessage = ref<InstanceType<typeof ShowMessage>>();

// 获取图片验证码
const getImageCode = async () => {
    try {
        const request = await VerificationCode.getImageCode();
        imageCode.value = request.data;
    } catch (error) {
        if (error instanceof ApiException) {
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

// 短信验证码 倒计时
const countdown = ref(0);
let timer: number | null = null;
const startCountdown = () => { // 开始倒计时
    const expireTime = Date.now() + 60 * 1000;
    localStorage.setItem(SMS_TIMER_KEY, expireTime.toString());
};
const updateCountdown = () => { // 更新倒计时
    const expireTime = Number(localStorage.getItem(SMS_TIMER_KEY));

    if (!expireTime) {
        countdown.value = 0;
        return;
    }

    const remaining = Math.max(0, Math.floor((expireTime - Date.now()) / 1000));

    countdown.value = remaining;

    if (remaining <= 0) {
        localStorage.removeItem(SMS_TIMER_KEY);
        stopTimer();
    }
};
// 开始计时
const startTimer = () => {
    if (timer) return;

    updateCountdown(); // 立即更新一次

    timer = window.setInterval(updateCountdown, 1000);
};
// 停止计时
const stopTimer = () => {
    if (timer) {
        clearInterval(timer);
        timer = null;
    }
};

// 组件挂载
onMounted(() => {
    const expireTime = Number(localStorage.getItem(SMS_TIMER_KEY));

    // 恢复短信验证码倒计时
    if (expireTime && expireTime > Date.now()) startTimer();
});

// 组件卸载
onUnmounted(() => {
    stopTimer();  // 清理计时器
});

// 获取短信验证码
const getSmsCode = async () => {
    try {
        registerInfo.value.smsCode.imageCode.sessionId = imageCode.value.sessionId;
        await VerificationCode.getSmsCode(registerInfo.value.smsCode);
        startCountdown();   // 开始倒计时
        startTimer();       // 启动计时器
        showMessage.value?.ShowModal("短信验证码发送成功", ["短信验证码已发送至您的手机，请注意查收！"], "success");
        messageDialog.value?.Close(); // 关闭对话框
        registerInfo.value.smsCode.imageCode.code = '';
    } catch (error) {
        if (error instanceof ApiException) {
            showMessage.value?.ShowModal("短信验证码发送失败", ["短信验证码发送失败：" + error.message, "是否重新发送"], "failure", (selectedButtonIndex) => {
                registerInfo.value.smsCode.imageCode.code = '';
                if(selectedButtonIndex === 1) getImageCode();
                else messageDialog.value?.Close(); // 关闭对话框
            });
        } else {
            showMessage.value?.ShowModal("短信验证码发送失败", ["短信验证码发送失败：未知错误！", "是否重新发送"], "failure", (selectedButtonIndex) => {
                registerInfo.value.smsCode.imageCode.code = '';
                if(selectedButtonIndex === 1) getImageCode();
                else messageDialog.value?.Close(); // 关闭对话框
            });
        }
    }
};

// 注册用户
const registerUser = async () => {
    // 验证信息是否填写完整
    if (!ValidateInfo()) return;
    // 验证码是否输入
    if (!registerInfo.value.smsCode.code) {
        showMessage.value?.ShowModal("信息不完整", ["请输入短信验证码！"], "warning");
        return;
    }
    // 协议是否同意
    if (!agreeProtocol.value) {
        showMessage.value?.ShowModal("信息不完整", ["请先阅读并同意用户协议与隐私政策！"], "warning");
        return;
    }
    try {
        await Authentication.register(registerInfo.value);
        showMessage.value?.ShowModal("注册成功", ["注册成功！","是否立即前往登录"], "success", (selectedButtonIndex) => {
            if(selectedButtonIndex === 1) router.replace({
                name: 'login',
                query:{
                    account: registerInfo.value.account
                }
            });
        });
    } catch (error) {
        if (error instanceof ApiException) {
            if(error.code == 3001) {
                showMessage.value?.ShowModal("注册成功", ["账号注册成功！","在创建邮箱时出现错误，","需要登录后手动创建","是否立即前往登录"], "warning", (selectedButtonIndex) => {
                    if(selectedButtonIndex === 1) router.replace({
                        name: 'login',
                        query:{
                            account: registerInfo.value.account
                        }
                    });
                });
            } else {
                showMessage.value?.ShowModal("注册失败", ["注册失败：" + error.message], "failure");
            }
        } else {
            showMessage.value?.ShowModal("注册失败", ["注册失败：未知错误！"], "failure");
        }
    }
};
</script>

<style scoped>
.register {
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
.inputGroup.submitButton { 
    height: auto;
    padding-bottom: 20px;
}
.inputGroup.inputGroupDocument {
    height: 30px;
    min-height: 30px;
    display: flex;
}
.inputGroup .getCode { 
    height: 32px;
    display: flex;
    justify-content: center;
    align-items: center;
}
.inputGroup .button {
    width: calc(100% - 80px);
    max-width: calc(100% - 80px);
    height: 40px;
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
.messageBox {
    width: 320px;
}
.messageText {
    font-size: 17px;
    color: var(--text-secondary);
    margin-bottom: 10px;
    display: block;
}
.messageText .phone {
    font-weight: bold;
    padding: 0px 5px;
    color: var(--text-link);
}
.inputGroup.message label {
    min-width: 90px;

}
</style>