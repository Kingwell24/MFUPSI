#ifndef UTILS_H
#define UTILS_H

#include <cstdint>
#include <vector>
#include <cstring>
#include <algorithm>
#include <chrono>

/**
 * 工具函数：哈希、伪随机函数、计时、有限域运算
 */

class Utils {
public:
    /**
     * PRF/密钥哈希函数：模拟F_1(K_1, x)
     * 将元素x映射到分区索引
     */
    static uint64_t hash_partition(const uint64_t& k1, const uint64_t& element) {
        const uint64_t PRIME = 11400714819323198485ULL;
        uint64_t h = k1 ^ element;
        h ^= h >> 33;
        h *= PRIME;
        h ^= h >> 33;
        return h;
    }
    
    /**
     * 生成稀疏向量
     * 模拟RandVector(K_2, x, d_1, w)
     */
    static std::vector<uint8_t> sparse_vector(
        const uint64_t& k2,
        const uint64_t& element,
        size_t dimension,
        size_t band_width
    ) {
        std::vector<uint8_t> v(dimension, 0);
        
        uint64_t h_pos = hash_partition(k2, element);
        size_t pos = h_pos % (dimension - band_width + 1);
        
        for (size_t i = 0; i < band_width; i++) {
            uint64_t h = hash_partition(k2 ^ (element + i), element);
            v[pos + i] = (h % 2);
        }
        
        return v;
    }
    
    /**
     * 生成伪随机值，模拟PRF输出
     */
    static uint64_t prf_value(const uint64_t& key, const uint64_t& input) {
        uint64_t h = key ^ input;
        h ^= h >> 33;
        h *= 11400714819323198485ULL;
        h ^= h >> 33;
        return h;
    }
    
    /**
     * 矩阵转为字节大小（用于计算通信开销）
     */
    static size_t matrix_size_bytes(size_t rows, size_t cols) {
        return rows * cols * 8;
    }
    
    /**
     * 计时类：用于精确测量各阶段耗时
     */
    class Timer {
    private:
        std::chrono::high_resolution_clock::time_point start_time;
        std::chrono::high_resolution_clock::time_point end_time;
        
    public:
        Timer() = default;
        
        void start() {
            start_time = std::chrono::high_resolution_clock::now();
        }
        
        void stop() {
            end_time = std::chrono::high_resolution_clock::now();
        }
        
        double elapsed_ms() const {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                end_time - start_time
            );
            return duration.count();
        }
        
        double elapsed_us() const {
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                end_time - start_time
            );
            return duration.count();
        }
    };
    
    using Matrix = std::vector<std::vector<uint64_t>>;
    using Vector = std::vector<uint64_t>;
    
    /**
     * 在有限域Z_q中进行加法
     */
    static uint64_t add_mod(uint64_t a, uint64_t b, uint64_t q) {
        __uint128_t sum = (__uint128_t)a + (__uint128_t)b;
        return static_cast<uint64_t>(sum % q);
    }
    
    /**
     * 在有限域Z_q中进行减法
     */
    static uint64_t sub_mod(uint64_t a, uint64_t b, uint64_t q) {
        __uint128_t result = (__uint128_t)a + ((__uint128_t)q - (__uint128_t)b);
        return static_cast<uint64_t>(result % q);
    }
    
    /**
     * 在有限域Z_q中进行乘法
     */
    static uint64_t mul_mod(uint64_t a, uint64_t b, uint64_t q) {
        __uint128_t product = (__uint128_t)a * (__uint128_t)b;
        return static_cast<uint64_t>(product % q);
    }
    
    /**
     * 快速幂运算：计算 base^exp mod q
     * 使用二进制分解法（binary exponentiation），复杂度O(log exp)
     * 这在计算模逆元 a^(q-2) mod q 时至关重要，因为 q-2 可能很大
     */
    static uint64_t fast_pow(uint64_t base, uint64_t exp, uint64_t q) {
        if (q == 1) return 0;
        
        __uint128_t result = 1;
        __uint128_t b = base % q;
        __uint128_t mod = q;
        
        while (exp > 0) {
            if (exp & 1) {
                result = (result * b) % mod;
            }
            b = (b * b) % mod;
            exp >>= 1;
        }
        
        return static_cast<uint64_t>(result);
    }
    
    /**
     * 计算 a 在有限域 Z_q 中的乘法逆元
     * 使用Fermat小定理：当 q 是素数时，a^(-1) ≡ a^(q-2) mod q
     * 
     * 前置条件：q 必须是素数，a != 0
     * 
     * 这是高斯消元在有限域上进行的核心操作，直接影响效率
     */
    static uint64_t mod_inverse(uint64_t a, uint64_t q) {
        if (a == 0) {
            return 0;  
        }
        
        return fast_pow(a, q - 2, q);
    }
};

#endif // UTILS_H
