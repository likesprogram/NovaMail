export type NovaShellWindowAction = 'minimize' | 'maximize' | 'close';

export interface NovaShellWindowMetrics {
    initialWidth?: number;
    initialHeight?: number;
    minWidth?: number;
    minHeight?: number;
}

export interface NovaShellReadyOptions {
    title?: string;
    window?: NovaShellWindowMetrics;
}

export interface NovaShellNotification {
    title: string;
    body?: string;
    unreadCount?: number;
}

export interface NovaShellTransportStatus {
    hasHostBridge: boolean;
    hasWebViewBridge: boolean;
}

export interface NovaShellRuntime {
    name: 'NovaMailDesktop';
    version: string;
    bridge: 'webview2' | 'host';
}

export type NovaShellMessage =
| {
    type: 'nova-mail:ready';
    title?: string;
    initialWidth?: number;
    initialHeight?: number;
    minWidth?: number;
    minHeight?: number;
    payload: NovaShellReadyOptions;
}
| {
    type: 'nova-mail:shell:set-title';
    title: string;
    payload: {
      title: string;
    };
}
| {
    type: 'nova-mail:shell:set-window-metrics';
    initialWidth?: number;
    initialHeight?: number;
    minWidth?: number;
    minHeight?: number;
    payload: NovaShellWindowMetrics;
}
| {
    type: 'nova-mail:shell:notify';
    title: string;
    body?: string;
    unreadCount?: number;
    payload: NovaShellNotification;
}
| {
    type: 'nova-mail:window-control';
    action: NovaShellWindowAction;
};

interface NovaShellHostBridge {
    ready?: (payload: NovaShellReadyOptions) => void;
    setTitle?: (title: string) => void;
    setWindowMetrics?: (payload: NovaShellWindowMetrics) => void;
    notify?: (payload: NovaShellNotification) => void;
    windowControl?: (action: NovaShellWindowAction) => void;
    postMessage?: (message: NovaShellMessage) => void;
}

interface WebViewBridge {
    postMessage: (message: unknown) => void;
}

interface NovaShellWindow extends Window {
    __NOVA_MAIL_SHELL__?: NovaShellRuntime;
    NovaMailShell?: NovaShellHostBridge;
    chrome?: {
        webview?: WebViewBridge;
    };
}

function getShellWindow() {
    return window as NovaShellWindow;
}

function textOrUndefined(value: unknown) {
    return typeof value === 'string' && value.trim() ? value.trim() : undefined;
}

function positiveIntegerOrUndefined(value: unknown) {
    if (typeof value !== 'number' || !Number.isFinite(value)) return undefined;
    const nextValue = Math.trunc(value);
    return nextValue > 0 ? nextValue : undefined;
}

function normalizeWindowMetrics(metrics?: NovaShellWindowMetrics) {
    if (!metrics) return {};
    return {
        initialWidth: positiveIntegerOrUndefined(metrics.initialWidth),
        initialHeight: positiveIntegerOrUndefined(metrics.initialHeight),
        minWidth: positiveIntegerOrUndefined(metrics.minWidth),
        minHeight: positiveIntegerOrUndefined(metrics.minHeight),
    };
}

function flattenWindowMetrics(metrics: NovaShellWindowMetrics) {
    return {
        initialWidth: metrics.initialWidth,
        initialHeight: metrics.initialHeight,
        minWidth: metrics.minWidth,
        minHeight: metrics.minHeight,
    };
}

function normalizeUnreadCount(value: unknown) {
    if (typeof value !== 'number' || !Number.isFinite(value)) return undefined;
    return Math.max(0, Math.trunc(value));
}

function postShellMessage(message: NovaShellMessage) {
    const shellWindow = getShellWindow();
    shellWindow.NovaMailShell?.postMessage?.(message);
    shellWindow.chrome?.webview?.postMessage(JSON.stringify(message));
    window.dispatchEvent(new CustomEvent(message.type, { detail: message }));
}

function postShellMessageWithRetry(message: NovaShellMessage) {
    postShellMessage(message);

    window.setTimeout(() => postShellMessage(message), 50);
    window.setTimeout(() => postShellMessage(message), 250);
}

export function getNovaShellTransportStatus(): NovaShellTransportStatus {
    const shellWindow = getShellWindow();
    return {
        hasHostBridge: Boolean(shellWindow.NovaMailShell),
        hasWebViewBridge: Boolean(shellWindow.chrome?.webview),
    };
}

export function getNovaShellRuntime(): NovaShellRuntime | undefined {
    return getShellWindow().__NOVA_MAIL_SHELL__;
}

export function isRunningInNovaShell(): boolean {
    return Boolean(getNovaShellRuntime());
}

export function notifyShellReady(options: NovaShellReadyOptions = {}) {
    const shellWindow = getShellWindow();
    const title = textOrUndefined(options.title);
    const windowMetrics = normalizeWindowMetrics(options.window);
    const payload: NovaShellReadyOptions = {
        ...(title ? { title } : {}),
        window: windowMetrics,
    };
    const message: NovaShellMessage = {
        type: 'nova-mail:ready',
        ...(title ? { title } : {}),
        ...flattenWindowMetrics(windowMetrics),
        payload,
    };
    shellWindow.NovaMailShell?.ready?.(payload);
    if (title) shellWindow.NovaMailShell?.setTitle?.(title);
    shellWindow.NovaMailShell?.setWindowMetrics?.(windowMetrics);
    postShellMessageWithRetry(message);
}

export function setShellTitle(title: string) {
    const shellWindow = getShellWindow();
    const normalizedTitle = textOrUndefined(title) ?? '';
    const message: NovaShellMessage = {
        type: 'nova-mail:shell:set-title',
        title: normalizedTitle,
        payload: {
            title: normalizedTitle,
        },
    };
    shellWindow.NovaMailShell?.setTitle?.(normalizedTitle);
    postShellMessage(message);
}

export function setShellWindowMetrics(metrics: NovaShellWindowMetrics) {
    const shellWindow = getShellWindow();
    const normalizedMetrics = normalizeWindowMetrics(metrics);
    const message: NovaShellMessage = {
        type: 'nova-mail:shell:set-window-metrics',
        ...flattenWindowMetrics(normalizedMetrics),
        payload: normalizedMetrics,
    };
    shellWindow.NovaMailShell?.setWindowMetrics?.(normalizedMetrics);
    postShellMessage(message);
}

export function showShellNotification(notification: NovaShellNotification) {
    const shellWindow = getShellWindow();
    const title = textOrUndefined(notification.title) ?? '';
    const payload: NovaShellNotification = {
        title,
        ...(textOrUndefined(notification.body) ? { body: textOrUndefined(notification.body) } : {}),
        ...(normalizeUnreadCount(notification.unreadCount) !== undefined ? { unreadCount: normalizeUnreadCount(notification.unreadCount) } : {}),
    };
    const message: NovaShellMessage = {
        type: 'nova-mail:shell:notify',
        title,
        body: payload.body,
        unreadCount: payload.unreadCount,
        payload,
    };
    shellWindow.NovaMailShell?.notify?.(payload);
    postShellMessage(message);
}

export function requestShellWindowControl(action: NovaShellWindowAction) {
    const shellWindow = getShellWindow();
    const message: NovaShellMessage = {
      type: 'nova-mail:window-control',
      action,
    };
    shellWindow.NovaMailShell?.windowControl?.(action);
    postShellMessage(message);
}
