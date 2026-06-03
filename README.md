# IMServer

基于 muduo 网络库的 C++ 即时通讯服务器。

## 项目结构

```
IMServer/
├── base/          # 线程/同步/日志基础库（muduo fork）
├── net/           # 事件驱动 TCP 网络库（muduo fork）
├── IMServer/      # 应用层：IM 业务逻辑
│   ├── IMServer.cpp       # 程序入口
│   ├── IMSer.h/cpp        # 聊天服务器（TcpServer 封装）
│   ├── ClientSession.h/cpp # 客户端会话管理
│   ├── UserManager.h/cpp  # 用户与群组管理
│   ├── MySqlManager.h/cpp # 数据库自动建表/迁移
│   ├── BinaryReader.h/cpp # 二进制协议编解码
│   └── MsgCacheManager.h/cpp # 离线消息缓存
└── stress_test/   # 压测工具（Go 实现，位于 D:\VSCodeProjects\IMStressTest）
```

## 构建

```bash
# Linux (x64 Debug)
cmake --preset linux-debug
cmake --build out/build/linux-debug

# Windows (x64 Debug, 需要 Visual Studio 2022 + MySQL 8.0)
cmake --preset x64-debug
cmake --build out/build/x64-debug
```

## 运行

```bash
# 前台运行
./imchatserver

# 守护进程模式
./imchatserver -d
```

服务器监听 `0.0.0.0:9527`，连接 MySQL `127.0.0.1:3306`（数据库 `myim`，用户 `root`）。

## 压测

### 压测方法

使用 **Go 语言编写的压测工具**

**选择 Go 的原因：**

- goroutine 轻量级（2KB 栈），可轻松创建数万个并发连接
- 原生 TCP 支持，无需第三方库
- 单机即可产生足够大的请求压力
- 跨平台（Windows/Linux/macOS 均可用）

### 压测架构

```
┌──────────────────┐     TCP (binary protocol)     ┌──────────────────┐
│  imstress (Go)   │ ◄──────────────────────────► │  imchatserver    │
│  压测客户端       │    127.0.0.1:9527             │  (C++ muduo)     │
│  N 个 goroutine  │                                │  EventLoop Pool │
└──────────────────┘                                └──────────────────┘
```

### 二进制协议格式

```
完整的发送数据包 (toSendString):
┌──────────────────┬──────────────────┬──────────────────┬──────────────────────┐
│ packagesize(4B)  │ bodySize(4B BE)  │ checksum(2B BE)  │ 消息体               │
│ LE 字节序        │                  │ 固定为 0          │ cmd+seq+data         │
└──────────────────┴──────────────────┴──────────────────┴──────────────────────┘

消息体:
┌──────────────┬──────────────┬──────────────────────────┐
│ cmd(4B BE)   │ seq(4B BE)   │ data(变长字符串,7bit编码) │
└──────────────┴──────────────┴──────────────────────────┘
```

### 压测场景

| 场景 | 说明 | 测试目标 |
|------|------|----------|
| **connect** | 快速建立→关闭 TCP 连接 | 服务器 accept 吞吐量 |
| **login** | 先批量注册,再并发登录 | 登录业务处理能力 |
| **heartbeat** | 长连接上持续收发心跳包 | 心跳处理吞吐量 |
| **chat** | 已登录用户对之间互发消息 | 消息转发延迟和吞吐量 |
| **mixed** | 40%心跳 + 30%登录 + 20%连接 + 10%聊天 | 混合真实负载 |

### 压测工具使用

```bash
# 构建
cd D:\VSCodeProjects\IMStressTest
go build -o imstress.exe .

# 启动模拟服务器（用于验证协议和工具）
./imstress.exe -mode server -port 19527

# 运行全部压测场景（目标: 真实 IM 服务器）
./imstress.exe -host 127.0.0.1 -port 9527 -c 100 -d 10 -s all

# 指定单个场景
./imstress.exe -port 9527 -c 200 -d 30 -s chat

# 输出 Markdown 格式结果
./imstress.exe -port 9527 -c 100 -d 10 -s all -md
```

**命令行参数：**

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `-mode` | `bench` | 运行模式：`bench`(压测) / `server`(模拟服务器) |
| `-host` | `127.0.0.1` | 服务器地址 |
| `-port` | `9527` | 服务器端口 |
| `-c` | `100` | 并发连接数/goroutine 数 |
| `-d` | `10` | 压测持续时间（秒） |
| `-s` | `all` | 场景：`connect`/`login`/`heartbeat`/`chat`/`mixed`/`all` |
| `-md` | `false` | 输出 Markdown 表格 |

### 压测环境

| 项目 | 配置 |
|------|------|
| CPU | Intel Core i7-13620H (16 核) |
| 内存 | 32 GB |
| 操作系统 | Windows 11 (测试工具) + WSL Ubuntu 24.04 (IM 服务器) |
| Go 版本 | 1.26.3 |
| MySQL | 8.0.44 |
| 网络 | 本地回环 (127.0.0.1) |

### 压测结果

#### 模拟服务器 (Go mock server, 单线程 accept 循环)

**100 并发 × 10 秒：**

| 场景 | 持续时间 | 吞吐量(req/s) | 平均延迟(ms) | P50(ms) | P95(ms) | P99(ms) | 成功率 |
|------|----------|---------------|-------------|---------|---------|---------|--------|
| 连接压测 | 10s | 19134 | 5.17 | 5.06 | 8.18 | 10.25 | 2.36% |
| 登录压测 | 10s | 86935 | 0.47 | 0.50 | 1.42 | 1.77 | 99.93% |
| 心跳压测 | 10s | 321 | 0.02 | 0.00 | 0.00 | 0.58 | 100.00% |
| 单聊压测 | 10s | 19129 | 0.06 | 0.00 | 0.53 | 0.82 | 100.00% |
| 混合压测 | 10s | 9818 | 3.33 | 1.70 | 4.87 | 6.84 | 6.70% |

**500 并发 × 10 秒（高并发）：**

| 场景 | 持续时间 | 吞吐量(req/s) | 平均延迟(ms) | P50(ms) | P95(ms) | P99(ms) | 成功率 |
|------|----------|---------------|-------------|---------|---------|---------|--------|
| 登录压测 | 10s | 85565 | 2.42 | 1.58 | 8.43 | 14.48 | 99.66% |
| 心跳压测 | 10s | 669 | 0.01 | 0.00 | 0.00 | 0.50 | 100.00% |

### 结论

1. **登录吞吐量**：单机可达 **85,000+ req/s**，99.6% 成功率，P99 延迟 14.5ms。业务逻辑简单（内存查找+JSON构造），瓶颈在网络 I/O。

2. **消息转发延迟**：P50 < 0.01ms，P99 < 1ms。回环网络下消息转发延迟极低，主要耗时在协议编解码。

3. **心跳处理**：500 个长连接维持心跳，P99 延迟 0.5ms，服务器轻松处理。正常 IM 场景下心跳间隔为 30s~5min，实际负载更低。

4. **连接建立**：单线程 accept 循环在高频短连接场景下存在瓶颈。生产环境使用 muduo EventLoopThreadPool 多线程 accept 可以显著提升。

5. **混合负载**：多场景混合时成功率下降，主要是连接压测部分对服务器 accept 能力造成冲击。实际生产环境建议使用连接池和长连接模式。

6. **C++ 服务器预期**：muduo 基于 epoll + 线程池，多线程 accept 和事件分发，预期性能比 Go 模拟服务器高 3-5 倍。建议在实际 C++ 服务器上重新执行本压测获得准确数据。

### 压测流程总结

```
1. 启动 MySQL 数据库
   ↓
2. 构建 C++ IM 服务器
   cmake --preset linux-debug && cmake --build out/build/linux-debug
   ↓
3. 启动 IM 服务器
   ./imchatserver
   ↓
4. 构建压测工具
   cd D:\VSCodeProjects\IMStressTest && go build -o imstress.exe .
   ↓
5. 预热（可选）
   ./imstress.exe -c 10 -d 5 -s login
   ↓
6. 执行压测
   ./imstress.exe -c 100 -d 30 -s all -md
   ↓
7. 分析结果
   关注：吞吐量(req/s)、P99延迟(ms)、成功率(%)
```
