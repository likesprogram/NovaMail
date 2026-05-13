package com.example.novamailservice.util.MinIo.Impl;

import com.example.novamailservice.util.MinIo.FileInfo;
import com.example.novamailservice.util.MinIo.MinIoBucket;
import io.minio.*;
import io.minio.messages.Item;
import org.apache.tika.Tika;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.HashMap;
import java.util.Map;

@SuppressWarnings("unused") // 仅限 MinIoFactoryImpl 内部使用
class MinIoBucketImpl implements MinIoBucket {
    private static final Logger logger = LoggerFactory.getLogger(MinIoBucketImpl.class);
    private final String bucketName;
    private final MinioClient minioClient;

    MinIoBucketImpl(String bucketName, MinioClient minioClient) {
        this.bucketName = bucketName;
        this.minioClient = minioClient;
    }

    @Override
    public String getBucketName() { return bucketName; }

    @Override
    public void deleteBucket() throws Exception {
        minioClient.removeBucket(RemoveBucketArgs.builder().bucket(bucketName).build());
    }

    // 上传文件
    @Override
    public void uploadFile(String objectName, InputStream inputStream, String contentType, long size) throws Exception {
        if (objectName == null || objectName.isBlank()) throw new IllegalArgumentException("objectName 不能为空");
        if (inputStream == null) throw new IllegalArgumentException("inputStream 不能为空");

        // 如果 size <= 0，尝试计算 size
        if (size <= 0) {
            try {
                size = inputStream.available(); // 不是所有流都支持 available，部分网络流/分片流会返回 0
                if (size <= 0 && inputStream.markSupported()) {
                    inputStream.mark(Integer.MAX_VALUE);
                    size = inputStream.transferTo(OutputStream.nullOutputStream()); // Java 11+
                    inputStream.reset();
                }
            } catch (IOException e) {
                throw new IOException("无法自动获取文件大小，请显式传入", e);
            }
        }

        // 自动推断 contentType
        if (contentType == null || contentType.isBlank()) contentType = new Tika().detect(objectName);
        if (contentType == null || contentType.isBlank()) contentType = "application/octet-stream"; // fallback

        try {
            PutObjectArgs putObjectArgs = PutObjectArgs.builder().bucket(bucketName).object(objectName).stream(inputStream, size, -1).contentType(contentType).build();
            minioClient.putObject(putObjectArgs);
            logger.info("文件上传成功: [{}], 大小: {} 字节, 类型: {}", objectName, size, contentType);
        } catch (Exception e) {
            logger.error("文件上传失败: [{}]", objectName, e);
            throw new RuntimeException("上传失败: " + objectName, e);
        }
    }

    // 更新上传时间
    @Override
    public void updateFileTime(String objectName) throws Exception {
        // 原始对象
        CopySource source = CopySource.builder().bucket(bucketName).object(objectName).build();
        // 重写 metadata
        CopyObjectArgs args = CopyObjectArgs.builder().bucket(bucketName).object(objectName).source(source).metadataDirective(Directive.REPLACE).build();
        // 执行拷贝
        minioClient.copyObject(args);
    }

    // 判断文件是否存在
    @Override
    public boolean fileExists(String objectName) {
        try {
            minioClient.statObject(StatObjectArgs.builder().bucket(bucketName).object(objectName).build());
            return true;
        } catch (Exception e) {
            return false;
        }
    }

    // 获取文件列表
    @Override
    public List<FileInfo> listFiles(String objectName, SortOrder sortOrder) throws Exception {
        List<FileInfo> files = new ArrayList<>();
        Iterable<Result<Item>> results = minioClient.listObjects(
                ListObjectsArgs.builder().bucket(bucketName).prefix(objectName).recursive(true).build()
        );
        for (Result<Item> result : results) {
            Item item = result.get();
            files.add(new FileInfo(item.objectName(), item.lastModified(), item.size(), item.etag()));
        }

        // 排序
        Comparator<FileInfo> comparator = Comparator.comparing(FileInfo::getLastModified);
        if (sortOrder == SortOrder.DESC) comparator = comparator.reversed();
        files.sort(comparator);

        return files;
    }

    // 下载文件
    @Override
    public InputStream downloadFile(String objectName, long start, long end) throws Exception {
        if (objectName == null || objectName.isBlank()) throw new IllegalArgumentException("objectName 不能为空");

        GetObjectArgs.Builder builder = GetObjectArgs.builder().bucket(bucketName).object(objectName);

        if (start >= 0 && end >= start) builder.offset(start).length(end - start + 1);

        return minioClient.getObject(builder.build());
    }

    // 下载文件
    @Override
    public InputStream downloadFile(String objectName) throws Exception {
        return downloadFile(objectName, -1, -1);
    }

    // 获取文件大小
    @Override
    public long getFileSize(String objectName) throws Exception {
        StatObjectResponse stat = minioClient.statObject(
                StatObjectArgs.builder().bucket(bucketName).object(objectName).build()
        );
        return stat.size();
    }

    // 获取文件类型
    @Override
    public String getFileType(String objectName) throws Exception {
        StatObjectResponse stat = minioClient.statObject(
                StatObjectArgs.builder().bucket(bucketName).object(objectName).build()
        );
        return stat.contentType();
    }

    // 删除文件
    @Override
    public void deleteFile(String objectName) throws Exception {
        minioClient.removeObject(RemoveObjectArgs.builder().bucket(bucketName).object(objectName).build());
    }

    // 文件重命名
    @Override
    public void copyFile(String sourcePath, String destPath) throws Exception {
        CopySource copySource = CopySource.builder().bucket(bucketName).object(sourcePath).build();
        minioClient.copyObject(CopyObjectArgs.builder().bucket(bucketName).object(destPath).source(copySource).build());

    }

    // 文件移动
    @Override
    public void moveFile(String sourcePath, String destPath) throws Exception {
        copyFile(sourcePath, destPath);
        deleteFile(sourcePath);
    }

    @Override
    public String getFileName(String objectPath)  {
        int lastSlash = objectPath.lastIndexOf("/");
        return objectPath.substring(lastSlash + 1);
    }
}
