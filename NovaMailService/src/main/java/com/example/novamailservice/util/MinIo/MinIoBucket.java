package com.example.novamailservice.util.MinIo;

import java.io.InputStream;
import java.util.List;

public interface MinIoBucket {
    String getBucketName();

    // 删除当前Bucket
    void deleteBucket() throws Exception;

    // 上传文件
    void uploadFile(String objectName, InputStream inputStream, String contentType, long size) throws Exception;

    // 更新文件时间
    void updateFileTime(String objectName) throws Exception;

    // 判断文件是否存在
    boolean fileExists(String objectName) throws Exception;

    enum SortOrder {
        ASC,    // 正序（旧 → 新）
        DESC    // 倒序（新 → 旧）
    }
    // 获取文件列表
    List<FileInfo> listFiles(String objectName, SortOrder sortOrder) throws Exception;

    // 下载文件
    InputStream downloadFile(String objectName, long start, long end) throws Exception;
    InputStream downloadFile(String objectName) throws Exception;

    // 获取文件大小
    long getFileSize(String objectName) throws Exception;

    // 获取文件类型
    String getFileType(String objectName) throws Exception;

    // 删除文件
    void deleteFile(String objectName) throws Exception;

    // 复制文件
    void copyFile(String sourcePath, String destPath) throws Exception;

    // 移动文件
    void moveFile(String sourcePath, String destPath) throws Exception;

    // 获取文件名
    String getFileName(String objectPath);
}
