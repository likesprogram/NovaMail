package com.example.novamailservice.mapper.Mail;

import com.example.novamailservice.dto.Mail.*;
import com.example.novamailservice.entity.Mail.Email;
import org.apache.ibatis.annotations.Mapper;
import org.apache.ibatis.annotations.Param;
import org.apache.ibatis.annotations.Select;
import org.apache.ibatis.annotations.Update;

import java.util.List;

@Mapper
public interface EmailMapper {

    @Select("""
        SELECT
            f.folderId AS folderId,
            f.folderName AS folderName,
            COALESCE(uc.noReadCont, 0) AS noReadCont
        FROM MailFolder f
        LEFT JOIN (
            SELECT
                mbm.folderId AS folderId,
                COUNT(*) AS noReadCont
            FROM MailBoxMailMapping mbm
            INNER JOIN MailBoxes mb
                ON mb.mailBoxId = mbm.mailBoxId
            WHERE mb.userId = #{userId}
              AND mb.isOpen = 1
              AND mbm.isRead = 0
            GROUP BY mbm.folderId
        ) uc
            ON uc.folderId = f.folderId
        WHERE f.isSystem = 1
           OR f.userId = #{userId}
        ORDER BY
            f.isSystem DESC,
            f.folderId ASC
    """)
    List<RestMailFolder> getMailFolder(@Param("userId") Long userId);

    @Select("""
        SELECT
            mbm.mappingId AS mappingId,
            mbm.mailBoxId AS mailBoxId,
            mbm.mailId AS mailId,
            m.senderName AS fromName,
            m.senderEmail AS fromMail,
            m.subject AS subject,
            m.preview AS preview,
            mbm.status AS status,
            mbm.folderId AS folderId,
            mbm.isRead AS isRead,
            mbm.isStarred AS isStarred,
            mbm.isAnswered AS isAnswered,
            mbm.createTime AS createTime,
            mbm.updateTime AS updateTime
        FROM MailBoxMailMapping mbm
        INNER JOIN MailBoxes mb
            ON mb.mailBoxId = mbm.mailBoxId
        INNER JOIN Mails m
            ON m.mailId = mbm.mailId
        WHERE mb.userId = #{userId}
          AND mb.isOpen = 1
          AND mbm.folderId = #{folderId}
        ORDER BY
            mbm.createTime DESC,
            mbm.mappingId DESC
    """)
    List<MailBoxMailMapping> getMailBoxMailMappings(@Param("userId") Long userId, @Param("folderId") Long folderId);

    @Select("""
        SELECT
            mbm.mappingId AS mappingId,
            mbm.mailBoxId AS mailBoxId,
            mbm.mailId AS mailId,
            m.senderName AS fromName,
            m.senderEmail AS fromMail,
            m.subject AS subject,
            m.preview AS preview,
            mbm.status AS status,
            mbm.folderId AS folderId,
            mbm.isRead AS isRead,
            mbm.isStarred AS isStarred,
            mbm.isAnswered AS isAnswered,
            mbm.createTime AS createTime,
            mbm.updateTime AS updateTime
        FROM MailBoxMailMapping mbm
        INNER JOIN MailBoxes mb
            ON mb.mailBoxId = mbm.mailBoxId
        INNER JOIN Mails m
            ON m.mailId = mbm.mailId
        WHERE mb.userId = #{userId}
          AND mb.isOpen = 1
          AND mbm.isStarred = 1
        ORDER BY
            mbm.createTime DESC,
            mbm.mappingId DESC
    """)
    List<MailBoxMailMapping> getStarredMailMappings(@Param("userId") Long userId);

    @Select("""
        SELECT
            COUNT(*) AS noReadCont
        FROM MailBoxMailMapping mbm
        INNER JOIN MailBoxes mb
            ON mb.mailBoxId = mbm.mailBoxId
        WHERE mb.userId = #{userId}
          AND mb.isOpen = 1
          AND mbm.isStarred = 1
          AND mbm.isRead = 0
    """)
    Long getStarredNoReadCont(@Param("userId") Long userId);

    @Select("""
        SELECT
            mbm.mappingId AS mappingId,
            mbm.mailBoxId AS mailBoxId,
            mbm.mailId AS mailId,
            mbm.isRead AS isRead,
            mbm.isStarred AS isStarred,
            mbm.isAnswered AS isAnswered,
            mbm.createTime AS createTime,
            m.senderName AS senderName,
            m.senderEmail AS senderEmail,
            m.subject AS subject,
            m.preview AS preview,
            m.filePath AS filePath,
            m.storageBucket AS storageBucket
        FROM MailBoxMailMapping mbm
        INNER JOIN MailBoxes mb
            ON mb.mailBoxId = mbm.mailBoxId
        INNER JOIN Mails m
            ON m.mailId = mbm.mailId
        WHERE mb.userId = #{userId}
        AND mb.isOpen = 1
        AND mbm.mappingId = #{mappingId}
        LIMIT 1
    """)
    MailDetailStorageInfo getMailDetailStorageInfo(@Param("userId") Long userId, @Param("mappingId") Long mappingId);

    @Update("""
        UPDATE MailBoxMailMapping mbm
        INNER JOIN MailBoxes mb ON mb.mailBoxId = mbm.mailBoxId
        SET mbm.isRead = #{reader}
        WHERE mb.userId = #{userId} AND mbm.mappingId = #{mappingId}
    """)
    int setMailReader(@Param("userId") Long userId, @Param("mappingId") Long mappingId, @Param("reader") boolean reader);


    @Update("""
        UPDATE MailBoxMailMapping mbm
        INNER JOIN MailBoxes mb ON mb.mailBoxId = mbm.mailBoxId
        SET mbm.isStarred = #{starred}
        WHERE mb.userId = #{userId} AND mbm.mappingId = #{mappingId}
    """)
    int setStarred(@Param("userId") Long userId, @Param("mappingId") Long mappingId, @Param("starred") boolean starred);

    @Select("""
        SELECT
            a.attachmentId AS attachmentId,
            a.mailId AS mailId,
            a.fileName AS fileName,
            a.fileType AS fileType,
            a.fileSize AS fileSize,
            a.fileUrl AS filePath,
            m.storageBucket AS storageBucket
        FROM MailAttachments a
        INNER JOIN Mails m
            ON m.mailId = a.mailId
        INNER JOIN MailBoxMailMapping mbm
            ON mbm.mailId = m.mailId
        INNER JOIN MailBoxes mb
            ON mb.mailBoxId = mbm.mailBoxId
        WHERE mb.userId = #{userId}
        AND mb.isOpen = 1
        AND a.attachmentId = #{attachmentId}
        LIMIT 1
    """)
    MailAttachmentDownloadInfo getAttachmentDownloadInfo(@Param("userId") Long userId, @Param("attachmentId") Long attachmentId);

    @Select("""
        SELECT
            attachmentId AS attachmentId,
            mailId AS mailId,
            fileName AS fileName,
            fileType AS fileType,
            fileSize AS fileSize,
            isInline AS isInline
        FROM MailAttachments
        WHERE mailId = #{mailId}
        ORDER BY
            isInline ASC,
            attachmentId ASC
    """)
    List<MailAttachmentResponse> getMailAttachments(@Param("mailId") Long mailId);

    @Select("""
    SELECT
        mbm.mappingId AS id,
        'mail' AS type,
        m.subject AS subText,
        m.preview AS text,
        m.mailId AS mailId,
        mbm.mappingId AS mappingId
    FROM MailBoxMailMapping mbm
    INNER JOIN MailBoxes mb ON mb.mailBoxId = mbm.mailBoxId
    INNER JOIN Mails m ON m.mailId = mbm.mailId
    WHERE mb.userId = #{userId}
      AND (
             m.subject LIKE CONCAT('%', #{keyword}, '%') ESCAPE '\\\\'
          OR m.senderName LIKE CONCAT('%', #{keyword}, '%') ESCAPE '\\\\'
          OR m.senderEmail LIKE CONCAT('%', #{keyword}, '%') ESCAPE '\\\\'
          OR m.preview LIKE CONCAT('%', #{keyword}, '%') ESCAPE '\\\\'
      )
    ORDER BY m.createTime DESC
    LIMIT #{limit}
    """)
    List<SearchSuggestionVO> searchSuggestions(@Param("userId") Long userId, @Param("keyword") String keyword, @Param("limit") Integer limit);
}