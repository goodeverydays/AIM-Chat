package store

import (
	"crypto/rand"
	"fmt"
	"math/big"
	"sync"
	"time"
)

// codeEntry 单个邮箱的验证码记录
type codeEntry struct {
	code     string
	expireAt time.Time
	lastSent time.Time
}

// CodeStore 验证码内存存储，按邮箱地址索引
type CodeStore struct {
	mu     sync.Mutex
	codes  map[string]codeEntry
	ttl    time.Duration
	cool   time.Duration
	digits int
}

// NewCodeStore 创建验证码存储
func NewCodeStore(ttl, cooldown time.Duration, digits int) *CodeStore {
	return &CodeStore{
		codes:  make(map[string]codeEntry),
		ttl:    ttl,
		cool:   cooldown,
		digits: digits,
	}
}

// Generate 为邮箱生成新验证码
// 若距上次发送时间未超过冷却时间，返回 ok=false 及剩余冷却秒数
func (s *CodeStore) Generate(email string) (code string, ok bool, cooldownRemaining int) {
	s.mu.Lock()
	defer s.mu.Unlock()

	now := time.Now()
	if entry, exists := s.codes[email]; exists {
		elapsed := now.Sub(entry.lastSent)
		if elapsed < s.cool {
			remaining := s.cool - elapsed
			return "", false, int(remaining.Seconds()) + 1
		}
	}

	code = randomDigits(s.digits)
	s.codes[email] = codeEntry{
		code:     code,
		expireAt: now.Add(s.ttl),
		lastSent: now,
	}
	return code, true, 0
}

// Verify 校验邮箱+验证码，成功后立即删除（一次性）
func (s *CodeStore) Verify(email, code string) bool {
	s.mu.Lock()
	defer s.mu.Unlock()

	entry, exists := s.codes[email]
	if !exists {
		return false
	}
	if time.Now().After(entry.expireAt) {
		delete(s.codes, email)
		return false
	}
	if entry.code != code {
		return false
	}
	delete(s.codes, email)
	return true
}

// randomDigits 生成指定位数的纯数字验证码
func randomDigits(digits int) string {
	max := big.NewInt(1)
	for i := 0; i < digits; i++ {
		max.Mul(max, big.NewInt(10))
	}
	n, err := rand.Int(rand.Reader, max)
	if err != nil {
		n = big.NewInt(0)
	}
	return fmt.Sprintf("%0*d", digits, n.Int64())
}
