package com.example.novamailservice.mapper.Mail;

import com.example.novamailservice.entity.Mail.MailBoxFolderState;
import org.apache.ibatis.annotations.*;

import java.util.List;

@Mapper
public interface MailBoxFolderStateMapper {

    // 初始化某个邮箱的所有系统文件夹 IMAP UID 状态
    @Insert("""
            INSERT INTO MailBoxFolderState (
                mailBoxId,
                folderId,
                uidNext,
                uidValidity
            )
            SELECT
                #{mailBoxId},
                folderId,
                1,
                GREATEST(1, CRC32(CONCAT(#{mailBoxId}, ':', folderId)))
            FROM MailFolder
            WHERE userId IS NULL
              AND isSystem = 1
            ON DUPLICATE KEY UPDATE
                uidNext = uidNext
    """)
    int initSystemFolderStates(@Param("mailBoxId") Long mailBoxId);

    // 初始化某个邮箱的指定文件夹 IMAP UID 状态，适合用户自定义文件夹
    @Insert("""
            INSERT INTO MailBoxFolderState (
                mailBoxId,
                folderId,
                uidNext,
                uidValidity
            )
            VALUES (
                #{mailBoxId},
                #{folderId},
                1,
                GREATEST(1, CRC32(CONCAT(#{mailBoxId}, ':', #{folderId})))
            )
            ON DUPLICATE KEY UPDATE
                uidNext = uidNext
            """)
    int initFolderState(@Param("mailBoxId") Long mailBoxId,
                        @Param("folderId") Long folderId);

    // 根据邮箱ID和文件夹ID查询状态
    @Select("""
            SELECT
                mailBoxId,
                folderId,
                uidNext,
                uidValidity,
                createTime,
                updateTime
            FROM MailBoxFolderState
            WHERE mailBoxId = #{mailBoxId}
              AND folderId = #{folderId}
            """)
    MailBoxFolderState selectByMailBoxIdAndFolderId(@Param("mailBoxId") Long mailBoxId,
                                                    @Param("folderId") Long folderId);

    // 查询某个邮箱下所有文件夹状态
    @Select("""
            SELECT
                mailBoxId,
                folderId,
                uidNext,
                uidValidity,
                createTime,
                updateTime
            FROM MailBoxFolderState
            WHERE mailBoxId = #{mailBoxId}
            ORDER BY folderId ASC
            """)
    List<MailBoxFolderState> selectByMailBoxId(@Param("mailBoxId") Long mailBoxId);

    // 锁定某个邮箱文件夹状态，用于分配 IMAP UID，必须在事务中调用
    @Select("""
            SELECT
                mailBoxId,
                folderId,
                uidNext,
                uidValidity,
                createTime,
                updateTime
            FROM MailBoxFolderState
            WHERE mailBoxId = #{mailBoxId}
              AND folderId = #{folderId}
            FOR UPDATE
            """)
    MailBoxFolderState selectForUpdate(@Param("mailBoxId") Long mailBoxId,
                                       @Param("folderId") Long folderId);

    // uidNext 自增，分配 UID 后调用
    @Update("""
            UPDATE MailBoxFolderState
            SET uidNext = uidNext + 1
            WHERE mailBoxId = #{mailBoxId}
              AND folderId = #{folderId}
            """)
    int increaseUidNext(@Param("mailBoxId") Long mailBoxId,
                        @Param("folderId") Long folderId);

    // 删除某个邮箱的所有文件夹状态
    @Delete("""
            DELETE FROM MailBoxFolderState
            WHERE mailBoxId = #{mailBoxId}
            """)
    int deleteByMailBoxId(@Param("mailBoxId") Long mailBoxId);

    // 删除某个邮箱指定文件夹状态，删除用户自定义文件夹时使用
    @Delete("""
            DELETE FROM MailBoxFolderState
            WHERE mailBoxId = #{mailBoxId}
              AND folderId = #{folderId}
            """)
    int deleteByMailBoxIdAndFolderId(@Param("mailBoxId") Long mailBoxId,
                                     @Param("folderId") Long folderId);
}