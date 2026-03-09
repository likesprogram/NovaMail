package com.example.novamailservice.util;

import com.example.novamailservice.config.Filter.IpRegionProperties;
import io.netty.handler.ssl.SslContextBuilder;
import org.apache.tika.utils.StringUtils;
import org.lionsoul.ip2region.xdb.Searcher;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.DisposableBean;
import org.springframework.beans.factory.InitializingBean;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.HttpStatusCode;
import org.springframework.http.client.reactive.ReactorClientHttpConnector;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Component;
import org.springframework.web.reactive.function.client.WebClient;
import reactor.core.publisher.Mono;
import reactor.netty.http.client.HttpClient;

import javax.net.ssl.SSLException;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.StandardOpenOption;
import java.time.Duration;
import java.util.Date;
import java.util.concurrent.locks.ReentrantReadWriteLock;

// IP 地理位置 查询工具
@Component
public class IpRegion implements InitializingBean, DisposableBean {
    private static final Logger logger = LoggerFactory.getLogger(IpRegion.class);
    private final ReentrantReadWriteLock rwLock = new ReentrantReadWriteLock();
    private volatile Searcher searcher;
    private volatile byte[] vectorIndex;
    private final IpRegionProperties properties;
    private Path localPath;
    @Autowired
    public IpRegion(IpRegionProperties properties) {
        this.properties = properties;
    }

    // 生命周期管理
    @Override
    public void afterPropertiesSet() { initialize(); }

    // 初始化
    private void initialize() {
        rwLock.writeLock().lock();
        try {
            localPath = Paths.get(properties.getLocalPath());
            if (Files.exists(localPath)) {
                try {
                    this.vectorIndex = Files.readAllBytes(localPath);
                    refreshSearcher();
                    logger.info("已从本地加载IP数据库: {}", localPath);
                } catch (IOException e) {
                    logger.warn("本地IP数据库加载失败", e);
                }
            } else {
                logger.warn("本地IP数据库加载失败, 文件【{}】不存在", localPath);
                logger.info("正在尝试下载。。。");
                try {
                    doUpdate();
                } catch (Exception ignored) { }
            }
        } finally {
            rwLock.writeLock().unlock();
        }
    }

    @Override
    public void destroy() throws IOException {
        rwLock.writeLock().lock();
        try {
            if (searcher != null) {
                searcher.close();
                logger.info("IP地域服务资源已释放");
            }
        } finally {
            rwLock.writeLock().unlock();
        }
    }

    public String getRegion(String ip) {
        if (!isValidIp(ip)) {
            return "Invalid_IP";
        }

        rwLock.readLock().lock();
        try {
            return searcher.search(ip);
//            return searcher.search("220.185.233.40");
        } catch (Exception e) {
            logger.warn("IP查询失败: {}", ip, e);
            return "Unknown";
        } finally {
            rwLock.readLock().unlock();
        }
    }

    // 定时任务

    @Scheduled(cron = "${ip-region.update-cron:0 0 3 * * ?}")
    public void scheduledUpdate() {
        if (org.springframework.util.StringUtils.hasText(properties.getUpdateUrl())) {
            doUpdate();
        } else {
            throw new IllegalStateException("未配置IP数据库URL");
        }
    }

    private void refreshSearcher() throws IOException {
        if (searcher != null) {
            searcher.close();
        }
        // 直接从内存字节数组创建searcher
        searcher = Searcher.newWithBuffer(vectorIndex);
    }

    private synchronized void doUpdate() {
        if (StringUtils.isBlank(properties.getUpdateUrl())) {
            logger.info("未配置更新URL，跳过更新");
            return;
        }

        rwLock.writeLock().lock();
        try {
            logger.info("开始更新IP数据库...");

            // 原子切换
            this.vectorIndex = downloadDbFile(properties.getUpdateUrl());
            refreshSearcher();

            // 如果更新成功，保存到本地
            if (this.vectorIndex != null && this.vectorIndex.length > 0) {
                try {
                    Files.createDirectories(localPath.getParent());
                    Files.write(localPath, this.vectorIndex,
                            StandardOpenOption.CREATE,
                            StandardOpenOption.TRUNCATE_EXISTING);
                    logger.info("IP数据库已保存到本地: {}", localPath);
                } catch (IOException e) {
                    logger.warn("无法保存IP数据库到本地", e);
                }
            }
            logger.info("IP数据库更新完成，更新时间: {}", new Date());
        } catch (Exception e) {
            logger.error("IP数据库更新失败", e);
        } finally {
            rwLock.writeLock().unlock();
        }
    }

    private byte[] downloadDbFile(String url) {
        // 创建自定义WebClient
        WebClient client = WebClient.builder().codecs(configurer -> configurer
                        .defaultCodecs()
                        .maxInMemorySize(1024 * 1024 * 1024) // 设置下载大小为1GB
                )
                .defaultHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/115.0.0.0 Safari/537.36")
                .defaultHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8")
                .defaultHeader("Accept-Language", "zh-CN,zh;q=0.9,en;q=0.8")
                .defaultHeader("Connection", "keep-alive")
                .clientConnector(
                        new ReactorClientHttpConnector(HttpClient.create().followRedirect(true)
                                .secure(sslContextSpec -> {
                                    try {
                                        sslContextSpec.sslContext(SslContextBuilder.forClient().build());
                                    } catch (SSLException e) {
                                        throw new RuntimeException(e);
                                    }
                                })
                        )
                ).build();

        try {
            return client.get()
                .uri(url)
                .retrieve()
                .onStatus(HttpStatusCode::isError, response -> {
                    // 获取服务端返回的错误信息
                    return response.bodyToMono(String.class).flatMap(body -> Mono.error(
                        new IOException("下载失败 [" + response.statusCode() + "]: " + body)
                    ));
                })
                .bodyToMono(byte[].class)
                .timeout(Duration.ofMinutes(30)) // 修改为30分钟超时
                .block();
        } catch (Exception e) {
            logger.error("下载IP数据库失败", e);
            return null;
        }
    }

    private boolean isValidIp(String ip) {
        return ip != null && ip.matches("^((25[0-5]|2[0-4]\\d|[01]?\\d\\d?)\\.){3}(25[0-5]|2[0-4]\\d|[01]?\\d\\d?)$");
    }
}
