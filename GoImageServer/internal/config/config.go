package config

import (
	"fmt"
	"os"
	"strconv"
)

type Config struct {
	Server  ServerConfig
	Storage StorageConfig
}

type ServerConfig struct {
	Host       string // 监听地址（绑定用，如 0.0.0.0）
	PublicHost string // 对外可访问地址（拼接头像URL用，如 192.168.100.128）
	GRPCPort   int    // gRPC 端口（供 IMServer 调用）
	HTTPPort   int    // HTTP 端口（静态文件 + 上传 API）
}

type StorageConfig struct {
	AvatarDir string // 头像存储目录
	MaxSizeMB int    // 最大文件大小（MB）
	MaxWidth  int    // 缩放宽（px）
	MaxHeight int    // 缩放高（px）
}

func DefaultConfig() *Config {
	return &Config{
		Server: ServerConfig{
			Host:       "0.0.0.0",
			PublicHost: "192.168.100.128",
			GRPCPort:   19529,
			HTTPPort:   8080,
		},
		Storage: StorageConfig{
			AvatarDir: "uploads/avatars",
			MaxSizeMB: 5,
			MaxWidth:  256,
			MaxHeight: 256,
		},
	}
}

func Load() *Config {
	cfg := DefaultConfig()
	if v := os.Getenv("AVATAR_HOST"); v != "" {
		cfg.Server.Host = v
	}
	if v := os.Getenv("AVATAR_PUBLIC_HOST"); v != "" {
		cfg.Server.PublicHost = v
	}
	if v := os.Getenv("AVATAR_GRPC_PORT"); v != "" {
		if p, err := strconv.Atoi(v); err == nil {
			cfg.Server.GRPCPort = p
		}
	}
	if v := os.Getenv("AVATAR_HTTP_PORT"); v != "" {
		if p, err := strconv.Atoi(v); err == nil {
			cfg.Server.HTTPPort = p
		}
	}
	if v := os.Getenv("AVATAR_MAX_SIZE_MB"); v != "" {
		if n, err := strconv.Atoi(v); err == nil {
			cfg.Storage.MaxSizeMB = n
		}
	}
	return cfg
}

func (s ServerConfig) GRPCAddress() string {
	return fmt.Sprintf("%s:%d", s.Host, s.GRPCPort)
}

func (s ServerConfig) HTTPAddress() string {
	return fmt.Sprintf("%s:%d", s.Host, s.HTTPPort)
}

func (s ServerConfig) PublicHTTPURL() string {
	return fmt.Sprintf("http://%s:%d", s.PublicHost, s.HTTPPort)
}
