package com.example.novamailservice.util;

import javax.imageio.ImageIO;
import java.awt.*;
import java.awt.image.BufferedImage;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.Base64;

public class AvatarProcessing {
    // 压缩图片方法
    private static BufferedImage resizeImage(BufferedImage originalImage, int targetWidth, int targetHeight) {
        // 使用更平滑的缩放算法，质量会更好
        Image resultingImage = originalImage.getScaledInstance(targetWidth, targetHeight, Image.SCALE_SMOOTH);
        // 创建新的 BufferedImage 对象，用于保存缩放后的图像
        BufferedImage outputImage = new BufferedImage(targetWidth, targetHeight, BufferedImage.TYPE_INT_ARGB);

        // 获取图形上下文，绘制缩放后的图像
        Graphics2D g2d = outputImage.createGraphics();
        g2d.drawImage(resultingImage, 0, 0, null);
        g2d.dispose();  // 释放资源

        return outputImage;
    }

    // 处理头像
    public static String AvatarProcessing(String avatarBase64, int targetWidth, int targetHeight)
    {
        try {
            // 解析头像并获取尺寸
            if (avatarBase64 != null && !avatarBase64.isEmpty()) {
                int commaIndex = avatarBase64.indexOf(",");
                byte[] avatarData = Base64.getDecoder().decode(avatarBase64.substring(commaIndex + 1));
                BufferedImage image = ImageIO.read(new ByteArrayInputStream(avatarData));
                int width = image.getWidth();
                int height = image.getHeight();
                if (width != targetWidth || height != targetHeight) {
                    // 压缩头像
                    BufferedImage compressedImage = resizeImage(image, targetWidth, targetHeight);
                    // 将压缩后的头像转换为 Base64 字符串
                    ByteArrayOutputStream baos = new ByteArrayOutputStream();
                    ImageIO.write(compressedImage, "png", baos);
                    byte[] compressedImageData = baos.toByteArray();
                    String compressedAvatar = Base64.getEncoder().encodeToString(compressedImageData);
                    return "data:image/png;base64," + compressedAvatar;
                }
                return avatarBase64;
            }
            return null;
        } catch (IOException e) {
            return null;
        }
    }
}
