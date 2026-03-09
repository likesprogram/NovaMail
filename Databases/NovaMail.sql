 -- 创建数据库
 CREATE DATABASE IF NOT EXISTS NovaMailDatabase DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci;

 -- 使用数据库
 USE NovaMailDatabase;

  -- 用户表
CREATE TABLE IF NOT EXISTS Users (
    userId BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT COMMENT '用户ID, 主键',
    `account` VARCHAR(25) COLLATE utf8mb4_bin NOT NULL UNIQUE COMMENT '账号(大小写敏感), 唯一',
    `password` VARCHAR(255) NOT NULL COMMENT '密码',
    isAdmin TINYINT(1) DEFAULT 0 COMMENT '是否为管理员(0: 否, 1: 是)',
    -- 用户状态
    `status` ENUM(
        'NORMAL',      -- 正常状态
        'DISABLED',    -- 禁用状态
        'DELETED'      -- 删除状态
    ) DEFAULT 'NORMAL' COMMENT '用户状态',
    registerTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '注册时间',
    lastLoginTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '最后登录时间',
    -- 添加索引
    INDEX idx_registerTime (registerTime),              -- 注册统计分析
    INDEX idx_lastLoginTime (lastLoginTime)             -- 活跃用户查询
) COMMENT='用户表';
 -- 创建管理员
INSERT IGNORE INTO Users (userId, `account`, `password`, isAdmin)
VALUES (1, 'NovaMail', '$2a$15$kiMRzgvS3DpUrfmm.FOfoODUEBwTYrhH.TcfFhBb6nhturmqnqcS6', 1);

 -- 用户登录记录表
CREATE TABLE IF NOT EXISTS UserLoginLog (
    logId BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY COMMENT 'ID',
    userId BIGINT UNSIGNED NOT NULL COMMENT '用户ID(NULL表示账号不存在/已删除)',
    loginTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '登录时间',
    -- 网络信息
    clientIp VARCHAR(64) COMMENT '客户端IP地址',
    ipLocation VARCHAR(255) COMMENT 'IP归属地',
    networkType VARCHAR(50) COMMENT '网络类型',
    carrier VARCHAR(50) COMMENT '网络运营商',
    -- 设备信息
    userAgent TEXT COMMENT '用户代理',
    deviceType VARCHAR(50) DEFAULT '未知设备' COMMENT '设备类型',
    osFamily VARCHAR(50) COMMENT '操作系统大类(Windows/iOS/Android等)',
    osVersion VARCHAR(50) COMMENT '操作系统版本',
    osMajor VARCHAR(50) COMMENT '操作系统主版本号',
    osMinor VARCHAR(50) COMMENT '操作系统次版本号',
    -- 浏览器信息
    browser VARCHAR(50) COMMENT '浏览器类型(Chrome/Safari/Firefox/Edge等)',
    browserVersion VARCHAR(50) COMMENT '浏览器完整版本号',
    browserMajor VARCHAR(50) COMMENT '浏览器主版本号',
    -- 设备详细
    deviceBrand VARCHAR(50) COMMENT '设备品牌, 如Apple/Huawei/Xiaomi等',
    deviceModel VARCHAR(50) COMMENT '设备型号, 如iPhone 13/Mate 40等',
    deviceCode VARCHAR(50) COMMENT '设备唯一标识码',
    -- 扩展元数据
    isBot TINYINT(1) DEFAULT 0 COMMENT '是否爬虫(0:否 1:是)',
    platform VARCHAR(50) COMMENT '平台类型(Web/iOS/Android/Windows/macOS等)',
    -- 登录结果
    loginResult ENUM('SUCCESS', 'FAIL') NOT NULL DEFAULT 'SUCCESS' COMMENT '登录结果，成功或失败',
    failReason VARCHAR(255) DEFAULT NULL COMMENT '失败原因(密码错误/验证码错误等)',
    responseTime INT UNSIGNED COMMENT '登录处理耗时(ms)',
    -- 添加索引
    INDEX idx_userId_loginTime (userId, loginTime DESC),    -- 按账号查最近登录记录
    INDEX idx_loginTime (loginTime),                        -- 按时间清理或统计
    INDEX idx_clientIp (clientIp),                          -- 查找同IP登录记录
    INDEX idx_platform (platform),                          -- 平台统计分析
    INDEX idx_loginResult (loginResult)                     -- 成功/失败统计
) COMMENT='用户登录记录表';

 -- 域名表
CREATE TABLE IF NOT EXISTS DomainNames (
    domainNameId BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT COMMENT '域名ID',
    domainName VARCHAR(255) UNIQUE NOT NULL COMMENT '域名',
    userId BIGINT UNSIGNED DEFAULT NULL COMMENT '用户ID(NULL表示公共域名)',
    hasCertificate TINYINT(1) DEFAULT 0 COMMENT '是否有TLS证书',
    hasDKIM TINYINT(1) DEFAULT 0 COMMENT '是否有DKIM密钥',
    isDefault TINYINT(1) UNIQUE DEFAULT 0 COMMENT '是否为默认域名(0: 否, 1: 是)',
    updateTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '最后更新时间',
    -- 添加外键关联
    FOREIGN KEY (userId) REFERENCES Users(userId) ON DELETE SET NULL ON UPDATE CASCADE,
    -- 添加索引
    INDEX idx_domainName (domainName),
    INDEX idx_userId (userId),
    INDEX idx_isDefault (isDefault)
) COMMENT='域名表';

-- 域名密钥与证书表
CREATE TABLE IF NOT EXISTS DomainCredentials (
    domainNameId BIGINT UNSIGNED NOT NULL COMMENT '域名ID',
    privateKey MEDIUMTEXT NOT NULL COMMENT 'TLS 私钥(PEM)',
    publicKey MEDIUMTEXT NOT NULL COMMENT 'TLS 公钥(PEM)',
    `certificate` MEDIUMTEXT NOT NULL COMMENT 'TLS 证书(PEM)',
    caChain MEDIUMTEXT DEFAULT NULL COMMENT '中间证书链(PEM)',
    sigPrivateKey MEDIUMTEXT NOT NULL COMMENT 'DKIM 签名私钥',
    sigPublicKey MEDIUMTEXT NOT NULL COMMENT 'DKIM 签名公钥',
    lastRotated TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '最后密钥轮换时间',
    -- 添加外键关联
    PRIMARY KEY (domainNameId),
    FOREIGN KEY (domainNameId) REFERENCES DomainNames(domainNameId) ON DELETE CASCADE ON UPDATE CASCADE,
    -- 添加索引
    INDEX idx_lastRotated (lastRotated),         -- 按轮换时间查询最新/过期证书
    INDEX idx_domainNameId_lastRotated (domainNameId, lastRotated)  -- 优化批量更新或同步任务
) COMMENT='域名证书与签名密钥表';

 -- 域名授权表
CREATE TABLE IF NOT EXISTS DomainNameAuthorization (
    authorizationId BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT COMMENT '授权ID',
    domainNameId BIGINT UNSIGNED NOT NULL COMMENT '域名ID',
    userId BIGINT UNSIGNED NOT NULL COMMENT '用户ID',
    `permission` ENUM('USE', 'ADMIN') NOT NULL COMMENT '权限(USE: 使用, ADMIN: 管理)',
    createTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '授权时间',
    expireTime TIMESTAMP DEFAULT NULL COMMENT '授权过期时间',
    -- 添加外键关联
    FOREIGN KEY (userId) REFERENCES Users(userId) ON DELETE CASCADE ON UPDATE CASCADE,
    FOREIGN KEY (domainNameId) REFERENCES DomainNames(domainNameId) ON DELETE CASCADE ON UPDATE CASCADE,
    -- 添加索引
    UNIQUE INDEX uniq_domain_user (domainNameId, userId),  -- 防止重复授权
    INDEX idx_userId (userId),                             -- 查询用户授权
    INDEX idx_domainId (domainNameId),                     -- 查询域名授权
    INDEX idx_expireTime (expireTime)                      -- 定期清理过期授权
) COMMENT = '域名授权表';

 -- 域名与证书变更记录表
CREATE TABLE IF NOT EXISTS DomainChangeLog (
    logId BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT COMMENT '日志ID',
    domainNameId BIGINT UNSIGNED NOT NULL COMMENT '域名ID',
    userId BIGINT UNSIGNED DEFAULT NULL COMMENT '操作用户ID(可能为空,例如系统任务)',
    changeType ENUM(
        'CREATE_DOMAIN',       -- 新增域名
        'UPDATE_DOMAIN',       -- 更新域名信息
        'DELETE_DOMAIN',       -- 删除域名
        'ROTATE_CERT',         -- TLS 证书轮换
        'ROTATE_DKIM',         -- DKIM 密钥轮换
        'ADD_AUTH',            -- 添加授权
        'REMOVE_AUTH',         -- 移除授权
        'EXPIRE_AUTH',         -- 授权到期
        'SYSTEM_MAINTENANCE'   -- 系统维护/同步
    ) NOT NULL COMMENT '变更类型',
    `description` TEXT COMMENT '详细描述(可选)',
    changeTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '变更时间',
    ipAddress VARCHAR(64) DEFAULT NULL COMMENT '操作来源IP',
    userAgent VARCHAR(255) DEFAULT NULL COMMENT '操作客户端信息',
    changeResult ENUM('SUCCESS', 'FAIL') NOT NULL DEFAULT 'SUCCESS' COMMENT '变更结果',
    failReason VARCHAR(255) DEFAULT NULL COMMENT '失败原因(如果有)',
    -- 外键约束
    FOREIGN KEY (domainNameId) REFERENCES DomainNames(domainNameId) ON DELETE CASCADE,
    FOREIGN KEY (userId) REFERENCES Users(userId) ON DELETE SET NULL,
    -- 添加索引
    INDEX idx_domainNameId (domainNameId),
    INDEX idx_userId (userId),
    INDEX idx_changeType (changeType),
    INDEX idx_changeTime (changeTime)
) COMMENT='域名与证书变更记录表';

 -- 邮箱表
CREATE TABLE IF NOT EXISTS MailBoxes (
    mailBoxId BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT COMMENT '邮箱ID',
    userId BIGINT UNSIGNED NOT NULL COMMENT '所属用户ID',
    domainNameId BIGINT UNSIGNED NOT NULL COMMENT '域名ID',
    localPart VARCHAR(255) NOT NULL COMMENT '邮箱地址',
    isOpen TINYINT(1) DEFAULT 1 COMMENT '是否启用(0: 禁用, 1: 启用, 2: 删除)', 
    createTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updateTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    UNIQUE KEY uniq_domain_mail (domainNameId, localPart), -- 域名 + 地址 唯一
    -- 添加外键关联
    FOREIGN KEY (userId) REFERENCES Users(userId) ON DELETE CASCADE ON UPDATE CASCADE,
    FOREIGN KEY (domainNameId) REFERENCES DomainNames(domainNameId) ON DELETE CASCADE ON UPDATE CASCADE,
    -- 添加索引
    INDEX idx_userId (userId),
    INDEX idx_domainNameId (domainNameId),
    INDEX idx_isOpen (isOpen),
    INDEX idx_updateTime (updateTime)
) COMMENT='邮箱表';

 -- 邮箱变更表
CREATE TABLE IF NOT EXISTS MailBoxChangeLog (
    logId BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT COMMENT '日志ID',
    mailBoxId BIGINT UNSIGNED NOT NULL COMMENT '邮箱ID',
    userId BIGINT UNSIGNED DEFAULT NULL COMMENT '操作用户ID',
    `action` ENUM('CREATE', 'UPDATE', 'DELETE', 'DISABLE', 'ENABLE', 'TRANSFER') NOT NULL COMMENT '操作类型',
    oldValue JSON DEFAULT NULL COMMENT '变更前的值快照',
    newValue JSON DEFAULT NULL COMMENT '变更后的值快照',
    actionTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '操作时间',
    remark VARCHAR(255) DEFAULT NULL COMMENT '备注信息(可用于记录原因或来源)',
    -- 添加外键关联
    FOREIGN KEY (mailBoxId) REFERENCES MailBoxes(mailBoxId) ON DELETE CASCADE ON UPDATE CASCADE,
    FOREIGN KEY (userId) REFERENCES Users(userId) ON DELETE SET NULL ON UPDATE CASCADE,
    -- 添加索引
    INDEX idx_mailBoxId (mailBoxId),
    INDEX idx_userId (userId),
    INDEX idx_actionTime (actionTime)
) COMMENT='邮箱变更记录表';

 -- 用户个人信息表
CREATE TABLE IF NOT EXISTS UserProfile (
    userId BIGINT UNSIGNED PRIMARY KEY COMMENT '用户ID',
    nickname VARCHAR(64) DEFAULT '未命名用户' COMMENT '用户昵称',
    avatar VARCHAR(512) DEFAULT NULL COMMENT '用户头像URL',
    `address` VARCHAR(255) DEFAULT NULL COMMENT '现居地址', 
    `resume` VARCHAR(512) DEFAULT '这个人很懒，什么都没有留下~' COMMENT '用户简介',
    defaultMailBoxId  BIGINT UNSIGNED DEFAULT NULL COMMENT '默认发件邮箱',
    updateTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '最后更新时间',
    -- 添加外键关联
    FOREIGN KEY (userId) REFERENCES Users(userId) ON DELETE CASCADE ON UPDATE CASCADE,
    FOREIGN KEY (defaultMailBoxId) REFERENCES MailBoxes(mailBoxId) ON DELETE SET NULL ON UPDATE CASCADE,
    -- 添加索引
    INDEX idx_nickname (nickname),
    INDEX idx_updateTime (updateTime)
) COMMENT = '用户个人信息表';
 -- 插入管理员个人信息
INSERT IGNORE INTO UserProfile (userId, nickname, avatar, `address`)
VALUES (1, 'NovaMail', 'defaultAvatar', '中国 湖南省 长沙市');

 -- 邮箱SMTP/POP3授权码表
CREATE TABLE IF NOT EXISTS EmailAuthorization (
    emailAuthorizationId BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT COMMENT '授权ID',
    userId BIGINT UNSIGNED NOT NULL COMMENT '用户ID',
    mailBoxId BIGINT UNSIGNED NOT NULL COMMENT '邮箱ID', -- 一个邮箱可以有多个授权码
    authCode VARCHAR(255) UNIQUE NOT NULL COMMENT '授权码(客户端使用,用于SMTP/POP3登录验证) 存储哈希值',
    createTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '授权创建时间',
    expireTime TIMESTAMP DEFAULT NULL COMMENT '授权过期时间',
    isActivated TINYINT(1) DEFAULT 0 COMMENT '是否激活',
    lastLoginTime TIMESTAMP NULL DEFAULT NULL COMMENT '上次通过该授权成功登录的时间',
    loginCount INT UNSIGNED DEFAULT 0 COMMENT '该授权成功登录次数',
    clientInfo VARCHAR(255) DEFAULT NULL COMMENT '创建授权的客户端信息(如User-Agent或设备标识)',
    updateTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '最后更新时间',
    -- 添加外键关联
    FOREIGN KEY (userId) REFERENCES Users(userId) ON DELETE CASCADE ON UPDATE CASCADE,
    FOREIGN KEY (mailBoxId) REFERENCES MailBoxes(mailBoxId) ON DELETE CASCADE ON UPDATE CASCADE,
    -- 添加索引
    INDEX idx_user_mail_active (userId, mailBoxId, isActivated),
    INDEX idx_userId (userId),
    INDEX idx_expireTime (expireTime),
    INDEX idx_isActivated (isActivated)
) COMMENT='邮箱SMTP/POP3授权表';

 -- 用户偏好设置表
CREATE TABLE IF NOT EXISTS UserSettings (
    userId BIGINT UNSIGNED PRIMARY KEY COMMENT '用户ID',
    settings JSON NOT NULL COMMENT '用户设置JSON对象(如主题、布局、语言、分页数等)',
    `version` SMALLINT UNSIGNED DEFAULT 1 COMMENT '配置版本号',
    createTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updateTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '最后更新时间',
    -- 添加外键关联
    FOREIGN KEY (userId) REFERENCES Users(userId) ON DELETE CASCADE ON UPDATE CASCADE
) COMMENT='用户偏好设置表';

 -- 用户联系人分组
CREATE TABLE IF NOT EXISTS ContactGroups (
    groupId BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT COMMENT '分组ID',
    userId BIGINT UNSIGNED NOT NULL COMMENT '用户ID',
    groupName VARCHAR(255) NOT NULL COMMENT '分组名称',
    groupType ENUM('custom', 'system') DEFAULT 'custom' COMMENT '分组类型',
    sortOrder INT UNSIGNED DEFAULT 0 COMMENT '分组排序序号',
    createTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updateTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '最后更新时间',
    -- 添加外键关联
    FOREIGN KEY (userId) REFERENCES Users(userId) ON DELETE CASCADE ON UPDATE CASCADE,
    UNIQUE INDEX uniq_user_group (userId, groupName), -- 防止重复分组
    -- 添加索引
    INDEX idx_userId (userId)
) COMMENT='用户分组表';

 -- 用户联系人表
CREATE TABLE IF NOT EXISTS Contacts (
    contactId BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT COMMENT '联系人ID',
    userId BIGINT UNSIGNED NOT NULL COMMENT '用户ID',
    groupId BIGINT UNSIGNED DEFAULT NULL COMMENT '所属分组ID',
    contactName VARCHAR(255) NOT NULL COMMENT '联系人姓名',
    email VARCHAR(255) NOT NULL COMMENT '邮箱地址',
    phone VARCHAR(64) DEFAULT NULL COMMENT '电话号码',
    remark VARCHAR(255) DEFAULT NULL COMMENT '备注信息',
    createTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updateTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '最后更新时间',
    -- 添加外键关联
    FOREIGN KEY (userId) REFERENCES Users(userId) ON DELETE CASCADE ON UPDATE CASCADE,
    FOREIGN KEY (groupId) REFERENCES ContactGroups(groupId) ON DELETE SET NULL ON UPDATE CASCADE,
    UNIQUE INDEX uniq_user_email (userId, email), -- 防止重复联系人邮箱
    -- 添加索引
    INDEX idx_userId (userId),
    INDEX idx_email (email)
) COMMENT='用户联系人表';

 -- 用户文件表
CREATE TABLE IF NOT EXISTS UserFiles (
    fileId BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT COMMENT '文件ID',
    userId BIGINT UNSIGNED NOT NULL COMMENT '用户ID',
    `fileName` VARCHAR(255) NOT NULL COMMENT '文件名',
    fileType VARCHAR(50) DEFAULT NULL COMMENT '文件类型(MIME)',
    filePath VARCHAR(255) NOT NULL COMMENT '存储路径(完整逻辑路径)',
    parentPath VARCHAR(255) DEFAULT NULL COMMENT '父路径(用于层级结构)',
    storageBucket VARCHAR(128) DEFAULT 'default' COMMENT '存储桶(物理存储空间)',
    fileHash CHAR(64) NOT NULL COMMENT '文件内容哈希(SHA256)',
    fileSize BIGINT UNSIGNED NOT NULL COMMENT '文件大小(字节)',
    `version` INT UNSIGNED DEFAULT 1 COMMENT '文件版本号',
    fileStatus ENUM('active', 'deleted', 'uploading', 'archived') DEFAULT 'active' COMMENT '文件状态',
    `description` VARCHAR(512) DEFAULT NULL COMMENT '文件描述',
    uploadTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '上传时间',
    updateTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '最后更新时间',
    -- 添加外键关联
    FOREIGN KEY (userId) REFERENCES Users(userId) ON DELETE CASCADE ON UPDATE CASCADE,
    -- 添加索引
    UNIQUE KEY uniq_user_file (userId, filePath, fileName, `version`), -- 防止同路径同版本重复文件
    INDEX idx_user_status_bucket (userId, fileStatus, storageBucket),
    INDEX idx_userId (userId),
    INDEX idx_fileHash (fileHash),
    INDEX idx_uploadTime (uploadTime),
    INDEX idx_status (fileStatus)
) COMMENT='用户文件表';

 -- 用户签名表
CREATE TABLE IF NOT EXISTS UserSignatures (
    signatureId BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT COMMENT '签名ID',
    userId BIGINT UNSIGNED NOT NULL COMMENT '用户ID',
    signatureName VARCHAR(255) NOT NULL COMMENT '签名名称',
    signatureContent TEXT NOT NULL COMMENT '签名内容(只支持HTML格式)',
    isDefault TINYINT(1) DEFAULT 0 COMMENT '是否为默认签名(0: 否, 1: 是)',
    createTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updateTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '最后更新时间',
    -- 添加外键关联
    FOREIGN KEY (userId) REFERENCES Users(userId) ON DELETE CASCADE ON UPDATE CASCADE,
    -- 添加索引
    UNIQUE KEY uniq_user_default_signature (userId, isDefault), -- 每用户只能有一个默认签名
    UNIQUE INDEX uniq_user_signature (userId, signatureName), -- 防止重复签名名称
    INDEX idx_userId (userId),
    INDEX idx_signatureName (signatureName),
    INDEX idx_isDefault (isDefault)
) COMMENT='用户签名表';

 -- 邮件文件夹
CREATE TABLE IF NOT EXISTS MailFolder (
    folderId BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT COMMENT '文件夹ID',
    userId BIGINT UNSIGNED DEFAULT NULL COMMENT '所属用户ID(系统文件夹为NULL)',
    folderName VARCHAR(255) NOT NULL COMMENT '文件夹名称',
    `description` VARCHAR(512) DEFAULT NULL COMMENT '文件夹描述',
    isSystem TINYINT(1) DEFAULT 0 COMMENT '是否系统文件夹(0: 否, 1: 是)',
    createTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updateTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    -- 添加外键关联
    FOREIGN KEY (userId) REFERENCES Users(userId) ON DELETE CASCADE ON UPDATE CASCADE,
    -- 添加索引
    UNIQUE INDEX uniq_user_folder (userId, folderName),
    INDEX idx_folderName (folderName),
    INDEX idx_isSystem (isSystem)
) COMMENT='邮件文件夹表';
 -- 插入系统默认文件夹
INSERT IGNORE INTO MailFolder (folderId, userId, folderName, `description`, isSystem)
VALUES
(1, NULL, 'DRAFT', '草稿邮件', 1),
(2, NULL, 'INBOX', '系统默认收件箱', 1),
(3, NULL, 'SENT', '系统已发送邮件', 1),
(4, NULL, 'TRASH', '被识别为垃圾邮件的邮件', 1),
(5, NULL, 'SPAM', '系统识别的广告邮件', 1),
(6, NULL, 'RECYCLE', '被用户移动到回收站', 1),
(7, NULL, 'DELETED', '被用户标记为已删除', 1);

 -- 邮件标签表
CREATE TABLE IF NOT EXISTS MailLabels (
    labelId BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT COMMENT '标签ID',
    userId BIGINT UNSIGNED DEFAULT NULL COMMENT '所属用户ID(系统标签为NULL)',
    labelName VARCHAR(255) NOT NULL COMMENT '标签名称',
    `description` VARCHAR(512) DEFAULT NULL COMMENT '标签描述',
    color VARCHAR(20) DEFAULT '#999999' COMMENT '标签颜色(HEX)',
    isSystem TINYINT(1) DEFAULT 0 COMMENT '是否系统标签(0: 否, 1: 是)',
    createTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updateTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    -- 添加外键关联
    FOREIGN KEY (userId) REFERENCES Users(userId) ON DELETE CASCADE ON UPDATE CASCADE,
    -- 添加索引
    UNIQUE INDEX uniq_user_label (userId, labelName),
    INDEX idx_labelName (labelName),
    INDEX idx_isSystem (isSystem)
) COMMENT='邮件标签表';
 -- 插入系统默认标签
INSERT IGNORE INTO MailLabels (labelId, userId, labelName, `description`, color, isSystem)
VALUES
(1, NULL, '重要', '用户标记为重要的邮件', '#F44336', 1),
(2, NULL, '星标', '用户星标收藏的邮件', '#FFC107', 1);

 -- 邮件表
CREATE TABLE IF NOT EXISTS Mails (
    mailId BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT COMMENT '邮件ID',
    senderName VARCHAR(255) NOT NULL COMMENT '发件人显示名',
    senderEmail VARCHAR(255) NOT NULL COMMENT '发件人邮箱地址',
    `subject` VARCHAR(255) NOT NULL COMMENT '邮件主题',
    `subtitle` MEDIUMTEXT DEFAULT NULL COMMENT '邮件副标题，用于内容预览(从EML解析的纯文本)',
    `content` MEDIUMTEXT NOT NULL COMMENT '邮件内容(EML格式)',
    sendTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '发送时间',
    `status` ENUM(
        'NORMAL',      -- 正常状态
        'QUEUED',      -- 已进入队列，待发送
        'SENDING',     -- 发送中
        'SENT',        -- 发送成功
        'FAILED',      -- 发送失败
        'RECEIVED',    -- 接收到的邮件
        'DELIVERED'    -- 已投递但未确认收信
    ) DEFAULT 'NORMAL' COMMENT '邮件状态',
    -- 添加索引
    INDEX idx_sender_status_time (senderEmail, `status`, sendTime),
    INDEX idx_senderName (senderName),
    INDEX idx_senderEmail (senderEmail),
    INDEX idx_subject (`subject`),
    FULLTEXT INDEX idx_content_subtitle (`subtitle`),
    FULLTEXT INDEX idx_content_fulltext (`content`),
    INDEX idx_status (`status`),
    INDEX idx_sendTime (sendTime)
) COMMENT='邮件表';

 -- 邮件附件表
CREATE TABLE IF NOT EXISTS MailAttachments (
    attachmentId BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT COMMENT '附件ID',
    mailId BIGINT UNSIGNED NOT NULL COMMENT '所属邮件ID',
    `fileName` VARCHAR(255) NOT NULL COMMENT '附件文件名',
    fileType VARCHAR(100) DEFAULT NULL COMMENT '附件类型(MIME type)',
    fileSize BIGINT UNSIGNED DEFAULT 0 COMMENT '附件大小(字节)',
    isInline TINYINT(1) DEFAULT 0 COMMENT '是否为内嵌附件(0: 否, 1: 是)',
    `content` LONGTEXT DEFAULT NULL COMMENT '如果是内嵌附件, 存储base64编码',
    fileUrl VARCHAR(1024) DEFAULT NULL COMMENT '非内嵌附件存储路径或URL',
    createTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '附件添加时间',
    -- 添加外键关联
    FOREIGN KEY (mailId) REFERENCES Mails(mailId) ON DELETE CASCADE ON UPDATE CASCADE,
    -- 添加索引
    INDEX idx_mailId (mailId),
    INDEX idx_fileName (`fileName`),
    INDEX idx_isInline (isInline)
) COMMENT='邮件附件表';

 -- 邮箱与邮件关联表
CREATE TABLE IF NOT EXISTS MailBoxMails (
    mappingId BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT COMMENT '记录ID',
    mailBoxId BIGINT UNSIGNED NOT NULL COMMENT '邮箱ID',
    mailId BIGINT UNSIGNED NOT NULL COMMENT '邮件ID',
    recipientType ENUM(
        'FROM',   -- 发件人
        'TO',     -- 主送
        'CC',     -- 抄送
        'BCC'     -- 密送
    ) DEFAULT 'TO' COMMENT '邮箱角色类型',
    folderId BIGINT UNSIGNED DEFAULT 1 COMMENT '所属文件夹ID',
    isRead TINYINT(1) DEFAULT 0 COMMENT '是否已读, 0: 未读, 1: 已读',
    isStarred TINYINT(1) DEFAULT 0 COMMENT '是否星标, 0: 否, 1: 是',
    createTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '邮件添加到邮箱的时间',
    -- 添加外键
    FOREIGN KEY (mailBoxId) REFERENCES MailBoxes(mailBoxId) ON DELETE CASCADE ON UPDATE CASCADE,
    FOREIGN KEY (mailId) REFERENCES Mails(mailId) ON DELETE CASCADE ON UPDATE CASCADE,
    FOREIGN KEY (folderId) REFERENCES MailFolder(folderId) ON DELETE SET DEFAULT ON UPDATE CASCADE,
    INDEX idx_mailBox_folder_recipient (mailBoxId, folderId, recipientType),
    -- 添加索引
    INDEX idx_mailBoxId (mailBoxId),
    INDEX idx_mailId (mailId),
    INDEX idx_folderId (folderId),
    INDEX idx_isRead (isRead),
    INDEX idx_isStarred (isStarred),
    INDEX idx_recipientType (recipientType)
) COMMENT='邮箱与邮件关联表，存储收件人、抄送、密送及发件人信息，同时记录邮件在各个文件夹的状态';

-- 邮件与标签关联表（多对多）
CREATE TABLE IF NOT EXISTS MailLabelsMapping (
    mappingId BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT COMMENT '记录ID',
    mailBoxMailId BIGINT UNSIGNED NOT NULL COMMENT '邮箱邮件记录ID(MailBoxMails.id)',
    labelId BIGINT UNSIGNED NOT NULL COMMENT '邮件标签ID',
    createTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '标签添加时间',
    -- 外键
    FOREIGN KEY (mailBoxMailId) REFERENCES MailBoxMails(mappingId) ON DELETE CASCADE ON UPDATE CASCADE,
    FOREIGN KEY (labelId) REFERENCES MailLabels(labelId) ON DELETE CASCADE ON UPDATE CASCADE,
    -- 索引
    UNIQUE KEY uniq_mail_label (mailBoxMailId, labelId),
    INDEX idx_mailBoxMailId (mailBoxMailId),
    INDEX idx_labelId (labelId)
) COMMENT='邮件与标签关联表，支持一封邮件拥有多个标签';

 -- 投递记录表
CREATE TABLE IF NOT EXISTS DeliveryLog (
    logId BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT COMMENT '日志ID',
    mailId BIGINT UNSIGNED NOT NULL COMMENT '邮件ID',
    recipientEmail VARCHAR(255) DEFAULT NULL COMMENT '收件人邮箱',
    smtpServer VARCHAR(255) DEFAULT NULL COMMENT 'SMTP服务器地址',
    attempt INT UNSIGNED DEFAULT 1 COMMENT '投递尝试次数',
    `status` ENUM('SUCCESS', 'FAIL') NOT NULL COMMENT '投递状态',
    deliveryTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '投递时间',
    errorMessage TEXT DEFAULT NULL COMMENT '错误信息(如果有)',
    -- 添加外键关联
    FOREIGN KEY (mailId) REFERENCES Mails(mailId) ON DELETE CASCADE ON UPDATE CASCADE,
    -- 添加索引
    INDEX idx_mailId (mailId),
    INDEX idx_status (`status`),
    INDEX idx_deliveryTime (deliveryTime),
    INDEX idx_mailId_deliveryTime(mailId, deliveryTime)
) COMMENT='邮件投递记录表';

 -- 分类器模型表
CREATE TABLE IF NOT EXISTS ClassifierModels (
    modelId BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT COMMENT '模型ID',
    modelFile VARCHAR(512) NOT NULL COMMENT '模型文件(完整路径)',
    createTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updateTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    `version` INT UNSIGNED DEFAULT 1 COMMENT '模型版本号',
    -- 添加索引
    INDEX idx_version (`version`),
    INDEX idx_createTime (createTime)
) COMMENT='分类模型表';

 -- 待训练数据表
CREATE TABLE IF NOT EXISTS TrainingData (
    dataId BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT COMMENT '数据ID',
    `dataFile` VARCHAR(512) NOT NULL COMMENT '数据文件(完整路径)',
    dataType VARCHAR(50) DEFAULT 'EMAIL' COMMENT '数据类型(邮件/附件等)',
    isUsed TINYINT(1) DEFAULT 0 COMMENT '是否已用于训练',
    createTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    -- 添加索引
    INDEX idx_isUsed (isUsed),
    INDEX idx_createTime (createTime)
) COMMENT='待训练数据表';

 -- 模型训练结果表
CREATE TABLE IF NOT EXISTS ModelTrainingResults (
    resultId BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT COMMENT '结果ID',
    modelId BIGINT UNSIGNED NOT NULL COMMENT '模型ID',
    trainingDataId BIGINT UNSIGNED NOT NULL COMMENT '训练数据ID',
    accuracy DECIMAL(5,2) NOT NULL COMMENT '准确率',
    `precision` DECIMAL(5,2) NOT NULL COMMENT '精确度',
    f1_score DECIMAL(5,2) DEFAULT NULL COMMENT 'F1得分',
    createTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '记录时间',
    -- 添加外键关联
    FOREIGN KEY (modelId) REFERENCES ClassifierModels(modelId) ON DELETE CASCADE ON UPDATE CASCADE,
    FOREIGN KEY (trainingDataId) REFERENCES TrainingData(dataId) ON DELETE CASCADE ON UPDATE CASCADE,
    -- 添加索引
    INDEX idx_modelId (modelId),
    INDEX idx_trainingDataId (trainingDataId)
) COMMENT='模型训练结果表';

 -- 系统设置表
CREATE TABLE IF NOT EXISTS SystemSettings (
    settingId BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT COMMENT '设置ID',
    settingName VARCHAR(255) NOT NULL COMMENT '设置名称',
    settingValue LONGTEXT NOT NULL COMMENT '设置值',
    settingType VARCHAR(255) NOT NULL COMMENT '设置类型',
    createTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updateTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '最后更新时间',
    -- 添加索引
    INDEX idx_settingName (settingName),
    INDEX idx_settingType (settingType)
) COMMENT='系统设置表';
 -- 存储默认头像
INSERT IGNORE INTO SystemSettings(settingId, settingName, settingValue, settingType)
VALUES (1,
    'defaultAvatar',
    'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAADwAAAA8CAYAAAA6/NlyAAAAAXNSR0IArs4c6QAAApJJREFUaEPtmc9PE0EUx787u93WUqUaWmsgxgMHDh4w4UqiiRwI/vhbFQ0HTTTxaiIHDxw4EANpbYlWKKXtdmfM2GwC2GaH7bx13M5eO52+z/vMezOzdbbrA4EZehwLnHHb1nDGBcMatoYzlgG7pDMm9C8ca9gazlgG7JLOmNB/27TkPfR+kWExz/4EctTn+NblcFLMcmpLeqnAsFp2x6LttkMc9ngq2KkAPygyPLw1Hjai/HoS4qBLD50K8LNaTsnem0agNG6aQeTAKyUXy6VRzcY9+x2OvU4YN2yqz8mBt2o55aYkm9pbYsukwBLgueJyjrRtNwLlBCVRbYGTZG3Sd2bOsEzETNWwBH4072LxhlqXPjrn+PLrP+/SXAAv7qntw6/rARjxOZO0aUW1vTrvYinG8uE5xy6xXRlPKsDyhzaqHvIT9PW5wLvmUGe/nDhXasCyY6/f8VD2L6/Z9kDg048h6d57kT414OhHZU1X8iPoVl+Q1+xV1aTAt30H8o50rPiP7ILvQPbon4rjk9SAduAnFQ9z7vhWu38WYu/08hVw5SbD8tz4q+NZKPChpbe2tQEXGPC0Gr/9DDmw0xxdAzerOXgKW/T7ZgBd7we0Aa+VXdQkNcHT6HF8bus5kGgDVr3kJ82HrpcDWoBDAbxUPE0lBX5VDzChNVxrSi3AMpDNu/H1e63Irgze+R5AJnbaRwuwDGKmlnSU9ccLHkqe3tN/Zyjw8Vjf1qTN8MWlJleez4ByzkHNZ6gUHBRjCrAbCrR6Ao0BRzsQGHCQHDdJgKetM8rvW2DK7JowtzVsggXKGKxhyuyaMLc1bIIFyhisYcrsmjC3NWyCBcoYrGHK7JowtzVsggXKGKxhyuyaMPdvK7NxwJf6+eYAAAAASUVORK5CYII=',
    'file:image/png'
);

 -- 系统日志表
CREATE TABLE IF NOT EXISTS SystemLog (
    logId BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT COMMENT '日志ID',
    logLevel ENUM('DEBUG', 'INFO', 'WARN', 'ERROR') NOT NULL COMMENT '日志级别',
    `module` VARCHAR(100) DEFAULT NULL COMMENT '日志来源模块',
    logMessage TEXT NOT NULL COMMENT '日志信息',
    logTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '日志时间',
    -- 添加索引
    INDEX idx_logLevel (logLevel),
    INDEX idx_logTime (logTime),
    INDEX idx_logLevel_logTime (logLevel, logTime)
) COMMENT='系统日志表';
