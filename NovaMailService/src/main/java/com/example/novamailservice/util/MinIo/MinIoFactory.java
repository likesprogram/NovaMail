package com.example.novamailservice.util.MinIo;

public interface MinIoFactory {
    // 判断Bucket是否存在
    boolean bucketExists(String bucketName) throws Exception;
    // 创建Bucket
    MinIoBucket createBucket(String bucketName) throws Exception;
    // 打开Bucket
    MinIoBucket openBucket(String bucketName, Boolean createIfNotExists) throws Exception;
    MinIoBucket openBucket(String bucketName) throws Exception;
}
