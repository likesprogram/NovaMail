# NovaMail

NovaMail 是一个完整的邮件系统毕业设计项目，覆盖 Web 前端、Java 应用服务、C++ 邮件协议服务、Windows 桌面客户端以及基于 BERT/ONNX 的邮件智能分类模块。

项目目标是实现一套可以自主管理用户、域名、邮箱、邮件、附件和服务状态的邮件平台，并提供浏览器端和 Windows 桌面端两种使用方式。

## 项目状态

- 本项目为**毕业设计作品**，仓库内容已随最后一次提交整体归档。
- **项目已停止维护**，不再接受功能更新、依赖升级与问题修复。
- 代码保留的是提交时的可用状态，第三方依赖版本（尤其 C++ 侧）已固定，未做长期可维护性处理。
- 仓库包含部分无法随仓库分发的大体积产物和含密钥的配置文件，**只克隆本仓库并不足以直接跑起全部模块**，请先阅读下方「仓库内容与运行依赖对照」和「克隆后完整运行清单」。

## 功能概览

- 用户注册、登录、JWT 鉴权、访问令牌刷新、图形验证码和短信验证码。
- 邮箱管理、默认邮箱、邮箱授权码、域名管理、DNS 校验、DKIM/TLS 配置。
- 邮件收发、草稿箱、收件箱、已发送、星标、已删除、垃圾邮件、广告邮件、已读/未读状态和附件预览/下载。
- 通讯录、联系人分组、用户资料、头像、签名、用户文件空间。
- WebSocket 邮件通知和系统服务监控。
- SMTP、IMAP、服务注册中心 LikeHub、邮件智能分类等独立 C++ 服务。
- Windows WebView2 桌面壳与 Inno Setup 安装包构建。

## 技术栈

| 模块 | 技术 |
| --- | --- |
| 前端 | Vue 3、TypeScript、Vite、Vue Router、Pinia、Element Plus、Axios、TinyMCE/CKEditor |
| 应用服务 | Java 17、Spring Boot 3.3.3、MyBatis Plus、MySQL、Redis、MinIO、JWT、WebSocket、SpringDoc OpenAPI |
| 邮件服务 | C++20、Makefile、g++-11、OpenSSL、libcurl、MySQL Connector/C++ X DevAPI、AWS S3 SDK、ONNX Runtime |
| 桌面客户端 | Visual Studio 2022、C++17、WebView2、Windows Resource、Inno Setup 6 |
| 智能分类 | Python 训练脚本、BERT、ONNX Runtime |

## 目录结构

```text
NovaMail
├── BertMailClass/        邮件分类模型训练与推理脚本
├── Databases/            MySQL 数据库初始化脚本
├── NovaMailDesktop/      Windows WebView2 桌面客户端
├── NovaMailServers/      C++ 邮件基础服务
│   ├── Classification/   邮件智能分类服务
│   ├── Imap/             IMAP 服务
│   ├── LikeHub/          服务注册与调用中心
│   └── Smtp/             SMTP 服务
├── NovaMailService/      Spring Boot 应用服务
└── novamail-ui/          Vue 前端项目
```

## 仓库内容与运行依赖对照

Git 忽略规则中同时排除了**构建产物**和**必需但不宜入库的依赖**，两者性质不同，下面分开说明。

### 已随仓库提供（克隆即得）

| 路径 | 说明 |
| --- | --- |
| 各模块源码 | Vue 前端、Spring Boot 后端、四个 C++ 服务、桌面端 C++ 工程、Python 训练脚本 |
| `Databases/NovaMail.sql` | 28 张表的建表脚本与默认管理员数据 |
| `NovaMailServers/include/` | 933 个头文件，含项目自有头文件与第三方 SDK 头文件（AWS SDK 1.11.210、MySQL X DevAPI 9.6.0、ONNX Runtime API 25、s2n、miniocpp）；其中 `include/LikesProgram/` 是 LikesProgram 旧版本的 49 个头文件 |
| `NovaMailServers/lib/libLikesProgramStatic.a` | LikesProgram 旧版本的链接库。上游已做不向下兼容的重构、无法再获取，为保证 C++ 侧仍有机会编译，这个约 8.9 MB 的静态库随仓库保留 |
| `NovaMailServers/lib/cmake/LikesProgram/` | 上述库的 CMake 导出信息（使用相对路径，未写死本机绝对路径） |
| `NovaMailServers/dkim/` | DKIM 默认密钥对（`default.private` / `default.public`） |
| `NovaMailService/src/main/resources/fonts/custom.ttf` | 图形验证码字体 |
| `NovaMailService/src/main/resources/ipRegion/*.xdb` | IP 归属地库（v4 约 11 MB，v6 约 36 MB） |
| `novamail-ui/package-lock.json` | 已锁定前端依赖版本，可直接 `npm ci` |
| `novamail-ui/public/tinymce/` | TinyMCE 运行时静态资源，由 `index.html` 直接引用 |
| `NovaMailDesktop/VueSDK/`、`tools/`、`installer/` | 桌面端前端 SDK、资源打包脚本与 Inno Setup 安装脚本 |
| `*.example.yml` / `*.example.json` | 配置模板，用于生成被忽略的真实配置文件 |

### 未入库的依赖与配置（必须自行补齐）

| 路径 | 为什么未入库 | 补齐方式 |
| --- | --- | --- |
| `NovaMailService/src/main/resources/application.yml` | 含数据库、Redis、MinIO、JWT、短信服务密钥 | 复制同目录 `application.example.yml` 并填写 |
| `NovaMailServers/{LikeHub,Smtp,Imap,Classification}/Configuration.json` | 含服务认证密钥与对象存储密钥 | 复制同目录 `Configuration.example.json` 并填写 |
| `NovaMailServers/lib/` 中除 LikesProgram 以外的库 | 约 102 MB 第三方预编译库，体积过大，且都能从上游按对应版本获取 | 见下方「C++ 服务依赖库补齐」 |
| `NovaMailServers/Classification/models/` | 单个 `bert.onnx` 就有约 438 MB | 见「邮件分类模型」一节 |
| `NovaMailServers/**/{bin,obj,logs}/` | C++ 构建输出与运行日志 | `make all` 生成 |
| `BertMailClass/{data,models,results}`、`.venv` | 训练数据集与训练产物体积过大 | 仅在需要重新训练时准备 |
| `novamail-ui/node_modules`、`dist` | 前端依赖与构建产物 | `npm ci` / `npm run build` |
| `NovaMailDesktop/NovaMailDesktop/res/vue-dist` 及 `vue-dist-generated.rc`、`vue-dist-resources.h`、`vue-dist-resource-map.h` | 由前端构建产物再加工得到的资源，可完全再生 | `node tools/pack-vue-dist.mjs ../novamail-ui/dist NovaMailDesktop/res/vue-dist` |
| `NovaMailDesktop/NovaMailDesktop/{.vs,x64,packages}` | Visual Studio 本地状态、编译输出、NuGet 还原目录 | Visual Studio 打开解决方案后自动还原 |
| `NovaMailService/target`、`.idea`、`*.iml` | Maven 构建输出与 IDE 配置 | Maven / IDE 生成 |

### C++ 服务依赖库补齐（重点）

`NovaMailServers/lib/` 是 C++ 服务的链接输入目录，Makefile 直接以文件路径引用其中的静态库。仓库中**没有任何构建这些库的规则**，必须自行准备同名文件：

| 库文件 | 来源 |
| --- | --- |
| `libLikesProgramStatic.a` | **已随仓库提供**，克隆即得，不需要自行准备（原因见下节） |
| `libmysqlcppconnx-static.a` | MySQL Connector/C++ 9.6.0 的 X DevAPI 静态库 |
| `libaws-c-*.a`、`libaws-cpp-sdk-core.a`、`libaws-cpp-sdk-s3.a`、`libaws-crt-cpp.a`、`libaws-checksums.a`、`libs2n.a` | AWS SDK for C++ 1.11.210 及其 aws-crt 依赖链 |
| `libonnxruntime.so`、`libonnxruntime.so.1`、`libonnxruntime.so.1.25.0`、`libonnxruntime_providers_shared.so` | ONNX Runtime 1.25.0 Linux x64 |
| `libLikeHubStatic.a` | 由本仓库源码构建：`make install-LikeHub` |

注意：仓库 `NovaMailServers/include/` 中的第三方头文件版本与上表一一对应（可在 `include/aws/core/VersionConfig.h`、`include/mysqlx/version_info.h`、`include/onnxruntime_c_api.h` 中核对）。**必须使用同版本的库文件**，否则会出现 ABI 不匹配的链接错误。另外 `make all` 会先执行 `clean`，而 `clean` 只会删除 `libLikeHubStatic.a`，第三方库需要常驻 `lib/` 目录。

系统侧依赖（以 Debian/Ubuntu 为例）：

```bash
sudo apt install make g++-11 libssl-dev libcurl4-openssl-dev zlib1g-dev
# 可选：生成 compile_commands.json 时需要
sudo apt install bear
```

### 关于 LikesProgram 的版本兼容性（重要）

本项目 C++ 代码依赖 **LikesProgram 旧版本**，而这个依赖**已经无法从上游重新获取**：

- LikesProgram 上游仓库已经**完整重构**，是一次**不向下兼容**的全面更新，当前上游版本无法与本项目一起编译和链接。
- 因此，本项目把实际用到的旧版本依赖**随仓库保留**，克隆即得：
  - 头文件：`NovaMailServers/include/LikesProgram/`（49 个头文件）；
  - 链接库：`NovaMailServers/lib/libLikesProgramStatic.a`（约 8.9 MB，各服务 Makefile 直接按路径链接它）；
  - CMake 导出信息：`NovaMailServers/lib/cmake/LikesProgram/`。
- 仓库内**没有 LikesProgram 的源码**，也没有构建它的规则。该静态库是二进制产物，只对应现有头文件接口与当时的工具链。

请注意两点：

1. **不要用上游新版 LikesProgram 替换这些文件**。接口与 ABI 都已不兼容，替换后必然编译或链接失败；仓库中的旧版本副本就是为此保留的。
2. 由于是二进制依赖，对编译器与运行库版本较敏感，建议使用本文档描述的 `g++-11`、`-std=c++20` 环境构建。

`libLikesProgramStatic.a` 是唯一无法从上游获取的依赖，已随仓库提供；其余第三方库（MySQL Connector/C++、AWS SDK、s2n、ONNX Runtime）仍需按上一节的清单自行准备。

## 克隆后完整运行清单

按顺序执行。只运行 Web 版（前端 + 后端）时，完成第 1 至第 6 步即可登录、管理域名/邮箱/通讯录/文件；**真正的邮件收发依赖 C++ 的 SMTP/IMAP 服务**，仍需完成第 7 步之后的内容。

1. **准备基础环境**：JDK 17、Maven 3.8+、Node.js 20+、MySQL 8（启用 X Plugin，默认 `33060`）、Redis、MinIO（或兼容 S3 的对象存储）。
2. **初始化数据库**：`mysql -u root -p < Databases/NovaMail.sql`。
3. **创建对象存储桶**：在 MinIO 中创建 `mails`、`users`、`system` 三个桶。写入路径多数会自动建桶，但读取路径在桶不存在时会直接抛 `Bucket does not exist`，建议手工创建。
4. **生成后端配置**：复制 `NovaMailService/src/main/resources/application.example.yml` 为同目录 `application.yml`，填写 MySQL、Redis、MinIO、JWT、短信与 DNS 校验相关值。注意 `jwt.strength` 必须大于 0，否则启动时直接报 `BCrypt strength 配置错误`。
5. **启动后端**：`cd NovaMailService && mvn spring-boot:run`，或直接在 IDE 中运行 `com.example.novamailservice.NovaMailServiceApplication`。默认 `http://127.0.0.1:9099`，接口文档 `http://127.0.0.1:9099/swagger-ui/index.html`。
6. **启动前端**：`cd novamail-ui && npm ci && npm run dev`，默认 `http://127.0.0.1:3000`。接口地址写在 `novamail-ui/src/utils/Request.ts`，指向其他后端时需修改该文件。
7. **完成首次启动配置**：登录默认管理员账号，按「首次启动配置」一节依次完成域名、TLS/DKIM、DNS 解析与验证、邮箱注册。
8. **补齐 C++ 依赖并构建**：按「C++ 服务依赖库补齐」准备第三方库并放入 `NovaMailServers/lib/`（LikesProgram 的旧版静态库已随仓库提供，不必自行寻找），然后 `cd NovaMailServers && make all`。
9. **准备 C++ 服务配置**：把四个 `Configuration.example.json` 复制为同目录下的 `Configuration.json` 并填写。服务从**自身工作目录**读取 `./Configuration.json`，因此配置必须放在各自服务目录下，并在对应目录中启动。四个服务的 `hubServer.port`、`hubServer.access`、`hubServer.secret` 必须与 LikeHub 的 `server.port`、`authentication.access`、`authentication.secret` 完全一致，同时与后端 `application.yml` 的 `hub-server` 保持一致。
10. **准备分类模型**：把 `bert.onnx`、`vocab.txt`、`label_map.json` 放到 `NovaMailServers/Classification/models/NovaMailClassification_1_0_2/`，与 `Configuration.json` 中 `models` 段的路径对应。
11. **构建桌面客户端**（可选）：Visual Studio 2022 打开 `NovaMailDesktop/NovaMailDesktop.sln`，先打包前端资源，再按 `Release | x64` 构建，最后用 Inno Setup 生成安装包。

## 环境要求

- JDK 17
- Maven 3.8+
- Node.js 20+ 和 npm
- MySQL 8，C++ 服务需要启用 MySQL X Plugin，默认使用 `33060`
- Redis
- MinIO 或兼容 S3 的对象存储服务
- Linux 构建环境：`make`、`g++-11`、`openssl`、`libcurl`、`zlib` 等
- Windows 桌面端构建环境：Visual Studio 2022、WebView2 Runtime、Inno Setup 6

## 数据库初始化

将数据库脚本导入 MySQL：

```bash
mysql -u root -p < Databases/NovaMail.sql
```

脚本包含默认数据，但不包含默认域名数据。默认管理员账号：

```text
账号：NovaMail
密码：Lby180607230.
```

安全提示：该默认密码与作者开发环境中的 MySQL、MinIO 密码相同，且已随公开仓库历史发布。任何实际部署都必须立即更换数据库、Redis、MinIO、JWT、短信与 LikeHub 的全部密钥，不要沿用示例值。

## 配置说明

**仓库不包含可运行的配置文件**，它们都已被 `.gitignore` 排除（原因是含有密钥），需要由模板生成：

```text
NovaMailService/src/main/resources/application.example.yml
    -> 复制为同目录 application.yml

NovaMailServers/LikeHub/Configuration.example.json
NovaMailServers/Smtp/Configuration.example.json
NovaMailServers/Imap/Configuration.example.json
NovaMailServers/Classification/Configuration.example.json
    -> 分别复制为同目录 Configuration.json
```

后端 `application.yml` 需要重点填写：

- `spring.datasource`：MySQL JDBC 地址、账号和密码。
- `spring.data.redis`：Redis 地址、端口、密码和数据库编号。
- `minio`：对象存储地址、Access Key 和 Secret Key。
- `jwt.strength`：BCrypt 强度，必须大于 0。
- `jwt.secret`：HMAC256 对称密钥字符串，任意足够长的随机值即可（不需要 RSA 密钥对）。
- `hub-server`：Java 应用服务连接 LikeHub 的地址、端口和认证信息。
- `mail.dns-verification`：域名 DNS 校验期望值，如 IMAP A、SMTP A、MX、SPF。
- `ip-region.local-path`：IP 数据库文件路径，建议改成当前机器可访问的绝对路径。

C++ 侧需要重点保持以下配置一致：

- `LikeHub.server.port` 与其他服务的 `hubServer.port` 一致。模板统一使用 `19090`。
- `hubServer.access`、`hubServer.secret` 与 LikeHub 的 `authentication.access`、`authentication.secret` 一致。
- Java 后端的 `hub-server.port` 与实际部署的 LikeHub 端口一致。
- MySQL、MinIO、SMTP/IMAP 域名和端口按部署环境调整。

注意：模板中的值都是占位符；生产环境请更换数据库密码、JWT 密钥、短信密钥、对象存储密钥和服务认证密钥。

## 首次启动配置

数据库初始化脚本不包含默认域名数据，因此系统首次启动完成后，管理员需要先进入系统管理界面完成基础配置，否则无法正常注册邮箱和完成邮件收发。

建议按以下顺序配置：

1. 登录默认管理员账号，进入 `设置 -> 系统配置`。
2. 在域名管理中添加系统域名，填写域名和所属用户 ID。公共域名可将所属用户 ID 留空，个人域名需要绑定对应用户。
3. 为域名完成 TLS 配置，并查看系统生成的 DKIM 解析记录。
4. 到域名 DNS 服务商处添加并等待解析生效：
   - DKIM TXT 记录。
   - 根域 SPF TXT 记录。
   - `imap` A 记录。
   - `smtp` A 记录。
   - 根域 MX 记录。
5. 回到系统配置中执行 DNS 验证，确认 DKIM、SPF、IMAP、SMTP、MX 均验证通过。
6. 在系统配置的客户端下载配置中上传 Windows 桌面端安装包，使侧边栏“下载客户端”能够指向最新安装包。
7. 进入 `设置 -> 邮箱管理`，为管理员手动注册邮箱，并按需设置为默认邮箱和启用状态。

注意：由于初始化时没有域名，管理员账号不会自动拥有可用邮箱，需要在域名添加并验证完成后手动注册管理员邮箱。

系统配置总览示例：

![系统配置总览示例](docs/images/ScreenShot_2026-05-26_164358_052.png)

域名 TLS、DKIM、DNS 验证示例：

![域名 TLS、DKIM、DNS 验证示例](docs/images/first-start-domain-verification.png)

上传 Windows 客户端安装包示例：

![上传 Windows 客户端安装包示例](docs/images/first-start-client-package.png)

邮箱与授权码管理示例：

![邮箱与授权码管理示例](docs/images/ScreenShot_2026-05-26_164424_297.png)

管理员手动注册邮箱示例：

![管理员手动注册邮箱示例](docs/images/first-start-mailbox-registration.png)

## 启动后端服务

进入 Spring Boot 项目：

```bash
cd NovaMailService
```

开发阶段推荐直接在 IDE 中运行：

```text
com.example.novamailservice.NovaMailServiceApplication
```

也可以使用 Maven：

```bash
mvn -Dspring-boot.run.skip=false spring-boot:run
```

默认端口为：

```text
http://127.0.0.1:9099
```

接口文档地址：

```text
http://127.0.0.1:9099/swagger-ui/index.html
```

## 启动前端

进入前端项目：

```bash
cd novamail-ui
npm install
npm run dev
```

默认访问地址：

```text
http://127.0.0.1:3000
```

前端接口地址目前在 `novamail-ui/src/utils/Request.ts` 中配置，默认连接：

```text
HTTP: http://127.0.0.1:9099
WS:   ws://127.0.0.1:9099
```

如后端部署在其他地址，需要同步修改该文件或后续改造成环境变量配置。

构建生产包：

```bash
npm run build
```

## 编译和运行 C++ 邮件服务

`NovaMailServers` 需要在 Linux 环境中编译，不支持直接在 Windows 下编译。**开始前请先按「C++ 服务依赖库补齐」准备 `NovaMailServers/lib/`，否则链接必然失败。** 进入目录：

```bash
cd NovaMailServers
make all
```

生成结果位于各子模块的 `bin/` 目录，部分公共库会安装到 `NovaMailServers/lib/`。

推荐启动顺序（每个服务都要先在其目录下准备好 `Configuration.json`）：

```bash
make run-LikeHub
make run-Smtp
make run-Imap
make run-Classification
```

也可以进入子目录单独构建和运行：

```bash
cd NovaMailServers/Smtp
make all
make run
```

常用默认端口：

| 服务 | 默认端口 | 说明 |
| --- | --- | --- |
| LikeHub | 2525 或部署配置中的端口 | 服务注册与调用中心，模板统一为 19090 |
| SMTP | 25 | 低端口，通常需要 root 权限或修改端口 |
| IMAP | 143 | 低端口，通常需要 root 权限或修改端口 |
| Classification | 无固定公网端口 | 通过 LikeHub 注册调用 |

生成 `compile_commands.json` 需要安装 `bear`：

```bash
make build
```

## Windows 桌面客户端

桌面客户端位于 `NovaMailDesktop`，使用 Visual Studio 2022 打开：

```text
NovaMailDesktop/NovaMailDesktop.sln
```

推荐使用 `Release | x64` 构建。

如果需要把 Vue 前端打进桌面客户端资源，先构建前端：

```bash
cd novamail-ui
npm run build
```

然后打包前端资源到桌面端工程：

```bash
cd ../NovaMailDesktop
node tools/pack-vue-dist.mjs ../novamail-ui/dist NovaMailDesktop/res/vue-dist
```

该命令会重新生成 `res/vue-dist/` 目录以及 `vue-dist-generated.rc`、`vue-dist-resources.h`、`vue-dist-resource-map.h`，这些文件都不入库。完成后再使用 Visual Studio 构建桌面端。

安装包依赖 Inno Setup 6。先构建出 `NovaMailDesktop/x64/Release/NovaMailDesktop.exe`，再执行：

```powershell
cd NovaMailDesktop
.\tools\build-installer.ps1 -Version 1.0.0
```

安装包输出目录：

```text
NovaMailDesktop/dist/installer
```

## 邮件分类模型

训练和推理脚本位于：

```text
BertMailClass/training
├── EmailProcess.py
├── InferMail.py
└── TrainBert.py
```

仓库**不包含训练数据与训练产物**（`data/`、`models/`、`results/`、`.venv/` 均未入库），BERT 模型也没有随仓库分发。需要重新训练时，请自行准备 `.eml` 数据集（可用 `cmd.bat` 批量改扩展名），安装以下 Python 依赖后运行 `TrainBert.py`：

```text
torch、transformers、scikit-learn、numpy、beautifulsoup4、onnxruntime
```

训练得到的产物需要按 `NovaMailServers/Classification/Configuration.json` 中 `models` 段的路径放置：

```text
NovaMailServers/Classification/models/NovaMailClassification_1_0_2/
├── bert.onnx        约 438 MB，未入库
├── vocab.txt
└── label_map.json
```

分类服务通过 ONNX Runtime 加载该模型，运行依赖 `libonnxruntime.so.1`；构建时会从 `NovaMailServers/lib/` 复制到分类服务 `bin/` 目录。

## 常用命令

```bash
# 前端开发
cd novamail-ui
npm run dev

# 前端构建
cd novamail-ui
npm run build

# 后端测试
cd NovaMailService
mvn test

# C++ 服务构建
cd NovaMailServers
make all

# C++ 服务清理
cd NovaMailServers
make clean
```

## 端口速查

| 端口 | 模块 | 用途 |
| --- | --- | --- |
| 3000 | novamail-ui | Vite 开发服务器 |
| 9099 | NovaMailService | Spring Boot HTTP/WebSocket 服务 |
| 25 | Smtp | SMTP 服务 |
| 143 | Imap | IMAP 服务 |
| 2525/19090 | LikeHub | 服务注册中心，按实际配置为准 |
| 3306 | MySQL | Java 后端 JDBC 连接 |
| 33060 | MySQL X Plugin | C++ 服务 MySQL X DevAPI 连接 |
| 9000 | MinIO | 对象存储 API |

## 已知限制与注意事项

- **C++ 服务需自备部分第三方库**：LikesProgram 的旧版头文件与静态库已随仓库提供（上游已不兼容，无法另行获取），但 MySQL Connector/C++、AWS SDK、s2n、ONNX Runtime 仍需按版本自行准备，详见「C++ 服务依赖库补齐」。
- **分类模型未入库**：`bert.onnx` 约 438 MB，需要自行训练生成或另行获取。
- **配置文件未入库**：所有含密钥的配置都需要由 `*.example.*` 模板生成。
- **LikeHub 端口需统一**：仓库中 LikeHub 自身配置写的是 `2525`，而其他服务与后端都指向 `19090`，属于提交时遗留的不一致；模板已统一为 `19090`，使用时务必确认四处配置一致。
- 低端口如 `25`、`143` 在 Linux 上通常需要 `sudo`，也可以在对应 `Configuration.json` 中改成高端口调试。
- 经测试，阿里云服务器默认禁止 `25` 端口外发，部署 SMTP 外发能力时需要申请解封或改用服务商允许的邮件投递端口。
- 经测试，阿里云环境下 `143` 端口无法从普通 Socket 平滑提升为 TLS 连接；需要申请解除。
- `Configuration.json` 和 `application.yml` 中的地址、密钥、密码需要根据本地或服务器环境调整。
- 前端默认写死连接 `127.0.0.1:9099`，联调远程后端前需要修改请求配置。
- C++ 分类服务依赖 `libonnxruntime.so.1`，构建时会从 `NovaMailServers/lib/` 复制到分类服务 `bin/` 目录。
- Windows 桌面端依赖 WebView2 Runtime；打包安装包前请确认 Release 目录中存在 `NovaMailDesktop.exe` 和 `WebView2Loader.dll`。
- 生产部署前请重新生成 JWT 密钥，替换所有示例凭据，并限制 LikeHub 白名单。
