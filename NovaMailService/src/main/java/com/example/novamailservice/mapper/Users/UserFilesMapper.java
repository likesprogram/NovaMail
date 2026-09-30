package com.example.novamailservice.mapper.Users;

import com.example.novamailservice.dto.Users.UserFileStorageInfo;
import com.example.novamailservice.entity.Users.UserFiles;
import org.apache.ibatis.annotations.Insert;
import org.apache.ibatis.annotations.Mapper;
import org.apache.ibatis.annotations.Options;
import org.apache.ibatis.annotations.Param;
import org.apache.ibatis.annotations.Select;
import org.apache.ibatis.annotations.Update;

import java.util.List;

@Mapper
public interface UserFilesMapper {
    @Select("""
        SELECT *
        FROM UserFiles
        WHERE userId = #{userId}
          AND fileStatus <> 'DELETED'
        ORDER BY updateTime DESC, fileId DESC
    """)
    List<UserFiles> selectActiveByUserId(@Param("userId") Long userId);

    @Select("""
        SELECT *
        FROM UserFiles
        WHERE userId = #{userId}
          AND fileId = #{fileId}
        LIMIT 1
    """)
    UserFiles selectByIdForUser(@Param("userId") Long userId, @Param("fileId") Long fileId);

    @Select("""
        SELECT
            uf.fileId AS fileId,
            uf.userId AS userId,
            u.account AS account,
            uf.fileName AS fileName,
            uf.fileType AS fileType,
            uf.storageBucket AS storageBucket,
            uf.fileSize AS fileSize,
            uf.fileStatus AS fileStatus
        FROM UserFiles uf
        INNER JOIN Users u
            ON u.userId = uf.userId
        WHERE uf.fileId = #{fileId}
          AND uf.fileStatus = 'ACTIVE'
        LIMIT 1
    """)
    UserFileStorageInfo selectActiveStorageInfoById(@Param("fileId") Long fileId);

    @Select("""
        SELECT COALESCE(MAX(version), 0)
        FROM UserFiles
        WHERE userId = #{userId}
          AND filePath = #{filePath}
          AND fileName = #{fileName}
    """)
    Integer selectMaxVersion(@Param("userId") Long userId, @Param("filePath") String filePath, @Param("fileName") String fileName);

    @Insert("""
        INSERT INTO UserFiles(
            userId, fileName, fileType, filePath, parentPath, storageBucket,
            fileHash, fileSize, version, fileStatus, description
        )
        VALUES(
            #{userId}, #{fileName}, #{fileType}, #{filePath}, #{parentPath}, #{storageBucket},
            #{fileHash}, #{fileSize}, #{version}, #{fileStatus}, #{description}
        )
    """)
    @Options(useGeneratedKeys = true, keyProperty = "fileId")
    int insert(UserFiles userFile);

    @Update("""
        UPDATE UserFiles
        SET fileStatus = #{fileStatus}
        WHERE userId = #{userId}
          AND fileId = #{fileId}
    """)
    int updateStatus(@Param("userId") Long userId, @Param("fileId") Long fileId, @Param("fileStatus") String fileStatus);

    @Update("""
        UPDATE UserFiles
        SET fileStatus = 'ACTIVE'
        WHERE userId = #{userId}
          AND fileId = #{fileId}
          AND fileStatus = 'UPLOADING'
    """)
    int markActive(@Param("userId") Long userId, @Param("fileId") Long fileId);

    @Update("""
        UPDATE UserFiles
        SET fileName = #{fileName},
            description = #{description},
            version = #{version}
        WHERE userId = #{userId}
          AND fileId = #{fileId}
          AND fileStatus <> 'DELETED'
    """)
    int updateMetadata(UserFiles userFile);
}
