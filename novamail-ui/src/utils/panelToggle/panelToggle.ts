// Panel.ts
export type ShowMode = 'slide' | 'fade' | 'scale';
export type Position = { top?: number | string; left?: number | string; right?: number | string; bottom?: number | string; center?: boolean, transform?: string };
export type PanelOptions = {
    animation?: ShowMode;
    duration?: number;
    zIndex?: number;
    dragHandle?: HTMLElement;
    onShow?: () => void;
    onShown?: () => void;
    onHide?: () => void;
    onHidden?: () => void;
};

export class Panel {
    private el: HTMLElement;
    private animation: ShowMode;
    private duration: number;
    private zIndex: number;
    private state: 'hidden' | 'showing' | 'shown' | 'hiding' = 'hidden';
    private timer: number | null = null;

    private callbacks: {
        onShow?: () => void;
        onShown?: () => void;
        onHide?: () => void;
        onHidden?: () => void;
    } = {};

    constructor(content: HTMLElement, parent?: HTMLElement, options?: PanelOptions) {
        // 创建容器
        this.el = document.createElement('div');
        (parent ?? document.body).appendChild(this.el);

        // 插入内容
        this.el.appendChild(content);

        this.animation = options?.animation || 'slide';
        this.duration = options?.duration ?? 300;
        this.zIndex = options?.zIndex ?? 999;
        this.callbacks = {
            onShow: options?.onShow,
            onShown: options?.onShown,
            onHide: options?.onHide,
            onHidden: options?.onHidden
        };

        // 初始化样式
        this.el.style.position = 'absolute';
        this.el.style.display = 'none';
        this.el.style.zIndex = `${this.zIndex}`;
        this.el.style.transition = '';

        // console.log(options);
        // 若传入拖拽手柄，则启用拖拽
        if (options?.dragHandle) this._initDrag(options.dragHandle);
    }

    /** 显示 Panel，可指定位置 */
    /** 显示 Panel，可指定位置 */
    show(position?: Position) {
        if (this.state === 'shown' || this.state === 'showing') return;
        this.callbacks.onShow?.();

        this.state = 'showing';
        this.el.style.display = 'flex';
        this.move(position);
        this.el.style.overflow = 'hidden';

        // 等待浏览器渲染一帧后再执行动画逻辑
        requestAnimationFrame(() => {
            switch (this.animation) {
                case 'slide': {
                    // 先移除 transition，计算真实高度
                    this.el.style.transition = 'none';
                    this.el.style.height = 'auto';
                    const fullHeight = this.el.scrollHeight;

                    // 重置为 0，强制重绘
                    this.el.style.height = '0px';
                    void this.el.offsetHeight;

                    // 启动动画
                    this.el.style.transition = `height ${this.duration}ms ease`;
                    this.el.style.height = `${fullHeight}px`;
                    break;
                }
                case 'fade': {
                    this.el.style.opacity = '0';
                    this.el.style.transition = `opacity ${this.duration}ms ease`;
                    void this.el.offsetHeight;
                    this.el.style.opacity = '1';
                    break;
                }
            }

            this._onTransitionEnd(() => {
                if (this.animation === 'slide') this.el.style.height = 'auto';
                if (this.animation === 'scale') this.el.style.transform = '';
                if (this.animation === 'fade') this.el.style.opacity = '';
                this.state = 'shown';
                this.callbacks.onShown?.();
            });
        });
    }

    /** 隐藏 Panel */
    hide() {
        if (this.state === 'hidden' || this.state === 'hiding') return;
        this.callbacks.onHide?.();
        this.state = 'hiding';
        this.el.style.overflow = 'hidden';

        switch (this.animation) {
            case 'slide': {
                this.el.style.height = `${this.el.offsetHeight}px`;
                void this.el.offsetHeight;
                this.el.style.transition = `height ${this.duration}ms ease`;
                this.el.style.height = '0px';
                break;
            }
            case 'fade': {
                this.el.style.opacity = '1';
                void this.el.offsetHeight;
                this.el.style.transition = `opacity ${this.duration}ms ease`;
                this.el.style.opacity = '0';
                break;
            }
        }

        this._onTransitionEnd(() => {
            this.el.style.display = 'none';
            if (this.animation === 'slide') this.el.style.height = '';
            if (this.animation === 'scale') this.el.style.transform = '';
            if (this.animation === 'fade') this.el.style.opacity = '';
            this.state = 'hidden';
            this.callbacks.onHidden?.();
        });
    }

    /** 切换显示状态 */
    toggle(position?: Position) {
        if (this.state === 'shown') this.hide();
        else this.show(position);
    }

    /** 移动到指定位置 */
    move(position?: Position) {
        if (!position) return;
        if (position.center) {
            this.el.style.top = '50%';
            this.el.style.left = '50%';
            this.el.style.transform = 'translate(-50%, -50%)';
        } else {
            if (position.top !== undefined) this.el.style.top = typeof position.top === 'number' ? `${position.top}px` : position.top;
            if (position.left !== undefined) this.el.style.left = typeof position.left === 'number' ? `${position.left}px` : position.left;
            if (position.right !== undefined) this.el.style.right = typeof position.right === 'number' ? `${position.right}px` : position.right;
            if (position.bottom !== undefined) this.el.style.bottom = typeof position.bottom === 'number' ? `${position.bottom}px` : position.bottom;
            if (position.transform !== undefined) this.el.style.transform = position.transform;
        }
    }

    /** 私有方法：处理动画完成回调 */
    private _onTransitionEnd(callback: () => void) {
        if (this.timer) clearTimeout(this.timer);
        const handler = (e: TransitionEvent) => {
            if (e.target !== this.el) return;
            this.el.removeEventListener('transitionend', handler);
            callback();
        };
        this.el.addEventListener('transitionend', handler);
        this.timer = window.setTimeout(() => {
            this.el.removeEventListener('transitionend', handler);
            callback();
        }, this.duration + 50);
    }

    /** 私有方法：初始化拖拽 */
    private _initDrag(handle: HTMLElement) {
        let isDragging = false;
        let offsetX = 0;
        let offsetY = 0;

        const onMouseDown = (e: MouseEvent) => {
            if (e.button !== 0) return; // 仅左键
            if (!(e.target instanceof HTMLElement)) return;
            if (!handle.contains(e.target)) return;

            isDragging = true;

            const rect = this.el.getBoundingClientRect();
            if (!this.el.style.left) this.el.style.left = `${rect.left + window.scrollX}px`;
            if (!this.el.style.top) this.el.style.top = `${rect.top + window.scrollY}px`;

            this.el.style.transform = ''; // 防止居中跳动

            offsetX = e.clientX - rect.left;
            offsetY = e.clientY - rect.top;

            document.addEventListener('mousemove', onMouseMove);
            document.addEventListener('mouseup', onMouseUp);
            e.preventDefault();
        };

        const onMouseMove = (e: MouseEvent) => {
            if (!isDragging) return;
            this.el.style.left = `${e.clientX - offsetX + window.scrollX}px`;
            this.el.style.top = `${e.clientY - offsetY + window.scrollY}px`;
        };

        const onMouseUp = () => {
            isDragging = false;
            document.removeEventListener('mousemove', onMouseMove);
            document.removeEventListener('mouseup', onMouseUp);
        };

        handle.addEventListener('mousedown', onMouseDown);
    }
}
