package main

import (
	"log"
	"os"
	"os/signal"
	"syscall"

	"goagent/internal/cache"
	"goagent/internal/config"
	"goagent/internal/llm"
	"goagent/internal/server"
	"goagent/internal/service"
)

// 版本号，构建时通过 -ldflags 注入
var Version = "0.1.0"

// stdLogger 标准库日志适配
type stdLogger struct{}

func (l *stdLogger) Info(msg string, keysAndValues ...interface{}) {
	log.Printf("[INFO] %s %v", msg, keysAndValues)
}
func (l *stdLogger) Warn(msg string, keysAndValues ...interface{}) {
	log.Printf("[WARN] %s %v", msg, keysAndValues)
}
func (l *stdLogger) Error(msg string, keysAndValues ...interface{}) {
	log.Printf("[ERROR] %s %v", msg, keysAndValues)
}

func main() {
	logger := &stdLogger{}

	// 加载配置
	cfg := config.Load()

	logger.Info("goagent 启动中...",
		"version", Version,
		"address", cfg.Server.Address(),
		"llm_provider", cfg.LLM.Provider,
	)

	// 初始化缓存层 —— Redis 优先，不可用时降级到 Memory
	var c cache.Cache
	if cfg.Redis.Enabled {
		redisCache, err := cache.NewRedisCache(cfg.Redis.Addr, cfg.Redis.Password, cfg.Redis.DB)
		if err != nil {
			logger.Warn("Redis 连接失败，降级到内存缓存", "addr", cfg.Redis.Addr, "error", err)
			c = cache.NewMemoryCache()
		} else {
			logger.Info("Redis 已连接", "addr", cfg.Redis.Addr, "db", cfg.Redis.DB)
			c = redisCache
		}
	} else {
		logger.Info("Redis 未启用，使用内存缓存")
		c = cache.NewMemoryCache()
	}

	logger.Info("缓存层就绪", "backend", c.Name())

	// 创建LLM Provider
	provider, err := createProvider(cfg, c)
	if err != nil {
		logger.Error("创建Provider失败", "error", err)
		os.Exit(1)
	}

	// 创建gRPC服务实现（注入 Cache）
	svc := service.NewAgentService(provider, Version, c)

	// HTTP REST 服务器
	httpSrv := server.NewHTTPServer(cfg.Server.HTTPAddress(), provider, Version, logger)
	if err := httpSrv.Start(); err != nil {
		logger.Error("HTTP服务器启动失败", "error", err)
		os.Exit(1)
	}

	// gRPC 服务器
	srv := server.NewGRPCServer(cfg, logger, svc)

	// 优雅退出
	go func() {
		sigCh := make(chan os.Signal, 1)
		signal.Notify(sigCh, syscall.SIGINT, syscall.SIGTERM)
		sig := <-sigCh
		logger.Info("收到信号，准备退出", "signal", sig.String())
		srv.Stop()
	}()

	logger.Info("goagent 已启动",
		"version", Version,
		"grpc_port", cfg.Server.Port,
		"http_port", cfg.Server.HTTPPort,
		"cache", c.Name())

	if err := srv.Start(); err != nil {
		logger.Error("服务器退出", "error", err)
		os.Exit(1)
	}

	logger.Info("goagent 已退出")
}

func createProvider(cfg *config.Config, c cache.Cache) (llm.Provider, error) {
	switch cfg.LLM.Provider {
	case "empty":
		log.Printf("[INFO] 使用 EmptyProvider（LLM未配置，返回占位回复）")
		return llm.NewEmptyProvider(c), nil

	default:
		log.Printf("[WARN] 未知的LLM Provider: %s，回退到 EmptyProvider", cfg.LLM.Provider)
		return llm.NewEmptyProvider(c), nil
	}
}
