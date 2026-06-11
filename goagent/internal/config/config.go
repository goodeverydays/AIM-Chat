package config

import (
	"fmt"
	"os"
	"strconv"
)

// Config 服务全局配置
type Config struct {
	Server ServerConfig
	LLM    LLMConfig
	Redis  RedisConfig
	Log    LogConfig
}

// ServerConfig gRPC服务器配置
type ServerConfig struct {
	Host     string // 监听地址
	Port     int    // gRPC监听端口
	HTTPPort int    // HTTP REST监听端口
}

// LLMConfig 大模型配置
type LLMConfig struct {
	Provider string // 当前使用的provider: "empty" / "openai" / "anthropic"

	// 具体provider的配置（后续按需扩展）
	OpenAI    OpenAIConfig
	Anthropic AnthropicConfig
}

type OpenAIConfig struct {
	APIKey  string
	BaseURL string
	Model   string
}

type AnthropicConfig struct {
	APIKey string
	Model  string
}

// RedisConfig Redis 配置
type RedisConfig struct {
	Enabled  bool   // 是否启用 Redis
	Addr     string // 地址，如 "192.168.100.128:6379"
	Password string // 密码，无密码留空
	DB       int    // 数据库编号
}

// LogConfig 日志配置
type LogConfig struct {
	Level  string // debug / info / warn / error
	Format string // json / text
}

// DefaultConfig 返回默认配置
func DefaultConfig() *Config {
	return &Config{
		Server: ServerConfig{
			Host:     "0.0.0.0",
			Port:     19527,
			HTTPPort: 19528,
		},
		LLM: LLMConfig{
			Provider: "empty",
		},
		Redis: RedisConfig{
			Enabled:  true,
			Addr:     "192.168.100.128:6379",
			Password: "",
			DB:       0,
		},
		Log: LogConfig{
			Level:  "info",
			Format: "text",
		},
	}
}

// Load 从环境变量加载配置（后续可扩展为配置文件加载）
func Load() *Config {
	cfg := DefaultConfig()

	// 从环境变量覆盖
	if v := os.Getenv("AGENT_HOST"); v != "" {
		cfg.Server.Host = v
	}
	if v := os.Getenv("AGENT_PORT"); v != "" {
		if p, err := strconv.Atoi(v); err == nil {
			cfg.Server.Port = p
		}
	}
	if v := os.Getenv("AGENT_HTTP_PORT"); v != "" {
		if p, err := strconv.Atoi(v); err == nil {
			cfg.Server.HTTPPort = p
		}
	}
	if v := os.Getenv("LLM_PROVIDER"); v != "" {
		cfg.LLM.Provider = v
	}
	if v := os.Getenv("OPENAI_API_KEY"); v != "" {
		cfg.LLM.OpenAI.APIKey = v
	}
	if v := os.Getenv("OPENAI_BASE_URL"); v != "" {
		cfg.LLM.OpenAI.BaseURL = v
	}
	if v := os.Getenv("OPENAI_MODEL"); v != "" {
		cfg.LLM.OpenAI.Model = v
	}
	if v := os.Getenv("ANTHROPIC_API_KEY"); v != "" {
		cfg.LLM.Anthropic.APIKey = v
	}
	if v := os.Getenv("ANTHROPIC_MODEL"); v != "" {
		cfg.LLM.Anthropic.Model = v
	}
	if v := os.Getenv("LOG_LEVEL"); v != "" {
		cfg.Log.Level = v
	}
	if v := os.Getenv("LOG_FORMAT"); v != "" {
		cfg.Log.Format = v
	}

	// Redis 环境变量
	if v := os.Getenv("REDIS_ENABLED"); v != "" {
		cfg.Redis.Enabled = (v == "true" || v == "1")
	}
	if v := os.Getenv("REDIS_ADDR"); v != "" {
		cfg.Redis.Addr = v
	}
	if v := os.Getenv("REDIS_PASSWORD"); v != "" {
		cfg.Redis.Password = v
	}
	if v := os.Getenv("REDIS_DB"); v != "" {
		if db, err := strconv.Atoi(v); err == nil {
			cfg.Redis.DB = db
		}
	}

	return cfg
}

// Address 返回gRPC监听地址
func (s ServerConfig) Address() string {
	return fmt.Sprintf("%s:%d", s.Host, s.Port)
}

// HTTPAddress 返回HTTP监听地址
func (s ServerConfig) HTTPAddress() string {
	return fmt.Sprintf("%s:%d", s.Host, s.HTTPPort)
}
