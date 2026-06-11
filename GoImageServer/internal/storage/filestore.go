package storage

import (
	"fmt"
	"image"
	_ "image/gif"  // 注册 GIF 解码器
	_ "image/jpeg" // 注册 JPEG 解码器
	"image/png"
	"os"
	"path/filepath"
	"strings"
	"time"

	"golang.org/x/image/draw"
	_ "golang.org/x/image/webp" // 注册 WebP 解码器
)

// FileStore 头像文件存储
type FileStore struct {
	dir      string
	maxSize  int64
	maxW, maxH int
}

func NewFileStore(dir string, maxSizeMB, maxW, maxH int) (*FileStore, error) {
	if err := os.MkdirAll(dir, 0755); err != nil {
		return nil, fmt.Errorf("create avatar dir: %w", err)
	}
	return &FileStore{
		dir:      dir,
		maxSize:  int64(maxSizeMB) * 1024 * 1024,
		maxW:     maxW,
		maxH:     maxH,
	}, nil
}

// Save 保存图片（缩放后覆盖），返回最终文件名
func (s *FileStore) Save(userID int32, data []byte, format string) (string, error) {
	if int64(len(data)) > s.maxSize {
		return "", fmt.Errorf("文件过大，最大 %dMB", s.maxSize/(1024*1024))
	}

	img, _, err := image.Decode(strings.NewReader(string(data)))
	if err != nil {
		return "", fmt.Errorf("图片解码失败: %w", err)
	}

	// 删除旧头像
	s.Delete(userID)

	// 缩放到指定尺寸
	resized := resizeToFit(img, s.maxW, s.maxH)

	// 存储为 PNG
	filename := fmt.Sprintf("%d_%d.png", userID, time.Now().Unix())
	path := filepath.Join(s.dir, filename)

	f, err := os.Create(path)
	if err != nil {
		return "", fmt.Errorf("创建文件失败: %w", err)
	}
	defer f.Close()

	if err := png.Encode(f, resized); err != nil {
		os.Remove(path)
		return "", fmt.Errorf("编码PNG失败: %w", err)
	}

	return filename, nil
}

// Delete 删除用户的所有头像文件
func (s *FileStore) Delete(userID int32) {
	prefix := fmt.Sprintf("%d_", userID)
	entries, _ := os.ReadDir(s.dir)
	for _, e := range entries {
		if strings.HasPrefix(e.Name(), prefix) {
			os.Remove(filepath.Join(s.dir, e.Name()))
		}
	}
}

// GetPath 获取头像文件完整路径
func (s *FileStore) GetPath(userID int32) string {
	prefix := fmt.Sprintf("%d_", userID)
	entries, _ := os.ReadDir(s.dir)
	for _, e := range entries {
		if strings.HasPrefix(e.Name(), prefix) {
			return filepath.Join(s.dir, e.Name())
		}
	}
	return ""
}

// GetFilename 获取头像文件名（不含路径）
func (s *FileStore) GetFilename(userID int32) string {
	prefix := fmt.Sprintf("%d_", userID)
	entries, _ := os.ReadDir(s.dir)
	for _, e := range entries {
		if strings.HasPrefix(e.Name(), prefix) {
			return e.Name()
		}
	}
	return ""
}

// resizeToFit 缩放图片到指定尺寸（保持比例，填充透明背景）
func resizeToFit(src image.Image, maxW, maxH int) image.Image {
	bounds := src.Bounds()
	srcW, srcH := bounds.Dx(), bounds.Dy()
	if srcW <= maxW && srcH <= maxH {
		return src // 不需要缩放
	}

	// 计算缩放比例（适应，不裁剪）
	scale := float64(maxW) / float64(srcW)
	if hScale := float64(maxH) / float64(srcH); hScale < scale {
		scale = hScale
	}
	newW := int(float64(srcW) * scale)
	newH := int(float64(srcH) * scale)

	dst := image.NewRGBA(image.Rect(0, 0, newW, newH))
	draw.ApproxBiLinear.Scale(dst, dst.Bounds(), src, src.Bounds(), draw.Over, nil)
	return dst
}
