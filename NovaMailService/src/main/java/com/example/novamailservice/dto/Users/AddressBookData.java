package com.example.novamailservice.dto.Users;

import com.example.novamailservice.entity.Users.Contact;
import com.example.novamailservice.entity.Users.ContactGroup;
import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.NoArgsConstructor;

import java.util.List;

@Data
@NoArgsConstructor
@AllArgsConstructor
public class AddressBookData {
    private List<ContactGroup> groups;
    private List<Contact> contacts;
}
