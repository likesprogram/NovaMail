<template>
    <div class="loginAndRegister">
        <div class="body">
            <div class="header">
                <router-link class="button btn-primary" to="/">
                    <span class="iconfont icon-likesprogramshouye"></span>
                    <span class="text">首页</span>
                </router-link>
                <router-link class="button btn-primary" :to="{ name: 'login' }">
                    <span class="iconfont icon-likesprogramyonghu"></span>
                    <span class="text">登录</span>
                </router-link>
                <router-link class="button btn-primary" :to="{ name: 'register' }">
                    <span class="iconfont icon-likesprogramuser_informationyonghuxinxi"></span>
                    <span class="text">注册</span>
                </router-link>
            </div>
            <div class="content">
                <div class="leftImage">
                    <img src="@assets/illustration/Login.svg" alt="">
                </div>
                <div class="rightContent">
                    <router-view></router-view>
                </div>
            </div>
        </div>
    </div>
</template>

<script lang="ts">
export default {
    name: 'LoginAndRegister',
}
</script>

<script setup lang="ts">

</script>

<style scoped>
.loginAndRegister {
    width: 100%;
    height: 100%;
    padding: 20px;
    position: relative;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    overflow: hidden;
    overflow-y: auto;
}
.loginAndRegister .body {
    min-width: 0;
    min-height: 0;
    display: flex;
    position: relative;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    padding: 15px;
    border-radius: 10px;
    background-color: rgba(255, 255, 255, .8);
	backdrop-filter: blur(2rem);
	-webkit-backdrop-filter: blur(2rem);
	-moz-backdrop-filter: blur(2rem);
	-ms-backdrop-filter: blur(2rem);
    box-shadow: var(--shadow-card);
}

/* 头部 */
.loginAndRegister .header {
    width: 100%;
    height: 40px;
    padding: 0 10px;
    display: flex;
    align-items: center;
    justify-content: space-between;
}
.loginAndRegister .header .button { 
    display: flex;
    align-items: center;
    justify-content: center;
    font-size: 14px;
    text-decoration: none;
    cursor: pointer;
    border-radius: 5px;
    padding: 5px 10px;
    gap: 5px;
}
.loginAndRegister .header .button .iconfont { 
    font-size: 18px;
}
.loginAndRegister .header .button.router-link-active,
.loginAndRegister .header .button.router-link-exact-active {
    display: none;
}

.loginAndRegister .content {
    flex: 1;
    position: relative;
    z-index: 1;
    padding: 10px;
    padding-top: 0px;
    display: flex;
    align-items: center;
    justify-content: center;
}

.loginAndRegister .content .leftImage {
    max-width: 350px;
    height: 100%;
    padding-right: 20px;
    display: flex;
    align-items: center;
    justify-content: center;
    position: relative;
    overflow: hidden;
}
.loginAndRegister .content .leftImage::after { 
    content: '';
    position: absolute;
    top: 0;
    right: 0;
    width: 1px;
    height: 100%;
    background-color: rgba(65, 185, 255, .2);
}
.loginAndRegister .content .leftImage img { 
    width: 100%;
    height: auto;           /* 保持比例自适应 */
    max-height: 100%;       /* 不超过父容器高度 */
    object-fit: contain;    /* 保持完整显示 */
}

.loginAndRegister .content .rightContent { 
    flex-shrink: 0;
    padding-left: 20px;
    display: flex;
    align-items: center;
    justify-content: center;
}
</style>
