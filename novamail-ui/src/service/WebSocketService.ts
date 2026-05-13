// src/service/WebSocketService.ts
import { wsBaseUrl } from '@utils/Request';
import { TokenStore } from '@store/TokenStore';

type MessageHandler<T = any> = (message: WebSocketMessage<T>) => void;

export type WebSocketMessageType = 'CONNECTED' | 'NEW_EMAIL' | 'MAIL_MOVED' | 'MAIL_STATUS_CHANGED' | string;

export interface WebSocketMessage<T = any> {
    type: WebSocketMessageType;
    data?: T;
    timestamp?: number;
}

export interface NewEmailMessageData {
    mappingId: number;
    mailBoxId: number;
    mailId: number;
    folderId: number;
}

class WebSocketService {
    private socket: WebSocket | null = null;
    private reconnectTimer: number | null = null;
    private manualClose = false;
    private handlers: Set<MessageHandler> = new Set();

    connect() {
        const token = TokenStore.get();

        if (!token) return;
        if (this.socket && (this.socket.readyState === WebSocket.OPEN || this.socket.readyState === WebSocket.CONNECTING)) return;

        this.manualClose = false;
        const url = `${wsBaseUrl}/ws/mail?token=${encodeURIComponent(token)}`;

        this.socket = new WebSocket(url);
        this.socket.onopen = () => {
            console.log('Mail WebSocket connected');
        };

        this.socket.onmessage = (event) => {
            this.handleMessage(event.data);
        };

        this.socket.onclose = () => {
            console.log('Mail WebSocket closed');

            this.socket = null;

            if (!this.manualClose && TokenStore.get()) this.reconnect();
        };

        this.socket.onerror = () => {
            console.error('Mail WebSocket error');
            this.socket?.close();
        };
    }

    close() {
        this.manualClose = true;

        if (this.reconnectTimer) {
            clearTimeout(this.reconnectTimer);
            this.reconnectTimer = null;
        }

        if (this.socket) {
            this.socket.close();
            this.socket = null;
        }
    }

    onMessage(handler: MessageHandler) {
        this.handlers.add(handler);
        return () => { this.handlers.delete(handler); };
    }

    private handleMessage(rawData: string) {
        if (!rawData || rawData === 'PONG') return;

        let message: WebSocketMessage;

        try {
            message = JSON.parse(rawData);
        } catch {
            console.error('Invalid WebSocket message:', rawData);
            return;
        }

        if (!message || !message.type) {
            console.warn('Unknown WebSocket message format:', message);
            return;
        }

        this.notify(message);
    }

    private notify(message: WebSocketMessage) {
        for (const handler of this.handlers) {
            try {
                handler(message);
            } catch (error) {
                console.error('WebSocket message handler error:', error);
            }
        }
    }

    private reconnect() {
        if (this.reconnectTimer) return;

        this.reconnectTimer = window.setTimeout(() => {
            this.reconnectTimer = null;
            this.connect();
        }, 3000);
    }
}

export const webSocketService = new WebSocketService();
