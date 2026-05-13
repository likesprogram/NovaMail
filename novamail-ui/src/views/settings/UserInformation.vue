<template>
    <div class="userInformation">
        <div class="settingItem">
            <div class="settingContent">
                <div class="settingTitle">昵称</div>
                <input class="nicknameInput" v-model="form.nickname" type="text" placeholder="昵称" @blur="updateUserInfo(form.nickname, undefined)" />
            </div>
        </div>
        <div class="settingItem">
            <div class="settingContent">
                <div class="settingTitle">头像</div>
                <div class="avatar">
                    <img v-if="userStore.isLogin" :src="userStore.avatarUrl" alt="avatar"></img>
                    <img v-else src="@assets/avatar.png" alt="avatar"></img>
                </div>
                <div class="operation">
                    <button class="btn-primary" @click="imageProcessingDialog?.Show()">
                        <span class="iconfont icon-likesprogramshangchuan"></span>
                        上传新头像
                    </button>
                </div>
            </div>
            <div class="settingContent">
                <div class="settingBox">
                    <div class="header">
                        <div class="title">历史头像</div>
                        <button class="btn-text" @click="imageProcessingDialog?.Show()"><span class="iconfont icon-likesprogramshangchuan"></span>上传新头像</button>
                    </div>
                    <div class="settings historyAvatars">
                        <div class="historyAvatar" :class="{ choose: historyAvatar.isCurrent }" @click="updateAvatar(historyAvatar)" v-for="historyAvatar in historyAvatars" :key="historyAvatar.uploadTime">
                            <div class="avatar">
                                <img :src="historyAvatar.avatarBase64" alt="avatar"></img>
                            </div>
                            <div class="uploadTime">{{ formatDateTime(historyAvatar.uploadTime, true) }}</div>
                        </div>
                        <div class="noData" v-if="!historyAvatars || historyAvatars.length === 0">暂无历史头像</div>
                    </div>
                </div>
            </div>
        </div>
        <div class="settingItem">
            <div class="settingContent alignItemsStart">
                <div class="settingTitle">简介</div>
                <textarea class="input resumeInput" maxlength="200" v-model="form.resume" placeholder="简介" @blur="updateUserInfo(undefined, form.resume)"></textarea>
            </div>
        </div>
        <div class="settingItem">
            <div class="settingContent alignItemsStart">
                <div class="settingTitle">签名</div>
                <div class="settingContent">
                    <div class="signatureBox settingBox">
                        <div class="header">
                            <div class="title">签名管理</div>
                            <button class="btn-text" @click="createSignature"><span class="iconfont icon-likesprogramplus"></span>添加签名</button>
                        </div>
                        <div class="settings">
                            <div class="signature" v-for="signature in signatures" :key="signature.signatureId">
                                <input type="checkbox" v-model="signature.default" @change="setDefaultSignature(signature)" class="checkbox" name="" id="">
                                <div class="title">
                                    {{ signature.signatureName }}
                                </div>
                                <div class="operation">
                                    <button class="btn-primary" @click="showSignaturePreview(signature)">
                                        <span class="iconfont icon-likesprogrammimakejian"></span>
                                        预览
                                    </button>
                                    <template v-if="signature.editable">
                                        <button class="btn-primary" @click="showSignaturePreview(signature, true)">
                                            <span class="iconfont icon-likesprogrambianji"></span>
                                            编辑
                                        </button>
                                        <button class="btn-danger" @click="deleteSignature(signature)">
                                            <span class="iconfont icon-likesprogramhuishouzhan"></span>
                                            移除
                                        </button>
                                    </template>
                                </div>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
            
        </div>
        <div class="settingItem">
            <div class="settingContent">
                <div class="settingTitle">最后登录时间</div>
                <div class="lastLoginTime">{{ formatDateTime(userStore.userInfo?.lastLoginTime) }}</div>
                <div class="operation">
                    <button class="btn-primary" @click="viewLoginLog">
                        <span class="iconfont icon-likesprogramwenzhang"></span>
                        查看日志
                    </button>
                    <button class="btn-warning" @click="showUpdatePasswordDialog">
                        <span class="iconfont icon-likesprogramsuoding"></span>
                        修改密码
                    </button>
                </div>
            </div>
        </div>
        <div class="settingItem">
            <div class="settingContent">
                <div class="settingTitle">注册地与时间</div>
                <div class="registerTime">
                    <span>{{ userStore.userInfo?.address }}</span>
                    <span>{{ formatDateTime(userStore.userInfo?.registerTime) }}</span>
                </div>
                <div class="operation">
                    <button :class="userStore.isAdmin ? 'btn-disabled' : 'btn-danger'">
                        <span class="iconfont icon-likesprogramhuishouzhan"></span>
                        注销账号
                    </button>
                </div>
            </div>
        </div>
        <!-- 图片编辑对话框 -->
        <ImageProcessingDialog ref="imageProcessingDialog" :previeRadius="'50%'" @returnImage="returnImage" />

        <!-- 签名预览与编辑 -->
        <MessageDialog ref="signaturePreviewDialog" :width="'400px'">
            <template v-slot:title>
                <span>{{ signatureEdit && !signaturePreviewEdit ? '编辑签名' : newSignature.signatureName }}</span>
            </template>
            <template v-slot:content>
                <div v-if="signatureEdit && !signaturePreviewEdit" class="signatureEditContent">
                    <div class="signatureName">
                        <span class="title">签名名称：</span>
                        <input type="text" class="input" v-model="newSignature.signatureName" :disabled="!newSignature.editable">
                    </div>
                    <div class="signatureContent">
                        <span class="title">
                            <span class="text">签名内容：</span>
                            <span class="subtitle">支持使用基础 HTML 标签进行格式化设置，并提供以下可用占位符：</span>
                        </span>
                        <div class="prompt">
                            <div class="prompts">
                                <div class="promptItem">
                                    <span class="label" v-pre>{{ profileCardUrl }}</span>
                                    <span class="description">个人资料卡链接</span>
                                </div>
                                <div class="promptItem">
                                    <span class="label" v-pre>{{ avatarUrl }}</span>
                                    <span class="description">用户头像地址</span>
                                </div>
                                <div class="promptItem">
                                    <span class="label" v-pre>{{ nickname }}</span>
                                    <span class="description">发件人昵称</span>
                                </div>
                                <div class="promptItem">
                                    <span class="label" v-pre>{{ email }}</span>
                                    <span class="description">发件邮箱地址</span>
                                </div>
                            </div>
                        </div>
                        <div class="signatureContentEdit">
                            <textarea class="input" v-model="newSignature.signatureContent"></textarea>
                        </div>
                    </div>
                </div>
                <div v-else class="signaturePreviewContent" v-html="SignatureService.renderSignature(newSignature.signatureContent, userStore.account, userStore.nickname, userStore.mailBoxView)"></div>
            </template>
            <template v-slot:footer>
                <div class="dialogFooterButton failure" @click="signaturePreviewDialog?.Close">
                    <span class="textBox">
                        <span class="iconfont icon-likesprogramcuowu"></span>
                        <span class="text">取消</span>
                    </span>
                    <span class="line"></span>
                </div>
                <template v-if="signatureEdit">
                    <div v-if="signaturePreviewEdit" @click="()=>{signaturePreviewEdit = false;}" class="dialogFooterButton success">
                        <span class="textBox">
                            <span class="iconfont icon-likesprogrambianji"></span>
                            <span class="text">编辑</span>
                        </span>
                        <span class="line"></span>
                    </div>
                    <div v-else @click="()=>{signaturePreviewEdit = true;}" class="dialogFooterButton success">
                        <span class="textBox">
                            <span class="iconfont icon-likesprogrammimakejian"></span>
                            <span class="text">预览</span>
                        </span>
                        <span class="line"></span>
                    </div>
                    <div class="dialogFooterButton success" @click="completeSignatureEdit">
                        <span class="textBox">
                            <span class="iconfont icon-likesprogramzhengque"></span>
                            <span class="text">完成</span>
                        </span>
                    </div>
                </template>
                <div v-else class="dialogFooterButton" @click="()=>{signatureEdit = true;}" :class="newSignature.editable ? 'success' : 'disabled'">
                    <span class="textBox">
                        <span class="iconfont icon-likesprogrambianji"></span>
                        <span class="text">编辑</span>
                    </span>
                </div>
            </template>
        </MessageDialog>

        <!-- 查看登录日志消息框 -->
        <MessageDialog ref="loginLogDialog" :width="'400px'">
            <template v-slot:title>
                <span>登录日志</span>
            </template>
            <template v-slot:content>
                <div class="loginLogs">
                    <div class="logItem">
                        <span class="logTime">登录时间</span>
                        <span class="logClientIp">操作IP</span>
                        <span class="logLocation">位置信息</span>
                        <span class="logDeviceType">设备类型</span>
                        <span class="logBrowser">浏览器</span>
                        <span class="loginResult">结果</span>
                    </div>
                    <div class="logItem" @click="() => {
                        showMessage?.ShowModal('日志消息', [log.failReason ? log.failReason : '无消息', log.userAgent], 'primary');
                    }" v-for="log in loginLogs" :key="log.loginTime">
                        <span class="logTime">{{ formatDateTime(log.loginTime) }}</span>
                        <span class="logClientIp">{{ log.clientIp }}</span>
                        <span class="logLocation">
                            <span class="text">{{ formatIpLocation(log.ipLocation) }}</span>
                        </span>
                        <span class="logDeviceType">{{ log.deviceType }}</span>
                        <span class="logBrowser">{{ log.browser }}</span>
                        <span class="loginResult" :class="log.loginResult">{{ log.loginResult }}</span>
                    </div>
                    <div class="noData" v-if="!loginLogs || loginLogs.length === 0">暂无登录日志</div>
                </div>
            </template>
            <template v-slot:footer>
                <div class="dialogFooterButton failure" @click="loginLogDialog?.Close">
                    <span class="textBox">
                        <span class="text">确认</span>
                    </span>
                </div>
            </template>
        </MessageDialog>

        <!-- 修改密码对话框 -->
        <MessageDialog ref="updatePasswordDialog" :width="'450px'">
            <template v-slot:title>
                <span>修改密码</span>
            </template>
            <template v-slot:content>
                <div class="updatePassword">
                    <div class="inputGroup">
                        <label for="NovaMailOldPassword">旧密码</label>
                        <input type="password" v-model="updatePasswordData.oldPassword" id="NovaMailOldPassword" placeholder="请输入旧密码">
                    </div>
                    <div class="inputGroup">
                        <label for="NovaMailNewPassword">新密码</label>
                        <input type="password" v-model="updatePasswordData.newPassword" id="NovaMailNewPassword" placeholder="请输入新密码">
                    </div>
                    <div class="inputGroup">
                        <label for="NovaMailConfirmPassword">确认密码</label>
                        <input type="password" v-model="updatePasswordData.confirmPassword" id="NovaMailConfirmPassword" placeholder="请确认新密码">
                    </div>
                </div>
            </template>
            <template v-slot:footer>
                <div class="dialogFooterButton failure" @click="updatePasswordDialog?.Close">
                    <span class="textBox">
                        <span class="iconfont icon-likesprogramcuowu"></span>
                        <span class="text">取消</span>
                    </span>
                </div>
                <div class="dialogFooterButton failure" @click="updatePassword">
                    <span class="textBox">
                        <span class="iconfont icon-likesprogramzhengque"></span>
                        <span class="text">确认</span>
                    </span>
                </div>
            </template>
        </MessageDialog>

        <!-- 消息提示 -->
        <ShowMessage ref="showMessage"/>
    </div>
</template>

<script lang="ts">
import { onMounted, ref } from 'vue';
import { UserStore } from '@store/UserStore';
import { formatDateTime, formatIpLocation } from '@utils/Utils';
import { UserInfo, type HistoryAvatar, type LoginLog, type UpdatePasswordData } from '@api/users/UserInfo';
import { SignatureService } from '@service/SignatureService';
import { type Signature } from '@api/users/Signature';
import { userService } from '@service/UserService';
import ImageProcessingDialog from '@components/messageDialog/ImageProcessingDialog.vue';
import MessageDialog from '@components/messageDialog/MessageDialog.vue';
import ShowMessage from '@components/messageDialog/ShowMessage.vue';
import { AuthService } from '@service/AuthService';

export default {
    name: 'UserInformation',
}
</script>

<script setup lang="ts">
// 用户
const userStore = UserStore();

// 对话框实例
const showMessage = ref<InstanceType<typeof ShowMessage>>();

const form = ref<{
    nickname: string;
    resume: string;
}>({
    nickname: userStore.nickname,
    resume: userStore.resume
});
// 昵称或简介输入框失去焦点时更新用户信息
const updateUserInfo = async (nickname?: string, resume?: string) => {
    if (nickname !== undefined) {
        userService.updateUserInfo(nickname).then(() => { }).catch((error) => {
            setTimeout(() => {
                showMessage.value?.ShowModal("昵称更新失败", ["昵称更新失败：" + error.message, "是否重试"], "failure", (selectedButtonIndex) => {
                    if(selectedButtonIndex === 1) updateUserInfo(nickname, resume);
                });
            }, 300);
        });
    }
    if (resume !== undefined) {
        userService.updateUserInfo(undefined, resume).then(() => { }).catch((error) => {
            setTimeout(() => {
                showMessage.value?.ShowModal("简介更新失败", ["简介更新失败：" + error.message, "是否重试"], "failure", (selectedButtonIndex) => {
                    if(selectedButtonIndex === 1) updateUserInfo(nickname, resume);
                });
            }, 300);
        });
    }
};

// 图片编辑对话框
const imageProcessingDialog = ref<typeof ImageProcessingDialog>();
// 修改头像
const updateAvatar = (historyAvatar: HistoryAvatar) => {
    if (historyAvatar.isCurrent) return;

    // 调用API上传头像
    UserInfo.updateAvatar({ fileName: historyAvatar.avatarName }).then(() => {
        // 更新用户信息
        userService.refreshUser();
        
        // 更新历史头像列表
        getHistoryAvatars();
    }).catch((error) => {
        setTimeout(() => {
            showMessage.value?.ShowModal("头像更新失败", ["头像更新失败：" + error.message, "是否重试"], "failure", (selectedButtonIndex) => {
                if(selectedButtonIndex === 1) updateAvatar(historyAvatar);
            });
        }, 300);
    });
};
// 返回图片
const returnImage = (image: string) => {
    // 调用API上传头像
    UserInfo.updateAvatar({ base64: image }).then(() => {
        // 更新用户信息
        userService.refreshUser();
        
        // 更新历史头像列表
        getHistoryAvatars();
    }).catch((error) => {
        setTimeout(() => {
            showMessage.value?.ShowModal("头像上传失败", ["头像上传失败：" + error.message, "是否重试"], "failure", (selectedButtonIndex) => {
                if(selectedButtonIndex === 1) returnImage(image);
            });
        }, 300);
    });
};

// 历史头像
const historyAvatars = ref<HistoryAvatar[]>();
// 签名列表
const signatures = ref<Signature[]>();
// 默认签名ID
const setDefaultSignature = async (signature: Signature) => {
    try {
        await SignatureService.setDefaultSignature(signature);
        getSignatures();
    } catch (error) {
        setTimeout(() => {
            showMessage.value?.ShowModal("设置默认签名失败", ["设置默认签名失败，是否重试"], "failure", (selectedButtonIndex) => {
                if(selectedButtonIndex === 1) setDefaultSignature(signature);
            });
        }, 300);
    }
};

// 获取历史头像
const getHistoryAvatars = async () => {
    try {
        const response = await UserInfo.getHistoryAvatars();
        historyAvatars.value = response.data;
    } catch (error) {
        setTimeout(() => {
            showMessage.value?.ShowModal("获取历史头像失败", ["获取历史头像失败，是否重试"], "failure", (selectedButtonIndex) => {
                if(selectedButtonIndex === 1) getHistoryAvatars();
            });
        }, 300);
    }
};

// 获取签名列表
const getSignatures = async () => {
    try {
        const response = await SignatureService.getSignatures();
        signatures.value = response.data;
    } catch (error) {
        setTimeout(() => {
            showMessage.value?.ShowModal("获取签名列表失败", ["获取签名列表失败，是否重试"], "failure", (selectedButtonIndex) => {
                if(selectedButtonIndex === 1) getSignatures();
            });
        }, 300);
    }
};

// 签名预览对话框
const signaturePreviewDialog = ref<typeof MessageDialog>();
// 签名编辑状态
const signatureEdit = ref(false);
// 预览编辑中的签名
const signaturePreviewEdit = ref(false);
const newSignature = ref<Signature>({
    signatureId: '',
    signatureName: '',
    signatureContent: '',
    default: false,
    createTime: '',
    updateTime: '',
    editable: false
});
// 创建签名
const createSignature = () => {
    signatureEdit.value = true;
    signaturePreviewEdit.value = false;
    newSignature.value = {
        signatureId: '',
        signatureName: '新签名',
        signatureContent: '',
        default: false,
        createTime: '',
        updateTime: '',
        editable: true
    };
    signaturePreviewDialog.value?.ShowModal();
};
// 显示签名预览
const showSignaturePreview = async (signature: Signature, isExit: boolean = false) => {
    newSignature.value = signature;
    signatureEdit.value = isExit;
    signaturePreviewEdit.value = false;
    try {
        const renderedContent = await SignatureService.getSignatureContent(signature.signatureId);
        newSignature.value.signatureContent = renderedContent.data;
        signaturePreviewDialog.value?.ShowModal();
    } catch (error) {
        setTimeout(() => {
            showMessage.value?.ShowModal("获取签名内容失败", ["获取签名内容失败，是否重试"], "failure", (selectedButtonIndex) => {
                if(selectedButtonIndex === 1) showSignaturePreview(signature, isExit);
            });
        }, 300);
    }
};
// 签名编辑完成
const completeSignatureEdit = () => {
    if (newSignature.value.signatureId) {
        // 已存在签名，执行更新操作
        SignatureService.updateSignature(newSignature.value).then(() => {
            getSignatures();
            signaturePreviewDialog.value?.Close();
        }).catch((error) => {
            setTimeout(() => {
                showMessage.value?.ShowModal("更新签名失败", ["更新签名失败：" + error.message, "是否重试"], "failure", (selectedButtonIndex) => {
                    if(selectedButtonIndex === 1) completeSignatureEdit();
                });
            }, 300);
        });
    } else {
        // 新签名，执行创建操作
        SignatureService.createSignature(newSignature.value).then(() => {
            getSignatures();
            signaturePreviewDialog.value?.Close();
        }).catch((error) => {
            setTimeout(() => {
                showMessage.value?.ShowModal("创建签名失败", ["创建签名失败：" + error.message, "是否重试"], "failure", (selectedButtonIndex) => {
                    if(selectedButtonIndex === 1) completeSignatureEdit();
                });
            }, 300);
        });
    }
};
// 删除签名
const deleteSignature = (signature: Signature) => {
    showMessage.value?.ShowModal("确认删除", ["确定要删除这个签名吗？"], "warning", (selectedButtonIndex) => {
        if(selectedButtonIndex === 1) {
            executeDeleteSignature(signature);
        }
    });
};
const executeDeleteSignature = (signature: Signature) => {
    SignatureService.deleteSignature(signature).then(() => {
        getSignatures();
    }).catch((error) => {
        setTimeout(() => {
            showMessage.value?.ShowModal("删除签名失败", ["删除签名失败：" + error.message, "是否重试"], "failure", (selectedButtonIndex) => {
                if(selectedButtonIndex === 1) executeDeleteSignature(signature);
            });
        }, 300);
    });
};

// 组件挂载
onMounted(() => {
    userService.getUserInfo().then(() => {
        form.value.nickname = userStore.nickname;
        form.value.resume = userStore.resume;
    });
    // 获取历史头像
    getHistoryAvatars();
    // 获取签名列表
    getSignatures();
});

// 查看登录日志
const loginLogDialog = ref<typeof MessageDialog>();
const loginLogs = ref<LoginLog[]>([]);
const viewLoginLog = () => {
    userService.getLoginLogs(userStore.account).then((response: LoginLog[]) => {
        loginLogs.value = response;
        // 这里可以将 response.data 赋值给一个 ref 变量，然后在登录日志对话框中展示
        loginLogDialog.value?.ShowModal();
    }).catch((error) => {
        setTimeout(() => {
            showMessage.value?.ShowModal("获取登录日志失败", ["获取登录日志失败:" + error.message, "是否重试"], "failure", (selectedButtonIndex) => {
                if(selectedButtonIndex === 1) viewLoginLog();
            });
        }, 300);
    });
}

// 修改密码
const updatePasswordData = ref<UpdatePasswordData>({
    oldPassword: '',
    newPassword: '',
    confirmPassword: ''
});
// 验证信息完整性
const ValidateInfo = (): boolean => {
    if (!updatePasswordData.value.oldPassword) {
        showMessage.value?.ShowModal("信息不完整", ["请输入旧密码！"], "warning");
        return false;
    }
    if (!updatePasswordData.value.newPassword) {
        showMessage.value?.ShowModal("信息不完整", ["请输入新密码！"], "warning");
        return false;
    }
    if (!updatePasswordData.value.confirmPassword) {
        showMessage.value?.ShowModal("信息不完整", ["请确认新密码！"], "warning");
        return false;
    }
    if (updatePasswordData.value.newPassword === updatePasswordData.value.oldPassword) {
        showMessage.value?.ShowModal("信息不完整", ["新密码不能与旧密码相同！"], "warning");
        return false;
    }
    // 密码长度至少8位，且包含大小写字母、数字和特殊字符
    const passwordRegex = /^(?=.*[a-z])(?=.*[A-Z])(?=.*\d)(?=.*[!@#$%^&*()_+\-=\[\]{};':"\\|,.<>/?]).+$/;
    if (updatePasswordData.value.newPassword.length < 8 || !passwordRegex.test(updatePasswordData.value.newPassword)) {
        showMessage.value?.ShowModal("信息不完整", ["密码长度不得少于8位，且包含","大小写字母、数字和特殊字符！"], "warning");
        return false;
    }
    if (updatePasswordData.value.newPassword !== updatePasswordData.value.confirmPassword) {
        showMessage.value?.ShowModal("信息不完整", ["两次输入的密码不一致！"], "warning");
        return false;
    }

    return true;
};
const updatePasswordDialog = ref<typeof MessageDialog>();
const showUpdatePasswordDialog = () => {
    updatePasswordData.value = {
        oldPassword: '',
        newPassword: '',
        confirmPassword: ''
    };
    updatePasswordDialog.value?.ShowModal();
};
const updatePassword = () => {
    if (!ValidateInfo()) return;
    userService.updatePassword(updatePasswordData.value).then(() => {
        showMessage.value?.ShowModal("密码修改成功", ["密码修改成功，请使用新密码重新登录！"], "success", () => {
            console.log("aaaaa");
            AuthService.logout(); // 退出登录
        });
    }).catch((error) => {
        setTimeout(() => {
            showMessage.value?.ShowModal("密码修改失败", ["密码修改失败：" + error.message, "是否重试"], "failure", (selectedButtonIndex) => {
                if(selectedButtonIndex === 1) updatePassword();
            });
        }, 300);
    });
}
</script>

<style src="@views/settings/SettingItem.css" scoped></style>

<style scoped>
.userInformation {
    width: 100%;
    position: relative;
    z-index: 999;
    top: 0;
    left: 0;
    padding: 0px 10px;
}

/* 头像展示 */
.userInformation .avatar {
    width: 60px;
    height: 60px;
    border-radius: 50%;
    flex-shrink: 0;
    overflow: hidden;
    display: flex;
    justify-content: center;
    align-items: center;
}
.userInformation .avatar img {
    width: 100%;
    height: 100%;
    object-fit: cover;
}

/* 历史头像展示 */
.userInformation .historyAvatars {
    display: flex;
    gap: 33px;
    flex-wrap: wrap;
}
.userInformation .historyAvatars .historyAvatar {
    display: flex;
    flex-direction: column;
    justify-content: center;
    align-items: center;
    cursor: pointer;
    gap: 5px;
}
.userInformation .historyAvatars .historyAvatar .avatar {
    border: 1px solid transparent;
}
.userInformation .historyAvatars .historyAvatar .uploadTime {
    font-size: 12px;
    color: var(--text-secondary1);
}
.userInformation .historyAvatars .historyAvatar:hover .avatar,
.userInformation .historyAvatars .historyAvatar.choose .avatar {
    border: 1px solid var(--border-facus);
}
.userInformation .historyAvatars .historyAvatar:hover .uploadTime,
.userInformation .historyAvatars .historyAvatar.choose .uploadTime {
    font-size: 12px;
    color: var(--text-link);
}

/* 昵称输入框 */
.userInformation .nicknameInput {
    flex: 1;
    font-size: 14px;
}
/* 简介输入框 */
.userInformation .resumeInput {
    flex: 1;
    height: 60px;
    font-size: 14px;
    resize: none;
}

/* 签名区域 */
.userInformation .settingItem .settingContent .settingBox.signatureBox {
    margin-left: 0px;
}
.userInformation .signatureBox .settings {
    gap: 0px;
    max-height: 170px;
}
.userInformation .signatureBox .settings .signature {
    width: 100%;
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 5px 0px;
    gap: 10px;
    border-bottom: 1px solid var(--bg-active-primary);
}
.userInformation .signatureBox .settings .signature:last-child {
    border-bottom: none;
}
.userInformation .signatureBox .settings .signature .checkbox {
    flex-shrink: 0;
}
.userInformation .signatureBox .settings .signature .title {
    width: 100%;
    font-size: 14px;
    /* 单行文本省略 */
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.userInformation .signatureBox .signatures .signature .operation button {
    padding: 5px 10px;
}

/* 签名编辑区域 */
.signatureEditContent {
    width: 700px;
    height: 400px;
    display: flex;
    flex-direction: column;
    justify-content: center;
    align-items: center;
    gap: 10px;
    padding: 10px;
    font-size: 14px;
}
.signatureEditContent .signatureName {
    width: 100%;
    display: flex;
    flex-shrink: 0;
    justify-content: flex-start;
    align-items: center;
    gap: 10px;
}
.signatureEditContent .signatureName .title {
    flex-shrink: 0;
}
.signatureEditContent .signatureName .input {
    flex: 1;
}
.signatureEditContent .signatureContent {
    width: 100%;
    flex: 1;
    display: flex;
    flex-direction: column;
    justify-content: center;
    align-items: flex-start;
    gap: 5px;
}
.signatureEditContent .signatureContent .title,
.signatureEditContent .signatureContent .prompt {
    width: 100%;
    flex-shrink: 0;
}
.signatureEditContent .signatureContent .title .text {
    flex-shrink: 0;
}
.signatureEditContent .signatureContent .title .subtitle {
    font-size: 12px;
    color: var(--text-secondary1);
}
.signatureEditContent .signatureContent .prompt {
    width: 100%;
    font-size: 12px;
    padding: 0px 10px;
    padding-bottom: 5px;
    color: var(--text-secondary1);
    display: flex;
    flex-direction: column;
    justify-content: center;
    align-items: flex-start;
}
.signatureEditContent .signatureContent .prompt .prompts {
    width: 100%;
    display: flex;
    justify-content:space-between;
    align-items: center;
}
.signatureEditContent .signatureContent .prompt .prompts .promptItem {
    display: flex;
    justify-content: center;
    align-items: center;
    gap: 5px;
}
.signatureEditContent .signatureContent .prompt .prompts .promptItem:not(:last-child) .description {
    padding-right: 5px;
    border-right: 1px solid var(--border-hover);
}
.signatureEditContent .signatureContent .prompt .prompts .promptItem .label {
    color: var(--color-warning);
}
.signatureEditContent .signatureContent .signatureContentEdit {
    width: 100%;
    flex: 1;
    min-height: 0;
    position: relative;
}
.signatureEditContent .signatureContent .signatureContentEdit .input {
    width: 100%;
    height: 100%;
    box-sizing: border-box;
    resize: none;
    overflow: auto;
}

/* 签名预览 */
.signaturePreviewContent {
    min-width: 300px;
    min-height: 150px;
    max-width: 700px;
    max-height: 300px;
    padding: 10px;
    display: flex;
    justify-content: center;
    align-items: center;
    overflow: auto;
}

/* 最后登录时间 */
.userInformation .lastLoginTime, .userInformation .registerTime {
    flex: 1;
    font-size: 14px;
    gap: 20px;
    display: flex;
    min-width: 0px;
    color: var(--text-secondary1);
}

.userInformation .lastLoginTime span, .userInformation .registerTime span {
    /* 单行文本省略 */
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}

/* 登录日志 */
.loginLogs {
    width: 780px;
    max-height: 400px;
    overflow: auto;
    display: flex;
    flex-direction: column;
    gap: 5px;
    padding: 0px 10px;
}
.loginLogs .logItem {
    width: 100%;
    display: flex;
    justify-content: space-between;
    align-items: center;
    gap: 10px;
    font-size: 12px;
    padding: 10px 0px;
    border-bottom: 1px solid var(--bg-active-primary);
    transition: all .3s;
}
.loginLogs .logItem:last-child {
    border-bottom: none;
}
.loginLogs .logItem:not(:first-child):hover { 
    background-color: var(--bg-hover-primary);
}.loginLogs .logItem:not(:first-child):active {
    transition: all .1s; 
    background-color: var(--bg-active-primary);
}
.loginLogs .logItem span {
    display: flex;
    justify-content: center;
    align-items: center;
    min-width: 0;
}
.loginLogs .logItem .logTime {
    width: 130px;
    flex-shrink: 0;
}
.loginLogs .logItem .logClientIp {
    width: 90px;
    flex-shrink: 0;
}
.loginLogs .logItem .logLocation {
    flex: 1;
    min-width: 0;
}
.loginLogs .logItem .logLocation .text {
    display: block;
    min-width: 0;

    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.loginLogs .logItem .logBrowser {
    width: 90px;
    flex-shrink: 0;
}
.loginLogs .logItem .logDeviceType,
.loginLogs .logItem .loginResult {
    width: 50px;
    flex-shrink: 0;
}
.loginLogs .logItem .loginResult.SUCCESS {
    font-size: 10px;
    color: var(--color-success);
}
.loginLogs .logItem .loginResult.FAIL {
    font-size: 10px;
    color: var(--color-error);
}

/* 密码修改 */
.updatePassword {
    width: 350px;
    padding: 0px 15px;
    display: flex;
    flex-direction: column;
    gap: 15px;
    font-size: 14px;
}
.updatePassword .inputGroup {
    display: flex;
    gap: 5px;
}
.updatePassword .inputGroup label {
    width: 80px;
    flex-shrink: 0;
    display: flex;
    justify-content: center;
    align-items: center;
}
.updatePassword .inputGroup input {
    flex: 1;
    padding: 5px 10px;
    font-size: 14px;
}
</style>
