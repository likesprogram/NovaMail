<template>
    <div class="forget">
        <div class="title">
            <span>找</span>
            <span>回</span>
            <span>密</span>
            <span>码</span>
        </div>
        <form @submit.prevent="resetPassword">
            <div class="inputGroup">
                <label for="NovaMailForgetAccount">手机号</label>
                <input type="text" v-model="resetInfo.account" name="NovaMailForgetAccount" id="NovaMailForgetAccount" class="inputGroupInput" placeholder="请输入手机号">
            </div>
            <div class="inputGroup">
                <label for="NovaMailForgetPassword">新密码</label>
                <input type="password" v-model="resetInfo.password" name="NovaMailForgetPassword" id="NovaMailForgetPassword" class="inputGroupInput" placeholder="请输入新密码">
            </div>
            <div class="inputGroup">
                <label for="NovaMailForgetConfirmPassword">确认密码</label>
                <input type="password" v-model="confirmPassword" name="NovaMailForgetConfirmPassword" id="NovaMailForgetConfirmPassword" class="inputGroupInput" placeholder="请再次输入新密码">
            </div>
            <div class="inputGroup">
                <label for="NovaMailForgetCode">验证码</label>
                <span class="inputGroupBetween inputGroupInput">
                    <input type="text" v-model="resetInfo.smsCode.code" name="NovaMailForgetCode" id="NovaMailForgetCode" class="inputGroupInput" placeholder="请输入短信验证码">
                    <button class="btn-primary inputGroupInput getCode" :class="{ 'btn-countdown': countdown > 0 }" type="button" :disabled="countdown > 0" @click="ShowSMSBox">
                        {{ countdown > 0 ? `${countdown}s 后重试` : '获取验证码' }}
                    </button>
                </span>
            </div>
            <div class="inputGroup">
                <button class="btn-primary inputGroupInput button" type="submit">
                    <span>重</span>
                    <span>置</span>
                    <span>密</span>
                    <span>码</span>
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
                        <span class="messageText">短信验证码将会发送至<span class="phone">{{ resetInfo.smsCode.phoneNumber }}</span></span>
                        <div class="inputGroup message">
                            <label for="NovaMailForgetImageCode">图片验证码</label>
                            <span class="inputGroupBetween inputGroupInput">
                                <input type="text" v-model="resetInfo.smsCode.imageCode.code" id="NovaMailForgetImageCode" class="inputGroupInput" placeholder="请输入验证码">
                                <img class="input mailCode" @click="getImageCode" :src="imageCode!.code" alt="验证码"></img>
                            </span>
                        </div>
                    </div>
                </div>
            </template>
            <template v-slot:footer>
                <div class="dialogFooterButton" @click="closeSmsBox">
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
import { useRoute, useRouter } from 'vue-router';
import MessageDialog from '@components/messageDialog/MessageDialog.vue';
import '@components/messageDialog/MessageDialog.css'
import { VerificationCode } from '@api/VerificationCode';
import type { ImageCode } from '@api/VerificationCode';
import ShowMessage from '@components/messageDialog/ShowMessage.vue';
import { Authentication, type ResetPasswordData } from '@api/users/Authentication';
import { ApiException } from '@utils/Request';

const SMS_TIMER_KEY = 'forget_sms_code_expire_time';

export default {
    name: 'Forget',
}
</script>

<script setup lang="ts">
const route = useRoute();
const router = useRouter();
const messageDialog = ref<InstanceType<typeof MessageDialog>>();
const showMessage = ref<InstanceType<typeof ShowMessage>>();

const imageCode = ref<ImageCode>({
    code: '',
    sessionId: ''
});

const resetInfo = ref<ResetPasswordData>({
    account: '',
    password: '',
    smsCode: {
        phoneNumber: '',
        code: '',
        imageCode: {
            code: '',
            sessionId: ''
        }
    }
});

const confirmPassword = ref<string>('');
const countdown = ref(0);
let timer: number | null = null;

const validateInfo = (): boolean => {
    if (!resetInfo.value.account) {
        showMessage.value?.ShowModal("信息不完整", ["请输入手机号！"], "warning");
        return false;
    }

    const phoneRegex = /^1[3-9]\d{9}$/;
    if (!phoneRegex.test(resetInfo.value.account)) {
        showMessage.value?.ShowModal("信息不完整", ["请输入合法的手机号！"], "warning");
        return false;
    }

    resetInfo.value.smsCode.phoneNumber = resetInfo.value.account;

    if (!resetInfo.value.password) {
        showMessage.value?.ShowModal("信息不完整", ["请输入新密码！"], "warning");
        return false;
    }

    const passwordRegex = /^(?=.*[a-z])(?=.*[A-Z])(?=.*\d)(?=.*[!@#$%^&*()_+\-=\[\]{};':"\\|,.<>/?]).+$/;
    if (resetInfo.value.password.length < 8 || !passwordRegex.test(resetInfo.value.password)) {
        showMessage.value?.ShowModal("信息不完整", ["密码长度不得少于8位，且包含","大小写字母、数字和特殊字符！"], "warning");
        return false;
    }

    if (resetInfo.value.password !== confirmPassword.value) {
        showMessage.value?.ShowModal("信息不完整", ["两次输入的密码不一致！"], "warning");
        return false;
    }

    return true;
};

const ShowSMSBox = () => {
    if (countdown.value > 0) return;
    if (!validateInfo()) return;
    resetInfo.value.smsCode.imageCode.code = '';
    getImageCode();
    messageDialog.value?.ShowModal();
};

const closeSmsBox = () => {
    resetInfo.value.smsCode.imageCode.code = '';
    messageDialog.value?.Close();
};

const getImageCode = async () => {
    try {
        const request = await VerificationCode.getImageCode();
        imageCode.value = request.data;
        resetInfo.value.smsCode.imageCode.sessionId = request.data.sessionId;
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

const startCountdown = () => {
    const expireTime = Date.now() + 60 * 1000;
    localStorage.setItem(SMS_TIMER_KEY, expireTime.toString());
};

const updateCountdown = () => {
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

const startTimer = () => {
    if (timer) return;
    updateCountdown();
    timer = window.setInterval(updateCountdown, 1000);
};

const stopTimer = () => {
    if (timer) {
        clearInterval(timer);
        timer = null;
    }
};

onMounted(() => {
    const account = route.query.account;
    if (typeof account === 'string') {
        resetInfo.value.account = account;
        router.replace({ query: {} });
    }

    const expireTime = Number(localStorage.getItem(SMS_TIMER_KEY));
    if (expireTime && expireTime > Date.now()) startTimer();
});

onUnmounted(() => {
    stopTimer();
});

const getSmsCode = async () => {
    if (!resetInfo.value.smsCode.imageCode.code) {
        showMessage.value?.ShowModal("信息不完整", ["请输入图片验证码！"], "warning");
        return;
    }

    try {
        resetInfo.value.smsCode.imageCode.sessionId = imageCode.value.sessionId;
        await VerificationCode.getSmsCode(resetInfo.value.smsCode);
        startCountdown();
        startTimer();
        showMessage.value?.ShowModal("短信验证码发送成功", ["短信验证码已发送至您的手机，请注意查收！"], "success");
        closeSmsBox();
    } catch (error) {
        if (error instanceof ApiException) {
            showMessage.value?.ShowModal("短信验证码发送失败", ["短信验证码发送失败：" + error.message, "是否重新发送"], "failure", (selectedButtonIndex) => {
                resetInfo.value.smsCode.imageCode.code = '';
                if(selectedButtonIndex === 1) getImageCode();
                else messageDialog.value?.Close();
            });
        } else {
            showMessage.value?.ShowModal("短信验证码发送失败", ["短信验证码发送失败：未知错误！", "是否重新发送"], "failure", (selectedButtonIndex) => {
                resetInfo.value.smsCode.imageCode.code = '';
                if(selectedButtonIndex === 1) getImageCode();
                else messageDialog.value?.Close();
            });
        }
    }
};

const resetPassword = async () => {
    if (!validateInfo()) return;

    if (!resetInfo.value.smsCode.code) {
        showMessage.value?.ShowModal("信息不完整", ["请输入短信验证码！"], "warning");
        return;
    }

    try {
        await Authentication.resetPassword(resetInfo.value);
        showMessage.value?.ShowModal("密码重置成功", ["密码已重置，是否立即前往登录"], "success", (selectedButtonIndex) => {
            if(selectedButtonIndex === 1) router.replace({
                name: 'login',
                query: {
                    account: resetInfo.value.account
                }
            });
        });
    } catch (error) {
        if (error instanceof ApiException) {
            showMessage.value?.ShowModal("密码重置失败", ["密码重置失败：" + error.message], "failure");
        } else {
            showMessage.value?.ShowModal("密码重置失败", ["密码重置失败：未知错误！"], "failure");
        }
    }
};
</script>

<style scoped>
.forget {
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
.mailCode {
    height: 30px;
    padding: 0px;
    overflow: hidden;
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
