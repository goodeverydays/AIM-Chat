package service

import (
	"context"
	"fmt"

	"goagent/internal/cache"
	"goagent/internal/llm"
	"goagent/pkg/pb"
)

// AgentService 实现 pb.AgentServiceServer 接口
// 持有 LLM Provider 和 Cache，将gRPC请求转换为Provider调用
type AgentService struct {
	pb.UnimplementedAgentServiceServer

	provider llm.Provider
	cache    cache.Cache
	version  string
}

// NewAgentService 创建AgentService实例
func NewAgentService(provider llm.Provider, version string, c cache.Cache) *AgentService {
	return &AgentService{
		provider: provider,
		cache:    c,
		version:  version,
	}
}

// ProcessMessage 处理用户消息
func (s *AgentService) ProcessMessage(
	ctx context.Context,
	req *pb.ProcessMessageReq,
) (*pb.ProcessMessageRsp, error) {
	// 输入校验
	if req.Content == "" {
		return &pb.ProcessMessageRsp{
			Code: 1,
			Msg:  "消息内容不能为空",
		}, nil
	}

	// 构建ChatRequest
	chatReq := &llm.ChatRequest{
		UserID:   req.UserId,
		TargetID: req.TargetId,
		Content:  req.Content,
		ChatType: req.ChatType,
		Model:    req.Model,
	}

	// 调用LLM Provider
	chatResp, err := s.provider.Chat(ctx, chatReq)
	if err != nil {
		return &pb.ProcessMessageRsp{
			Code: 2,
			Msg:  fmt.Sprintf("LLM调用失败: %s", err.Error()),
		}, nil
	}

	return &pb.ProcessMessageRsp{
		Code:  0,
		Msg:   "成功",
		Reply: chatResp.Reply,
		Model: chatResp.Model,
	}, nil
}

// HealthCheck 健康检查
func (s *AgentService) HealthCheck(
	ctx context.Context,
	req *pb.HealthCheckReq,
) (*pb.HealthCheckRsp, error) {
	healthy := true
	llmStatus := "active"

	if !s.provider.IsHealthy(ctx) {
		// 服务自身正常但LLM不可用，整体仍返回healthy=false
		// 因为Agent服务的核心价值在于LLM回复
		healthy = false
		llmStatus = "error"
	}

	return &pb.HealthCheckRsp{
		Healthy:   healthy,
		Version:   s.version,
		LlmStatus: llmStatus,
	}, nil
}

// ListModels 获取模型列表
func (s *AgentService) ListModels(
	ctx context.Context,
	req *pb.ListModelsReq,
) (*pb.ListModelsRsp, error) {
	models, err := s.provider.ListModels(ctx)
	if err != nil {
		return &pb.ListModelsRsp{}, nil
	}

	pbModels := make([]*pb.ListModelsRsp_ModelInfo, 0, len(models))
	for _, m := range models {
		pbModels = append(pbModels, &pb.ListModelsRsp_ModelInfo{
			Name:      m.Name,
			Provider:  m.Provider,
			MaxTokens: m.MaxTokens,
		})
	}

	return &pb.ListModelsRsp{
		Models: pbModels,
	}, nil
}
