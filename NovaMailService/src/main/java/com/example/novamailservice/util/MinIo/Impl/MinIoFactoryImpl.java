package com.example.novamailservice.util.MinIo.Impl;

import com.example.novamailservice.util.MinIo.MinIoBucket;
import com.example.novamailservice.util.MinIo.MinIoFactory;
import io.minio.BucketExistsArgs;
import io.minio.MakeBucketArgs;
import io.minio.MinioClient;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

@Service
public class MinIoFactoryImpl implements MinIoFactory {
    private static final Logger logger = LoggerFactory.getLogger(MinIoFactoryImpl.class);
    private final MinioClient minioClient;

    @Autowired
    public MinIoFactoryImpl(MinioClient minioClient) {
        this.minioClient = minioClient;
    }

    // 判断bucket是否存在
    @Override
    public boolean bucketExists(String bucketName) throws Exception {
        return minioClient.bucketExists(BucketExistsArgs.builder().bucket(bucketName).build());
    }

    // 创建bucket
    @Override
    public MinIoBucket createBucket(String bucketName) throws Exception {
        if (!bucketExists(bucketName)) {
            minioClient.makeBucket(MakeBucketArgs.builder().bucket(bucketName).build());
            logger.info("桶创建成功: {}", bucketName);
        }
        return new MinIoBucketImpl(bucketName, minioClient);
    }

    @Override
    public MinIoBucket openBucket(String bucketName, Boolean createIfNotExists) throws Exception {
        if (!bucketExists(bucketName)) {
            if (createIfNotExists) {
                minioClient.makeBucket(MakeBucketArgs.builder().bucket(bucketName).build());
                logger.warn("打开不存在的桶，桶【{}】创建成功", bucketName);
            } else {
                logger.error("桶打开失败: 桶不存在");
                throw new IllegalArgumentException("Bucket does not exist");
            }
        }
        return new MinIoBucketImpl(bucketName, minioClient);
    }
    @Override
    public MinIoBucket openBucket(String bucketName) throws Exception {
        return openBucket(bucketName, false);
    }
}
