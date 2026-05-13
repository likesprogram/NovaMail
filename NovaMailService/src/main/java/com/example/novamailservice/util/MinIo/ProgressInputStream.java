package com.example.novamailservice.util.MinIo;
import com.example.novamailservice.config.RedisConfig;
import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.util.Cache;
import org.jetbrains.annotations.NotNull;

import java.io.FilterInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.time.Duration;

public class ProgressInputStream extends FilterInputStream {
    private final long totalBytes;
    private long bytesRead;
    private final Cache cache;

    // 进度监听器
    public interface ProgressListener {
        void onProgress(float progress);
    }

    private final ProgressListener progressListener;
    private final String sessionId;

    private static final String ProgressInputStreamCachePrefix = RedisConfig.NovaCache + "ProgressInputStreamCache:";
    private static final String redisKeyPrefixSession = ProgressInputStreamCachePrefix + "Session:";

    public ProgressInputStream(InputStream in, long totalBytes, Cache cache, ProgressListener progressListener, String sessionId) {
        super(in);
        this.totalBytes = totalBytes;
        this.cache = cache;
        this.bytesRead = 0;
        this.progressListener = progressListener;
        this.sessionId = sessionId;
    }

    @Override
    public int read() throws IOException {
        int byteData = super.read();
        if (byteData != -1) {
            bytesRead++;
            updateProgress();
        }
        return byteData;
    }

    @Override
    public int read(@NotNull byte[] b, int off, int len) throws IOException {
        int bytesReadNow = super.read(b, off, len);
        if (bytesReadNow != -1) {
            bytesRead += bytesReadNow;
            updateProgress();
        }
        return bytesReadNow;
    }

    private void updateProgress() {
        float progress = (float) ((bytesRead * 100.0) / totalBytes);
        progressListener.onProgress(progress);
        cache.setCache(redisKeyPrefixSession + sessionId, progress, Duration.ofSeconds(600)); // 只缓存 10 分钟
    }

    public static float getProgress(Cache cache, String sessionId) {
        ResponseResult<Float> cacheResult = cache.getCache(redisKeyPrefixSession + sessionId, Float.class);
        float progress = cacheResult.isSuccess() ? cacheResult.getData() : 0;
        if (progress >= 100) cache.removeCache(redisKeyPrefixSession + sessionId);
        return progress;
    }
}
