package server

import (
	"encoding/json"
	"net/http"

	"goagent/internal/llm"
	"goagent/internal/middleware"
)

// HTTPServer HTTP REST 服务器，为 Qt 客户端提供轻量调用方式
type HTTPServer struct {
	svc     *httpService
	log     middleware.Logger
	address string
}

// httpService 封装 Agent 业务逻辑供 HTTP 使用
type httpService struct {
	provider llm.Provider
	version  string
}

// chatRequest HTTP 请求
type chatRequest struct {
	UserID   int32  `json:"user_id"`
	TargetID int32  `json:"target_id"`
	Content  string `json:"content"`
	ChatType int32  `json:"chat_type"`
}

// chatResponse HTTP 响应
type chatResponse struct {
	Code   int32  `json:"code"`
	Msg    string `json:"msg"`
	Reply  string `json:"reply"`
	Model  string `json:"model"`
}

// healthResponse 健康检查响应
type healthResponse struct {
	Healthy   bool   `json:"healthy"`
	Version   string `json:"version"`
	LlmStatus string `json:"llm_status"`
}

// NewHTTPServer 创建 HTTP 服务器
func NewHTTPServer(address string, provider llm.Provider, version string, log middleware.Logger) *HTTPServer {
	return &HTTPServer{
		svc: &httpService{
			provider: provider,
			version:  version,
		},
		log:     log,
		address: address,
	}
}

// Start 启动 HTTP 服务器（在 goroutine 中运行）
func (s *HTTPServer) Start() error {
	mux := http.NewServeMux()
	mux.HandleFunc("/api/chat", s.handleChat)
	mux.HandleFunc("/api/health", s.handleHealth)

	s.log.Info("HTTP REST 服务器启动", "address", s.address)
	go func() {
		if err := http.ListenAndServe(s.address, mux); err != nil {
			s.log.Error("HTTP 服务器退出", "error", err)
		}
	}()
	return nil
}

// handleChat POST /api/chat
func (s *HTTPServer) handleChat(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	var req chatRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		writeJSON(w, http.StatusBadRequest, chatResponse{Code: 1, Msg: "请求解析失败"})
		return
	}

	if req.Content == "" {
		writeJSON(w, http.StatusBadRequest, chatResponse{Code: 1, Msg: "消息内容不能为空"})
		return
	}

	chatReq := &llm.ChatRequest{
		UserID:   req.UserID,
		TargetID: req.TargetID,
		Content:  req.Content,
		ChatType: req.ChatType,
	}

	resp, err := s.svc.provider.Chat(r.Context(), chatReq)
	if err != nil {
		writeJSON(w, http.StatusInternalServerError, chatResponse{Code: 2, Msg: "LLM调用失败: " + err.Error()})
		return
	}

	s.log.Info("HTTP chat 请求处理完成",
		"user_id", req.UserID,
		"content_len", len(req.Content),
		"reply_len", len(resp.Reply),
	)

	writeJSON(w, http.StatusOK, chatResponse{
		Code:  0,
		Msg:   "成功",
		Reply: resp.Reply,
		Model: resp.Model,
	})
}

// handleHealth GET /api/health
func (s *HTTPServer) handleHealth(w http.ResponseWriter, r *http.Request) {
	healthy := s.svc.provider.IsHealthy(r.Context())
	llmStatus := "active"
	if !healthy {
		llmStatus = "error"
	}

	writeJSON(w, http.StatusOK, healthResponse{
		Healthy:   healthy,
		Version:   s.svc.version,
		LlmStatus: llmStatus,
	})
}

func writeJSON(w http.ResponseWriter, status int, data interface{}) {
	w.Header().Set("Content-Type", "application/json; charset=utf-8")
	w.WriteHeader(status)
	json.NewEncoder(w).Encode(data)
}
