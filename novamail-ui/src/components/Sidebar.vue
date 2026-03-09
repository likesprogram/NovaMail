<template>
    <div class="sidebar">
        <div class="menuBox">
            <div class="menuList">
                <router-link class="menuItem" to="/compose">
                    <span class="iconfont icon-likesprogramxiexin"></span>
                    <span class="title">写信</span>
                </router-link>
                <router-link class="menuItem" to="/inbox">
                    <span class="iconfont icon-likesprogramyoujian" style="font-size: 20px; padding: 1px;"></span>
                    <span class="title">收件箱</span>
                    <span class="count">15</span>
                </router-link>
                <router-link class="menuItem" to="/sent">
                    <span class="iconfont icon-likesprogramfasong"></span>
                    <span class="title">已发送</span>
                    <span class="count">3</span>
                </router-link>
            </div>
            <div class="menuList">
                <router-link class="menuItem" to="/star">
                    <span class="iconfont icon-likesprograma-shoucang"></span>
                    <span class="title">星标件</span>
                    <span class="count">0</span>
                </router-link>
                <router-link class="menuItem" to="/draft">
                    <span class="iconfont icon-likesprogramcaogaoxiang"></span>
                    <span class="title">草稿箱</span>
                </router-link>
                <router-link class="menuItem" to="/deleted">
                    <span class="iconfont icon-likesprogramhuishouzhan"></span>
                    <span class="title">已删除</span>
                </router-link>
                <router-link class="menuItem" to="/trash">
                    <span class="iconfont icon-likesprogramkuloutou"></span>
                    <span class="title">垃圾箱</span>
                    <span class="count">99</span>
                </router-link>
                <router-link class="menuItem" to="/advertisement">
                    <span class="iconfont icon-likesprogramjubao"></span>
                    <span class="title">广告箱</span>
                    <span class="count">999+</span>
                </router-link>
            </div>
            <div class="menuList" :class="{ 'menuListNoBorder': !isWindows }">
                <router-link class="menuItem" to="/address">
                    <span class="iconfont icon-likesprogramtongxunlu"></span>
                    <span class="title">通讯录</span>
                    <span class="count">2</span>
                </router-link>
                <router-link class="menuItem" to="/files">
                    <span class="iconfont icon-likesprogramwenzhang"></span>
                    <span class="title">文件中转</span>
                </router-link>
            </div>
            <!-- 下载按钮 -->
            <button v-if="isWindows" class="download btn-primary button">
                <span class="iconfont icon-likesprogramxiazai"></span>
                <span class="title">下载客户端</span>
            </button>
        </div>
        <Footer></Footer>
    </div>
</template>

<script lang="ts">
import { defineComponent, onMounted, ref } from 'vue';
export default defineComponent({
    name: 'Sidebar',
});
</script>

<script setup lang="ts">
import Footer from '@components/Footer.vue'

const isWindows = ref(false);

onMounted(() => {
  const ua = navigator.userAgent || navigator.vendor || (window as any).opera;
  isWindows.value = /win/i.test(ua.toLowerCase());;
});

</script>

<style scoped>
.sidebar {
    width: 220px;
    flex-shrink: 0;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: space-between;
    padding-bottom: 20px;
}
.sidebar .menuBox {
    width: 100%;
    flex: 1;
    display: flex;
    flex-direction: column;
    align-items: center;
}
.sidebar .menuBox .menuList { 
    width: 100%;
    padding: 5px 15px;
    padding-top: 0px;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: flex-start;
    gap: 5px;

}.sidebar .menuBox .menuList::after { 
    content: '';
    width: 80%;
    height: 1px;
    background-color: var(--bg-active-primary);
}.sidebar .menuBox .menuList.menuListNoBorder::after { 
    display: none;
}
.sidebar .menuBox .menuItem { 
    width: calc(100% - 30px);
    padding: 5px 10px;
    font-size: 14px;
    display: flex;
    align-items: center;
    justify-content: flex-start;
    gap: 5px;
    cursor: pointer;
    transition: all 0.3s;
    border-radius: 10px;
    color: var(--text-primary);
} .sidebar .menuBox .menuList .menuItem { 
    width: 100%;
}.sidebar .menuBox .menuItem:hover { 
    background-color: var(--bg-hover-primary);
}.sidebar .menuBox .menuItem:active {
    transition: all .1s; 
    background-color: var(--bg-active-primary);
}
.sidebar .menuBox .menuItem .iconfont { 
    font-size: 22px;
    flex-shrink: 0;
}
.sidebar .menuBox .menuItem .title { 
    font-size: 14px;
    flex: 1;
}
.sidebar .menuBox .menuItem .count { 
    font-size: 12px;
    flex-shrink: 0;
    font-weight: 700;
    color: var(--text-link);
}
.sidebar .menuBox .menuItem.router-link-active,
.sidebar .menuBox .menuItem.router-link-exact-active { 
    background-color: var(--bg-hover-primary);
}.sidebar .menuBox .menuItem.router-link-active .title,
.sidebar .menuBox .menuItem.router-link-exact-active .title { 
    font-weight: 700;
}

/* 下载按钮 */
.sidebar .menuBox .download {
    width: calc(100% - 50px);
    padding: 8px;
}.sidebar .menuBox .download .title { 
    flex: none;
    font-size: 14px;
}.sidebar .menuBox .download .iconfont { 
    font-size: 20px;
}
</style>
