<template>
    <div class="footer">
        <div class="footerContent">
            <span class="footerContentItem">© 2025-现在 龙兵寅版权所有</span>
            <span class="footerContentItem" v-for="(filing, index) in filings" :key="index">
                <button @click="openFilingVerify(filing)">
                    <img :src="filing.filingIcon" width="20" :alt="filing.filingName"></img>
                    {{ filing.filingCodeText }}
                </button>
            </span>
        </div>
        <div class="footerContent">
            <span class="footerContentItem">
                联系邮箱：{{ email }}
            </span>
        </div>
        <div class="footerContent">
            <span class="footerContentItem"></span>
        </div>
    </div>
</template>

<script lang="ts">
import { defineComponent, ref } from 'vue';
export default defineComponent({
    name: 'Footer',
});

// 备案信息
export type Filing = {
    filingName: string,
    filingIcon?: string,
    filingCode?: string,
    filingCodeText?: string,
    verifyUrl?: string
}
</script>

<script setup lang="ts">
const filings = ref<Filing[]>([
    {
        filingName: 'ICP 备案',
        filingIcon: '/icons/official/icp.png',
        filingCode: '湘ICP备2020020576号-1',
        filingCodeText: '湘ICP备2020020576号-1',
        verifyUrl: 'https://beian.miit.gov.cn/#/Integrated/recordQuery'
    },
    {
        filingName: '公安备案',
        filingIcon: '/icons/official/gongan.png',
        filingCode: '湘公网安备43312302000077号',
        filingCodeText: '湘公网安备43312302000077号',
        verifyUrl: 'https://beian.mps.gov.cn/#/query/webSearch'
    }
]);
// 联系方式
const email = '1215065@qq.com';

// 打开备案验证链接并复制备案号
const openFilingVerify = (filing: Filing) => {
    // 将备案号复制到剪贴板
    if (filing.filingCode) {
        navigator.clipboard.writeText(filing.filingCode);
        alert(filing.filingName + '备案号已复制到剪贴板！');
    }
    // 打开验证链接
    if (filing.verifyUrl) {
        window.open(filing.verifyUrl);
    }
};
</script>

<style scoped>
.footer {
    width: 100%;
    flex-shrink: 0;
    display: flex;
    gap: 5px;
    flex-direction: column;
    justify-content: center;
    align-items: center;
    overflow: hidden;
}

.footer .footerContent {
    display: flex;
    flex-wrap: wrap;
    justify-content: center;
    align-items: center;
    font-size: 13px;
    color: var(--text-secondary1);
}
.footer .footerContent .footerContentItem {
    display: flex;
    justify-content: center;
    align-items: center;
    padding: 0px 10px;
    position: relative;
    white-space: nowrap;

}
.footer .footerContent .footerContentItem * {
    display: flex;
    justify-content: center;
    align-items: center;
    position: relative;
    gap: 5px;
    white-space: nowrap;
    color: var(--text-secondary1);
}
/* .footer .footerContent .footerContentItem::after {
    content: "";
    position: absolute;
    right: 0;
    top: 50%;
    transform: translateY(-50%);
    width: 1px;
    height: 14px;
    background-color: #ccc;
}.footer .footerContent .footerContentItem:last-child::after {
    display: none;
} */
</style>
