import request from '@utils/Request';
import type { ApiResponse } from '@utils/Request';

export type GroupId = string | null;

export interface ContactGroup {
    groupId: string;
    userId: string;
    groupName: string;
    groupType: 'CUSTOM' | 'SYSTEM';
    sortOrder: number;
    createTime?: string;
    updateTime?: string;
}

export interface Contact {
    contactId: string;
    userId: string;
    groupId: GroupId;
    contactName: string;
    email: string;
    phone: string;
    remark: string;
    createTime?: string;
    updateTime?: string;
}

export interface AddressBookData {
    groups: ContactGroup[];
    contacts: Contact[];
}

export interface SaveContactGroupData {
    groupId?: string;
    groupName: string;
}

export interface SaveContactData {
    contactId?: string;
    groupId: GroupId;
    contactName: string;
    email: string;
    phone: string;
    remark: string;
}

const normalizeGroup = (group: ContactGroup): ContactGroup => ({
    ...group,
    groupId: String(group.groupId),
    userId: String(group.userId),
});

const normalizeContact = (contact: Contact): Contact => ({
    ...contact,
    contactId: String(contact.contactId),
    userId: String(contact.userId),
    groupId: contact.groupId == null ? null : String(contact.groupId),
    phone: contact.phone ?? '',
    remark: contact.remark ?? '',
});

export const AddressBookApi = {
    get: async (): Promise<ApiResponse<AddressBookData>> => {
        const response = await request.post<ApiResponse<AddressBookData>>('addressBook/get');
        return {
            ...response.data,
            data: {
                groups: response.data.data.groups.map(normalizeGroup),
                contacts: response.data.data.contacts.map(normalizeContact),
            },
        };
    },
    saveGroup: async (data: SaveContactGroupData): Promise<ApiResponse<ContactGroup>> => {
        const response = await request.post<ApiResponse<ContactGroup>>('addressBook/saveGroup', data);
        return {
            ...response.data,
            data: normalizeGroup(response.data.data),
        };
    },
    deleteGroup: async (groupId: string): Promise<ApiResponse<string>> => {
        const response = await request.post<ApiResponse<string>>('addressBook/deleteGroup', { groupId });
        return response.data;
    },
    saveContact: async (data: SaveContactData): Promise<ApiResponse<Contact>> => {
        const response = await request.post<ApiResponse<Contact>>('addressBook/saveContact', data);
        return {
            ...response.data,
            data: normalizeContact(response.data.data),
        };
    },
    deleteContact: async (contactId: string): Promise<ApiResponse<string>> => {
        const response = await request.post<ApiResponse<string>>('addressBook/deleteContact', { contactId });
        return response.data;
    },
};
