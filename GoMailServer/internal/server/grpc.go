package server

import (
	"fmt"
	"log"
	"net"

	"google.golang.org/grpc"
	"google.golang.org/grpc/reflection"

	"GoMailServer/internal/config"
	"GoMailServer/internal/service"
	"GoMailServer/pkg/pb"
)

// GRPCServer gRPC 服务器
type GRPCServer struct {
	server *grpc.Server
	addr   string
}

func NewGRPCServer(cfg *config.Config, svc *service.MailService) *GRPCServer {
	s := grpc.NewServer()
	pb.RegisterMailServiceServer(s, svc)
	reflection.Register(s)

	return &GRPCServer{
		server: s,
		addr:   cfg.Server.Address(),
	}
}

func (s *GRPCServer) Start() error {
	lis, err := net.Listen("tcp", s.addr)
	if err != nil {
		return fmt.Errorf("gRPC listen %s: %w", s.addr, err)
	}
	log.Printf("[INFO] gRPC 服务器启动 %s", s.addr)
	return s.server.Serve(lis)
}

func (s *GRPCServer) Stop() {
	s.server.GracefulStop()
}
