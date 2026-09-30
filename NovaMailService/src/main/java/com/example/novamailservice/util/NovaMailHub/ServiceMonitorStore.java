package com.example.novamailservice.util.NovaMailHub;

import com.example.novamailservice.dto.System.SystemServiceLoadInfo;
import com.example.novamailservice.dto.System.SystemServiceMonitorInfo;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Component;

import java.lang.reflect.Array;
import java.time.Instant;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;

@Component
public class ServiceMonitorStore {
    private final ConcurrentMap<String, SystemServiceMonitorInfo> serviceCache = new ConcurrentHashMap<>();
    private final ConcurrentMap<String, Long> serviceRefreshTimeCache = new ConcurrentHashMap<>();
    private final Session session;

    @Autowired
    public ServiceMonitorStore(Session session) {
        this.session = session;
    }

    public synchronized SystemServiceMonitorInfo upsert(Map<String, Object> metadata) {
        SystemServiceMonitorInfo info = parseService(metadata, "event");
        if (info == null || info.getServiceId() == null || info.getServiceId().isBlank()) return null;

        if (isRemoved(info)) {
            serviceCache.remove(info.getServiceId());
            serviceRefreshTimeCache.remove(info.getServiceId());
            return copyOf(info, "event");
        }

        serviceCache.put(info.getServiceId(), info);
        serviceRefreshTimeCache.put(info.getServiceId(), System.currentTimeMillis());
        return copyOf(info, "event");
    }

    public synchronized List<SystemServiceMonitorInfo> replaceAll(Object rawServices) {
        Map<String, SystemServiceMonitorInfo> nextCache = new HashMap<>();

        for (Object rawService : asList(rawServices)) {
            SystemServiceMonitorInfo info = parseService(rawService, "hub");
            if (info == null || info.getServiceId() == null || info.getServiceId().isBlank()) continue;
            if (isRemoved(info)) {
                serviceRefreshTimeCache.remove(info.getServiceId());
                continue;
            }
            nextCache.put(info.getServiceId(), info);
        }

        serviceCache.clear();
        serviceCache.putAll(nextCache);
        long now = System.currentTimeMillis();
        nextCache.keySet().forEach(serviceId -> serviceRefreshTimeCache.put(serviceId, now));
        serviceRefreshTimeCache.keySet().removeIf(serviceId -> !nextCache.containsKey(serviceId));
        return snapshot("hub");
    }

    public synchronized SystemServiceMonitorInfo findByServiceId(String serviceId, String source) {
        if (serviceId == null || serviceId.isBlank()) return null;

        SystemServiceMonitorInfo info = serviceCache.get(serviceId);
        return info == null ? null : copyOf(info, source);
    }

    public synchronized SystemServiceMonitorInfo updateCurrentNodeHeartbeat(String serviceId, Map<String, Object> metrics, long delayMs) {
        if (serviceId == null || serviceId.isBlank()) return null;

        SystemServiceMonitorInfo info = serviceCache.get(serviceId);
        if (info == null) {
            info = new SystemServiceMonitorInfo();
            info.setServiceId(serviceId);
            info.setServiceName("NovaMailWeb");
            info.setServiceAddress("Current Node");
            info.setLifecycle(1);
            info.setLifecycleText(lifecycleText(1));
            info.setHealth(1);
            info.setHealthText(healthText(1));
        }

        info.setLoad(mergeHeartbeatLoad(info.getLoad(), metrics, delayMs));
        info.setLastUpdateTime(Instant.now().toString());
        info.setSource("heartbeat");
        info.setCurrentNode(true);
        serviceCache.put(serviceId, info);
        serviceRefreshTimeCache.put(serviceId, System.currentTimeMillis());
        return copyOf(info, "heartbeat");
    }

    public List<String> staleServiceIds(long staleMillis) {
        long now = System.currentTimeMillis();
        return serviceCache.keySet().stream()
                .filter(serviceId -> {
                    Long refreshedAt = serviceRefreshTimeCache.get(serviceId);
                    return refreshedAt == null || now - refreshedAt >= staleMillis;
                })
                .toList();
    }

    public List<SystemServiceMonitorInfo> snapshot(String source) {
        return serviceCache.values().stream()
                .map(item -> copyOf(item, source))
                .sorted(Comparator
                        .comparingInt(this::lifecycleRank)
                        .thenComparing(item -> safeString(item.getServiceName()))
                        .thenComparing(item -> safeString(item.getServiceId())))
                .toList();
    }

    private int lifecycleRank(SystemServiceMonitorInfo item) {
        Integer lifecycle = item.getLifecycle();
        if (lifecycle == null) return 9;
        return switch (lifecycle) {
            case 1 -> 0;
            case 0 -> 1;
            case 2 -> 2;
            case 3 -> 3;
            default -> 9;
        };
    }

    private boolean isRemoved(SystemServiceMonitorInfo item) {
        return item != null && Integer.valueOf(3).equals(item.getLifecycle());
    }

    private SystemServiceMonitorInfo parseService(Object rawService, String source) {
        Map<String, Object> metadata = asMap(rawService);
        if (metadata == null || metadata.isEmpty()) return null;

        Map<String, Object> serviceMeta = asMap(metadata.get("service"));
        Map<String, Object> target = serviceMeta == null ? metadata : serviceMeta;
        Map<String, Object> loadMeta = asMap(firstMetadataValue(target, metadata, "load"));

        String serviceId = asString(firstMetadataValue(target, metadata, "serviceId"));
        if (serviceId == null || serviceId.isBlank()) return null;

        Integer lifecycle = asInteger(firstMetadataValue(target, metadata, "lifecycle"));
        Integer health = asInteger(firstMetadataValue(target, metadata, "health"));

        SystemServiceMonitorInfo info = new SystemServiceMonitorInfo();
        info.setServiceId(serviceId);
        info.setServiceName(defaultString(asString(firstMetadataValue(target, metadata, "serviceName")), "Unknown"));
        info.setServiceAddress(defaultString(asString(firstMetadataValue(target, metadata, "serviceAddress")), "Unknown"));
        info.setLifecycle(lifecycle);
        info.setLifecycleText(lifecycleText(lifecycle));
        info.setHealth(health);
        info.setHealthText(healthText(health));
        info.setLoad(parseLoad(loadMeta));
        info.setLastUpdateTime(Instant.now().toString());
        info.setSource(source);
        info.setCurrentNode(isCurrentNode(serviceId));
        return info;
    }

    private SystemServiceLoadInfo parseLoad(Map<String, Object> loadMeta) {
        SystemServiceLoadInfo load = new SystemServiceLoadInfo();
        if (loadMeta == null) return load;

        load.setCpuPct(asInteger(loadMeta.get("cpuPct")));
        load.setMemPct(asInteger(loadMeta.get("memPct")));
        load.setStressIndicators(asLong(loadMeta.get("stressIndicators")));
        load.setDelayMs(asLong(loadMeta.get("delayMs")));
        load.setCpu(asDouble(loadMeta.get("cpu")));
        load.setMem(asDouble(loadMeta.get("mem")));
        load.setStress(asDouble(loadMeta.get("stress")));
        load.setLat(asDouble(loadMeta.get("lat")));
        load.setLoad(asDouble(loadMeta.get("load")));
        load.setEffectiveLoad(asDouble(loadMeta.get("effectiveLoad")));
        return load;
    }

    private SystemServiceLoadInfo mergeHeartbeatLoad(SystemServiceLoadInfo currentLoad, Map<String, Object> metrics, long fallbackDelayMs) {
        SystemServiceLoadInfo load = copyLoad(currentLoad);
        if (metrics == null) return load;

        Integer cpuPct = asInteger(metrics.get("cpuPct"));
        Integer memPct = asInteger(metrics.get("memPct"));
        Long stressIndicators = asLong(metrics.get("stressIndicators"));
        Long delayMs = asLong(metrics.get("delayMs"));

        if (cpuPct != null) load.setCpuPct(clampPct(cpuPct));
        if (memPct != null) load.setMemPct(clampPct(memPct));
        if (stressIndicators != null) load.setStressIndicators((long) clampPct(stressIndicators.intValue()));
        if (delayMs != null) load.setDelayMs(Math.max(0L, delayMs));
        else if (fallbackDelayMs >= 0L) load.setDelayMs(fallbackDelayMs);
        else if (load.getDelayMs() == null) load.setDelayMs(0L);

        double cpu = percentToRatio(load.getCpuPct());
        double mem = percentToRatio(load.getMemPct());
        double stress = percentToRatio(load.getStressIndicators());
        double lat = Math.max(0D, Math.min(1D, (load.getDelayMs() == null ? 0D : load.getDelayMs()) / 1000D));
        double loadValue = 0.4D * cpu + 0.2D * mem + 0.1D * stress + 0.3D * lat;

        load.setCpu(cpu);
        load.setMem(mem);
        load.setStress(stress);
        load.setLat(lat);
        load.setLoad(loadValue);
        load.setEffectiveLoad(loadValue);
        return load;
    }

    private SystemServiceMonitorInfo copyOf(SystemServiceMonitorInfo sourceInfo, String source) {
        SystemServiceMonitorInfo copy = new SystemServiceMonitorInfo();
        copy.setServiceId(sourceInfo.getServiceId());
        copy.setServiceName(sourceInfo.getServiceName());
        copy.setServiceAddress(sourceInfo.getServiceAddress());
        copy.setLifecycle(sourceInfo.getLifecycle());
        copy.setLifecycleText(sourceInfo.getLifecycleText());
        copy.setHealth(sourceInfo.getHealth());
        copy.setHealthText(sourceInfo.getHealthText());
        copy.setLoad(copyLoad(sourceInfo.getLoad()));
        copy.setLastUpdateTime(sourceInfo.getLastUpdateTime());
        copy.setCurrentNode(isCurrentNode(sourceInfo.getServiceId()));
        copy.setSource(source == null || source.isBlank() ? sourceInfo.getSource() : source);
        return copy;
    }

    private SystemServiceLoadInfo copyLoad(SystemServiceLoadInfo sourceLoad) {
        if (sourceLoad == null) return new SystemServiceLoadInfo();

        SystemServiceLoadInfo copy = new SystemServiceLoadInfo();
        copy.setCpuPct(sourceLoad.getCpuPct());
        copy.setMemPct(sourceLoad.getMemPct());
        copy.setStressIndicators(sourceLoad.getStressIndicators());
        copy.setDelayMs(sourceLoad.getDelayMs());
        copy.setCpu(sourceLoad.getCpu());
        copy.setMem(sourceLoad.getMem());
        copy.setStress(sourceLoad.getStress());
        copy.setLat(sourceLoad.getLat());
        copy.setLoad(sourceLoad.getLoad());
        copy.setEffectiveLoad(sourceLoad.getEffectiveLoad());
        return copy;
    }

    private boolean isCurrentNode(String serviceId) {
        return session.getServiceId()
                .map(currentServiceId -> currentServiceId.equals(serviceId))
                .orElse(false);
    }

    private Object firstMetadataValue(Map<String, Object> primary, Map<String, Object> fallback, String key) {
        Object value = primary == null ? null : primary.get(key);
        if (value != null) return value;
        return fallback == null ? null : fallback.get(key);
    }

    private Map<String, Object> asMap(Object value) {
        if (!(value instanceof Map<?, ?> rawMap)) return null;

        Map<String, Object> result = new HashMap<>();
        rawMap.forEach((key, mapValue) -> {
            if (key != null) result.put(String.valueOf(key), mapValue);
        });
        return result;
    }

    private List<Object> asList(Object value) {
        if (value == null) return List.of();
        if (value instanceof Collection<?> collection) return new ArrayList<>(collection);
        if (value.getClass().isArray()) {
            int length = Array.getLength(value);
            List<Object> result = new ArrayList<>(length);
            for (int index = 0; index < length; index++) result.add(Array.get(value, index));
            return result;
        }
        if (value instanceof Map<?, ?> rawMap) {
            if (rawMap.containsKey("serviceId") || rawMap.containsKey("service")) return List.of(value);
            return new ArrayList<>(rawMap.values());
        }
        return List.of(value);
    }

    private String asString(Object value) {
        if (value == null) return null;
        return String.valueOf(value);
    }

    private Integer asInteger(Object value) {
        Long longValue = asLong(value);
        if (longValue == null) return null;
        if (longValue > Integer.MAX_VALUE) return Integer.MAX_VALUE;
        if (longValue < Integer.MIN_VALUE) return Integer.MIN_VALUE;
        return longValue.intValue();
    }

    private Long asLong(Object value) {
        if (value == null) return null;
        if (value instanceof Number number) return number.longValue();
        try {
            return Long.parseLong(String.valueOf(value));
        } catch (NumberFormatException e) {
            return null;
        }
    }

    private int clampPct(int value) {
        if (value < 0) return 0;
        if (value > 100) return 100;
        return value;
    }

    private double percentToRatio(Number value) {
        if (value == null) return 0D;
        return clampPct(value.intValue()) / 100D;
    }

    private Double asDouble(Object value) {
        if (value == null) return null;
        if (value instanceof Number number) return number.doubleValue();
        try {
            return Double.parseDouble(String.valueOf(value));
        } catch (NumberFormatException e) {
            return null;
        }
    }

    private String lifecycleText(Integer lifecycle) {
        if (lifecycle == null) return "未知";
        return switch (lifecycle) {
            case 0 -> "注册中";
            case 1 -> "运行中";
            case 2 -> "下线中";
            case 3 -> "已移除";
            default -> "未知";
        };
    }

    private String healthText(Integer health) {
        if (health == null) return "未知";
        return switch (health) {
            case 1 -> "健康";
            case 2 -> "异常";
            case 3 -> "可疑";
            case 4 -> "失联";
            default -> "未知";
        };
    }

    private String defaultString(String value, String fallback) {
        return value == null || value.isBlank() ? fallback : value;
    }

    private String safeString(String value) {
        return value == null ? "" : value;
    }
}
