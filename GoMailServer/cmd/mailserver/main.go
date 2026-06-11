package main

import (
	"log"
	"os"
	"os/signal"
	"syscall"

	"GoMailServer/internal/config"
	"GoMailServer/internal/mailer"
	"GoMailServer/internal/server"
	"GoMailServer/internal/service"
	"GoMailServer/internal/store"
)

var Version = "0.1.0"

func main() {
	cfg := config.Load()

	codeStore := store.NewCodeStore(cfg.Code.TTL, cfg.Code.ResendCooldown, cfg.Code.Length)
	m := mailer.New(cfg.SMTP)
	svc := service.NewMailService(codeStore, m, cfg.Code, Version)
	grpcSrv := server.NewGRPCServer(cfg, svc)

	go func() {
		sigCh := make(chan os.Signal, 1)
		signal.Notify(sigCh, syscall.SIGINT, syscall.SIGTERM)
		<-sigCh
		log.Println("[INFO] 收到退出信号")
		grpcSrv.Stop()
	}()

	log.Printf("[INFO] GoMailServer v%s 启动", Version)
	log.Printf("[INFO] gRPC: %s", cfg.Server.Address())
	if cfg.SMTP.Host == "" {
		log.Println("[WARN] SMTP 未配置 (SMTP_HOST 为空)，发送验证码将失败")
	}

	if err := grpcSrv.Start(); err != nil {
		log.Fatalf("[FATAL] gRPC 服务异常: %v", err)
	}
}
