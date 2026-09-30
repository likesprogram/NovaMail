package com.example.novamailservice.mapper.Users;

import com.example.novamailservice.entity.Users.Contact;
import com.example.novamailservice.entity.Users.ContactGroup;
import org.apache.ibatis.annotations.Delete;
import org.apache.ibatis.annotations.Insert;
import org.apache.ibatis.annotations.Mapper;
import org.apache.ibatis.annotations.Options;
import org.apache.ibatis.annotations.Param;
import org.apache.ibatis.annotations.Select;
import org.apache.ibatis.annotations.Update;

import java.util.List;

@Mapper
public interface AddressBookMapper {
    @Select("""
        SELECT *
        FROM ContactGroups
        WHERE userId = #{userId}
        ORDER BY sortOrder ASC, groupId ASC
    """)
    List<ContactGroup> selectGroups(@Param("userId") Long userId);

    @Select("""
        SELECT *
        FROM Contacts
        WHERE userId = #{userId}
        ORDER BY updateTime DESC, contactId DESC
    """)
    List<Contact> selectContacts(@Param("userId") Long userId);

    @Select("""
        SELECT *
        FROM ContactGroups
        WHERE userId = #{userId}
          AND groupId = #{groupId}
        LIMIT 1
    """)
    ContactGroup selectGroupById(@Param("userId") Long userId, @Param("groupId") Long groupId);

    @Select("""
        SELECT *
        FROM Contacts
        WHERE userId = #{userId}
          AND contactId = #{contactId}
        LIMIT 1
    """)
    Contact selectContactById(@Param("userId") Long userId, @Param("contactId") Long contactId);

    @Select("""
        <script>
        SELECT COUNT(*)
        FROM ContactGroups
        WHERE userId = #{userId}
          AND groupName = #{groupName}
        <if test="excludeGroupId != null">
          AND groupId != #{excludeGroupId}
        </if>
        </script>
    """)
    int countGroupName(@Param("userId") Long userId, @Param("groupName") String groupName, @Param("excludeGroupId") Long excludeGroupId);

    @Select("""
        <script>
        SELECT COUNT(*)
        FROM Contacts
        WHERE userId = #{userId}
          AND email = #{email}
        <if test="excludeContactId != null">
          AND contactId != #{excludeContactId}
        </if>
        </script>
    """)
    int countEmail(@Param("userId") Long userId, @Param("email") String email, @Param("excludeContactId") Long excludeContactId);

    @Select("""
        SELECT COALESCE(MAX(sortOrder), 0)
        FROM ContactGroups
        WHERE userId = #{userId}
    """)
    int selectMaxSortOrder(@Param("userId") Long userId);

    @Insert("""
        INSERT INTO ContactGroups(userId, groupName, groupType, sortOrder)
        VALUES(#{userId}, #{groupName}, #{groupType}, #{sortOrder})
    """)
    @Options(useGeneratedKeys = true, keyProperty = "groupId")
    int insertGroup(ContactGroup group);

    @Update("""
        UPDATE ContactGroups
        SET groupName = #{groupName}
        WHERE userId = #{userId}
          AND groupId = #{groupId}
          AND groupType = 'CUSTOM'
    """)
    int updateGroupName(@Param("userId") Long userId, @Param("groupId") Long groupId, @Param("groupName") String groupName);

    @Delete("""
        DELETE FROM ContactGroups
        WHERE userId = #{userId}
          AND groupId = #{groupId}
          AND groupType = 'CUSTOM'
    """)
    int deleteCustomGroup(@Param("userId") Long userId, @Param("groupId") Long groupId);

    @Insert("""
        INSERT INTO Contacts(userId, groupId, contactName, email, phone, remark)
        VALUES(#{userId}, #{groupId}, #{contactName}, #{email}, #{phone}, #{remark})
    """)
    @Options(useGeneratedKeys = true, keyProperty = "contactId")
    int insertContact(Contact contact);

    @Update("""
        UPDATE Contacts
        SET groupId = #{groupId},
            contactName = #{contactName},
            email = #{email},
            phone = #{phone},
            remark = #{remark}
        WHERE userId = #{userId}
          AND contactId = #{contactId}
    """)
    int updateContact(Contact contact);

    @Delete("""
        DELETE FROM Contacts
        WHERE userId = #{userId}
          AND contactId = #{contactId}
    """)
    int deleteContact(@Param("userId") Long userId, @Param("contactId") Long contactId);
}
