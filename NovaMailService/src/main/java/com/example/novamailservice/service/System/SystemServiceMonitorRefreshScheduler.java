package com.example.novamailservice.service.System;

import com.example.novamailservice.dto.System.SystemServiceMonitorInfo;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Component;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.List;
import java.util.concurrent.atomic.AtomicBoolean;

@Component
public class SystemServiceMonitorRefreshScheduler {
    private static final Logger logger = LoggerFactory.getLogger(SystemServiceMonitorRefreshScheduler.class);
    private static final long STALE_MILLIS = 45_000L;

    private final SystemServiceMonitorService systemServiceMonitorService;
    private final SystemServiceMonitorNotifyService systemServiceMonitorNotifyService;
    private final AtomicBoolean refreshing = new AtomicBoolean(false);

    @Autowired
    public SystemServiceMonitorRefreshScheduler(
            SystemServiceMonitorService systemServiceMonitorService,
            SystemServiceMonitorNotifyService systemServiceMonitorNotifyService
    ) {
        this.systemServiceMonitorService = systemServiceMonitorService;
        this.systemServiceMonitorNotifyService = systemServiceMonitorNotifyService;
    }

    @Scheduled(fixedDelay = 10_000L, initialDelay = 15_000L)
    public void refreshStaleServices() {
        if (!refreshing.compareAndSet(false, true)) return;

        try {
            List<SystemServiceMonitorInfo> refreshedServices =
                    systemServiceMonitorService.refreshStaleServices(STALE_MILLIS);
            refreshedServices.forEach(systemServiceMonitorNotifyService::notifyServiceChanged);
            if (!refreshedServices.isEmpty()) {
                logger.debug("Pushed stale service monitor snapshots count={}", refreshedServices.size());
            }
        } catch (Exception e) {
            logger.warn("Refresh stale service monitor snapshots task failed", e);
        } finally {
            refreshing.set(false);
        }
    }
}
