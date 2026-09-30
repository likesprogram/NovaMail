<template>
    <div class="addressBook">
        <div class="addressHeader">
            <div class="titleGroup">
                <span class="iconfont icon-likesprogramtongxunlu"></span>
                <div class="titleText">
                    <span class="title">通讯录</span>
                    <span class="summary">{{ isLoading ? '加载中' : `${contacts.length} 位联系人 · ${contactGroups.length} 个分组` }}</span>
                </div>
            </div>
            <div class="headerActions">
                <button class="button btn-text" @click="openGroupCreate">
                    <span class="iconfont icon-likesprogramtianjiawenjianjia"></span>
                    <span>新建分组</span>
                </button>
                <button class="button btn-primary" @click="openContactCreate">
                    <span class="iconfont icon-likesprogramtianjiahaoyou"></span>
                    <span>新建联系人</span>
                </button>
            </div>
        </div>

        <div class="addressContent">
            <aside class="groupPanel">
                <div class="searchInput" :class="{ searchClearShown: searchKeyword.trim().length > 0 }">
                    <input v-model="searchKeyword" type="text" placeholder="搜索联系人或邮箱">
                    <button v-if="searchKeyword.trim().length > 0" class="button btn-text iconfont icon-likesprogramcancel"
                        title="清空搜索" @click="searchKeyword = ''"></button>
                    <div v-else class="button btn-text iconfont icon-likesprogramchaxun"></div>
                </div>

                <button class="groupItem" :class="{ selected: selectedGroupId === 'all' }" @click="selectGroup('all')">
                    <span class="groupName">
                        <span class="iconfont icon-likesprogramliebiao"></span>
                        <span>全部联系人</span>
                    </span>
                    <span class="groupCount">{{ contacts.length }}</span>
                </button>

                <button class="groupItem" :class="{ selected: selectedGroupId === 'ungrouped' }" @click="selectGroup('ungrouped')">
                    <span class="groupName">
                        <span class="iconfont icon-likesprogramyoujian2"></span>
                        <span>未分组</span>
                    </span>
                    <span class="groupCount">{{ getGroupContactCount(null) }}</span>
                </button>

                <div class="groupList">
                    <button v-for="group in contactGroups" :key="group.groupId" class="groupItem"
                        :class="{ selected: selectedGroupId === group.groupId }"
                        @click="selectGroup(group.groupId)">
                        <span class="groupName">
                            <span class="iconfont icon-likesprogramtianjiawenjianjia"></span>
                            <span>{{ group.groupName }}</span>
                        </span>
                        <span class="groupCount">{{ getGroupContactCount(group.groupId) }}</span>
                    </button>
                </div>
            </aside>

            <main class="contactPanel">
                <div class="listHeader">
                    <div class="listTitle">
                        <span>{{ selectedGroupName }}</span>
                        <span class="listCount">{{ filteredContacts.length }}</span>
                    </div>
                    <div class="listTools">
                        <button v-if="selectedCustomGroup" class="button btn-text" @click="openGroupEdit">
                            <span class="iconfont icon-likesprogrambianji"></span>
                            <span>重命名分组</span>
                        </button>
                        <button v-if="selectedCustomGroup" class="button btn-text danger" @click="deleteSelectedGroup">
                            <span class="iconfont icon-likesprogramhuishouzhan"></span>
                            <span>删除分组</span>
                        </button>
                        <button class="button btn-text" :class="{ 'btn-disabled': !selectedContact }" @click="openContactEdit">
                            <span class="iconfont icon-likesprogrambianji"></span>
                            <span>编辑</span>
                        </button>
                        <button class="button btn-text danger" :class="{ 'btn-disabled': !selectedContact }" @click="deleteSelectedContact">
                            <span class="iconfont icon-likesprogramhuishouzhan"></span>
                            <span>删除</span>
                        </button>
                    </div>
                </div>

                <div class="contactList">
                    <button v-for="contact in filteredContacts" :key="contact.contactId" class="contactItem"
                        :class="{ selected: selectedContactId === contact.contactId }"
                        @click="selectedContactId = contact.contactId">
                        <span class="avatar">
                            <img :src="getContactAvatarUrl(contact.email)" alt="avatar">
                        </span>
                        <span class="contactInfo">
                            <span class="contactName">{{ contact.contactName }}</span>
                            <span class="contactEmail">{{ contact.email }}</span>
                        </span>
                        <span class="contactGroupName">{{ getGroupName(contact.groupId) }}</span>
                    </button>

                    <div v-if="filteredContacts.length === 0" class="emptyContact">
                        <span class="iconfont icon-likesprogramchaxun"></span>
                        <span>没有匹配的联系人</span>
                    </div>
                </div>
            </main>

            <aside class="detailPanel">
                <template v-if="detailMode === 'view' && selectedContact">
                    <div class="detailHero">
                        <span class="avatar large">
                            <img :src="getContactAvatarUrl(selectedContact.email)" alt="avatar">
                        </span>
                        <span class="detailName">{{ selectedContact.contactName }}</span>
                        <span class="detailEmail">{{ selectedContact.email }}</span>
                    </div>

                    <div class="detailList">
                        <div class="detailItem">
                            <span class="label">所属分组</span>
                            <span class="value">{{ getGroupName(selectedContact.groupId) }}</span>
                        </div>
                        <div class="detailItem">
                            <span class="label">电话</span>
                            <span class="value">{{ selectedContact.phone || '未填写' }}</span>
                        </div>
                        <div class="detailItem">
                            <span class="label">备注</span>
                            <span class="value remark">{{ selectedContact.remark || '未填写' }}</span>
                        </div>
                    </div>
                </template>

                <template v-else-if="detailMode === 'contactForm'">
                    <div class="formHeader">
                        <span>{{ editingContactId ? '编辑联系人' : '新建联系人' }}</span>
                        <button class="button btn-text iconfont icon-likesprogramcancel" title="取消" @click="closeForm"></button>
                    </div>
                    <div class="formBody">
                        <label class="formField">
                            <span>姓名</span>
                            <input v-model="contactForm.contactName" type="text" placeholder="请输入联系人姓名">
                        </label>
                        <label class="formField">
                            <span>邮箱</span>
                            <input v-model="contactForm.email" type="text" placeholder="name@example.com">
                        </label>
                        <label class="formField">
                            <span>分组</span>
                            <span class="selectBox">
                                <select v-model="contactForm.groupId">
                                    <option :value="null">未分组</option>
                                    <option v-for="group in contactGroups" :key="group.groupId" :value="group.groupId">
                                        {{ group.groupName }}
                                    </option>
                                </select>
                                <span class="iconfont icon-likesprogramxiangyouxiayiye"></span>
                            </span>
                        </label>
                        <label class="formField">
                            <span>电话</span>
                            <input v-model="contactForm.phone" type="text" placeholder="可选">
                        </label>
                        <label class="formField">
                            <span>备注</span>
                            <textarea v-model="contactForm.remark" placeholder="可选"></textarea>
                        </label>
                    </div>
                    <div class="formActions">
                        <button class="button btn-text" @click="closeForm">取消</button>
                        <button class="button btn-primary" @click="saveContact">保存</button>
                    </div>
                </template>

                <template v-else-if="detailMode === 'groupForm'">
                    <div class="formHeader">
                        <span>{{ editingGroupId ? '重命名分组' : '新建分组' }}</span>
                        <button class="button btn-text iconfont icon-likesprogramcancel" title="取消" @click="closeForm"></button>
                    </div>
                    <div class="formBody">
                        <label class="formField">
                            <span>分组名称</span>
                            <input v-model="groupForm.groupName" type="text" placeholder="请输入分组名称">
                        </label>
                    </div>
                    <div class="formActions">
                        <button class="button btn-text" @click="closeForm">取消</button>
                        <button class="button btn-primary" @click="saveGroup">保存</button>
                    </div>
                </template>

                <div v-else class="emptyDetail">
                    <span class="iconfont icon-likesprogramtongxunlu"></span>
                    <span>选择联系人查看详情</span>
                </div>
            </aside>
        </div>

        <ShowMessage ref="showMessage"/>
    </div>
</template>

<script lang="ts">
export default {
    name: 'AddressBook',
}
</script>

<script setup lang="ts">
import { computed, onMounted, ref, watch } from 'vue';
import { ApiException, baseUrl } from '@utils/Request';
import ShowMessage from '@components/messageDialog/ShowMessage.vue';
import { AddressBookApi } from '@api/users/AddressBook';
import type { Contact, ContactGroup, GroupId } from '@api/users/AddressBook';

type SelectedGroupId = 'all' | 'ungrouped' | string;
type DetailMode = 'view' | 'contactForm' | 'groupForm';

const showMessage = ref<InstanceType<typeof ShowMessage>>();

const contactGroups = ref<ContactGroup[]>([]);
const contacts = ref<Contact[]>([]);

const selectedGroupId = ref<SelectedGroupId>('all');
const selectedContactId = ref('');
const searchKeyword = ref('');
const detailMode = ref<DetailMode>('view');
const editingContactId = ref<string>('');
const editingGroupId = ref<string>('');
const isLoading = ref(false);

const contactForm = ref({
    contactName: '',
    email: '',
    groupId: null as GroupId,
    phone: '',
    remark: '',
});
const groupForm = ref({ groupName: '' });

const normalizeEmail = (email: string) => email.trim().toLowerCase();
const getContactAvatarUrl = (email: string) => {
    const mailBoxAddress = encodeURIComponent(email.trim());
    return `${baseUrl}/userProfile/mailBoxAvatar?mailBoxAddress=${mailBoxAddress}&v=${mailBoxAddress}`;
};

const getGroupName = (groupId: GroupId) => {
    if (!groupId) return '未分组';
    return contactGroups.value.find(group => group.groupId === groupId)?.groupName ?? '未分组';
};

const getGroupContactCount = (groupId: GroupId) => {
    return contacts.value.filter(contact => contact.groupId === groupId).length;
};

const selectedGroupName = computed(() => {
    if (selectedGroupId.value === 'all') return '全部联系人';
    if (selectedGroupId.value === 'ungrouped') return '未分组';
    return getGroupName(selectedGroupId.value);
});

const filteredContacts = computed(() => {
    const keyword = searchKeyword.value.trim().toLowerCase();
    return contacts.value.filter(contact => {
        const matchGroup = selectedGroupId.value === 'all'
            || (selectedGroupId.value === 'ungrouped' && !contact.groupId)
            || contact.groupId === selectedGroupId.value;
        const matchKeyword = !keyword
            || contact.contactName.toLowerCase().includes(keyword)
            || contact.email.toLowerCase().includes(keyword)
            || getGroupName(contact.groupId).toLowerCase().includes(keyword)
            || contact.phone.toLowerCase().includes(keyword)
            || contact.remark.toLowerCase().includes(keyword);
        return matchGroup && matchKeyword;
    });
});

const selectedContact = computed(() => {
    return filteredContacts.value.find(contact => contact.contactId === selectedContactId.value) ?? null;
});
const selectedCustomGroup = computed(() => {
    if (selectedGroupId.value === 'all' || selectedGroupId.value === 'ungrouped') return null;
    const group = contactGroups.value.find(item => item.groupId === selectedGroupId.value) ?? null;
    return group?.groupType === 'CUSTOM' ? group : null;
});

const loadAddressBook = async (preferredContactId?: string) => {
    isLoading.value = true;
    try {
        const result = await AddressBookApi.get();
        contactGroups.value = result.data.groups;
        contacts.value = result.data.contacts;
        selectedContactId.value = preferredContactId && result.data.contacts.some(contact => contact.contactId === preferredContactId)
            ? preferredContactId
            : filteredContacts.value[0]?.contactId ?? result.data.contacts[0]?.contactId ?? '';
    } catch (error) {
        showMessage.value?.ShowModal("通讯录", ["通讯录加载失败，请稍后重试"], "failure");
    } finally {
        isLoading.value = false;
    }
};

const syncSelectedContact = () => {
    if (detailMode.value !== 'view') return;
    const isVisible = filteredContacts.value.some(contact => contact.contactId === selectedContactId.value);
    if (!isVisible) selectedContactId.value = filteredContacts.value[0]?.contactId ?? '';
};

watch([filteredContacts, detailMode], () => {
    syncSelectedContact();
});

const getErrorMessage = (error: unknown, fallback: string) => {
    return error instanceof ApiException ? error.message : fallback;
};

const selectGroup = (groupId: SelectedGroupId) => {
    selectedGroupId.value = groupId;
    detailMode.value = 'view';
    selectedContactId.value = filteredContacts.value[0]?.contactId ?? '';
};

const closeForm = () => {
    detailMode.value = 'view';
    editingContactId.value = '';
    editingGroupId.value = '';
};

const openContactCreate = () => {
    editingContactId.value = '';
    contactForm.value = {
        contactName: '',
        email: '',
        groupId: selectedGroupId.value === 'all' || selectedGroupId.value === 'ungrouped' ? null : selectedGroupId.value,
        phone: '',
        remark: '',
    };
    detailMode.value = 'contactForm';
};

const openContactEdit = () => {
    if (!selectedContact.value) return;
    editingContactId.value = selectedContact.value.contactId;
    contactForm.value = {
        contactName: selectedContact.value.contactName,
        email: selectedContact.value.email,
        groupId: selectedContact.value.groupId,
        phone: selectedContact.value.phone,
        remark: selectedContact.value.remark,
    };
    detailMode.value = 'contactForm';
};

const saveContact = async () => {
    const contactName = contactForm.value.contactName.trim();
    const email = normalizeEmail(contactForm.value.email);
    if (!contactName || !email) {
        showMessage.value?.ShowModal("保存联系人", ["联系人姓名和邮箱不能为空"], "failure");
        return;
    }
    if (!/^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(email)) {
        showMessage.value?.ShowModal("保存联系人", ["邮箱地址格式不正确"], "failure");
        return;
    }

    const duplicated = contacts.value.some(contact => normalizeEmail(contact.email) === email && contact.contactId !== editingContactId.value);
    if (duplicated) {
        showMessage.value?.ShowModal("保存联系人", ["该邮箱已经存在于通讯录中"], "failure");
        return;
    }

    try {
        const result = await AddressBookApi.saveContact({
            contactId: editingContactId.value || undefined,
            contactName,
            email,
            groupId: contactForm.value.groupId,
            phone: contactForm.value.phone.trim(),
            remark: contactForm.value.remark.trim(),
        });
        if (editingContactId.value) {
            contacts.value = contacts.value.map(contact => contact.contactId === result.data.contactId ? result.data : contact);
        } else {
            contacts.value = [result.data, ...contacts.value];
        }
        selectedContactId.value = result.data.contactId;
        closeForm();
        showMessage.value?.ShowModal("保存联系人", ["联系人已保存"], "success");
    } catch (error) {
        showMessage.value?.ShowModal("保存联系人", [getErrorMessage(error, "联系人保存失败，请稍后重试")], "failure");
    }
};

const deleteSelectedContact = async () => {
    if (!selectedContact.value) return;
    try {
        const deletedContactId = selectedContact.value.contactId;
        await AddressBookApi.deleteContact(deletedContactId);
        const nextContacts = contacts.value.filter(contact => contact.contactId !== deletedContactId);
        contacts.value = nextContacts;
        selectedContactId.value = filteredContacts.value[0]?.contactId ?? nextContacts[0]?.contactId ?? '';
        showMessage.value?.ShowModal("删除联系人", ["联系人已删除"], "success");
    } catch (error) {
        showMessage.value?.ShowModal("删除联系人", [getErrorMessage(error, "联系人删除失败，请稍后重试")], "failure");
    }
};

const openGroupCreate = () => {
    editingGroupId.value = '';
    groupForm.value.groupName = '';
    detailMode.value = 'groupForm';
};

const openGroupEdit = () => {
    if (!selectedCustomGroup.value) return;
    editingGroupId.value = selectedCustomGroup.value.groupId;
    groupForm.value.groupName = selectedCustomGroup.value.groupName;
    detailMode.value = 'groupForm';
};

const saveGroup = async () => {
    const groupName = groupForm.value.groupName.trim();
    if (!groupName) {
        showMessage.value?.ShowModal("保存分组", ["分组名称不能为空"], "failure");
        return;
    }
    if (contactGroups.value.some(group => group.groupName === groupName && group.groupId !== editingGroupId.value)) {
        showMessage.value?.ShowModal("保存分组", ["该分组名称已存在"], "failure");
        return;
    }
    try {
        const result = await AddressBookApi.saveGroup({
            groupId: editingGroupId.value || undefined,
            groupName,
        });
        if (editingGroupId.value) {
            contactGroups.value = contactGroups.value.map(group => group.groupId === result.data.groupId ? result.data : group);
        } else {
            contactGroups.value = [...contactGroups.value, result.data].sort((a, b) => a.sortOrder - b.sortOrder);
        }
        selectedGroupId.value = result.data.groupId;
        closeForm();
        showMessage.value?.ShowModal("保存分组", ["分组已保存"], "success");
    } catch (error) {
        showMessage.value?.ShowModal("保存分组", [getErrorMessage(error, "分组保存失败，请稍后重试")], "failure");
    }
};

const deleteSelectedGroup = async () => {
    if (!selectedCustomGroup.value) return;
    const groupId = selectedCustomGroup.value.groupId;
    try {
        await AddressBookApi.deleteGroup(groupId);
        contactGroups.value = contactGroups.value.filter(group => group.groupId !== groupId);
        contacts.value = contacts.value.map(contact => contact.groupId === groupId ? { ...contact, groupId: null } : contact);
        selectedGroupId.value = 'all';
        selectedContactId.value = contacts.value[0]?.contactId ?? '';
        detailMode.value = 'view';
        showMessage.value?.ShowModal("删除分组", ["分组已删除，原分组联系人已移入未分组"], "success");
    } catch (error) {
        showMessage.value?.ShowModal("删除分组", [getErrorMessage(error, "分组删除失败，请稍后重试")], "failure");
    }
};

onMounted(() => {
    void loadAddressBook();
});
</script>

<style scoped>
.addressBook {
    width: 100%;
    height: 100%;
    min-height: 0;
    padding: 14px;
    position: relative;
    z-index: 999;
    top: 0;
    left: 0;
    display: flex;
    flex-direction: column;
    gap: 12px;
}

.addressHeader {
    width: 100%;
    min-height: 46px;
    flex-shrink: 0;
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
}
.addressHeader .titleGroup {
    min-width: 0;
    display: flex;
    align-items: center;
    gap: 10px;
}
.addressHeader .titleGroup > .iconfont {
    width: 36px;
    height: 36px;
    flex-shrink: 0;
    display: flex;
    align-items: center;
    justify-content: center;
    border-radius: 8px;
    color: var(--text-link);
    font-size: 23px;
    background-color: var(--bg-hover-primary);
}
.addressHeader .titleText {
    min-width: 0;
    display: flex;
    flex-direction: column;
    gap: 3px;
}
.addressHeader .title {
    color: var(--text-secondary1);
    font-size: 18px;
    font-weight: 700;
}
.addressHeader .summary {
    color: var(--text-tertiary);
    font-size: 13px;
}
.addressHeader .headerActions {
    flex-shrink: 0;
    display: flex;
    align-items: center;
    gap: 8px;
}
.addressHeader .headerActions .button {
    height: 34px;
    font-size: 14px;
}
.addressHeader .headerActions .iconfont {
    font-size: 18px;
}

.addressContent {
    width: 100%;
    min-height: 0;
    flex: 1;
    display: grid;
    grid-template-columns: 220px minmax(300px, 1fr) 310px;
    gap: 1px;
    overflow: hidden;
    border-radius: 10px;
    background-color: var(--bg-hover-primary);
}

.groupPanel,
.contactPanel,
.detailPanel {
    min-height: 0;
    background-color: var(--bg-primary);
}

.groupPanel {
    padding: 12px;
    display: flex;
    flex-direction: column;
    gap: 8px;
}
.searchInput {
    width: 100%;
    height: 34px;
    flex-shrink: 0;
    display: flex;
    align-items: center;
    justify-content: space-between;
    color: var(--primary-color);
    border: 1px solid var(--border-primary);
    border-radius: 8px;
    overflow: hidden;
    transition: all .3s;
    background-color: var(--bg-primary);
}
.searchInput:hover,
.searchInput:focus-within,
.searchInput.searchClearShown {
    border-color: var(--border-hover);
}
.searchInput input {
    width: 100%;
    height: 100%;
    border: none;
    outline: none;
    padding: 0px 6px 0px 10px;
    font-size: 14px;
    background-color: transparent;
}
.searchInput .button {
    width: 32px;
    height: 32px;
    flex-shrink: 0;
    padding: 0px;
    font-size: 20px;
    --text-primary: var(--text-secondary2);
}
.groupList {
    min-height: 0;
    display: flex;
    flex-direction: column;
    gap: 6px;
    overflow-y: auto;
}
.groupItem {
    width: 100%;
    height: 36px;
    padding: 0px 8px;
    flex-shrink: 0;
    justify-content: space-between;
    border-radius: 8px;
    color: var(--text-secondary2);
    transition: all .3s;
}
.groupItem:hover {
    color: var(--text-link-hover);
    background-color: var(--bg-hover-secondary);
}
.groupItem.selected {
    color: var(--text-link);
    background-color: var(--bg-hover-primary);
}
.groupItem .groupName {
    min-width: 0;
    display: flex;
    align-items: center;
    gap: 6px;
}
.groupItem .groupName .iconfont {
    width: 18px;
    flex-shrink: 0;
    font-size: 17px;
}
.groupItem .groupName span:last-child {
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.groupItem .groupCount {
    min-width: 22px;
    padding: 1px 6px;
    border-radius: 999px;
    text-align: center;
    font-size: 12px;
    background-color: var(--bg-tertiary);
}

.contactPanel {
    min-width: 0;
    display: flex;
    flex-direction: column;
}
.listHeader {
    height: 54px;
    padding: 0px 14px;
    flex-shrink: 0;
    display: flex;
    align-items: center;
    justify-content: space-between;
    border-bottom: 1px solid var(--bg-hover-primary);
}
.listTitle {
    min-width: 0;
    display: flex;
    align-items: center;
    gap: 8px;
    color: var(--text-secondary1);
    font-size: 16px;
    font-weight: 700;
}
.listTitle .listCount {
    min-width: 24px;
    padding: 1px 7px;
    border-radius: 999px;
    color: var(--text-secondary2);
    background-color: var(--bg-tertiary);
    font-size: 12px;
    text-align: center;
}
.listTools {
    flex-shrink: 0;
    display: flex;
    align-items: center;
    gap: 6px;
}
.listTools .button {
    height: 30px;
    padding: 4px 8px;
    font-size: 13px;
}
.listTools .danger {
    --text-primary: var(--color-error);
}
.contactList {
    min-height: 0;
    flex: 1;
    padding: 10px;
    display: flex;
    flex-direction: column;
    gap: 6px;
    overflow-y: auto;
}
.contactItem {
    width: 100%;
    min-height: 58px;
    padding: 8px 10px;
    gap: 10px;
    border-radius: 8px;
    justify-content: flex-start;
    color: var(--text-primary);
    border: 1px solid transparent;
    background-color: var(--bg-secondary);
    transition: background-color .3s, border-color .3s, transform .3s;
}
.contactItem:hover {
    border-color: var(--border-hover);
    background-color: var(--bg-hover-secondary);
}
.contactItem:active {
    transform: translateY(1px);
    background-color: var(--bg-active-primary);
}
.contactItem.selected {
    border-color: var(--border-hover);
    background-color: var(--bg-hover-primary);
}
.avatar {
    width: 40px;
    height: 40px;
    flex-shrink: 0;
    border-radius: 50%;
    overflow: hidden;
    background-color: var(--bg-primary);
}
.avatar img {
    width: 100%;
    height: 100%;
    display: block;
    object-fit: cover;
}
.contactInfo {
    min-width: 0;
    flex: 1;
    display: flex;
    flex-direction: column;
    gap: 4px;
    align-items: flex-start;
}
.contactName,
.contactEmail {
    width: 100%;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
    text-align: left;
}
.contactName {
    color: var(--text-secondary1);
    font-size: 14px;
    font-weight: 700;
}
.contactEmail {
    color: var(--text-tertiary);
    font-size: 12px;
}
.contactGroupName {
    max-width: 88px;
    padding: 2px 7px;
    flex-shrink: 0;
    border-radius: 999px;
    color: var(--text-link);
    background-color: var(--bg-hover-primary);
    font-size: 12px;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}

.detailPanel {
    min-width: 0;
    padding: 16px;
    display: flex;
    flex-direction: column;
}
.detailHero {
    padding: 12px 0px 18px;
    flex-shrink: 0;
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 8px;
    border-bottom: 1px solid var(--border-primary);
}
.avatar.large {
    width: 72px;
    height: 72px;
}
.detailName {
    max-width: 100%;
    color: var(--text-secondary1);
    font-size: 18px;
    font-weight: 700;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.detailEmail {
    max-width: 100%;
    color: var(--text-secondary2);
    font-size: 13px;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.detailList {
    padding-top: 12px;
    display: flex;
    flex-direction: column;
    gap: 10px;
}
.detailItem {
    display: flex;
    flex-direction: column;
    gap: 5px;
}
.detailItem .label {
    color: var(--text-tertiary);
    font-size: 12px;
}
.detailItem .value {
    min-height: 26px;
    padding: 6px 8px;
    border-radius: 8px;
    color: var(--text-secondary1);
    background-color: var(--bg-secondary4);
    font-size: 14px;
    line-height: 1.5;
    word-break: break-word;
}
.detailItem .remark {
    min-height: 64px;
}
.formHeader {
    height: 38px;
    flex-shrink: 0;
    display: flex;
    align-items: center;
    justify-content: space-between;
    color: var(--text-secondary1);
    font-size: 16px;
    font-weight: 700;
    border-bottom: 1px solid var(--border-primary);
}
.formHeader .button {
    width: 30px;
    height: 30px;
    padding: 0px;
    font-size: 18px;
}
.formBody {
    width: 100%;
    padding-top: 12px;
    display: flex;
    flex-direction: column;
    gap: 10px;
}
.formField {
    width: 100%;
    min-width: 0;
    display: flex;
    flex-direction: column;
    gap: 5px;
    color: var(--text-secondary2);
    font-size: 13px;
}
.formField input,
.formField select,
.formField textarea {
    width: 100%;
    height: 42px;
    min-height: 42px;
    padding: 6px 9px;
    box-sizing: border-box;
    border-radius: 8px;
    border: 1px solid var(--border-primary);
    background-color: var(--bg-primary);
    color: var(--text-primary);
    font-size: 14px;
    outline: none;
    transition: border-color .3s;
}
.formField input,
.formField textarea {
    cursor: text;
}
.formField select {
    appearance: none;
    cursor: pointer;
}
.formField .selectBox {
    width: 100%;
    min-width: 0;
    position: relative;
    display: flex;
    align-items: center;
}
.formField .selectBox select {
    padding-right: 34px;
}
.formField .selectBox .iconfont {
    width: 30px;
    height: 100%;
    position: absolute;
    right: 3px;
    top: 0px;
    display: flex;
    align-items: center;
    justify-content: center;
    color: var(--text-secondary2);
    font-size: 12px;
    transform: rotate(90deg);
    pointer-events: none;
}
.formField textarea {
    height: 104px;
    min-height: 104px;
    resize: none;
    line-height: 1.5;
}
.formField input:hover,
.formField select:hover,
.formField textarea:hover,
.formField input:focus,
.formField select:focus,
.formField textarea:focus {
    border-color: var(--border-hover);
}
.formActions {
    margin-top: auto;
    padding-top: 14px;
    display: flex;
    justify-content: flex-end;
    gap: 8px;
}
.formActions .button {
    min-width: 68px;
    height: 34px;
}
.emptyContact,
.emptyDetail {
    min-height: 220px;
    flex: 1;
    display: flex;
    flex-direction: column;
    gap: 10px;
    align-items: center;
    justify-content: center;
    color: var(--text-tertiary);
    font-size: 14px;
}
.emptyContact .iconfont,
.emptyDetail .iconfont {
    color: var(--text-tertiary);
    font-size: 36px;
}
</style>
