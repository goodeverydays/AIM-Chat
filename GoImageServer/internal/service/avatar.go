package service

import (
	"context"
	"fmt"

	"GoImageServer/internal/storage"
	"GoImageServer/pkg/pb"
)

// AvatarService 实现 pb.AvatarServiceServer
type AvatarService struct {
	pb.UnimplementedAvatarServiceServer

	store   *storage.FileStore
	version string
	httpURL string // 外部访问的 HTTP 前缀
}

func NewAvatarService(store *storage.FileStore, version, httpURL string) *AvatarService {
	return &AvatarService{
		store:   store,
		version: version,
		httpURL: httpURL,
	}
}

func (s *AvatarService) Upload(ctx context.Context, req *pb.UploadReq) (*pb.UploadRsp, error) {
	if len(req.ImageData) == 0 {
		return &pb.UploadRsp{Code: 1, Msg: "图片数据为空"}, nil
	}

	filename, err := s.store.Save(req.UserId, req.ImageData, req.Format)
	if err != nil {
		return &pb.UploadRsp{Code: 2, Msg: fmt.Sprintf("保存失败: %s", err)}, nil
	}

	url := fmt.Sprintf("%s/avatars/%s", s.httpURL, filename)
	return &pb.UploadRsp{
		Code:     0,
		Msg:      "上传成功",
		Filename: filename,
		Url:      url,
	}, nil
}

func (s *AvatarService) Delete(ctx context.Context, req *pb.DeleteReq) (*pb.CommonRsp, error) {
	s.store.Delete(req.UserId)
	return &pb.CommonRsp{Code: 0, Msg: "已删除"}, nil
}

func (s *AvatarService) GetUrl(ctx context.Context, req *pb.GetUrlReq) (*pb.GetUrlRsp, error) {
	filename := s.store.GetFilename(req.UserId)
	if filename == "" {
		return &pb.GetUrlRsp{Code: 1, Msg: "未找到头像", Filename: "", Url: ""}, nil
	}
	url := fmt.Sprintf("%s/avatars/%s", s.httpURL, filename)
	return &pb.GetUrlRsp{Code: 0, Filename: filename, Url: url}, nil
}

func (s *AvatarService) HealthCheck(ctx context.Context, req *pb.HealthCheckReq) (*pb.HealthCheckRsp, error) {
	return &pb.HealthCheckRsp{Healthy: true, Version: s.version}, nil
}
