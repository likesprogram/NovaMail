# 目录解析
 - BertMailClass : 邮件分类模型训练脚本
 - Databases : 系统数据库
 - novamail-ui : 系统前端
 - NovaMailService : 系统应用服务
 - NovaMailServiceCenter : 系统服务
    - ServiceCenter ：服务中心
    - SMTPServiceCenter ：邮件服务中心
    - SMTPWorker ：SMTP工作节点


# 服务中心接口设计统一数据格式
 - 数据类型：uint32 lenegth + uint16 cmd + String data（Json）
 ``` Json 
    {
        "service":"name",
        "service-type":"gateway/service",
        "service-ip":"192.168.1.1",
        "port":8080,
        "message":"hello world",
        "time":1234567890,
        "date-type":"list/object/string/int/float/bool/null",
        "data": "{data}"
    }
 ```


# CMD 表
* **100–119**：注册 / 生命周期 + 心跳与健康（核心节点功能）
* **120–139**：发现 / 订阅（服务发现相关）
* **140–159**：配置 / 策略（配置与调度）
* **160–179**：元数据 / 鉴权 / 统计（治理、安全、监控）
* **180–199**：网关专属（单独划一段便于权限/角色区分）
* **200–219**：日志 / 错误 / 事件（可观测性）
* **220–239**：系统控制 / 广播 / 扩展（系统级指令 & 未来扩展）

---

## 100–119：注册与心跳（核心节点功能）

|    新CMD | 名称            | 方向            | 描述                 |
| ------: | ------------- | ------------- | ------------------ |
|     100 | REGISTER      | Node → Center | 注册新的服务节点           |
|     101 | REGISTER_ACK  | Center → Node | 注册确认响应             |
|     102 | UNREGISTER    | Node → Center | 主动下线通知             |
|     103 | SHUTDOWN      | Center → Node | 中心要求节点下线（优雅下线）     |
|     104 | SHUTDOWN_ACK  | Node → Center | 节点确认下线完成           |
|     105 | HEARTBEAT     | Node → Center | 节点心跳包（可含负载、QPS、延迟） |
|     106 | PING          | Center → Node | 主动探测节点状态           |
|     107 | PONG          | Node → Center | 探测响应               |
|     108 | HEALTH_REPORT | Node → Center | （可选）更详细的健康上报（若需要）  |
| 109–119 | **保留**        | —             | 预留给核心心跳/健康扩展       |

## 120–139：服务发现与订阅

|    新CMD | 名称              | 方向            | 描述                          |
| ------: | --------------- | ------------- | --------------------------- |
|     120 | DISCOVER        | Node → Center | 查询指定服务实例                    |
|     121 | DISCOVER_RESULT | Center → Node | 返回服务列表                      |
|     122 | SUBSCRIBE       | Node → Center | 订阅服务变更事件（用于 Gateway/Client） |
|     123 | SERVICE_UPDATE  | Center → Node | 服务上下线或权重更新通知（差分推送）          |
|     124 | SERVICE_CHANGED | Center → Node | 服务状态改变广播（兼容简版/全量）           |
| 125–139 | **保留**          | —             | 预留给发现/订阅扩展（例如按标签/区域订阅）      |

## 140–159：配置与策略同步

|    新CMD | 名称            | 方向            | 描述               |
| ------: | ------------- | ------------- | ---------------- |
|     140 | CONFIG_PUSH   | Center → Node | 推送配置更新           |
|     141 | CONFIG_ACK    | Node → Center | 节点确认配置应用完成       |
|     142 | POLICY_PUSH   | Center → Node | 推送负载均衡、限流、熔断等策略  |
|     143 | POLICY_ACK    | Node → Center | 策略应用确认           |
|     144 | POLICY_QUERY  | Node → Center | 节点主动请求当前策略       |
|     145 | POLICY_RESULT | Center → Node | 策略查询结果           |
| 146–159 | **保留**        | —             | 预留策略细化（灰度/生效范围等） |

## 160–179：元数据 / 鉴权 / 统计

|    新CMD | 名称              | 方向            | 描述                           |
| ------: | --------------- | ------------- | ---------------------------- |
|     160 | AUTH            | Node → Center | 注册鉴权请求（token / 签名 / 时间戳）     |
|     161 | AUTH_ACK        | Center → Node | 鉴权结果（success/expire等）        |
|     162 | METADATA_UPDATE | Node ↔ Center | 更新节点元信息（zone、version、labels） |
|     163 | STATS_QUERY     | Center → Node | 主动拉取节点统计信息（按需）               |
|     164 | STATS_RESULT    | Node → Center | 返回统计结果                       |
| 165–179 | **保留**          | —             | 监控/治理扩展（审计、配额查询等）            |

## 180–199：网关（Gateway）专属命令

单独划段，便于在中心区别普通 service 与 gateway 行为/权限

| 新CMD | 名称 | 方向 | 描述 |
|------:|------|------|------|
| 180 | GATEWAY_REGISTER | Gateway → Center | 注册网关实例 |
| 181 | GATEWAY_ACK | Center → Gateway | 网关注册确认 |
| 182 | GATEWAY_DISCOVER | Gateway → Center | 请求服务节点列表（可批量） |
| 183 | GATEWAY_DISCOVER_RESULT | Center → Gateway | 返回服务节点及权重 |
| 184 | GATEWAY_SUBSCRIBE | Gateway → Center | 网关订阅服务变更事件 |
| 185 | GATEWAY_SERVICE_UPDATE | Center → Gateway | 推送节点上下线或权重变更（给网关） |
| 186 | GATEWAY_HEALTH_REPORT | Gateway → Center | 网关上报自身指标（QPS/latency/err） |
| 187 | GATEWAY_POLICY_PUSH | Center → Gateway | 下发路由/负载均衡/限流等策略 |
| 188 | GATEWAY_POLICY_ACK | Gateway → Center | 网关策略确认 |
| 189–199 | **保留** | — | 网关进一步功能（路由表同步等） |

## 200–219：日志 / 错误 / 事件

|    新CMD | 名称        | 方向 | 描述                |
| ------: | --------- | -- | ----------------- |
|     200 | LOG       | 双向 | 调试日志与状态上报（轻量）     |
|     201 | ERROR     | 双向 | 错误响应或异常状态上报       |
|     202 | EVENT_LOG | 双向 | 结构化系统事件日志（供审计/告警） |
| 203–219 | **保留**    | —  | 日志/告警相关扩展（告警订阅等）  |

## 220–239：系统控制 / 广播 / 扩展

|    新CMD | 名称            | 方向            | 描述                   |
| ------: | ------------- | ------------- | -------------------- |
|     220 | BROADCAST     | Center → All  | 全局广播（刷新、升级通知、紧急通知）   |
|     221 | BROADCAST_ACK | Node → Center | 确认广播已接收              |
| 222–239 | **保留**        | —             | 系统级扩展（跨Region、集群同步等） |

---