package com.example.novamailservice.service.System;

import com.example.novamailservice.dto.Result.ErrorCode;
import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.dto.System.SystemServiceMonitorInfo;
import com.example.novamailservice.util.NovaMailHub.Message.RequestIdGenerator;
import com.example.novamailservice.util.NovaMailHub.Message.RpcCode;
import com.example.novamailservice.util.NovaMailHub.Message.RpcCodec;
import com.example.novamailservice.util.NovaMailHub.Message.RpcMessage;
import com.example.novamailservice.util.NovaMailHub.Message.RpcType;
import com.example.novamailservice.util.NovaMailHub.NovaMailHubClient;
import com.example.novamailservice.util.NovaMailHub.ServiceMonitorStore;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

import java.time.Instant;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

@Service
public class SystemServiceMonitorServiceImpl implements SystemServiceMonitorService {
    private static final Logger logger = LoggerFactory.getLogger(SystemServiceMonitorServiceImpl.class);

    private final NovaMailHubClient hubClient;
    private final ServiceMonitorStore serviceMonitorStore;

    @Autowired
    public SystemServiceMonitorServiceImpl(NovaMailHubClient hubClient, ServiceMonitorStore serviceMonitorStore) {
        this.hubClient = hubClient;
        this.serviceMonitorStore = serviceMonitorStore;
    }

    @Override
    public ResponseResult<List<SystemServiceMonitorInfo>> getServiceMonitorList() {
        try {
            return ResponseResult.success(fetchServiceMonitorList());
        } catch (HubServiceMonitorException e) {
            return cachedOrError(e.getMessage());
        }
    }

    @Override
    public List<SystemServiceMonitorInfo> refreshStaleServices(long staleMillis) {
        List<String> staleServiceIds = serviceMonitorStore.staleServiceIds(staleMillis);
        if (staleServiceIds.isEmpty()) return List.of();

        Map<String, SystemServiceMonitorInfo> previousStaleServices = new HashMap<>();
        for (String serviceId : staleServiceIds) {
            SystemServiceMonitorInfo service = serviceMonitorStore.findByServiceId(serviceId, "cache");
            if (service != null) previousStaleServices.put(serviceId, service);
        }

        try {
            fetchServiceMonitorList();
        } catch (HubServiceMonitorException e) {
            logger.warn("Refresh stale service monitor snapshots failed serviceIds={}", staleServiceIds, e);
            return List.of();
        }

        List<SystemServiceMonitorInfo> refreshedServices = new ArrayList<>();
        for (String serviceId : staleServiceIds) {
            SystemServiceMonitorInfo refreshedService = serviceMonitorStore.findByServiceId(serviceId, "hub");
            refreshedServices.add(refreshedService == null
                    ? removedSnapshot(previousStaleServices.get(serviceId))
                    : refreshedService
            );
        }
        return refreshedServices.stream().filter(service -> service != null).toList();
    }

    private SystemServiceMonitorInfo removedSnapshot(SystemServiceMonitorInfo service) {
        if (service == null) return null;

        SystemServiceMonitorInfo removedService = new SystemServiceMonitorInfo();
        removedService.setServiceId(service.getServiceId());
        removedService.setServiceName(service.getServiceName());
        removedService.setServiceAddress(service.getServiceAddress());
        removedService.setLifecycle(3);
        removedService.setLifecycleText("已移除");
        removedService.setHealth(service.getHealth());
        removedService.setHealthText(service.getHealthText());
        removedService.setLoad(service.getLoad());
        removedService.setCurrentNode(service.getCurrentNode());
        removedService.setSource("hub");
        removedService.setLastUpdateTime(Instant.now().toString());
        return removedService;
    }

    private List<SystemServiceMonitorInfo> fetchServiceMonitorList() {
        Map<String, Object> metadata = new HashMap<>();
        metadata.put("action", "GetServiceList");

        RpcMessage request = RpcCodec.encode(
                RpcType.REQUEST,
                RequestIdGenerator.nextId(),
                RpcCode.NONE,
                metadata,
                5000,
                1
        );

        try {
            RpcMessage response = hubClient.sendRequest(request).get(6500, TimeUnit.MILLISECONDS);
            if (response.getCode() != RpcCode.OK) {
                String message = readMessage(response.getMetadata(), "Service center returned: " + response.getCode().text());
                throw new HubServiceMonitorException(message);
            }

            Object services = firstValue(response.getMetadata(), "services", "data");
            return serviceMonitorStore.replaceAll(services);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            logger.warn("Get service monitor list interrupted, falling back to cache", e);
            throw new HubServiceMonitorException("Service monitor request was interrupted; showing cached status", e);
        } catch (ExecutionException | TimeoutException | IllegalStateException e) {
            logger.warn("Get service monitor list failed, falling back to cache", e);
            throw new HubServiceMonitorException("Service center is unavailable; showing cached status", e);
        } catch (HubServiceMonitorException e) {
            throw e;
        } catch (Exception e) {
            logger.warn("Parse service monitor list failed, falling back to cache", e);
            throw new HubServiceMonitorException("Failed to parse service status; showing cached status", e);
        }
    }

    private ResponseResult<List<SystemServiceMonitorInfo>> cachedOrError(String message) {
        List<SystemServiceMonitorInfo> cachedServices = serviceMonitorStore.snapshot("cache");
        if (!cachedServices.isEmpty()) return ResponseResult.success(message, cachedServices);
        return ResponseResult.error(ErrorCode.HUB_ERROR, message);
    }

    private Object firstValue(Map<String, Object> metadata, String firstKey, String secondKey) {
        if (metadata == null) return null;
        Object value = metadata.get(firstKey);
        return value == null ? metadata.get(secondKey) : value;
    }

    private String readMessage(Map<String, Object> metadata, String fallback) {
        if (metadata == null) return fallback;
        Object message = metadata.get("message");
        if (message == null || String.valueOf(message).isBlank()) return fallback;
        return String.valueOf(message);
    }

    private static class HubServiceMonitorException extends RuntimeException {
        public HubServiceMonitorException(String message) {
            super(message);
        }

        public HubServiceMonitorException(String message, Throwable cause) {
            super(message, cause);
        }
    }
}
