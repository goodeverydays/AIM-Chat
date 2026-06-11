package main

import (
	"log"
	"os"
	"os/signal"
	"syscall"

	"GoImageServer/internal/config"
	"GoImageServer/internal/server"
	"GoImageServer/internal/service"
	"GoImageServer/internal/storage"
)

var Version = "0.1.0"

func main() {
	cfg := config.Load()

	// 初始化文件存储
	store, err := storage.NewFileStore(
		cfg.Storage.AvatarDir,
		cfg.Storage.MaxSizeMB,
		cfg.Storage.MaxWidth,
		cfg.Storage.MaxHeight,
	)
	if err != nil {
		log.Fatalf("[FATAL] 存储初始化失败: %v", err)
	}

	// HTTP 基础 URL（供客户端拼接头像URL，使用对外可访问地址）
	httpURL := cfg.Server.PublicHTTPURL()

	// gRPC 服务
	svc := service.NewAvatarService(store, Version, httpURL)
	grpcSrv := server.NewGRPCServer(cfg, svc)

	// HTTP 服务（静态文件 + 上传API）
	httpSrv := server.NewHTTPServer(cfg.Server.HTTPAddress(), store)

	// 优雅退出
	go func() {
		sigCh := make(chan os.Signal, 1)
		signal.Notify(sigCh, syscall.SIGINT, syscall.SIGTERM)
		<-sigCh
		log.Println("[INFO] 收到退出信号")
		grpcSrv.Stop()
	}()

	// HTTP 在 goroutine 中启动
	go func() {
		if err := httpSrv.Start(); err != nil {
			log.Fatalf("[FATAL] HTTP 服务异常: %v", err)
		}
	}()

	log.Printf("[INFO] GoImageServer v%s 启动", Version)
	log.Printf("[INFO] gRPC: %s  HTTP: %s  Avatars: %s",
		cfg.Server.GRPCAddress(), cfg.Server.HTTPAddress(), cfg.Storage.AvatarDir)

	if err := grpcSrv.Start(); err != nil {
		log.Fatalf("[FATAL] gRPC 服务异常: %v", err)
	}
}
