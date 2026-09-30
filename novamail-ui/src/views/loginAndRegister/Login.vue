<template>
    <div class="login">
        <div class="title">
            <span>登</span>
            <span>录</span>
        </div>
        <form action="" method="post" autocomplete="off" @submit.prevent="LoginSubmit" @pointerdown.capture="enforceLoginFields" @focusin.capture="enforceLoginFields">
            <input class="browserAutofillTrap" type="text" name="username" autocomplete="username" tabindex="-1" aria-hidden="true">
            <input class="browserAutofillTrap" type="password" name="password" autocomplete="current-password" tabindex="-1" aria-hidden="true">
            <div class="inputGroup">
                <label for="NovaMailAccount">账号</label>
                <span ref="accountPickerRoot" class="inputGroupBetween inputGroupInput accountInputWrap" @focusout="handleAccountFocusOut">
                    <input ref="accountInput" type="text" v-model="loginInfo.account" :name="accountFieldName" id="NovaMailAccount" class="inputGroupInput" placeholder="请输入账号" autocomplete="off" data-lpignore="true" data-1p-ignore="true" :readonly="browserAutofillLocked" @pointerdown="prepareCredentialInput" @focus="handleAccountFocus" @keydown="prepareCredentialInput" @input="handleAccountInput" @change="handleAccountInput">
                    <SafePopover v-if="savedCredentials.length > 0" ref="accountPickerPanel" :NoBoxShadow="['all']" :Width="'100%'" :Placement="'Bottom,Center'">
                        <div class="accountPickerPanel">
                            <button
                                v-for="credential in savedCredentials"
                                :key="credential.account"
                                type="button"
                                class="accountPickerItem"
                                :class="{ active: credential.account === loginInfo.account }"
                                @mousedown.prevent="selectSavedCredential(credential.account)"
                            >
                                <span class="accountPickerText">{{ credential.account }}</span>
                                <span v-if="credential.account === loginInfo.account" class="iconfont icon-likesprogramzhengque"></span>
                            </button>
                        </div>
                    </SafePopover>
                </span>
            </div>
            <div class="inputGroup">
                <label for="NovaMailPassword">密码</label>
                <input ref="passwordInput" type="password" v-model="loginInfo.password" :name="passwordFieldName" id="NovaMailPassword" class="inputGroupInput" placeholder="请输入密码" autocomplete="new-password" data-lpignore="true" data-1p-ignore="true" :readonly="browserAutofillLocked" @pointerdown="prepareCredentialInput" @focus="prepareCredentialInput" @keydown="prepareCredentialInput" @input="handlePasswordInput" @change="handlePasswordInput">
            </div>
            <div class="inputGroup">
                <label for="NovaMailCode">验证码</label>
                <span class="inputGroupBetween inputGroupInput">
                    <input type="text" v-model="loginInfo.imageCode.code" name="NovaMailCode" id="NovaMailCode" class="inputGroupInput" placeholder="请输入验证码">
                    <img class="input mailCode" @click="getImageCode" :src="imageCode!.code" alt="验证码"></img>
                </span>
            </div>
            <div class="inputGroup inputGroupBetween inputGroupDocument">
                <span class="inputGroupBetween">
                    <input type="checkbox" v-model="autoLogin" id="NovaMailAutoLogin" name="NovaMailAutoLogin" @pointerdown="enforceLoginFields" @change="() => saveLoginPreference()"></input>
                    <label for="NovaMailAutoLogin">自动登录</label>
                </span>
                <span class="inputGroupBetween">
                    <input type="checkbox" v-model="rememberPassword" id="NovaMailRemember" name="NovaMailRemember" @pointerdown="enforceLoginFields" @change="() => saveLoginPreference()">
                    <label for="NovaMailRemember">记住密码</label>
                </span>
                <router-link class="inputGroupBetween" :to="{ name: 'forget', query: loginInfo.account ? { account: loginInfo.account } : {} }">
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
                    <input type="checkbox" v-model="agreeProtocol" id="NovaMailAgree" name="NovaMailAgree"></input>
                    <label class="agree" for="NovaMailAgree">
                        阅读并同意
                        <router-link class="agreementLink" :to="{ name: 'userAgreement' }" @click.stop>《NovaMail用户协议》</router-link>与
                        <router-link class="agreementLink" :to="{ name: 'privacyPolicy' }" @click.stop>《NovaMail隐私政策》</router-link>
                    </label>
                </span>
            </div>
        </form>
        <ShowMessage ref="showMessage"/>
    </div>
</template>

<script lang="ts">
import { nextTick, onMounted, ref, watch } from 'vue';
import { useRoute, useRouter } from 'vue-router';
import { VerificationCode } from '@api/VerificationCode';
import type { ImageCode } from '@api/VerificationCode';
import ShowMessage from '@/components/messageDialog/ShowMessage.vue';
import SafePopover from '@/components/SafePopover.vue';
import { type LoginData } from '@api/users/Authentication';
import { AuthService } from '@service/AuthService';
import { ApiException } from '@utils/Request';
export default {
    name: 'App',
}
</script>

<script setup lang="ts">
const route = useRoute();
const router = useRouter();
const LOGIN_PREFERENCE_KEY = 'novamail_login_preference';

interface LoginPreference {
    lastAccount: string;
    credentials: SavedCredential[];
    rememberPassword: boolean;
    autoLogin: boolean;
}

interface SavedCredential {
    account: string;
    password: string;
    updatedAt: number;
}

// 验证码消息
const imageCode = ref<ImageCode>({
    code: '',
    sessionId: ''
});

// 登录数据
const loginInfo = ref<LoginData>({
    account: '',
    password: '',
    imageCode: {
        code: '',
        sessionId: ''
    }
});
const accountInput = ref<HTMLInputElement>();
const passwordInput = ref<HTMLInputElement>();
const accountPickerRoot = ref<HTMLElement>();
const accountPickerPanel = ref<InstanceType<typeof SafePopover>>();
const savedCredentials = ref<SavedCredential[]>([]);
const browserAutofillLocked = ref<boolean>(true);
const fieldNameSuffix = Math.random().toString(36).slice(2);
const accountFieldName = `novamail_login_account_${fieldNameSuffix}`;
const passwordFieldName = `novamail_login_secret_${fieldNameSuffix}`;
let credentialIntentTime = 0;
let autofillGuardTimer: number | null = null;
let isLoadingPreference = false;

// 是否同意协议
const agreeProtocol = ref<boolean>(false);
// 自动登录
const autoLogin = ref<boolean>(false);
// 记住密码
const rememberPassword = ref<boolean>(true);

const showMessage = ref<InstanceType<typeof ShowMessage>>();

const normalizeLoginOptions = () => {
    if (!rememberPassword.value) autoLogin.value = false;
    if (autoLogin.value) rememberPassword.value = true;
};

const enforceLoginFields = () => {
    if (accountInput.value && accountInput.value.value !== loginInfo.value.account) {
        accountInput.value.value = loginInfo.value.account;
    }
    if (passwordInput.value && passwordInput.value.value !== loginInfo.value.password) {
        passwordInput.value.value = loginInfo.value.password;
    }
};

const startAutofillGuard = (duration = 5000) => {
    enforceLoginFields();
    if (autofillGuardTimer) window.clearInterval(autofillGuardTimer);

    const startTime = Date.now();
    autofillGuardTimer = window.setInterval(() => {
        enforceLoginFields();
        if (Date.now() - startTime >= duration) {
            if (autofillGuardTimer) window.clearInterval(autofillGuardTimer);
            autofillGuardTimer = null;
        }
    }, 100);
};

const prepareCredentialInput = () => {
    browserAutofillLocked.value = false;
    credentialIntentTime = Date.now();
};

const handleAccountFocus = () => {
    prepareCredentialInput();
    showSavedCredentialPanel();
};

const handleAccountFocusOut = () => {
    window.setTimeout(() => {
        if (document.activeElement && accountPickerRoot.value?.contains(document.activeElement)) return;
        accountPickerPanel.value?.hide();
    }, 120);
};

const isRecentCredentialInput = () => Date.now() - credentialIntentTime < 1500;

const handleAccountInput = () => {
    if (!isRecentCredentialInput()) {
        enforceLoginFields();
        return;
    }

    loginInfo.value.account = accountInput.value?.value || '';
    startAutofillGuard(8000);
};

const handlePasswordInput = () => {
    if (!isRecentCredentialInput()) {
        enforceLoginFields();
        return;
    }

    loginInfo.value.password = passwordInput.value?.value || '';
    startAutofillGuard(8000);
};

const getLoginPreference = (): LoginPreference | null => {
    const rawPreference = localStorage.getItem(LOGIN_PREFERENCE_KEY);
    if (!rawPreference) return null;

    try {
        const preference = JSON.parse(rawPreference) as Partial<LoginPreference> & { account?: string; password?: string };
        const credentials = Array.isArray(preference.credentials)
            ? preference.credentials.filter((credential): credential is SavedCredential => {
                return typeof credential?.account === 'string'
                    && typeof credential?.password === 'string'
                    && typeof credential?.updatedAt === 'number';
            })
            : [];

        if (credentials.length === 0 && typeof preference.account === 'string' && typeof preference.password === 'string') {
            credentials.push({
                account: preference.account,
                password: preference.password,
                updatedAt: Date.now(),
            });
        }

        return {
            lastAccount: typeof preference.lastAccount === 'string' ? preference.lastAccount : (typeof preference.account === 'string' ? preference.account : ''),
            credentials,
            rememberPassword: preference.rememberPassword === true,
            autoLogin: preference.autoLogin === true,
        };
    } catch {
        localStorage.removeItem(LOGIN_PREFERENCE_KEY);
        return null;
    }
};

const upsertCurrentCredential = () => {
    if (!loginInfo.value.account || !loginInfo.value.password) return;

    const nextCredential: SavedCredential = {
        account: loginInfo.value.account,
        password: loginInfo.value.password,
        updatedAt: Date.now(),
    };
    const otherCredentials = savedCredentials.value.filter((credential) => credential.account !== nextCredential.account);
    savedCredentials.value = [nextCredential, ...otherCredentials].slice(0, 5);
};

const saveLoginPreference = (saveCurrentCredential = false) => {
    enforceLoginFields();
    normalizeLoginOptions();

    if (!rememberPassword.value) {
        savedCredentials.value = [];
    } else if (saveCurrentCredential) {
        upsertCurrentCredential();
    }

    const preference: LoginPreference = {
        lastAccount: rememberPassword.value ? loginInfo.value.account : '',
        credentials: rememberPassword.value ? savedCredentials.value : [],
        rememberPassword: rememberPassword.value,
        autoLogin: autoLogin.value,
    };
    localStorage.setItem(LOGIN_PREFERENCE_KEY, JSON.stringify(preference));
};

const loadLoginPreference = () => {
    const preference = getLoginPreference();
    if (!preference) return;

    isLoadingPreference = true;
    savedCredentials.value = preference.rememberPassword ? preference.credentials : [];
    rememberPassword.value = preference.rememberPassword;
    autoLogin.value = preference.autoLogin && preference.rememberPassword;

    if (preference.rememberPassword) {
        const selectedCredential = savedCredentials.value.find((credential) => credential.account === preference.lastAccount) || savedCredentials.value[0];
        if (selectedCredential) {
            loginInfo.value.account = selectedCredential.account;
            loginInfo.value.password = selectedCredential.password;
            startAutofillGuard();
        }
    }
    isLoadingPreference = false;
};

const showSavedCredentialPanel = () => {
    if (!accountPickerRoot.value || savedCredentials.value.length === 0) return;

    enforceLoginFields();
    accountPickerPanel.value?.show(accountPickerRoot.value);
};

const selectSavedCredential = (account: string) => {
    const credential = savedCredentials.value.find((item) => item.account === account);
    if (!credential) return;

    browserAutofillLocked.value = false;
    loginInfo.value.account = credential.account;
    loginInfo.value.password = credential.password;
    accountPickerPanel.value?.hide();
    saveLoginPreference();
    nextTick(() => startAutofillGuard(8000));
};

watch(rememberPassword, (value, oldValue) => {
    if (isLoadingPreference) return;
    if (!value) autoLogin.value = false;
    if (oldValue && !value) AuthService.cancelAutoLogin();
    saveLoginPreference();
});

watch(autoLogin, (value, oldValue) => {
    if (isLoadingPreference) return;
    if (value) rememberPassword.value = true;
    if (oldValue && !value) AuthService.cancelAutoLogin();
    saveLoginPreference();
});

const getImageCode = async () => {
    try {
        const request = await VerificationCode.getImageCode();
        
        imageCode.value = request.data;
        loginInfo.value.imageCode.sessionId = request.data.sessionId;
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

// 组件挂载后获取验证码
onMounted(() => {
    loadLoginPreference();

    // 从参数中获取账号
    const account = route.query.account;
    if (typeof account === 'string') {
        const routeCredential = savedCredentials.value.find((credential) => credential.account === account);
        loginInfo.value.account = account;
        loginInfo.value.password = routeCredential?.password || '';
        router.replace({ query: {} }); // 清空参数，避免重复填入
    }

    // 获取验证码
    getImageCode();

    nextTick(() => {
        startAutofillGuard(8000);
    });
});

// 验证信息完整性
const ValidateInfo = (): boolean => {
    enforceLoginFields();

    // 账号
    if (!loginInfo.value.account) {
        showMessage.value?.ShowModal("信息不完整", ["请输入手机号！"], "warning");
        return false;
    }

    // 密码
    if (!loginInfo.value.password) {
        showMessage.value?.ShowModal("信息不完整", ["请输入密码！"], "warning");
        return false;
    }

    // 验证码
    if (!loginInfo.value.imageCode.code) {
        showMessage.value?.ShowModal("信息不完整", ["请输入验证码！"], "warning");
        return false;
    }

    return true;
};

const LoginSubmit = async () => {
    enforceLoginFields();
    if (!ValidateInfo()) return;

    // 协议是否同意
    if (!agreeProtocol.value) {
        showMessage.value?.ShowModal("信息不完整", ["请先阅读并同意用户协议与隐私政策！"], "warning");
        return;
    }

    try {
        await AuthService.login(loginInfo.value, { autoLogin: autoLogin.value });
        saveLoginPreference(true);
        router.replace({name: 'manager'});
    } catch (error) {
        if (error instanceof ApiException) {
            showMessage.value?.ShowModal("登录失败", ["登录失败" + error.message], "failure");
        } else {
            showMessage.value?.ShowModal("登录失败", ["登录失败：未知错误！"], "failure");
        }
    }
    
    // 刷新验证码
    getImageCode();
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
.browserAutofillTrap {
    position: absolute;
    width: 1px;
    height: 1px;
    padding: 0;
    border: 0;
    opacity: 0;
    pointer-events: none;
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
.accountInputWrap {
    position: relative;
}
.accountPickerPanel {
    width: 100%;
    max-height: 160px;
    padding: 5px;
    display: flex;
    flex-direction: column;
    background-color: var(--bg-primary);
    overflow-y: auto;
}
.accountPickerItem {
    width: 100%;
    min-height: 34px;
    padding: 0 8px;
    border: none;
    border-radius: 5px;
    background-color: transparent;
    color: var(--text-primary);
    cursor: pointer;
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 8px;
    transition: all .3s;
}
.accountPickerItem:hover,
.accountPickerItem.active {
    background-color: var(--bg-hover-primary);
}
.accountPickerItem:active {
    transition: all .1s;
    background-color: var(--bg-active-primary);
}
.accountPickerText {
    min-width: 0;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.accountPickerItem .iconfont {
    flex-shrink: 0;
    color: var(--text-link);
    font-size: 14px;
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
.inputGroup .agree .agreementLink:hover {
    color: var(--text-link-hover);
}
.inputGroup .agree .agreementLink:active {
    color: var(--text-link-active);
}
</style>
