import { FileProcessing } from "@utils/FileProcessing";

/**
 * 图像处理类
 */
export class ImageProcessing {
    private originalImage!: HTMLImageElement | null;       // 原图
    private processingImage!: HTMLImageElement | null;     // 正在处理的图片
    private image!: HTMLImageElement | null;               // 处理后的图片

    // 图像变换
    public transform: {
        cropping: { // 裁剪
            left:  number,
            top: number,
            right: number,
            bottom: number,
        },
        width: number;
        height: number;
        scale: number, // 缩放
        rotate: number // 旋转
    }

    // 原图大小
    public width!: number;
    public height!: number;

    private updated: boolean;
    
    /**
     * 等待图像加载完成
     */
    private async blockUntilUpdate(): Promise<void> {
        while (this.updated) {
            await new Promise(resolve => setTimeout(resolve, 50));  // 非阻塞的等待
        }
    }

    /**
     * 通用的图片加载函数
     * @param {string} base64 图像的 Base64 数据 
     * @param {boolean} isProcessing 载入到正在处理的图像中
     * @param {boolean} isOriginal 载入到原图中
     * @returns 
     */
    private async LoadImage(base64: string, isProcessing: boolean = false, isOriginal: boolean = false): Promise<void> {
        const img = new Image();
        img.src = base64;
        return new Promise((resolve, reject) => {
            img.onload = () => {
                if (isProcessing || isOriginal) {
                    if(isOriginal) this.originalImage = img; // 更新原始图像
                    // 更新正在处理的图像
                    this.processingImage = img;
                    this.width = img.width;
                    this.height = img.height;
                    // 更新处理后的图像
                    this.image = img;
                    // 更新 transform
                    this.transform.width = this.width;
                    this.transform.height = this.height;
                    this.transform.cropping = { left: 0, top: 0, right: this.width, bottom: this.height };
                    this.transform.scale = 1;
                    this.transform.rotate = 0;

                } else {
                    // 更新处理后的图像
                    this.image = img;
                    this.transform.width = img.width;
                    this.transform.height = img.height;
                }
                resolve();
            };

            img.onerror = (error) => {
                if (isOriginal) {
                    // 更新原始图像
                    this.processingImage = null;
                    this.width = 0;
                    this.height = 0;
                    // 更新处理后的图像
                    this.image = null;
                    this.transform.width = 0;
                    this.transform.height = 0;
                    this.transform.cropping = { left: 0, top: 0, right: this.width, bottom: this.height };
                    this.transform.scale = 1;
                    this.transform.rotate = 0;
                } else {
                    // 更新处理后的图像
                    this.image = null;
                    this.transform.width = 0;
                    this.transform.height = 0;
                    this.transform.cropping = { left: 0, top: 0, right: this.width, bottom: this.height };
                    this.transform.scale = 1;
                    this.transform.rotate = 0;
                }
                reject(error);
            };
        });
    }

    /**
     * 更新原图像
     * @param {string} image 图像的 Base64 数据 
     */
    private async UpdateOriginalImage(image?: string): Promise<void> {
        if (!image) {
            // 提供默认的透明占位图片
            image = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVQImWNgYGBgAAAABQABh6FO1AAAAABJRU5ErkJggg==';
        }
        try {
            // 加载图片并更新为原始图像
            await this.LoadImage(image, false, true);
        } catch (error) {
            console.error("初始化图片失败:", error);
        } finally {
            this.updated = false; // 图片加载完成
        }
    }

    /**
     * 更新正在处理的图像
     * @param {string} image 图像的 Base64 数据 
     */
    private async UpdateProcessingImage(image?: string): Promise<void> {
        if (!image) {
            // 提供默认的透明占位图片
            image = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVQImWNgYGBgAAAABQABh6FO1AAAAABJRU5ErkJggg==';
        }
        try {
            // 加载图片并更新为原始图像
            await this.LoadImage(image, true);
        } catch (error) {
            console.error("初始化图片失败:", error);
        } finally {
            this.updated = false; // 图片加载完成
        }
    }

    
    /**
     * 更新处理完成的图像
     * @param {string} image 图像的 Base64 数据 
     */
    private async UpdateImage(image?: string): Promise<void> {
        if (!image) {
            // 提供默认的透明占位图片
            image = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVQImWNgYGBgAAAABQABh6FO1AAAAABJRU5ErkJggg==';
        }
        try {
            // 加载图片并更新为处理图像
            await this.LoadImage(image);
        } catch (error) {
            console.error("初始化图片失败:", error);
        } finally {
            this.updated = false; // 图片加载完成
        }
    }

    /**
     * 图像处理类 构造方法
     * @param {string} image 图像的 Base64 数据 
     */
    constructor(image?: string) {
        this.updated = true;  // 开始更新图片
        // 初始化 Transform
        this.transform = {
            cropping: { left: 0, top: 0, right: 0, bottom: 0 },
            width: 0,
            height: 0,
            scale: 1,
            rotate: 0,
        };
        this.UpdateOriginalImage(image);
    }
    
    // 设置图像
    public SetImage(image?: string): Promise<void> {
        return new Promise((resolve, reject) => {
            this.updated = true; // 开始更新图片
            if (image) {
                this.UpdateOriginalImage(image)
                    .then(() => {
                        this.updated = false; // 停止更新状态
                        resolve(); // 操作完成
                    })
                    .catch((error) => {
                        console.error("更新原图失败:", error);
                        this.updated = false;
                        reject(error); // 操作失败
                    });
            } else {
                FileProcessing.openFiles('image/*', false)
                    .then(async (file) => {
                        const base64 = await FileProcessing.fileToBase64(file as File) as string;
                        await this.UpdateOriginalImage(base64);
                        this.updated = false; // 停止更新状态
                        resolve(); // 操作完成
                    })
                    .catch(({ error, code }) => {
                        if (code != 0) console.error("文件读取失败:", error);
                        this.updated = false; // 停止更新状态
                        reject(error); // 操作失败
                    });
            }
        });
    }    

    // 缩放与旋转图像
    public async ResizeRotate(scale: number = 0, rotate: number = 0): Promise<boolean> {
        return await this.SetResizeRotate(this.transform.scale + scale, this.transform.rotate + rotate);  // 设置旋转与缩放
    }

    // 设置缩放与旋转图像
    public async SetResizeRotate(scale: number = 1, rotate: number = 0): Promise<boolean> {
        await this.blockUntilUpdate();  // 等待图片更新完成
        this.updated = true;
        try {
            if (this.width > 0 && this.height > 0) {
                let canvas = document.createElement('canvas');
                let ctx = canvas.getContext('2d');
                if (ctx && this.processingImage) {
                    // 缩放后的宽度和高度
                    let scaledWidth = Math.max(this.width * scale, 1);
                    let scaledHeight = Math.max(this.height * scale, 1);
                    
                    // 计算旋转后的画布大小
                    let angle = (rotate * Math.PI) / 180;
                    let cosAngle = Math.abs(Math.cos(angle));
                    let sinAngle = Math.abs(Math.sin(angle));
                    
                    // 确保宽度和高度至少有一个是整数
                    let rotatedWidth = Math.ceil(scaledWidth * cosAngle + scaledHeight * sinAngle);
                    let rotatedHeight = Math.ceil(scaledWidth * sinAngle + scaledHeight * cosAngle);
                    
                    // 设置 canvas 尺寸为旋转后完整容纳图像的尺寸
                    canvas.width = rotatedWidth;
                    canvas.height = rotatedHeight;
                    
                    // 平移到画布中心以进行旋转
                    ctx.save();
                    ctx.translate(rotatedWidth / 2, rotatedHeight / 2);
                    ctx.rotate(angle);
                    
                    // 缩放并绘制图像
                    ctx.drawImage(
                        this.processingImage,
                        -scaledWidth / 2,
                        -scaledHeight / 2,
                        scaledWidth,
                        scaledHeight
                    );
                    ctx.restore();
                    
                    // 更新 tranform
                    this.transform.cropping = { left: 0, top: 0, right: rotatedWidth, bottom: rotatedHeight };
                    this.transform.scale = scale;
                    this.transform.rotate = rotate;

                    await this.UpdateImage(canvas.toDataURL());  // 等待更新完成

                    return true;
                }
            }
            await this.UpdateImage();  // 等待更新完成
            return false;
        } catch (error) {
            console.error("发生错误:", error);
            await this.UpdateImage();  // 等待更新完成
            return false;
        }
    }

    // 裁剪
    public async Cropping(left?: number, top?: number, right?: number, bottom?: number
    ): Promise<boolean> {
        await this.blockUntilUpdate();  // 等待图片更新完成
        this.updated = true;

        try {
            if (this.width > 0 && this.height > 0) {
                // 设置裁剪坐标的默认值
                left = left || 0;
                top = top || 0;
                right = right || this.transform.width;
                bottom = bottom || this.transform.height;

                // 确保裁剪坐标是合理的
                if (left >= right || top >= bottom) {
                    // 交换坐标
                    [left, right] = [right, left];
                    [top, bottom] = [bottom, top];
                }

                // 限制裁剪区域的坐标不超出图片边界
                left = Math.max(left, 0);
                top = Math.max(top, 0);
                right = Math.min(right, this.transform.width);
                bottom = Math.min(bottom, this.transform.height);

                const canvas = document.createElement('canvas');
                const ctx = canvas.getContext('2d');
                if (ctx && this.image) {
                    // 设置 canvas 大小
                    let width = Math.max(right - left, 1);
                    let height = Math.max(bottom - top, 1);

                    canvas.width = width;
                    canvas.height = height;

                    // 裁剪图像
                    ctx.drawImage(
                        this.image,
                        left, top, right - left, bottom - top,
                        0, 0, width, height
                    );

                    // 更新 tranform
                    this.transform.cropping = { left: left, top: top, right: right - left, bottom: bottom - top };

                    await this.UpdateProcessingImage(canvas.toDataURL());  // 等待更新完成
                    return true;
                }
            }
            await this.UpdateProcessingImage();  // 等待更新完成
            return false;
        } catch (error) {
            console.error("发生错误:", error);
            await this.UpdateProcessingImage();  // 等待更新完成
            return false;
        }
    }

    // 还原
    public async Restore() {
        await this.blockUntilUpdate();  // 等待图片更新完成
        if (this.originalImage) {
            await this.UpdateOriginalImage(this.originalImage.src);
        } else {
            await this.UpdateOriginalImage(); // 使用默认占位图
        }
    }

    // 获取图片
    public async GetImage(): Promise<HTMLImageElement> {
        // 等待图片更新完成
        await this.blockUntilUpdate();
        if (this.image === null) throw new Error("图像尚未加载完成或加载失败");
        return this.image;
    }

    // 获取图片的 base64
    public async GetBase64(): Promise<string> {
        // 等待图片更新完成
        await this.blockUntilUpdate();
        if (this.image === null) throw new Error("图像尚未加载完成或加载失败");
        return this.image.src;
    }
}

