package llm

import (
	"context"
	"fmt"
	"strings"

	"goagent/internal/cache"
)

// EmptyProvider 是一个LLM Provider的空实现
// 当LLM尚未配置时使用，返回友好的占位回复并记录对话历史
type EmptyProvider struct {
	cache cache.Cache
}

// NewEmptyProvider 创建空Provider
func NewEmptyProvider(c cache.Cache) *EmptyProvider {
	return &EmptyProvider{cache: c}
}

func (p *EmptyProvider) Name() string {
	return "empty"
}

func (p *EmptyProvider) Chat(ctx context.Context, req *ChatRequest) (*ChatResponse, error) {
	// 获取对话历史（多轮上下文）
	history, _ := p.cache.GetHistory(ctx, int64(req.UserID), 10)

	// 保存用户消息
	_ = p.cache.AppendHistory(ctx, int64(req.UserID),
		cache.Message{Role: "user", Content: req.Content, Time: 0}, 100)

	// 构建带有历史摘要的回复
	var sb strings.Builder
	sb.WriteString("[Agent未配置]\n")
	sb.WriteString(fmt.Sprintf("收到消息: \"%s\"\n", req.Content))

	if len(history) > 1 {
		sb.WriteString(fmt.Sprintf("\n对话历史 (%d条):\n", len(history)))
		for _, m := range history {
			role := "🧑"
			if m.Role == "assistant" {
				role = "🤖"
			}
			sb.WriteString(fmt.Sprintf("  %s %s\n", role, truncate(m.Content, 60)))
		}
	}

	sb.WriteString("\n当前LLM Provider未配置，请在配置中设置 llm.provider 以启用AI回复。\n")
	sb.WriteString("支持的Provider: openai, anthropic")

	reply := sb.String()

	// 保存 assistant 回复
	_ = p.cache.AppendHistory(ctx, int64(req.UserID),
		cache.Message{Role: "assistant", Content: reply, Time: 0}, 100)

	return &ChatResponse{Reply: reply, Model: "none"}, nil
}

func (p *EmptyProvider) ListModels(ctx context.Context) ([]ModelInfo, error) {
	return []ModelInfo{}, nil
}

func (p *EmptyProvider) IsHealthy(ctx context.Context) bool {
	if p.cache != nil {
		return p.cache.Ping(ctx) == nil
	}
	return false
}

func truncate(s string, maxLen int) string {
	runes := []rune(s)
	if len(runes) <= maxLen {
		return s
	}
	return string(runes[:maxLen]) + "..."
}
