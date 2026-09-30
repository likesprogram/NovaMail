package com.example.novamailservice.service.DomainNames.Impl;

import com.example.novamailservice.config.RedisConfig;
import com.example.novamailservice.config.Mail.DnsVerificationProperties;
import com.example.novamailservice.dto.DomainNames.DomainDkimConfig;
import com.example.novamailservice.dto.DomainNames.DomainDnsVerification;
import com.example.novamailservice.dto.Result.ErrorCode;
import com.example.novamailservice.dto.Result.ResponseResult;
import com.example.novamailservice.entity.DomainNames.DomainCredentials;
import com.example.novamailservice.entity.DomainNames.DomainNames;
import com.example.novamailservice.entity.Mail.MailBoxes;
import com.example.novamailservice.mapper.DomainNames.DomainCredentialsMapper;
import com.example.novamailservice.mapper.DomainNames.DomainNamesMapper;
import com.example.novamailservice.mapper.Mail.MailBoxesMapper;
import com.example.novamailservice.service.DomainNames.DomainNamesService;
import com.example.novamailservice.util.Cache;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import javax.naming.NamingEnumeration;
import javax.naming.directory.Attribute;
import javax.naming.directory.Attributes;
import javax.naming.directory.DirContext;
import javax.naming.directory.InitialDirContext;
import java.io.ByteArrayInputStream;
import java.security.KeyPair;
import java.security.KeyPairGenerator;
import java.security.cert.Certificate;
import java.security.cert.CertificateFactory;
import java.util.ArrayList;
import java.util.Base64;
import java.util.Hashtable;
import java.util.List;
import java.util.Locale;
import java.util.Objects;
import java.util.stream.Collectors;

@Service
public class DomainNamesServiceImpl implements DomainNamesService {
    private final Logger logger = LoggerFactory.getLogger(DomainNamesServiceImpl.class.getName());
    private static final String DKIM_SELECTOR = "default";
    private final DomainNamesMapper domainNamesMapper;
    private final DomainCredentialsMapper domainCredentialsMapper;
    private final MailBoxesMapper mailBoxesMapper;
    private final DnsVerificationProperties dnsVerificationProperties;
    private final Cache cache;
    private static final String DomainNamesCachePrefix = RedisConfig.NovaCache + "DomainNamesCache:";
    private static final String redisKeyPrefixDefaultDomainName = DomainNamesCachePrefix + "DefaultDomainName";
    private static final String redisKeyPrefixDefaultDomainNameById = DomainNamesCachePrefix + "Id:";
    private static final String redisKeyPrefixDefaultDomainNameByName = DomainNamesCachePrefix + "Name:";
    private static final String redisKeyPrefixDefaultDomainNameByUserId = DomainNamesCachePrefix + "UserId:";
    @Autowired
    public DomainNamesServiceImpl(DomainNamesMapper domainNamesMapper, DomainCredentialsMapper domainCredentialsMapper, MailBoxesMapper mailBoxesMapper, DnsVerificationProperties dnsVerificationProperties, Cache cache) {
        this.domainNamesMapper = domainNamesMapper;
        this.domainCredentialsMapper = domainCredentialsMapper;
        this.mailBoxesMapper = mailBoxesMapper;
        this.dnsVerificationProperties = dnsVerificationProperties;
        this.cache = cache;
    }

    private ResponseResult<String> validateDomainName(String domainName) {
        if (domainName == null || domainName.isBlank()) return ResponseResult.error(ErrorCode.DOMAIN_INVALID, "域名不能为空");
        if (domainName.length() > 253) return ResponseResult.error(ErrorCode.DOMAIN_INVALID, "域名过长");
        if (!domainName.matches("^(?!-)(?:[A-Za-z0-9-]{1,63}\\.)+[A-Za-z]{2,}$")) {
            return ResponseResult.error(ErrorCode.DOMAIN_INVALID, "域名格式错误");
        }
        return ResponseResult.success();
    }

    private void clearDomainCaches() {
        cache.removeCacheByPrefix(DomainNamesCachePrefix);
    }

    private boolean hasDkimCredentials(DomainCredentials credentials) {
        return credentials != null
                && credentials.getSigPrivateKey() != null && !credentials.getSigPrivateKey().isBlank()
                && credentials.getSigPublicKey() != null && !credentials.getSigPublicKey().isBlank();
    }

    private String toPem(String type, byte[] encoded) {
        String body = Base64.getMimeEncoder(64, "\n".getBytes()).encodeToString(encoded);
        return "-----BEGIN " + type + "-----\n" + body + "\n-----END " + type + "-----";
    }

    private DomainCredentials generateDkimCredentials(Long domainNameId) throws Exception {
        KeyPairGenerator generator = KeyPairGenerator.getInstance("RSA");
        generator.initialize(2048);
        KeyPair keyPair = generator.generateKeyPair();

        DomainCredentials credentials = new DomainCredentials();
        credentials.setDomainNameId(domainNameId);
        credentials.setSigPrivateKey(toPem("PRIVATE KEY", keyPair.getPrivate().getEncoded()));
        credentials.setSigPublicKey(toPem("PUBLIC KEY", keyPair.getPublic().getEncoded()));
        return credentials;
    }

    private ResponseResult<DomainCredentials> ensureDkimCredentials(Long userId, DomainNames domainName) {
        if (userId == null || domainName == null || domainName.getDomainNameId() == null) {
            return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");
        }
        if (domainName.getUserId() == null || !domainName.getUserId().equals(userId)) {
            return ResponseResult.error(ErrorCode.FORBIDDEN, "只能配置自己的域名");
        }

        try {
            DomainCredentials credentials = domainCredentialsMapper.selectByDomainNameId(domainName.getDomainNameId());
            if (!hasDkimCredentials(credentials)) {
                credentials = generateDkimCredentials(domainName.getDomainNameId());
                if (domainCredentialsMapper.upsertDkimCredentials(credentials) <= 0) {
                    return ResponseResult.error(ErrorCode.DATABASE_ERROR, "DKIM 密钥生成失败");
                }
                credentials = domainCredentialsMapper.selectByDomainNameId(domainName.getDomainNameId());
            }

            if (!Boolean.TRUE.equals(domainName.getHasDKIM())) {
                domainNamesMapper.updateOwnedDkimStatus(domainName.getDomainNameId(), userId, true);
                domainName.setHasDKIM(true);
            }
            return ResponseResult.success(credentials);
        } catch (Exception e) {
            logger.error("DKIM 密钥生成异常: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "DKIM 密钥生成失败");
        }
    }

    private ResponseResult<DomainCredentials> ensureSystemDkimCredentials(DomainNames domainName) {
        if (domainName == null || domainName.getDomainNameId() == null) {
            return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");
        }

        try {
            DomainCredentials credentials = domainCredentialsMapper.selectByDomainNameId(domainName.getDomainNameId());
            if (!hasDkimCredentials(credentials)) {
                credentials = generateDkimCredentials(domainName.getDomainNameId());
                if (domainCredentialsMapper.upsertDkimCredentials(credentials) <= 0) {
                    return ResponseResult.error(ErrorCode.DATABASE_ERROR, "DKIM 密钥生成失败");
                }
                credentials = domainCredentialsMapper.selectByDomainNameId(domainName.getDomainNameId());
            }

            if (!Boolean.TRUE.equals(domainName.getHasDKIM())) {
                domainNamesMapper.updateDkimStatus(domainName.getDomainNameId(), true);
                domainName.setHasDKIM(true);
            }
            return ResponseResult.success(credentials);
        } catch (Exception e) {
            logger.error("DKIM 密钥生成异常: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "DKIM 密钥生成失败");
        }
    }

    private String extractPemBody(String pem) {
        if (pem == null) return "";
        return pem
                .replace("-----BEGIN PUBLIC KEY-----", "")
                .replace("-----END PUBLIC KEY-----", "")
                .replaceAll("\\s+", "");
    }

    private String normalizePem(String value) {
        return value == null ? "" : value.trim().replace("\r\n", "\n");
    }

    private String normalizeDnsHost(String value) {
        if (value == null) return "";
        String normalized = value.trim().toLowerCase(Locale.ROOT);
        while (normalized.endsWith(".")) {
            normalized = normalized.substring(0, normalized.length() - 1);
        }
        return normalized;
    }

    private String normalizeDnsText(String value) {
        if (value == null) return "";
        return value.replace("\"", "").replaceAll("\\s+", "");
    }

    private List<String> normalizeTxtRecords(List<String> records) {
        if (records == null || records.isEmpty()) return List.of();
        List<String> normalized = new ArrayList<>();
        for (String record : records) {
            String value = normalizeDnsText(record);
            if (!value.isBlank()) normalized.add(value);

            String joinedSegments = joinQuotedTxtSegments(record);
            if (!joinedSegments.isBlank() && !joinedSegments.equals(value)) normalized.add(joinedSegments);
        }
        return normalized.stream().distinct().toList();
    }

    private String joinQuotedTxtSegments(String record) {
        if (record == null || !record.contains("\"")) return "";
        StringBuilder builder = new StringBuilder();
        boolean inQuote = false;
        boolean escaped = false;
        StringBuilder current = new StringBuilder();
        boolean foundSegment = false;

        for (int index = 0; index < record.length(); index++) {
            char ch = record.charAt(index);
            if (escaped) {
                if (inQuote) current.append(ch);
                escaped = false;
                continue;
            }
            if (ch == '\\') {
                escaped = true;
                continue;
            }
            if (ch == '"') {
                if (inQuote) {
                    builder.append(current);
                    current.setLength(0);
                    foundSegment = true;
                }
                inQuote = !inQuote;
                continue;
            }
            if (inQuote) current.append(ch);
        }

        return foundSegment ? normalizeDnsText(builder.toString()) : "";
    }

    private List<String> queryDnsRecordsWithServer(String host, String type, String dnsUrl) {
        List<String> records = new ArrayList<>();
        try {
            Hashtable<String, String> env = new Hashtable<>();
            env.put("java.naming.factory.initial", "com.sun.jndi.dns.DnsContextFactory");
            if (dnsUrl != null && !dnsUrl.isBlank()) env.put("java.naming.provider.url", dnsUrl);
            DirContext context = new InitialDirContext(env);
            Attributes attributes = context.getAttributes(host, new String[]{type});
            Attribute attribute = attributes.get(type);
            if (attribute == null) return records;
            NamingEnumeration<?> allRecords = attribute.getAll();
            while (allRecords.hasMore()) {
                records.add(String.valueOf(allRecords.next()));
            }
        } catch (Exception e) {
            logger.warn("DNS 查询失败: host={}, type={}, server={}, error={}", host, type, dnsUrl == null ? "system" : dnsUrl, e.getMessage());
        }
        return records;
    }

    private List<String> normalizeList(List<String> values) {
        if (values == null) return List.of();
        return values.stream()
                .filter(Objects::nonNull)
                .map(String::trim)
                .filter(value -> !value.isBlank())
                .distinct()
                .toList();
    }

    private List<String> queryDnsRecords(String host, String type) {
        List<String> records = queryDnsRecordsWithServer(host, type, null);
        if (!records.isEmpty()) return records;

        records = queryDnsRecordsWithServer(host, type, "dns://8.8.8.8/");
        if (!records.isEmpty()) return records;

        return queryDnsRecordsWithServer(host, type, "dns://1.1.1.1/");
    }

    private String joinValues(List<String> values) {
        return values == null || values.isEmpty() ? "未查询到记录" : String.join(" | ", values);
    }

    private DomainDnsVerification.RecordCheck buildCheck(
            String key,
            String label,
            String host,
            String recordType,
            List<String> expectedValues,
            List<String> actualValues,
            boolean passed,
            String message
    ) {
        DomainDnsVerification.RecordCheck check = new DomainDnsVerification.RecordCheck();
        check.setKey(key);
        check.setLabel(label);
        check.setHost(host);
        check.setRecordType(recordType);
        check.setExpectedValue(joinValues(expectedValues));
        check.setActualValue(joinValues(actualValues));
        check.setPassed(passed);
        check.setMessage(message);
        return check;
    }

    private DomainDnsVerification.RecordCheck checkARecord(String key, String label, String host, List<String> expectedIps) {
        List<String> expected = normalizeList(expectedIps);
        List<String> actual = normalizeList(queryDnsRecords(host, "A"));
        boolean passed = !expected.isEmpty() && actual.containsAll(expected);
        String message = expected.isEmpty() ? "application.yml 未配置期望 IP" : (passed ? "解析正确" : "A 记录未指向配置的 IP");
        return buildCheck(key, label, host, "A", expected, actual, passed, message);
    }

    private DomainDnsVerification.RecordCheck checkTxtContains(String key, String label, String host, List<String> expectedValues) {
        List<String> expected = normalizeList(expectedValues);
        List<String> actual = queryDnsRecords(host, "TXT");
        List<String> normalizedActual = normalizeTxtRecords(actual).stream()
                .map(value -> value.toLowerCase(Locale.ROOT))
                .collect(Collectors.toList());
        boolean passed = !expected.isEmpty() && expected.stream()
                .map(value -> normalizeDnsText(value).toLowerCase(Locale.ROOT))
                .allMatch(expectedValue -> normalizedActual.stream().anyMatch(actualValue -> actualValue.contains(expectedValue)));
        String message = expected.isEmpty() ? "application.yml 未配置期望 TXT 内容" : (passed ? "解析正确" : "TXT 记录未包含期望内容");
        return buildCheck(key, label, host, "TXT", expected, actual, passed, message);
    }

    private DomainDnsVerification.RecordCheck checkDkimRecord(DomainDkimConfig dkimConfig) {
        List<String> expected = List.of(dkimConfig.getRecordValue());
        List<String> actual = queryDnsRecords(dkimConfig.getRecordName(), "TXT");
        String expectedValue = normalizeDnsText(dkimConfig.getRecordValue());
        boolean passed = normalizeTxtRecords(actual).stream().anyMatch(value -> value.equals(expectedValue));
        return buildCheck(
                "dkim",
                "DKIM TXT",
                dkimConfig.getRecordName(),
                "TXT",
                expected,
                actual,
                passed,
                passed ? "解析正确" : "DKIM TXT 记录与系统生成值不一致"
        );
    }

    private DomainDnsVerification.RecordCheck checkMxRecord(String domainName) {
        String expectedHost = normalizeDnsHost(dnsVerificationProperties.getMxHost());
        List<String> actual = queryDnsRecords(domainName, "MX");
        boolean passed = !expectedHost.isBlank() && actual.stream()
                .map(this::normalizeDnsHost)
                .anyMatch(value -> value.endsWith(" " + expectedHost) || value.equals(expectedHost));
        String message = expectedHost.isBlank() ? "application.yml 未配置 MX 主机" : (passed ? "解析正确" : "MX 记录未指向配置的邮件主机");
        return buildCheck(
                "mx",
                "根域 MX",
                domainName,
                "MX",
                expectedHost.isBlank() ? List.of() : List.of(expectedHost),
                actual,
                passed,
                message
        );
    }

    private ResponseResult<String> extractPublicKeyPem(String certificatePem) {
        try {
            CertificateFactory certificateFactory = CertificateFactory.getInstance("X.509");
            Certificate certificate = certificateFactory.generateCertificate(new ByteArrayInputStream(certificatePem.getBytes()));
            return ResponseResult.success(toPem("PUBLIC KEY", certificate.getPublicKey().getEncoded()));
        } catch (Exception e) {
            return ResponseResult.error(ErrorCode.PARAM_INVALID, "TLS 证书解析失败");
        }
    }

    private DomainDkimConfig buildDkimConfig(DomainNames domainName, DomainCredentials credentials) {
        DomainDkimConfig config = new DomainDkimConfig();
        config.setDomainNameId(domainName.getDomainNameId());
        config.setDomainName(domainName.getDomainName());
        config.setSelector(DKIM_SELECTOR);
        config.setRecordType("TXT");
        config.setRecordName(DKIM_SELECTOR + "._domainkey." + domainName.getDomainName());
        config.setRecordValue("v=DKIM1; k=rsa; p=" + extractPemBody(credentials.getSigPublicKey()));
        config.setHasDKIM(true);
        return config;
    }

    private ResponseResult<String> configureDomainTlsInternal(DomainCredentials domainCredentials) {
        if (domainCredentials == null || domainCredentials.getDomainNameId() == null || domainCredentials.getDomainNameId() <= 0) {
            return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");
        }

        String privateKey = normalizePem(domainCredentials.getPrivateKey());
        String certificate = normalizePem(domainCredentials.getCertificate());
        String caChain = normalizePem(domainCredentials.getCaChain());
        if (privateKey.isBlank() || certificate.isBlank()) {
            return ResponseResult.error(ErrorCode.PARAM_INVALID, "TLS 私钥和证书不能为空");
        }
        if (!privateKey.contains("BEGIN") || !privateKey.contains("PRIVATE KEY")) {
            return ResponseResult.error(ErrorCode.PARAM_INVALID, "TLS 私钥格式错误");
        }
        if (!certificate.contains("BEGIN CERTIFICATE")) {
            return ResponseResult.error(ErrorCode.PARAM_INVALID, "TLS 证书格式错误");
        }

        ResponseResult<String> publicKeyResult = extractPublicKeyPem(certificate);
        if (!publicKeyResult.isSuccess()) return ResponseResult.error(publicKeyResult.getCode(), publicKeyResult.getMessage());

        DomainCredentials tlsCredentials = new DomainCredentials();
        tlsCredentials.setDomainNameId(domainCredentials.getDomainNameId());
        tlsCredentials.setPrivateKey(privateKey);
        tlsCredentials.setPublicKey(publicKeyResult.getData());
        tlsCredentials.setCertificate(certificate);
        tlsCredentials.setCaChain(caChain.isBlank() ? null : caChain);

        if (domainCredentialsMapper.upsertTlsCredentials(tlsCredentials) <= 0) {
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "TLS 证书写入失败");
        }
        clearDomainCaches();
        return ResponseResult.success("TLS 配置成功");
    }

    private ResponseResult<DomainDnsVerification> verifyDomainDnsInternal(DomainNames domainName, DomainDkimConfig dkimConfig) {
        String normalizedDomainName = normalizeDnsHost(domainName.getDomainName());
        List<String> recordsToValidate = normalizeList(dnsVerificationProperties.getSpfIp4()).stream()
                .map(value -> "ip4:" + value)
                .toList();

        List<DomainDnsVerification.RecordCheck> checks = new ArrayList<>();
        checks.add(checkDkimRecord(dkimConfig));
        checks.add(checkTxtContains("spf", "根域 SPF", normalizedDomainName, recordsToValidate));
        checks.add(checkARecord("imap-a", "imap A", "imap." + normalizedDomainName, dnsVerificationProperties.getImapA()));
        checks.add(checkARecord("smtp-a", "smtp A", "smtp." + normalizedDomainName, dnsVerificationProperties.getSmtpA()));
        checks.add(checkMxRecord(normalizedDomainName));

        DomainDnsVerification verification = new DomainDnsVerification();
        verification.setDomainNameId(domainName.getDomainNameId());
        verification.setDomainName(normalizedDomainName);
        verification.setRecords(checks);
        verification.setPassed(checks.stream().allMatch(check -> Boolean.TRUE.equals(check.getPassed())));
        return ResponseResult.success(verification);
    }

    @Override // 获取系统默认域名
    public ResponseResult<DomainNames> getSystemDefaultDomainName() {
        // Redis 缓存查询
        ResponseResult<DomainNames> cachedResult = cache.getCache(redisKeyPrefixDefaultDomainName, DomainNames.class);
        if (cachedResult.isSuccess() || cachedResult.getCode() == ErrorCode.CACHE_ERROR) return cachedResult;

        // 数据库查询
        try {
            DomainNames defaultDomainName = domainNamesMapper.getSystemDefaultDomainName();
            if (defaultDomainName != null) {
                cache.setCache(redisKeyPrefixDefaultDomainName, defaultDomainName);
                return ResponseResult.success(defaultDomainName);
            }
            return ResponseResult.error(ErrorCode.DOMAIN_NOT_FOUND, "系统默认域名不存在");
        } catch (Exception e) {
            logger.error("数据库查询异常: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "数据库访问异常");
        }
    }

    @Override
    public ResponseResult<DomainNames> getDomainNameById(Long domainNameId) {
        if (domainNameId == null || domainNameId <= 0) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");

        // Redis 缓存查询
        String redisKey = redisKeyPrefixDefaultDomainNameById + domainNameId;
        ResponseResult<DomainNames> cachedResult = cache.getCache(redisKey, DomainNames.class);
        if (cachedResult.isSuccess() || cachedResult.getCode() == ErrorCode.CACHE_ERROR) return cachedResult;

        // 数据库查询
        try {
            DomainNames existing = domainNamesMapper.selectById(domainNameId);
            if (existing == null) return ResponseResult.error(ErrorCode.DOMAIN_NOT_FOUND, "域名不存在");

            // 同步写入缓存
            cache.setCache(redisKey, existing);
            return ResponseResult.success(existing);
        } catch (Exception e) {
            logger.error("数据库查询异常: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "数据库访问异常");
        }
    }

    @Override
    public ResponseResult<DomainNames> getDomainNameByName(String domainName) {
        if (domainName == null || domainName.isEmpty()) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");

        // Redis 缓存查询
        String redisKey = redisKeyPrefixDefaultDomainNameByName + domainName;
        ResponseResult<DomainNames> cachedResult = cache.getCache(redisKey, DomainNames.class);
        if (cachedResult.isSuccess() || cachedResult.getCode() == ErrorCode.CACHE_ERROR) return cachedResult;

        // 数据库查询
        try {
            DomainNames existing = domainNamesMapper.selectByName(domainName);
            if (existing == null) return ResponseResult.error(ErrorCode.DOMAIN_NOT_FOUND, "域名不存在");

            // 同步写入缓存
            cache.setCache(redisKey, existing);
            return ResponseResult.success(existing);
        } catch (Exception e) {
            logger.error("数据库查询异常: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "数据库访问异常");
        }
    }

    @Override
    public ResponseResult<List<DomainNames>> getDomainNameByUserId(Long userId) {
        if (userId == null || userId <= 0) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");

        // Redis 缓存查询
        String redisKey = redisKeyPrefixDefaultDomainNameByUserId + userId;
        ResponseResult<List<DomainNames>> cachedResult = cache.getCacheList(redisKey, DomainNames.class);
        if (cachedResult.isSuccess() || cachedResult.getCode() == ErrorCode.CACHE_ERROR) return cachedResult;

        // 数据库查询
        try {
            List<DomainNames> domainNamesList = domainNamesMapper.selectByUserId(userId);
            if (domainNamesList == null || domainNamesList.isEmpty()) return ResponseResult.error(ErrorCode.DOMAIN_NOT_FOUND, "该用户没有域名");

            // 同步写入缓存
            cache.setCache(redisKey, domainNamesList);
            return ResponseResult.success(domainNamesList);
        } catch (Exception e) {
            logger.error("数据库查询异常: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "数据库访问异常");
        }
    }

    @Override
    public ResponseResult<List<DomainNames>> getAvailableDomainNameByUserId(Long userId) {
        if (userId == null || userId <= 0) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");

        try {
            List<DomainNames> domainNamesList = domainNamesMapper.selectAvailableByUserId(userId);
            return ResponseResult.success(domainNamesList);
        } catch (Exception e) {
            logger.error("数据库查询异常: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "数据库访问异常");
        }
    }

    @Override
    public ResponseResult<List<DomainNames>> getAllDomainNames() {
        try {
            return ResponseResult.success(domainNamesMapper.selectAll());
        } catch (Exception e) {
            logger.error("数据库查询异常: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "数据库访问异常");
        }
    }

    @Override
    @Transactional
    public ResponseResult<DomainNames> createSystemDomainName(DomainNames domainName) {
        if (domainName == null) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");

        String normalizedName = domainName.getDomainName() == null ? "" : domainName.getDomainName().trim().toLowerCase();
        ResponseResult<String> validation = validateDomainName(normalizedName);
        if (!validation.isSuccess()) return ResponseResult.error(validation.getCode(), validation.getMessage());
        if (domainName.getUserId() != null && Boolean.TRUE.equals(domainName.getIsDefault())) {
            return ResponseResult.error(ErrorCode.FORBIDDEN, "用户私有域名不能设为系统默认域名");
        }

        try {
            DomainNames existing = domainNamesMapper.selectByName(normalizedName);
            if (existing != null) return ResponseResult.error(ErrorCode.CONFLICT, "域名已存在");

            DomainNames newDomainName = new DomainNames();
            newDomainName.setDomainName(normalizedName);
            newDomainName.setUserId(domainName.getUserId());
            newDomainName.setHasCertificate(Boolean.TRUE.equals(domainName.getHasCertificate()));
            newDomainName.setHasDKIM(false);
            newDomainName.setIsDefault(false);

            if (domainNamesMapper.createDomainName(newDomainName) <= 0) return ResponseResult.error(ErrorCode.DATABASE_ERROR, "域名创建失败");
            if (Boolean.TRUE.equals(domainName.getHasDKIM())) {
                ResponseResult<DomainCredentials> dkimResult = ensureSystemDkimCredentials(newDomainName);
                if (!dkimResult.isSuccess()) return ResponseResult.error(dkimResult.getCode(), dkimResult.getMessage());
            }
            if (Boolean.TRUE.equals(domainName.getIsDefault())) {
                ResponseResult<String> defaultResult = setSystemDefaultDomainName(newDomainName.getDomainNameId());
                if (!defaultResult.isSuccess()) return ResponseResult.error(defaultResult.getCode(), defaultResult.getMessage());
            }

            clearDomainCaches();
            DomainNames created = domainNamesMapper.selectById(newDomainName.getDomainNameId());
            return ResponseResult.success(created == null ? newDomainName : created);
        } catch (Exception e) {
            logger.error("域名创建异常: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "域名创建失败");
        }
    }

    @Override
    @Transactional
    public ResponseResult<DomainNames> updateSystemDomainName(DomainNames domainName) {
        if (domainName == null || domainName.getDomainNameId() == null || domainName.getDomainNameId() <= 0) {
            return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");
        }

        String normalizedName = domainName.getDomainName() == null ? "" : domainName.getDomainName().trim().toLowerCase();
        ResponseResult<String> validation = validateDomainName(normalizedName);
        if (!validation.isSuccess()) return ResponseResult.error(validation.getCode(), validation.getMessage());
        if (domainName.getUserId() != null && Boolean.TRUE.equals(domainName.getIsDefault())) {
            return ResponseResult.error(ErrorCode.FORBIDDEN, "用户私有域名不能设为系统默认域名");
        }

        try {
            DomainNames current = domainNamesMapper.selectById(domainName.getDomainNameId());
            if (current == null) return ResponseResult.error(ErrorCode.DOMAIN_NOT_FOUND, "域名不存在");
            if (domainName.getUserId() != null && Boolean.TRUE.equals(current.getIsDefault())) {
                return ResponseResult.error(ErrorCode.FORBIDDEN, "系统默认域名不能改为用户私有域名");
            }

            DomainNames duplicated = domainNamesMapper.selectByName(normalizedName);
            if (duplicated != null && !duplicated.getDomainNameId().equals(domainName.getDomainNameId())) {
                return ResponseResult.error(ErrorCode.CONFLICT, "域名已存在");
            }

            current.setDomainName(normalizedName);
            current.setUserId(domainName.getUserId());
            current.setHasCertificate(Boolean.TRUE.equals(domainName.getHasCertificate()));
            current.setHasDKIM(Boolean.TRUE.equals(domainName.getHasDKIM()));

            if (current.getUserId() != null && Boolean.TRUE.equals(domainName.getIsDefault())) {
                return ResponseResult.error(ErrorCode.FORBIDDEN, "用户私有域名不能设为系统默认域名");
            }

            if (Boolean.TRUE.equals(current.getHasDKIM())) {
                ResponseResult<DomainCredentials> dkimResult = ensureSystemDkimCredentials(current);
                if (!dkimResult.isSuccess()) return ResponseResult.error(dkimResult.getCode(), dkimResult.getMessage());
            }

            if (domainNamesMapper.updateDomainName(current) <= 0) return ResponseResult.error(ErrorCode.DATABASE_ERROR, "域名更新失败");
            if (Boolean.TRUE.equals(domainName.getIsDefault())) {
                ResponseResult<String> defaultResult = setSystemDefaultDomainName(current.getDomainNameId());
                if (!defaultResult.isSuccess()) return ResponseResult.error(defaultResult.getCode(), defaultResult.getMessage());
            }

            clearDomainCaches();
            DomainNames updated = domainNamesMapper.selectById(domainName.getDomainNameId());
            return ResponseResult.success(updated);
        } catch (Exception e) {
            logger.error("域名更新异常: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "域名更新失败");
        }
    }

    @Override
    @Transactional
    public ResponseResult<String> deleteSystemDomainName(Long domainNameId) {
        if (domainNameId == null || domainNameId <= 0) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");

        try {
            DomainNames current = domainNamesMapper.selectById(domainNameId);
            if (current == null) return ResponseResult.error(ErrorCode.DOMAIN_NOT_FOUND, "域名不存在");
            if (Boolean.TRUE.equals(current.getIsDefault())) return ResponseResult.error(ErrorCode.FORBIDDEN, "默认域名不能删除");
            List<MailBoxes> domainMailBoxes = mailBoxesMapper.selectByDomainId(domainNameId);
            if (domainMailBoxes != null && !domainMailBoxes.isEmpty()) {
                return ResponseResult.error(ErrorCode.CONFLICT, "该域名下仍有邮箱");
            }

            if (domainNamesMapper.deleteDomainName(domainNameId) <= 0) return ResponseResult.error(ErrorCode.DATABASE_ERROR, "域名删除失败");
            clearDomainCaches();
            return ResponseResult.success("删除成功");
        } catch (Exception e) {
            logger.error("域名删除异常: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "域名删除失败");
        }
    }

    @Override
    @Transactional
    public ResponseResult<String> setSystemDefaultDomainName(Long domainNameId) {
        if (domainNameId == null || domainNameId <= 0) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");

        try {
            DomainNames current = domainNamesMapper.selectById(domainNameId);
            if (current == null) return ResponseResult.error(ErrorCode.DOMAIN_NOT_FOUND, "域名不存在");
            if (current.getUserId() != null) return ResponseResult.error(ErrorCode.FORBIDDEN, "用户私有域名不能设为系统默认域名");

            domainNamesMapper.clearDefault();
            if (domainNamesMapper.setDefault(domainNameId) <= 0) return ResponseResult.error(ErrorCode.DATABASE_ERROR, "默认域名设置失败");
            clearDomainCaches();
            return ResponseResult.success("设置成功");
        } catch (Exception e) {
            logger.error("默认域名设置异常: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "默认域名设置失败");
        }
    }

    @Override
    public ResponseResult<DomainNames> createUserDomainName(Long userId, DomainNames domainName) {
        if (userId == null || userId <= 0 || domainName == null) return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");

        String normalizedName = domainName.getDomainName() == null ? "" : domainName.getDomainName().trim().toLowerCase();
        ResponseResult<String> validation = validateDomainName(normalizedName);
        if (!validation.isSuccess()) return ResponseResult.error(validation.getCode(), validation.getMessage());

        try {
            DomainNames existing = domainNamesMapper.selectByName(normalizedName);
            if (existing != null) return ResponseResult.error(ErrorCode.CONFLICT, "域名已存在");

            DomainNames newDomainName = new DomainNames();
            newDomainName.setDomainName(normalizedName);
            newDomainName.setUserId(userId);
            newDomainName.setHasCertificate(false);
            newDomainName.setHasDKIM(false);
            newDomainName.setIsDefault(false);

            if (domainNamesMapper.createDomainName(newDomainName) <= 0) return ResponseResult.error(ErrorCode.DATABASE_ERROR, "域名创建失败");
            ResponseResult<DomainCredentials> dkimResult = ensureDkimCredentials(userId, newDomainName);
            if (!dkimResult.isSuccess()) return ResponseResult.error(dkimResult.getCode(), dkimResult.getMessage());

            clearDomainCaches();
            DomainNames created = domainNamesMapper.selectById(newDomainName.getDomainNameId());
            return ResponseResult.success(created == null ? newDomainName : created);
        } catch (Exception e) {
            logger.error("域名创建异常: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "域名创建失败");
        }
    }

    @Override
    public ResponseResult<DomainNames> updateUserDomainName(Long userId, DomainNames domainName) {
        if (userId == null || userId <= 0 || domainName == null || domainName.getDomainNameId() == null || domainName.getDomainNameId() <= 0) {
            return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");
        }

        String normalizedName = domainName.getDomainName() == null ? "" : domainName.getDomainName().trim().toLowerCase();
        ResponseResult<String> validation = validateDomainName(normalizedName);
        if (!validation.isSuccess()) return ResponseResult.error(validation.getCode(), validation.getMessage());

        try {
            DomainNames current = domainNamesMapper.selectById(domainName.getDomainNameId());
            if (current == null) return ResponseResult.error(ErrorCode.DOMAIN_NOT_FOUND, "域名不存在");
            if (current.getUserId() == null || !current.getUserId().equals(userId)) {
                return ResponseResult.error(ErrorCode.FORBIDDEN, "只能编辑自己的域名");
            }

            DomainNames duplicated = domainNamesMapper.selectByName(normalizedName);
            if (duplicated != null && !duplicated.getDomainNameId().equals(domainName.getDomainNameId())) {
                return ResponseResult.error(ErrorCode.CONFLICT, "域名已存在");
            }

            current.setDomainName(normalizedName);
            current.setHasDKIM(true);
            ResponseResult<DomainCredentials> dkimResult = ensureDkimCredentials(userId, current);
            if (!dkimResult.isSuccess()) return ResponseResult.error(dkimResult.getCode(), dkimResult.getMessage());

            if (domainNamesMapper.updateOwnedDomainName(current) <= 0) return ResponseResult.error(ErrorCode.DATABASE_ERROR, "域名更新失败");

            clearDomainCaches();
            DomainNames updated = domainNamesMapper.selectById(domainName.getDomainNameId());
            return ResponseResult.success(updated);
        } catch (Exception e) {
            logger.error("域名更新异常: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "域名更新失败");
        }
    }

    @Override
    public ResponseResult<String> deleteUserDomainName(Long userId, Long domainNameId) {
        if (userId == null || userId <= 0 || domainNameId == null || domainNameId <= 0) {
            return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");
        }

        try {
            DomainNames current = domainNamesMapper.selectById(domainNameId);
            if (current == null) return ResponseResult.error(ErrorCode.DOMAIN_NOT_FOUND, "域名不存在");
            if (current.getUserId() == null || !current.getUserId().equals(userId)) {
                return ResponseResult.error(ErrorCode.FORBIDDEN, "只能删除自己的域名");
            }
            if (Boolean.TRUE.equals(current.getIsDefault())) return ResponseResult.error(ErrorCode.FORBIDDEN, "默认域名不能删除");
            List<MailBoxes> domainMailBoxes = mailBoxesMapper.selectByDomainId(domainNameId);
            if (domainMailBoxes != null && !domainMailBoxes.isEmpty()) {
                return ResponseResult.error(ErrorCode.CONFLICT, "该域名下仍有邮箱");
            }

            if (domainNamesMapper.deleteOwnedDomainName(domainNameId, userId) <= 0) return ResponseResult.error(ErrorCode.DATABASE_ERROR, "域名删除失败");

            clearDomainCaches();
            return ResponseResult.success("删除成功");
        } catch (Exception e) {
            logger.error("域名删除异常: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "域名删除失败");
        }
    }

    @Override
    public boolean canUseDomain(DomainNames domainName, Long userId) {
        if (domainName == null || userId == null || userId <= 0) return false;
        if (domainName.getUserId() == null) return true;
        if (domainName.getUserId().equals(userId)) return true;
        try {
            String permission = domainNamesMapper.selectActivePermission(domainName.getDomainNameId(), userId);
            return permission != null && !permission.isBlank();
        } catch (Exception e) {
            logger.error("域名授权查询异常: {}", e.getMessage());
            return false;
        }
    }

    @Override
    public String getUserDomainPermission(DomainNames domainName, Long userId) {
        if (domainName == null || userId == null || userId <= 0) return "NONE";
        if (domainName.getUserId() == null) return "PUBLIC";
        if (domainName.getUserId().equals(userId)) return "OWNER";
        try {
            String permission = domainNamesMapper.selectActivePermission(domainName.getDomainNameId(), userId);
            return permission == null || permission.isBlank() ? "NONE" : permission;
        } catch (Exception e) {
            logger.error("域名授权查询异常: {}", e.getMessage());
            return "NONE";
        }
    }

    @Override
    public ResponseResult<DomainDkimConfig> getUserDomainDkimConfig(Long userId, Long domainNameId) {
        if (userId == null || userId <= 0 || domainNameId == null || domainNameId <= 0) {
            return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");
        }

        try {
            DomainNames domainName = domainNamesMapper.selectById(domainNameId);
            if (domainName == null) return ResponseResult.error(ErrorCode.DOMAIN_NOT_FOUND, "域名不存在");
            if (domainName.getUserId() == null || !domainName.getUserId().equals(userId)) {
                return ResponseResult.error(ErrorCode.FORBIDDEN, "只能查看自己的域名 DKIM 配置");
            }

            ResponseResult<DomainCredentials> dkimResult = ensureDkimCredentials(userId, domainName);
            if (!dkimResult.isSuccess()) return ResponseResult.error(dkimResult.getCode(), dkimResult.getMessage());

            return ResponseResult.success(buildDkimConfig(domainName, dkimResult.getData()));
        } catch (Exception e) {
            logger.error("DKIM 配置查询异常: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "DKIM 配置查询失败");
        }
    }

    @Override
    public ResponseResult<DomainDkimConfig> getSystemDomainDkimConfig(Long domainNameId) {
        if (domainNameId == null || domainNameId <= 0) {
            return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");
        }

        try {
            DomainNames domainName = domainNamesMapper.selectById(domainNameId);
            if (domainName == null) return ResponseResult.error(ErrorCode.DOMAIN_NOT_FOUND, "域名不存在");

            ResponseResult<DomainCredentials> dkimResult = ensureSystemDkimCredentials(domainName);
            if (!dkimResult.isSuccess()) return ResponseResult.error(dkimResult.getCode(), dkimResult.getMessage());

            return ResponseResult.success(buildDkimConfig(domainName, dkimResult.getData()));
        } catch (Exception e) {
            logger.error("DKIM 配置查询异常: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "DKIM 配置查询失败");
        }
    }

    @Override
    public ResponseResult<String> configureUserDomainTls(Long userId, DomainCredentials domainCredentials) {
        if (userId == null || userId <= 0 || domainCredentials == null || domainCredentials.getDomainNameId() == null || domainCredentials.getDomainNameId() <= 0) {
            return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");
        }

        try {
            DomainNames domainName = domainNamesMapper.selectById(domainCredentials.getDomainNameId());
            if (domainName == null) return ResponseResult.error(ErrorCode.DOMAIN_NOT_FOUND, "域名不存在");
            if (domainName.getUserId() == null || !domainName.getUserId().equals(userId)) {
                return ResponseResult.error(ErrorCode.FORBIDDEN, "只能配置自己的域名 TLS");
            }

            ResponseResult<String> tlsResult = configureDomainTlsInternal(domainCredentials);
            if (!tlsResult.isSuccess()) return tlsResult;
            if (!Boolean.TRUE.equals(domainName.getHasCertificate())) {
                domainNamesMapper.updateOwnedCertificateStatus(domainName.getDomainNameId(), userId, true);
            }
            clearDomainCaches();
            return tlsResult;
        } catch (Exception e) {
            logger.error("TLS 配置异常: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "TLS 配置失败");
        }
    }

    @Override
    public ResponseResult<String> configureSystemDomainTls(DomainCredentials domainCredentials) {
        if (domainCredentials == null || domainCredentials.getDomainNameId() == null || domainCredentials.getDomainNameId() <= 0) {
            return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");
        }

        try {
            DomainNames domainName = domainNamesMapper.selectById(domainCredentials.getDomainNameId());
            if (domainName == null) return ResponseResult.error(ErrorCode.DOMAIN_NOT_FOUND, "域名不存在");

            ResponseResult<String> tlsResult = configureDomainTlsInternal(domainCredentials);
            if (!tlsResult.isSuccess()) return tlsResult;
            if (!Boolean.TRUE.equals(domainName.getHasCertificate())) {
                domainNamesMapper.updateCertificateStatus(domainName.getDomainNameId(), true);
            }
            clearDomainCaches();
            return tlsResult;
        } catch (Exception e) {
            logger.error("TLS 配置异常: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.DATABASE_ERROR, "TLS 配置失败");
        }
    }

    @Override
    public ResponseResult<DomainDnsVerification> verifyUserDomainDns(Long userId, Long domainNameId) {
        if (userId == null || userId <= 0 || domainNameId == null || domainNameId <= 0) {
            return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");
        }

        try {
            DomainNames domainName = domainNamesMapper.selectById(domainNameId);
            if (domainName == null) return ResponseResult.error(ErrorCode.DOMAIN_NOT_FOUND, "域名不存在");
            if (domainName.getUserId() == null || !domainName.getUserId().equals(userId)) {
                return ResponseResult.error(ErrorCode.FORBIDDEN, "只能验证自己的域名 DNS");
            }

            ResponseResult<DomainDkimConfig> dkimConfigResult = getUserDomainDkimConfig(userId, domainNameId);
            if (!dkimConfigResult.isSuccess()) {
                return ResponseResult.error(dkimConfigResult.getCode(), dkimConfigResult.getMessage());
            }

            return verifyDomainDnsInternal(domainName, dkimConfigResult.getData());
        } catch (Exception e) {
            logger.error("DNS 验证异常: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.INTERNAL_ERROR, "DNS 验证失败");
        }
    }

    @Override
    public ResponseResult<DomainDnsVerification> verifySystemDomainDns(Long domainNameId) {
        if (domainNameId == null || domainNameId <= 0) {
            return ResponseResult.error(ErrorCode.PARAM_INVALID, "参数错误");
        }

        try {
            DomainNames domainName = domainNamesMapper.selectById(domainNameId);
            if (domainName == null) return ResponseResult.error(ErrorCode.DOMAIN_NOT_FOUND, "域名不存在");

            ResponseResult<DomainDkimConfig> dkimConfigResult = getSystemDomainDkimConfig(domainNameId);
            if (!dkimConfigResult.isSuccess()) {
                return ResponseResult.error(dkimConfigResult.getCode(), dkimConfigResult.getMessage());
            }

            return verifyDomainDnsInternal(domainName, dkimConfigResult.getData());
        } catch (Exception e) {
            logger.error("DNS 验证异常: {}", e.getMessage());
            return ResponseResult.error(ErrorCode.INTERNAL_ERROR, "DNS 验证失败");
        }
    }

}
