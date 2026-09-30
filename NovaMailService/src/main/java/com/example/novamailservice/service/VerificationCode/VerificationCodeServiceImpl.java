package com.example.novamailservice.service.VerificationCode;

import com.aliyun.dysmsapi20170525.Client;
import com.aliyun.dysmsapi20170525.models.SendSmsRequest;
import com.aliyun.dysmsapi20170525.models.SendSmsResponse;
import com.aliyun.dysmsapi20170525.models.SendSmsResponseBody;
import com.aliyun.teaopenapi.models.Config;
import com.aliyun.auth.credentials.Credential;
import com.aliyun.auth.credentials.provider.StaticCredentialProvider;
import com.aliyun.sdk.service.dypnsapi20170525.models.*;
import com.aliyun.sdk.service.dypnsapi20170525.*;

import com.example.novamailservice.config.RedisConfig;
import com.example.novamailservice.config.VerificationCode.ImageCodeConfig;
import com.example.novamailservice.config.VerificationCode.SmsCodeConfig;
import com.example.novamailservice.config.Filter.RequestContext;
import com.example.novamailservice.dto.IpInfo;
import com.example.novamailservice.dto.Result.ErrorCode;
import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.dto.VerificationCode.ImageCode;
import darabonba.core.client.ClientOverrideConfiguration;
import jakarta.annotation.PostConstruct;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.data.redis.core.StringRedisTemplate;
import org.springframework.stereotype.Service;

import javax.imageio.ImageIO;
import java.awt.*;
import java.awt.image.BufferedImage;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Base64;
import java.util.UUID;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;

@Service
public class VerificationCodeServiceImpl implements VerificationCodeService {
    private static final Logger logger = LoggerFactory.getLogger(VerificationCodeServiceImpl.class);
    // 图片验证码配置类
    private final ImageCodeConfig imageCodeConfig;
    // 短信验证码配置类
    private final SmsCodeConfig smsCodeConfig;
    // 邮件验证码配置类
//    private final EmailCodeConfig emailCodeConfig;
    // redis模板类
    private final StringRedisTemplate redisTemplate;

    // 验证码字体
    private static Font customFont;

    private static final String VerificationCodeCachePrefix = RedisConfig.NovaCache + "VerificationCodeCache:";
    private static final String redisKeyPrefixImgCode = VerificationCodeCachePrefix + "ImageCodeSession:"; // 图片验证码
    private static final String redisKeyPrefixSmsCode = VerificationCodeCachePrefix + "SmsCodeSession:"; // 短信验证码
    private static final String redisKeyPrefixSmsCodeIp = VerificationCodeCachePrefix + "SmsRestriction:Ip:"; // Ip限制
    private static final String redisKeyPrefixSmsCodePhone = VerificationCodeCachePrefix + "SmsRestriction:Phone:"; // 手机限制

    // 构造函数注入
    @Autowired
    public VerificationCodeServiceImpl(ImageCodeConfig imageCodeConfig, SmsCodeConfig smsCodeConfig, StringRedisTemplate redisTemplate) {
        this.imageCodeConfig = imageCodeConfig;
        this.smsCodeConfig = smsCodeConfig;
        this.redisTemplate = redisTemplate;
    }
    /**
     * 🌟 Spring 启动时，自动初始化
     */
    @PostConstruct
    void initialize()
    {
        logger.info("🔄 初始化 字体...");
        // 加载自定义字体
        customFont = loadFontSafely(imageCodeConfig.getFontName(), imageCodeConfig.getFontSize());
    }

    // 获取图片验证码
    @Override
    public ResponseResult<ImageCode> getImageCode() {
        try {
            // 创建图片验证码
            BufferedImage imageCode = new BufferedImage(
                    imageCodeConfig.getWidth(),
                    imageCodeConfig.getHeight(),
                    BufferedImage.TYPE_INT_ARGB);
            Graphics2D g2d = imageCode.createGraphics();

            // 设置抗锯齿
            g2d.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);

            // 设置背景色
            g2d.setColor(new Color(255, 255, 255, 0));
            g2d.fillRect(0, 0, imageCodeConfig.getWidth(), imageCodeConfig.getHeight());

            // 生成随机验证码
            int codeLength = imageCodeConfig.getCodeLength();
            StringBuilder codeBuilder = new StringBuilder(codeLength); // 预分配容量
            for (int i = 0; i < codeLength; i++) {
                codeBuilder.append(imageCodeConfig.getCharacter().charAt((int) (Math.random() * imageCodeConfig.getCharacter().length())));
            }
            String code = codeBuilder.toString();

            // 生成随机 session 唯一 Id
            String sessionId;
            do {
                sessionId = redisKeyPrefixImgCode + UUID.randomUUID();
            } while (Boolean.TRUE.equals(redisTemplate.hasKey(sessionId)));

            logger.info("图片验证码：[sessionId] [{}]  ,  [验证码] [{}]", sessionId, code);

            // 存储验证码到 Redis 中
            redisTemplate.opsForValue().set(sessionId, code, imageCodeConfig.getExpiration(), TimeUnit.SECONDS);

            // 绘制干扰点
            for (int i = 0; i < 100; i++) {
                // 随机颜色
                g2d.setColor(new Color((int) (Math.random() * 255), (int) (Math.random() * 255), (int) (Math.random() * 255)));
                // 绘制干扰点
                g2d.drawOval((int) (Math.random() * imageCodeConfig.getWidth()), (int) (Math.random() * imageCodeConfig.getHeight()), 1, 1);
            }

            // 设置字体
            g2d.setFont(customFont);
            for (int i = 0; i < code.length(); i++) {
                // 随机颜色
                g2d.setColor(new Color((int) (Math.random() * 180), (int) (Math.random() * 180), (int) (Math.random() * 180)));

                // 字符的宽度和高度
                int charWidth = g2d.getFontMetrics().charWidth(code.charAt(i));
                int charHeight = imageCodeConfig.getFontSize();

                // 计算旋转的中心点
                int centerX = (imageCodeConfig.getWidth() / imageCodeConfig.getCodeLength()) * i + charWidth / 2;
                int centerY = imageCodeConfig.getHeight() / 2;

                // 限制旋转角度，防止字符过度旋转
                double rotateAngle = Math.random() * 20 - 10;  // 随机旋转角度：-10到10度

                // 计算旋转后的字符位置
                g2d.rotate(Math.toRadians(rotateAngle), centerX, centerY);

                // 绘制字符
                g2d.drawString(code.charAt(i) + "", (imageCodeConfig.getWidth() / imageCodeConfig.getCodeLength()) * i, charHeight);

                // 恢复旋转
                g2d.rotate(Math.toRadians(-rotateAngle), centerX, centerY);  // 恢复到原来的角度
            }

            // 绘制 干扰线
            for (int i = 0; i < 4; i++) {
                g2d.setColor(new Color(
                        (int) (Math.random() * 255),
                        (int) (Math.random() * 255),
                        (int) (Math.random() * 255))
                );

                int x1;
                int y1 = (int) (Math.random() * imageCodeConfig.getHeight());
                int x2;
                int y2;// 长度随机（短线）
                if (i % 2 == 0) {
                    // **偶数编号（贯穿整个图片）**
                    x1 = 0;
                    x2 = imageCodeConfig.getWidth();
                    y2 = (int) (Math.random() * imageCodeConfig.getHeight());

                } else {
                    // **奇数编号（短线）**
                    x1 = (int) (Math.random() * imageCodeConfig.getWidth());
                    x2 = x1 + (int) (Math.random() * 20) - 10;
                    y2 = y1 + (int) (Math.random() * 20) - 10;

                }
                g2d.drawLine(x1, y1, x2, y2);
            }

            // 释放资源
            g2d.dispose();

            // 创建一个字节输出流
            ByteArrayOutputStream bao = new ByteArrayOutputStream();

            // 将BufferedImage写入到输出流中，这里使用PNG格式
            ImageIO.write(imageCode, "png", bao);

            // 将输出流转换为字节数组
            byte[] imageBytes = bao.toByteArray();

            // 使用Base64编码器编码字节数组
            String base64String = Base64.getEncoder().encodeToString(imageBytes);

            // 创建ImageCode对象
            ImageCode imageCodeResult = new ImageCode();
            imageCodeResult.setCode("data:image/png;base64," + base64String);
            imageCodeResult.setSessionId(sessionId);

            // 返回Base64编码的字符串
            return ResponseResult.success(imageCodeResult);
        } catch (Exception e) {
            logger.error("图片验证码生成失败：{}", e.getMessage());
            return ResponseResult.error(ErrorCode.VC_IMAGE_GENERATE_FAIL, "图片验证码生成失败");
        }
    }

    // 发送短信信息
    private ResponseResult<String> sendSmsMesage(
            String redisIPSendIntervalKey,
            String redisPhoneSendIntervalKey,
            String phoneNumber, String code) {
        try {
            if (smsCodeConfig.isCommercial()) {
                // 初始化配置
                Config config = new Config()
                        .setAccessKeyId(smsCodeConfig.getAccessKeyId())
                        .setAccessKeySecret(smsCodeConfig.getAccessKeySecret())
                        .setEndpoint("dysmsapi.aliyuncs.com"); // 短信服务 API 地址

                // 初始化客户端
                Client client = new Client(config);
                SendSmsRequest sendSmsRequest  = new SendSmsRequest()
                        .setSignName(smsCodeConfig.getSignName())
                        .setTemplateCode(smsCodeConfig.getTemplateCode())
                        .setPhoneNumbers(phoneNumber)
                        .setTemplateParam("{\"code\":\"" + code + "\"}");

                // 获取响应对象
                SendSmsResponse sendSmsResponse = client.sendSms(sendSmsRequest);
                SendSmsResponseBody body = sendSmsResponse.getBody();
                if (body == null || body.code == null) {
                    logger.error("❌ 短信发送失败: 无返回或响应为空");
                    return ResponseResult.error(ErrorCode.VC_SMS_PLATFORM_ERROR, "短信平台返回异常");
                }
                String codeValue = body.code;
                String message = body.message != null ? body.message : "未知错误";

                // 异常处理
                if (!"OK".equalsIgnoreCase(codeValue)) {
                    int errorCode;
                    String errorMsg;

                    switch (codeValue) {
                        case "isv.MOBILE_NUMBER_ILLEGAL", "INVALID_NUMBER", "MOBILE_NOT_ON_SERVICE" -> {
                            errorCode = ErrorCode.VC_SMS_INVALID_PHONE;
                            errorMsg = "手机号码无效";
                        }
                        case "MOBLLE_TERMINAL_ERROR" -> {
                            errorCode = ErrorCode.VC_SMS_PLATFORM_ERROR;
                            errorMsg = "短信被手机拦截";
                        }
                        case "isv.BUSINESS_LIMIT_CONTROL", "biz.FREQUENCY" -> {
                            errorCode = ErrorCode.VC_SMS_RESTRICT_PHONE;
                            errorMsg = "该手机号发送过于频繁，请稍后再试";
                        }
                        case "isp.QPS_LIMIT", "isv.IP_BLACK_LIMIT" -> {
                            errorCode = ErrorCode.VC_SMS_RESTRICT_IP;
                            errorMsg = "发送请求过于频繁，请稍后再试";
                        }
                        case "InvalidBizToken.Expired" -> {
                            errorCode = ErrorCode.VC_SMS_CODE_EXPIRED;
                            errorMsg = "验证码已过期，请重新获取";
                        }
                        default -> {
                            errorCode = ErrorCode.VC_SMS_SEND_FAIL;
                            errorMsg = "短信验证码发送失败";
                        }
                    }

                    logger.error("❌ 短信发送失败: code={} msg={}", codeValue, message);
                    return ResponseResult.error(errorCode, errorMsg);
                }
            } else {
                // 初始化阿里云短信服务
                StaticCredentialProvider provider = StaticCredentialProvider.create(Credential.builder()
                        .accessKeyId(smsCodeConfig.getAccessKeyId())
                        .accessKeySecret(smsCodeConfig.getAccessKeySecret())
                        .build());

                // 创建AsyncClient
                AsyncClient client = AsyncClient.builder()
                        .region("cn-hangzhou") // Region ID
                        .credentialsProvider(provider)
                        .overrideConfiguration(
                            ClientOverrideConfiguration.create().setEndpointOverride("dypnsapi.aliyuncs.com")
                        )
                        .build();

                // 计算过期时间
                String min = String.valueOf(smsCodeConfig.getExpiration() / 60);

                // 构建请求参数
                SendSmsVerifyCodeRequest sendSmsVerifyCodeRequest = SendSmsVerifyCodeRequest.builder()
                        .phoneNumber(phoneNumber)
                        .signName(smsCodeConfig.getSignName())
                        .templateCode(smsCodeConfig.getTemplateCode())
                        .templateParam("{\"code\": \"" + code + "\", \"min\":\"" + min + "\"}")
                        .build();

                // 发送短信验证码
                CompletableFuture<SendSmsVerifyCodeResponse> response = client.sendSmsVerifyCode(sendSmsVerifyCodeRequest);
                SendSmsVerifyCodeResponse resp = response.get();
                client.close();
                // 处理响应
                SendSmsVerifyCodeResponseBody body = resp.getBody();
                if (body == null || body.getCode() == null) {
                    logger.error("❌ 短信发送失败: 无响应");
                    return ResponseResult.error(ErrorCode.VC_SMS_PLATFORM_ERROR, "短信平台返回异常");
                }

                String codeValue = body.getCode();
                String message = body.getMessage() != null ? body.getMessage() : "未知错误";

                // 异常处理
                if (!"OK".equalsIgnoreCase(codeValue)) {
                    int errorCode;
                    String errorMsg;
                    switch (codeValue) {
                        // 参数问题 / 格式错误
                        case "isv.MOBILE_NUMBER_ILLEGAL", "InvalidPhoneNumber.Check" -> {
                            errorCode = ErrorCode.VC_SMS_INVALID_PHONE;
                            errorMsg = "手机号码格式错误";
                        }
                        // 手机号发送频率限制
                        case "isv.BUSINESS_LIMIT_CONTROL", "biz.FREQUENCY" -> {
                            errorCode = ErrorCode.VC_SMS_RESTRICT_PHONE;
                            errorMsg = "该手机号发送过于频繁，请稍后再试";
                        }
                        // IP或QPS类限制
                        case "isp.QPS_LIMIT", "isv.IP_BLACK_LIMIT" -> {
                            errorCode = ErrorCode.VC_SMS_RESTRICT_IP;
                            errorMsg = "发送请求过于频繁，请稍后再试";
                        }
                        // 验证码过期
                        case "InvalidBizToken.Expired" -> {
                            errorCode = ErrorCode.VC_SMS_CODE_EXPIRED;
                            errorMsg = "验证码已过期，请重新获取";
                        }
                        // 平台 / 配置类问题
                        case "isv.PRODUCT_UNSUBSCRIBE", "isv.PRODUCT_UN_SUBSCRIPT", "FUNCTION_NOT_OPENED",
                                "isp.SYSTEM_ERROR", "isp.UNKNOWN", "isp.RES_OWNER_ID_UNKNOWN",
                                "isv.FORBIDDEN_ACTION", "isv.RAM_PERMISSION_DENY", "isp.RAM_PERMISSION_DENY" -> {
                            errorCode = ErrorCode.VC_SMS_PLATFORM_ERROR;
                            errorMsg = "短信平台服务异常，请稍后重试";
                        }
                        // 默认兜底
                        default -> {
                            errorCode = ErrorCode.VC_SMS_SEND_FAIL;
                            errorMsg = "短信验证码发送失败";
                        }
                    }

                    logger.error("❌ 短信发送失败: code={} msg={}", codeValue, message);
                    return ResponseResult.error(errorCode, errorMsg);
                }
            }

            // 存储验证码和限制信息
            redisTemplate.opsForValue().set(redisKeyPrefixSmsCode + phoneNumber, code, smsCodeConfig.getExpiration(), TimeUnit.SECONDS);
            redisTemplate.opsForValue().set(redisIPSendIntervalKey, "1", smsCodeConfig.getIpSendInterval(), TimeUnit.SECONDS);
            redisTemplate.opsForValue().set(redisPhoneSendIntervalKey, "1", smsCodeConfig.getPhoneSendInterval(), TimeUnit.SECONDS);

            logger.info("✅ 短信发送成功 -> {}", phoneNumber);
            return ResponseResult.success("短信验证码发送成功");
        } catch (Exception e) {
            logger.error("发送短信验证码失败：{}", e.getMessage());
            return ResponseResult.error(ErrorCode.VC_SMS_SEND_FAIL, "发送短信验证码失败");
        }
    }

    // 发送短信验证码
    @Override
    public ResponseResult<String> sendSmsCode(String phoneNumber) {
        IpInfo ipInfo = RequestContext.getCurrentIp();
        String clientIp = ipInfo.getIpAddress();
        String redisIPSendIntervalKey = redisKeyPrefixSmsCodeIp + clientIp; // 记录 IP 限制
        String redisPhoneSendIntervalKey = redisKeyPrefixSmsCodePhone + phoneNumber; // 记录 手机号 限制

        logger.info("[发送短信][{}]  ,  [IP] [{}]", phoneNumber, clientIp);
        if (phoneNumber == null || phoneNumber.length() != 11) {
            logger.warn("[手机号错误]：请输入正确手机号");
            return ResponseResult.error(ErrorCode.VC_SMS_INVALID_PHONE, "请输入正确手机号");
        }
        // 检查IP 规定时间内是否已成功发送
        Long lastIPSendTime = redisTemplate.getExpire(redisIPSendIntervalKey, TimeUnit.SECONDS);
        if (lastIPSendTime > 0) {
            logger.info("[IP限制]：请稍后重试，{} 秒后可重试", lastIPSendTime);
            return ResponseResult.error(ErrorCode.VC_SMS_RESTRICT_IP, "请稍后再试，" + lastIPSendTime + " 秒后可重试");
        }

        // 检查手机号 规定时间内是否已成功发送
        Long lastPhoneSendTime = redisTemplate.getExpire(redisPhoneSendIntervalKey, TimeUnit.SECONDS);
        if (lastPhoneSendTime > 0) {
            logger.info("[手机号限制]：请稍后重试，{} 秒后可重试", lastPhoneSendTime);
            return ResponseResult.error(ErrorCode.VC_SMS_RESTRICT_PHONE, "此号码需要" + lastPhoneSendTime + " 秒后才可重新发送");
        }

        // 生成随机验证码
        int codeLength = smsCodeConfig.getCodeLength();
        StringBuilder codeBuilder = new StringBuilder(codeLength); // 预分配容量
        for (int i = 0; i < smsCodeConfig.getCodeLength(); i++) {
            codeBuilder.append(smsCodeConfig.getCharacter().charAt((int) (Math.random() * smsCodeConfig.getCharacter().length())));
        }
        String code = codeBuilder.toString();

        try {
            return sendSmsMesage(redisIPSendIntervalKey, redisPhoneSendIntervalKey, phoneNumber, code);
        } catch (Exception e) {
            logger.error("❌ 短信验证码发送失败");
            return ResponseResult.error(ErrorCode.VC_SMS_SEND_FAIL, "短信验证码发送失败");
        }
    }

    @Override
    public ResponseResult<String> sendEmailCode(String email) {
        return null;
    }

    @Override
    public ResponseResult<String> checkImageCode(ImageCode imageCode) {
        if (imageCode == null || imageCode.getCode() == null || imageCode.getSessionId() == null) {
            return ResponseResult.error(ErrorCode.PARAM_INVALID, "验证码参数错误");
        }
        // 检查 Redis 是否存在该验证码
        String storedCode = redisTemplate.opsForValue().get(imageCode.getSessionId());
        if (storedCode == null) {
            return ResponseResult.error(ErrorCode.VC_IMAGE_EXPIRED, "验证码已过期或不存在");
        }

        // 忽略大小写比较验证码是否匹配
        if (!storedCode.equalsIgnoreCase(imageCode.getCode())) {
            return ResponseResult.error(ErrorCode.VC_IMAGE_INVALID, "验证码错误");
        }

        // 验证通过后删除 Redis 中的验证码，防止重复使用
        redisTemplate.delete(imageCode.getSessionId());
        return ResponseResult.success("验证码正确");
    }

    @Override
    public ResponseResult<String> checkSmsCode(String phoneNumber, String code) {
        // 检查 Redis 是否存在该验证码
        String storedCode = redisTemplate.opsForValue().get(redisKeyPrefixSmsCode + phoneNumber);
        if (storedCode == null) {
            return ResponseResult.error(ErrorCode.VC_SMS_CODE_EXPIRED, "验证码已过期或不存在");
        }

        // 忽略大小写比较验证码是否匹配
        if (!storedCode.equalsIgnoreCase(code)) {
            return ResponseResult.error(ErrorCode.VC_SMS_CODE_ERROR, "验证码错误");
        }

        // 验证通过后删除 Redis 中的验证码，防止重复使用
        redisTemplate.delete(redisKeyPrefixSmsCode + phoneNumber);
        return ResponseResult.success("验证码正确");
    }

    @Override
    public ResponseResult<String> checkEmailCode(String email, String code) {
        return null;
    }

    // 字体加载
    private Font loadFontSafely(String fontName, int fontSize) {
        // 首先检查系统是否已安装该字体
        Font systemFont = findSystemFont(fontName, fontSize);
        if (systemFont != null) {
            return systemFont;
        }

        // 尝试加载外部或嵌入的TTF字体
        try (InputStream fontStream = getFontResourceStream(fontName)) {
            if (fontStream != null) {
                return Font.createFont(Font.TRUETYPE_FONT, fontStream)
                        .deriveFont(Font.PLAIN, fontSize);
            }
        } catch (FontFormatException | IOException e) {
            logger.warn("❌ 字体文件加载失败: {}", fontName, e);
        }

        // 系统字体回退策略
        return getFallbackFont(fontSize);
    }

    // 检查系统已安装字体
    private Font findSystemFont(String fontName, int fontSize) {
        logger.info("正在尝试匹配系统字体：{}", fontName);
        GraphicsEnvironment ge = GraphicsEnvironment.getLocalGraphicsEnvironment();
        Font[] allFonts = ge.getAllFonts();

        // 精确匹配字体名称
        for (Font font : allFonts) {
            if (fontName.equalsIgnoreCase(font.getFontName())) {
                logger.info("✅  成功匹配字体名称：{}", fontName);
                return font.deriveFont(Font.PLAIN, fontSize);
            }
        }

        // 模糊匹配字体家族
        for (Font font : allFonts) {
            if (fontName.equalsIgnoreCase(font.getFamily())) {
                logger.info("✅  成功匹配字体家族：{}", fontName);
                return font.deriveFont(Font.PLAIN, fontSize);
            }
        }
        logger.warn("⚠️ 字体匹配失败：{}", fontName);
        return null;
    }

    // 系统回退字体（按优先级尝试）
    private Font getFallbackFont(int fontSize) {
        String[] fallbackList = {
                "SimSun",    // Windows 宋体
                "Microsoft YaHei",  // Windows 微软雅黑
                "Songti SC", // macOS 宋体
                "Noto Sans CJK SC", // Linux
                Font.SERIF,  // 通用衬线
                Font.SANS_SERIF // 通用无衬线
        };

        for (String name : fallbackList) {
            try {
                Font font = new Font(name, Font.PLAIN, fontSize);
                if (!font.getFamily().equals(Font.DIALOG)) {
                    logger.warn("⚠️ 已回退到系统字体: {}", name);
                    return font;
                }
            } catch (Exception e) {
                logger.warn("⚠️ 回退字体 {} 不可用", name);
            }
        }

        logger.warn("⚠️ 已回退到字体: {}", "MONOSPACED");
        // 终极回退
        return new Font(Font.MONOSPACED, Font.PLAIN, fontSize);
    }

    // 获取字体文件流
    private InputStream getFontResourceStream(String fontPath) {
        logger.info("正在尝试加载外部字体文件: {}", fontPath);
        // 尝试外部文件（优先级最高）
        Path externalFile = Paths.get("config", fontPath.split("/")); // 跨平台路径分隔符处理
        if (Files.exists(externalFile)) {
            try {
                InputStream externalInputStream = Files.newInputStream(externalFile);
                logger.info("✅  成功加载字体：{}", externalFile);
                return externalInputStream;
            } catch (IOException e) {
                logger.warn("⚠️ 外部字体加载失败，回退内置资源: {}", externalFile, e);
            }
        }
        else
        {
            logger.warn("⚠️ 外部字体文件未找到: {}", externalFile);
        }

        // 尝试从 ClassLoader 加载（兼容 JAR）
        logger.info("正在尝试加载内置资源: {}", fontPath);
        InputStream classLoaderInputStream = VerificationCodeServiceImpl.class
                .getClassLoader()
                .getResourceAsStream(fontPath);

        // 备用加载方式（极端情况兜底）
        if (classLoaderInputStream == null) {
            logger.warn("⚠️ 字体文件未找到: {}", fontPath);
            classLoaderInputStream = VerificationCodeServiceImpl.class.getResourceAsStream("/" + fontPath);
            if (classLoaderInputStream == null) {
                logger.warn("⚠️ 字体文件未找到: {}", "/" + fontPath);
            }
            else
            {
                logger.info("✅  成功加载字体：{}", "/" + fontPath);
            }
        }
        else
        {
            logger.info("✅  成功加载字体：{}", fontPath);
        }
        return classLoaderInputStream;
    }
}
