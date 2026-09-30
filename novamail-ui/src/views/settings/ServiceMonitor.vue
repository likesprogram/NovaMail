<template>
    <div class="serviceMonitor">
        <div class="serviceMonitorPanel">
            <div class="header" :class="{ warning: usingCache || !!noticeMessage && noticeMessage.includes('失败') }">
                <div class="headerStatus" :class="{ warning: usingCache || !!noticeMessage && noticeMessage.includes('失败') }">
                    <span class="liveDot"></span>
                    <span>WebSocket 实时推送</span>
                    <span v-if="lastRefreshTime" class="headerTime">{{ formatDateTime(lastRefreshTime) }}</span>
                </div>
            </div>

            <div class="serviceSettings">
                        <div class="monitorSummary">
                            <div class="summaryItem">
                                <span>总服务</span>
                                <strong>{{ summary.total }}</strong>
                            </div>
                            <div class="summaryItem">
                                <span>运行中</span>
                                <strong>{{ summary.active }}</strong>
                            </div>
                            <div class="summaryItem">
                                <span>健康</span>
                                <strong>{{ summary.healthy }}</strong>
                            </div>
                            <div class="summaryItem warning">
                                <span>异常</span>
                                <strong>{{ summary.abnormal }}</strong>
                            </div>
                        </div>

                        <div class="averageLoadView">
                            <div class="averageLoadHeader">
                                <span>平均负载</span>
                                <strong :class="averageLoadClass">{{ formatPercent(summary.averageLoad) }}</strong>
                            </div>
                            <div class="chartBox averageLoadChart" :class="averageLoadClass">
                                <svg viewBox="0 0 100 60" preserveAspectRatio="none">
                                    <polygon :points="averageChartAreaPoints()" />
                                    <polyline :points="averageChartPoints()" />
                                </svg>
                            </div>
                        </div>

                        <div class="taskManagerLayout">
                            <aside class="serviceNav">
                                <button
                                    v-for="service in serviceList"
                                    :key="service.serviceId"
                                    class="serviceNavItem"
                                    :class="{ active: service.serviceId === selectedServiceId }"
                                    type="button"
                                    @click="selectService(service.serviceId)"
                                >
                                    <span class="navIcon iconfont icon-likesprogramfuwuqi"></span>
                                    <span class="navBody">
                                        <span class="navTitle">{{ service.serviceName }}</span>
                                        <span class="navAddress">{{ service.serviceAddress }}</span>
                                        <span class="navMetrics">
                                            <span>CPU {{ metricPct(service.load?.cpuPct) }}%</span>
                                            <span>内存 {{ metricPct(service.load?.memPct) }}%</span>
                                        </span>
                                        <span class="navFooter">
                                            <span class="statusText" :class="healthClass(service)">{{ service.healthText || '未知' }}</span>
                                            <span>{{ formatLoad(service.load?.effectiveLoad ?? service.load?.load) }}</span>
                                        </span>
                                    </span>
                                </button>
                                <div v-if="!loading && serviceList.length === 0" class="noData">暂无服务</div>
                                <div v-if="loading && serviceList.length === 0" class="noData">正在初始同步</div>
                            </aside>

                            <main class="serviceDetail">
                                <template v-if="selectedService">
                                    <div class="detailHeader">
                                        <div>
                                            <h2>{{ selectedService.serviceName }}</h2>
                                            <p>{{ selectedService.serviceAddress }}</p>
                                        </div>
                                        <div class="detailTags">
                                            <span v-if="selectedService.currentNode" class="tag primary">当前节点</span>
                                            <span class="tag" :class="lifecycleClass(selectedService)">{{ selectedService.lifecycleText || '未知' }}</span>
                                            <span class="tag" :class="healthClass(selectedService)">{{ selectedService.healthText || '未知' }}</span>
                                        </div>
                                    </div>

                                    <div class="chartGrid">
                                        <div class="chartPanel">
                                            <div class="chartTitle">
                                                <span>CPU</span>
                                                <strong :class="metricClass(selectedService.load?.cpuPct)">{{ metricPct(selectedService.load?.cpuPct) }}%</strong>
                                            </div>
                                            <div class="chartBox" :class="metricClass(selectedService.load?.cpuPct)">
                                                <svg viewBox="0 0 100 60" preserveAspectRatio="none">
                                                    <polygon :points="chartAreaPoints('cpu')" />
                                                    <polyline :points="chartPoints('cpu')" />
                                                </svg>
                                            </div>
                                        </div>
                                        <div class="chartPanel">
                                            <div class="chartTitle">
                                                <span>内存</span>
                                                <strong :class="metricClass(selectedService.load?.memPct)">{{ metricPct(selectedService.load?.memPct) }}%</strong>
                                            </div>
                                            <div class="chartBox" :class="metricClass(selectedService.load?.memPct)">
                                                <svg viewBox="0 0 100 60" preserveAspectRatio="none">
                                                    <polygon :points="chartAreaPoints('mem')" />
                                                    <polyline :points="chartPoints('mem')" />
                                                </svg>
                                            </div>
                                        </div>
                                        <div class="chartPanel">
                                            <div class="chartTitle">
                                                <span>压力</span>
                                                <strong :class="metricClass(selectedService.load?.stressIndicators)">{{ metricPct(selectedService.load?.stressIndicators) }}%</strong>
                                            </div>
                                            <div class="chartBox" :class="metricClass(selectedService.load?.stressIndicators)">
                                                <svg viewBox="0 0 100 60" preserveAspectRatio="none">
                                                    <polygon :points="chartAreaPoints('stress')" />
                                                    <polyline :points="chartPoints('stress')" />
                                                </svg>
                                            </div>
                                        </div>
                                        <div class="chartPanel">
                                            <div class="chartTitle">
                                                <span>延迟</span>
                                                <strong :class="delayClass(selectedService.load?.delayMs)">{{ selectedService.load?.delayMs ?? 0 }}ms</strong>
                                            </div>
                                            <div class="chartBox" :class="delayClass(selectedService.load?.delayMs)">
                                                <svg viewBox="0 0 100 60" preserveAspectRatio="none">
                                                    <polygon :points="chartAreaPoints('delay')" />
                                                    <polyline :points="chartPoints('delay')" />
                                                </svg>
                                            </div>
                                        </div>
                                        <div class="chartPanel chartPanelWide">
                                            <div class="chartTitle">
                                                <span>综合负载</span>
                                                <strong :class="metricClass(loadPct(selectedService.load?.effectiveLoad ?? selectedService.load?.load))">{{ formatLoad(selectedService.load?.effectiveLoad ?? selectedService.load?.load) }}</strong>
                                            </div>
                                            <div class="chartBox chartBoxShort" :class="metricClass(loadPct(selectedService.load?.effectiveLoad ?? selectedService.load?.load))">
                                                <svg viewBox="0 0 100 60" preserveAspectRatio="none">
                                                    <polygon :points="chartAreaPoints('load')" />
                                                    <polyline :points="chartPoints('load')" />
                                                </svg>
                                            </div>
                                        </div>
                                    </div>

                                    <div class="detailStats">
                                        <div>
                                            <span>节点 ID</span>
                                            <strong>{{ selectedService.serviceId }}</strong>
                                        </div>
                                        <div>
                                            <span>生命周期</span>
                                            <strong>{{ selectedService.lifecycleText || '未知' }}</strong>
                                        </div>
                                        <div>
                                            <span>健康状态</span>
                                            <strong class="statusText" :class="healthClass(selectedService)">{{ selectedService.healthText || '未知' }}</strong>
                                        </div>
                                        <div>
                                            <span>CPU</span>
                                            <strong class="metricValue" :class="metricClass(selectedService.load?.cpuPct)">{{ metricPct(selectedService.load?.cpuPct) }}%</strong>
                                        </div>
                                        <div>
                                            <span>内存</span>
                                            <strong class="metricValue" :class="metricClass(selectedService.load?.memPct)">{{ metricPct(selectedService.load?.memPct) }}%</strong>
                                        </div>
                                        <div>
                                            <span>压力</span>
                                            <strong class="metricValue" :class="metricClass(selectedService.load?.stressIndicators)">{{ metricPct(selectedService.load?.stressIndicators) }}%</strong>
                                        </div>
                                        <div>
                                            <span>延迟</span>
                                            <strong class="metricValue" :class="delayClass(selectedService.load?.delayMs)">{{ selectedService.load?.delayMs ?? 0 }}ms</strong>
                                        </div>
                                        <div>
                                            <span>综合负载</span>
                                            <strong class="metricValue" :class="metricClass(loadPct(selectedService.load?.effectiveLoad ?? selectedService.load?.load))">{{ formatLoad(selectedService.load?.effectiveLoad ?? selectedService.load?.load) }}</strong>
                                        </div>
                                        <div>
                                            <span>更新时间</span>
                                            <strong>{{ formatDateTime(selectedService.lastUpdateTime) }}</strong>
                                        </div>
                                    </div>
                                </template>
                                <div v-else class="emptyDetail">等待服务状态</div>
                            </main>
                        </div>
            </div>
        </div>
    </div>
</template>

<script lang="ts">
export default {
    name: 'ServiceMonitor',
};
</script>

<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, ref } from 'vue';
import { formatDateTime } from '@utils/Utils';
import { SystemSettingService } from '@service/SystemSettingService';
import { webSocketService, type WebSocketMessage } from '@service/WebSocketService';
import type { SystemServiceMonitorInfo } from '@api/SystemSetting';

type ChartKind = 'cpu' | 'mem' | 'stress' | 'delay' | 'load';

interface HistoryPoint {
    time: number;
    cpu: number;
    mem: number;
    stress: number;
    load: number;
    delay: number;
}

const services = ref<SystemServiceMonitorInfo[]>([]);
const loading = ref(false);
const selectedServiceId = ref('');
const lastRefreshTime = ref('');
const noticeMessage = ref('');
const historyMap = ref<Record<string, HistoryPoint[]>>({});
const averageHistory = ref<number[]>([]);
let unsubscribeWebSocket: (() => void) | null = null;

const serviceList = computed(() => {
    return services.value.filter(item => !isRemovedService(item)).sort((left, right) => {
        const leftRank = lifecycleRank(left) - lifecycleRank(right);
        if (leftRank !== 0) return leftRank;
        return `${left.serviceName}${left.serviceId}`.localeCompare(`${right.serviceName}${right.serviceId}`);
    });
});

const selectedService = computed(() => {
    return serviceList.value.find(item => item.serviceId === selectedServiceId.value) || serviceList.value[0];
});

const usingCache = computed(() => services.value.some(item => item.source === 'cache'));

const averageLoadClass = computed(() => {
    return summary.value.averageLoad === undefined ? 'disabled' : metricClass(summary.value.averageLoad);
});

const summary = computed(() => {
    const visibleServices = services.value.filter(item => !isRemovedService(item));
    const total = visibleServices.length;
    const active = visibleServices.filter(item => Number(item.lifecycle) === 1).length;
    const healthy = visibleServices.filter(item => Number(item.health) === 1).length;
    const abnormal = visibleServices.filter(item => [2, 3, 4].includes(Number(item.health)) || Number(item.lifecycle) === 2).length;
    const loadValues = visibleServices
        .map(item => loadPct(item.load?.effectiveLoad ?? item.load?.load))
        .filter(value => Number.isFinite(value));
    const averageLoad = loadValues.length === 0 ? undefined : Math.round(loadValues.reduce((totalValue, value) => totalValue + value, 0) / loadValues.length);

    return { total, active, healthy, abnormal, averageLoad };
});

const loadInitialServices = async () => {
    if (loading.value) return;

    loading.value = true;
    try {
        const response = await SystemSettingService.getServiceMonitorListResponse();
        services.value = response.data || [];
        services.value.forEach(recordSnapshot);
        recordAverageSnapshot();
        if (!selectedServiceId.value && serviceList.value[0]) selectedServiceId.value = serviceList.value[0].serviceId;
        lastRefreshTime.value = new Date().toISOString();
        noticeMessage.value = usingCache.value ? response.message || '服务中心暂不可用，当前显示最近状态' : '已获取实时服务状态';
    } catch (error) {
        noticeMessage.value = getErrorMessage(error, '服务状态初始同步失败，等待实时推送');
        lastRefreshTime.value = new Date().toISOString();
    } finally {
        loading.value = false;
    }
};

const handleWebSocketMessage = (message: WebSocketMessage<SystemServiceMonitorInfo>) => {
    if (message.type !== 'SERVICE_CHANGED' || !message.data?.serviceId) return;

    upsertService(message.data);
    lastRefreshTime.value = new Date(message.timestamp || Date.now()).toISOString();
    noticeMessage.value = '已接收实时服务状态';
};

const upsertService = (service: SystemServiceMonitorInfo) => {
    if (isRemovedService(service)) {
        services.value = services.value.filter(item => item.serviceId !== service.serviceId);
        const nextHistoryMap = { ...historyMap.value };
        delete nextHistoryMap[service.serviceId];
        historyMap.value = nextHistoryMap;
        if (selectedServiceId.value === service.serviceId) {
            selectedServiceId.value = serviceList.value[0]?.serviceId || '';
        }
        recordAverageSnapshot();
        return;
    }

    const index = services.value.findIndex(item => item.serviceId === service.serviceId);
    if (index >= 0) services.value[index] = service;
    else services.value.push(service);

    recordSnapshot(service);
    recordAverageSnapshot();
    if (!selectedServiceId.value) selectedServiceId.value = service.serviceId;
};

const isRemovedService = (service: SystemServiceMonitorInfo) => {
    return Number(service.lifecycle) === 3;
};

const recordSnapshot = (service: SystemServiceMonitorInfo) => {
    if (!service.serviceId) return;

    const point: HistoryPoint = {
        time: Date.now(),
        cpu: metricPct(service.load?.cpuPct),
        mem: metricPct(service.load?.memPct),
        stress: metricPct(service.load?.stressIndicators),
        load: loadPct(service.load?.effectiveLoad ?? service.load?.load),
        delay: delayPct(service.load?.delayMs),
    };
    const current = historyMap.value[service.serviceId] || [];
    historyMap.value = {
        ...historyMap.value,
        [service.serviceId]: [...current, point].slice(-60),
    };
};

const recordAverageSnapshot = () => {
    if (summary.value.averageLoad === undefined) return;
    averageHistory.value = [...averageHistory.value, summary.value.averageLoad].slice(-60);
};

const chartPoints = (kind: ChartKind) => {
    return chartValuePoints(kind)
        .map(point => `${point.x.toFixed(2)},${point.y.toFixed(2)}`)
        .join(' ');
};

const chartAreaPoints = (kind: ChartKind) => {
    const points = chartValuePoints(kind);
    const topPoints = points.map(point => `${point.x.toFixed(2)},${point.y.toFixed(2)}`);
    return [`0.00,60.00`, ...topPoints, `100.00,60.00`].join(' ');
};

const averageChartPoints = () => {
    return chartCoordinatePoints(averageHistory.value)
        .map(point => `${point.x.toFixed(2)},${point.y.toFixed(2)}`)
        .join(' ');
};

const averageChartAreaPoints = () => {
    const points = chartCoordinatePoints(averageHistory.value);
    const topPoints = points.map(point => `${point.x.toFixed(2)},${point.y.toFixed(2)}`);
    return [`0.00,60.00`, ...topPoints, `100.00,60.00`].join(' ');
};

const chartValuePoints = (kind: ChartKind) => {
    const serviceId = selectedService.value?.serviceId;
    const points = serviceId ? historyMap.value[serviceId] || [] : [];
    return chartCoordinatePoints(points.map(point => point[kind]));
};

const chartCoordinatePoints = (rawValues: number[]) => {
    const values = [...rawValues];
    if (values.length === 0) values.push(0);
    if (values.length === 1) values.unshift(values[0] ?? 0);

    return values
        .map((value, index) => {
            const x = values.length === 1 ? 100 : (index / (values.length - 1)) * 100;
            const y = 60 - Math.max(0, Math.min(100, value)) * 0.6;
            return { x, y };
        });
};

const selectService = (serviceId: string) => {
    selectedServiceId.value = serviceId;
};

const lifecycleRank = (service: SystemServiceMonitorInfo) => {
    switch (Number(service.lifecycle)) {
        case 1: return 0;
        case 0: return 1;
        case 2: return 2;
        case 3: return 3;
        default: return 9;
    }
};

const lifecycleClass = (service: SystemServiceMonitorInfo) => {
    switch (Number(service.lifecycle)) {
        case 1: return 'success';
        case 0: return 'primary';
        case 2: return 'warning';
        case 3: return 'disabled';
        default: return 'disabled';
    }
};

const healthClass = (service: SystemServiceMonitorInfo) => {
    switch (Number(service.health)) {
        case 1: return 'success';
        case 2: return 'danger';
        case 3: return 'warning';
        case 4: return 'danger';
        default: return 'disabled';
    }
};

const metricClass = (value?: number) => {
    const pct = metricPct(value);
    if (pct >= 80) return 'danger';
    if (pct >= 60) return 'warning';
    return 'success';
};

const delayClass = (value?: number) => {
    const numberValue = Number(value ?? 0);
    if (!Number.isFinite(numberValue)) return 'disabled';
    if (numberValue >= 500) return 'danger';
    if (numberValue >= 150) return 'warning';
    return 'success';
};

const metricPct = (value?: number) => {
    const numberValue = Number(value ?? 0);
    if (!Number.isFinite(numberValue)) return 0;
    return Math.max(0, Math.min(100, Math.round(numberValue)));
};

const loadPct = (value?: number) => {
    const numberValue = Number(value ?? 0);
    if (!Number.isFinite(numberValue) || numberValue > 10000) return 100;
    const normalized = numberValue > 1 ? numberValue : numberValue * 100;
    return Math.max(0, Math.min(100, Math.round(normalized)));
};

const delayPct = (value?: number) => {
    const numberValue = Number(value ?? 0);
    if (!Number.isFinite(numberValue)) return 0;
    return Math.max(0, Math.min(100, Math.round(numberValue / 10)));
};

const formatPercent = (value?: number) => {
    if (value === undefined || value === null) return '--';
    return `${metricPct(value)}%`;
};

const formatLoad = (value?: number) => {
    if (value === undefined || value === null) return '--';
    return `${loadPct(value)}%`;
};

const getErrorMessage = (error: unknown, fallback: string) => {
    return error instanceof Error && error.message ? error.message : fallback;
};

onMounted(async () => {
    webSocketService.connect();
    unsubscribeWebSocket = webSocketService.onMessage(handleWebSocketMessage);
    await loadInitialServices();
});

onBeforeUnmount(() => {
    unsubscribeWebSocket?.();
    unsubscribeWebSocket = null;
});
</script>

<style scoped>
.serviceMonitor {
    width: 100%;
    height: 100%;
    padding: 0px;
    position: relative;
    z-index: 999;
    top: 0;
    left: 0;
    overflow-y: auto;
}

.serviceMonitorPanel {
    width: auto;
    height: 100%;
    flex: 1;
    min-width: 0;
    max-width: 100%;
    margin-left: 0px;
    display: flex;
    flex-direction: column;
    font-size: 14px;
}

.serviceMonitorPanel .header {
    width: 100%;
    min-height: 34px;
    padding: 0px 14px;
    margin: 6px 0px 8px;
    display: flex;
    align-items: center;
    justify-content: stretch;
    gap: 12px;
    border-radius: 7px;
    color: var(--text-primary);
    background-color: var(--bg-hover-primary);
}

.serviceMonitorPanel .title {
    font-size: 14px;
    font-weight: 500;
}

.serviceMonitorPanel .headerStatus {
    width: 100%;
    display: inline-flex;
    align-items: center;
    gap: 8px;
    color: var(--text-secondary2);
    font-size: 12px;
}

.serviceMonitorPanel .headerStatus.warning {
    color: var(--color-warning);
}

.serviceMonitorPanel .header.warning {
    background-color: rgba(250, 173, 20, .12);
}

.headerTime {
    margin-left: auto;
    color: var(--text-link);
}

.liveDot {
    width: 8px;
    height: 8px;
    border-radius: 50%;
    background-color: var(--color-success);
}

.serviceSettings {
    width: 100%;
    flex: 1;
    min-height: 0;
    max-height: none;
    display: flex;
    flex-direction: column;
    gap: 0px;
}

.monitorSummary {
    width: 100%;
    min-height: 58px;
    display: grid;
    grid-template-columns: repeat(4, minmax(0, 1fr));
}

.summaryItem {
    min-width: 0;
    padding: 8px 14px;
    display: flex;
    flex-direction: column;
    justify-content: center;
    gap: 4px;
    border-right: 1px solid var(--bg-active-primary);
}

.summaryItem:last-child {
    border-right: none;
}

.summaryItem span {
    color: var(--text-tertiary);
    font-size: 12px;
}

.summaryItem strong {
    color: var(--text-secondary1);
    font-size: 20px;
    font-weight: 700;
}

.summaryItem.warning strong {
    color: var(--color-warning);
}

.averageLoadView {
    height: 182px;
    min-height: 82px;
    max-height: 182px;
    padding: 8px 14px 10px;
    box-sizing: border-box;
    display: flex;
    flex-direction: column;
    gap: 6px;
    border-top: 1px solid var(--bg-active-primary);
    border-bottom: 1px solid var(--bg-active-primary);
}

.averageLoadHeader {
    min-width: 0;
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
    color: var(--text-secondary2);
    font-size: 13px;
}

.averageLoadHeader strong {
    flex-shrink: 0;
    color: var(--text-secondary1);
    font-size: 16px;
    font-weight: 600;
}

.averageLoadHeader strong.success {
    color: var(--color-success);
}

.averageLoadHeader strong.warning {
    color: var(--color-warning);
}

.averageLoadHeader strong.danger {
    color: var(--color-error);
}

.averageLoadHeader strong.disabled {
    color: var(--text-tertiary);
}

.averageLoadChart {
    height: 42px;
    min-height: 0;
    flex: 1;
}

.taskManagerLayout {
    flex: 1;
    min-height: 540px;
    display: grid;
    grid-template-columns: 250px minmax(0, 1fr);
    column-gap: 12px;
}

.serviceNav {
    min-width: 0;
    padding: 8px;
    display: flex;
    flex-direction: column;
    gap: 4px;
    border-right: 1px solid var(--bg-active-primary);
}

.noData {
    width: 100%;
    min-height: 120px;
    display: flex;
    align-items: center;
    justify-content: center;
    color: var(--text-tertiary);
    font-family: 'artisticFonts';
    font-size: 18px;
}

.serviceNavItem {
    width: 100%;
    min-height: 86px;
    padding: 8px 10px;
    box-sizing: border-box;
    display: grid;
    grid-template-columns: 24px minmax(0, 1fr);
    align-items: flex-start;
    column-gap: 8px;
    border: none;
    border-left: 3px solid transparent;
    color: var(--text-secondary1);
    background-color: transparent;
    text-align: left;
    cursor: pointer;
}

.serviceNavItem:hover {
    background-color: var(--bg-hover-primary);
}

.serviceNavItem.active {
    border-left-color: var(--text-link);
    background-color: rgba(65, 185, 255, .18);
}

.navIcon {
    margin-top: 2px;
    color: var(--text-link);
    font-size: 18px;
    line-height: 1;
}

.navBody {
    min-width: 0;
    display: flex;
    flex-direction: column;
    align-items: flex-start;
    gap: 5px;
}

.navTitle,
.navAddress,
.navMetrics,
.navFooter {
    min-width: 0;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}

.navTitle {
    font-size: 14px;
    font-weight: 700;
}

.navAddress {
    color: var(--text-tertiary);
    font-size: 12px;
}

.navMetrics,
.navFooter {
    display: flex;
    align-items: center;
    justify-content: flex-start;
    gap: 8px;
    color: var(--text-secondary2);
    font-size: 12px;
}

.statusText.success {
    color: var(--color-success);
}

.statusText.warning {
    color: var(--color-warning);
}

.statusText.danger {
    color: var(--color-error);
}

.statusText.disabled {
    color: var(--text-tertiary);
}

.metricValue.success,
.chartTitle strong.success {
    color: var(--color-success);
}

.metricValue.warning,
.chartTitle strong.warning {
    color: var(--color-warning);
}

.metricValue.danger,
.chartTitle strong.danger {
    color: var(--color-error);
}

.metricValue.disabled,
.chartTitle strong.disabled {
    color: var(--text-tertiary);
}

.serviceDetail {
    min-width: 0;
    min-height: 0;
    padding: 12px 18px 16px;
    display: grid;
    grid-template-rows: auto minmax(0, 1fr) 136px;
    gap: 12px;
}

.detailHeader {
    min-width: 0;
    display: flex;
    align-items: flex-start;
    justify-content: space-between;
    gap: 14px;
}

.detailHeader h2 {
    margin: 0px;
    color: var(--text-primary);
    font-size: 26px;
    line-height: 1.1;
    font-weight: 500;
}

.detailHeader p {
    margin: 6px 0px 0px;
    color: var(--text-tertiary);
    font-size: 13px;
}

.detailTags {
    display: flex;
    flex-wrap: wrap;
    justify-content: flex-end;
    gap: 6px;
}

.chartGrid {
    min-width: 0;
    min-height: 0;
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    grid-template-rows: repeat(2, minmax(108px, 1fr)) minmax(84px, .7fr);
    gap: 10px;
}

.chartPanelWide {
    grid-column: 1 / -1;
}

.chartPanel {
    min-width: 0;
    min-height: 0;
    display: flex;
    flex-direction: column;
    gap: 6px;
}

.chartTitle {
    min-width: 0;
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 10px;
    color: var(--text-secondary2);
    font-size: 13px;
}

.chartTitle strong {
    flex-shrink: 0;
    color: var(--text-secondary1);
    font-weight: 500;
}

.chartBox {
    flex: 1;
    min-height: 0;
    border: 1px solid rgba(65, 185, 255, .75);
    background-image:
        linear-gradient(to right, rgba(65, 185, 255, .16) 1px, transparent 1px),
        linear-gradient(to bottom, rgba(65, 185, 255, .16) 1px, transparent 1px);
    background-size: 24px 16px;
    background-color: rgba(255, 255, 255, .26);
}

.chartBoxShort {
    min-height: 0;
}

.chartBox svg {
    width: 100%;
    height: 100%;
    display: block;
}

.chartBox polyline {
    fill: none;
    stroke: var(--chart-line-color);
    stroke-width: 1.8;
    vector-effect: non-scaling-stroke;
}

.chartBox polygon {
    fill: var(--chart-fill-color);
    stroke: none;
}

.chartBox.success {
    --chart-line-color: var(--color-success);
    --chart-fill-color: rgba(82, 196, 26, .18);
}

.chartBox.warning {
    --chart-line-color: var(--color-warning);
    --chart-fill-color: rgba(250, 173, 20, .2);
}

.chartBox.danger {
    --chart-line-color: var(--color-error);
    --chart-fill-color: rgba(255, 77, 79, .18);
}

.chartBox.disabled {
    --chart-line-color: var(--text-tertiary);
    --chart-fill-color: rgba(150, 150, 150, .16);
}

.detailStats {
    min-width: 0;
    height: 136px;
    display: grid;
    grid-template-columns: repeat(3, minmax(0, 1fr));
    gap: 10px 18px;
    padding-top: 4px;
}

.detailStats div {
    min-width: 0;
    display: flex;
    flex-direction: column;
    gap: 4px;
}

.detailStats span {
    color: var(--text-tertiary);
    font-size: 12px;
}

.detailStats strong {
    min-width: 0;
    color: var(--text-secondary1);
    font-size: 14px;
    font-weight: 500;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}

.tag {
    height: 20px;
    padding: 0px 7px;
    flex-shrink: 0;
    display: inline-flex;
    align-items: center;
    border-radius: 999px;
    color: var(--text-secondary2);
    background-color: var(--bg-tertiary);
    font-size: 12px;
    font-weight: 400;
}

.tag.success {
    color: var(--color-success);
    background-color: rgba(82, 196, 26, .12);
}

.tag.primary {
    color: var(--text-link);
    background-color: var(--bg-hover-primary);
}

.tag.warning {
    color: var(--color-warning);
    background-color: rgba(250, 173, 20, .12);
}

.tag.danger {
    color: var(--color-error);
    background-color: rgba(255, 77, 79, .12);
}

.tag.disabled {
    color: var(--text-tertiary);
    background-color: var(--bg-secondary4);
}

.emptyDetail {
    flex: 1;
    min-height: 360px;
    display: flex;
    align-items: center;
    justify-content: center;
    color: var(--text-tertiary);
    font-family: 'artisticFonts';
    font-size: 18px;
}

@media (max-width: 1100px) {
    .monitorSummary {
        grid-template-columns: repeat(3, minmax(0, 1fr));
    }

    .taskManagerLayout {
        grid-template-columns: 1fr;
    }

    .serviceDetail {
        grid-template-rows: auto auto auto;
    }

    .serviceNav {
        max-height: 260px;
        overflow-y: auto;
        border-right: none;
        border-bottom: 1px solid var(--bg-active-primary);
        padding-right: 8px;
    }

    .chartGrid {
        grid-template-columns: 1fr;
        grid-template-rows: none;
    }

    .detailStats {
        height: auto;
        grid-template-columns: 1fr;
    }
}
</style>
