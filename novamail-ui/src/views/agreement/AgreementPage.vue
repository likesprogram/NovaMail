<template>
    <main class="agreementPage">
        <section class="agreementShell">
            <header class="agreementHeader">
                <div class="titleBox">
                    <span class="brand">NovaMail</span>
                    <h1>{{ title }}</h1>
                    <p>{{ description }}</p>
                </div>
                <nav class="agreementTabs" aria-label="协议导航">
                    <router-link class="tab" :to="{ name: 'userAgreement' }">
                        <span class="iconfont icon-likesprogramzhishiku"></span>
                        用户协议
                    </router-link>
                    <router-link class="tab" :to="{ name: 'privacyPolicy' }">
                        <span class="iconfont icon-likesprogrammimabukejian"></span>
                        隐私政策
                    </router-link>
                </nav>
            </header>
            
            <div class="metaBar">
                <span>生效日期：{{ effectiveDate }}</span>
                <span>最近更新：{{ updatedDate }}</span>
            </div>

            <article class="agreementBody">
                <slot></slot>
            </article>

            <footer class="agreementFooter">
                <button class="btn-primary" type="button" @click="goBack">
                    <span class="iconfont icon-likesprograma-shangyibu1"></span>
                    返回
                </button>
                <router-link class="button btn-primary homeLink" :to="{ name: 'login' }">
                    <span class="iconfont icon-likesprogramyonghu"></span>
                    前往登录
                </router-link>
            </footer>
        </section>
    </main>
</template>

<script setup lang="ts">
import { useRouter } from 'vue-router';

defineProps<{
    title: string;
    description: string;
    effectiveDate: string;
    updatedDate: string;
}>();

const router = useRouter();

const goBack = () => {
    if (window.history.length > 1) {
        router.back();
        return;
    }
    router.push({ name: 'login' });
};
</script>

<style scoped>
.agreementPage {
    width: 100%;
    height: 100%;
    min-width: 0;
    min-height: 0;
    padding: 32px;
    color: var(--text-primary);
}

.agreementShell {
    min-width: 0;
    min-height: 0;
    width: 100%;
    height: 100%;
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

.agreementHeader {
    display: flex;
    align-items: flex-start;
    justify-content: space-between;
    width: 100%;
    flex-shrink: 0;
    gap: 24px;
}

.titleBox {
    min-width: 0;
}

.brand {
    display: block;
    margin-bottom: 8px;
    font-size: 14px;
    font-weight: 700;
    color: var(--text-link);
}

h1 {
    margin: 0;
    font-size: 28px;
    line-height: 1.3;
    font-weight: 700;
}

p {
    margin: 8px 0 0;
    color: var(--text-secondary2);
    line-height: 1.7;
}

.agreementTabs {
    flex-shrink: 0;
    display: flex;
    gap: 8px;
}

.tab {
    min-width: 92px;
    height: 34px;
    padding: 0 12px;
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 6px;
    border-radius: 5px;
    color: var(--text-secondary2);
    background-color: var(--bg-secondary4);
    transition: all .2s;
}

.tab .iconfont {
    font-size: 22px;
}

.tab:hover {
    color: var(--text-link-hover);
    background-color: var(--bg-hover-secondary);
}

.tab.router-link-active,
.tab.router-link-exact-active {
    color: var(--text-link);
    background-color: var(--bg-hover-primary);
}

.metaBar {
    width: 100%;
    display: flex;
    justify-content: space-between;
    align-items: center;
    display: flex;
    flex-wrap: wrap;
    gap: 8px 24px;
    color: var(--text-tertiary);
    font-size: 14px;
    border-bottom: 1px solid var(--bg-active-primary);
    padding: 10px 0px;
    margin-bottom: 15px;
}

.agreementBody {
    flex: 1;
    width: 100%;
    line-height: 1.85;
    color: var(--text-secondary1);
    user-select: text;
    overflow: auto;
}

.agreementBody :deep(h2) {
    margin: 20px 0 0;
    font-size: 19px;
    line-height: 1.45;
    color: var(--text-primary);
}

.agreementBody :deep(p) {
    margin: 8px 0;
}

.agreementBody :deep(ul) {
    margin: 8px 0 0 20px;
}

.agreementBody :deep(li) {
    margin: 6px 0;
    padding-left: 2px;
}

.agreementFooter {
    width: 100%;
    margin-top: 15px;
    padding-top: 20px;
    border-top: 1px solid var(--bg-active-primary);
    flex-shrink: 0;
    display: flex;
    justify-content: space-between;
    align-items: center;
    gap: 12px;
}

.homeLink {
    min-width: 76px;
}
</style>
