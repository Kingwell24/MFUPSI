#ifndef MATRIX_H
#define MATRIX_H

#include "utils.h"
#include <vector>
#include <cstring>
#include <algorithm>
#include <random>

/**
 * 矩阵操作及高斯消元算法（有限域Z_q上）
 * 这是Setup阶段最耗时的部分
 * 
 * 关键改进：
 * 1) 实现完整的有限域高斯消元（包含主元归一化）
 * 2) 使用正确的前向消元逻辑
 * 3) 回代求解中正确使用模逆
 * 4) 所有运算在Z_q上进行
 */

class Matrix {
public:
    using MatrixType = std::vector<std::vector<uint64_t>>;
    using VectorType = std::vector<uint64_t>;
    
    /**
     * 有限域高斯消元：求解线性系统 M * e = y (mod q)
     * 
     * 输入参数：
     *  - M: m × d_1 的系数矩阵（带状矩阵）
     *  - y: m 维目标向量
     *  - q: 有限域模数（必须是素数）
     * 
     * 输出：
     *  - e: d_1 维解向量，使得 M * e ≡ y (mod q)
     *       （当系统有解时）
     * 
     * 算法原理：标准高斯消元的两个阶段
     * ===============================================
     * 
     * 【前向消元阶段】
     * 目标：将增广矩阵 [M | y] 化为上三角矩阵（行阶梯形式）
     * 核心步骤（对每一列）：
     *   1) 从当前行开始向下扫描，找到该列的首个非零元素（主元）
     *   2) 将主元行交换到当前行
     *   3) 主元归一化：主元行所有元素乘以主元的模逆，使主元变为1
     *      关键公式：e'[j] = e[j] * (pivot^(-1) mod q)
     *   4) 消元：对主元行下方的所有行，用该行的该列值作为消元因子
     *      对每一行i > pivot_row，计算消元因子 k = a[i][col]
     *      更新：a[i][j] = (a[i][j] - k * a[pivot_row][j]) mod q
     *      这样保证 a[i][col] 消为 0
     *
     * 时间复杂度：O(min(m,d1) * d1^2) ≈ O(w * d1^2)
     * 其中 w 是带宽（对带状矩阵可进一步优化）
     * 
     * 【回代求解阶段】
     * 利用上三角矩阵的性质，从后向前逐行求解
     * 对于上三角矩阵中的第i行：
     *   e[leading_col] = (rhs - sum of known terms) / a[i][leading_col]
     * 其中 "除以" 等价于 "乘以模逆"
     */
    static VectorType gaussian_elimination(
        const MatrixType& M,
        const VectorType& y,
        uint64_t q
    ) {
        size_t m = M.size();  // 方程个数
        if (m == 0) {
            return VectorType();
        }
        
        size_t d1 = M[0].size();  // 变量个数（编码维度）
        
        // ============ 第一步：构造增广矩阵 [M | y] ============
        MatrixType augmented(m, VectorType(d1 + 1));
        for (size_t i = 0; i < m; i++) {
            for (size_t j = 0; j < d1; j++) {
                augmented[i][j] = M[i][j];
            }
            augmented[i][d1] = y[i];  // 增广列（右侧）
        }
        
        // ============ 第二步：前向消元（Forward Elimination） ============
        // 目标：化为上三角矩阵（主对角线下方全为0，主元为1）
        
        size_t pivot_row = 0;  // 当前处理的行
        
        for (size_t col = 0; col < d1 && pivot_row < m; col++) {
            // --- 2.1 寻找主元 ---
            // 在 [pivot_row, m) 范围内找该列的首个非零元素
            int best_row = -1;
            for (size_t row = pivot_row; row < m; row++) {
                if (augmented[row][col] != 0) {
                    best_row = row;
                    break;
                }
            }
            
            if (best_row == -1) {
                // 该列下方全为0，这一列是自由变量，跳过
                continue;
            }
            
            // --- 2.2 行交换 ---
            // 将主元行交换到 pivot_row
            std::swap(augmented[pivot_row], augmented[best_row]);
            
            // --- 2.3 主元归一化（关键步骤） ---
            // 将 augmented[pivot_row][col] 归一化为1
            // 方法：主元行所有元素乘以主元的模逆
            uint64_t pivot = augmented[pivot_row][col];
            uint64_t pivot_inv = Utils::mod_inverse(pivot, q);
            
            for (size_t j = col; j <= d1; j++) {
                augmented[pivot_row][j] = Utils::mul_mod(augmented[pivot_row][j], pivot_inv, q);
            }
            // 现在 augmented[pivot_row][col] = 1，即主元已归一化
            
            // --- 2.4 消元（标准高斯消元） ---
            // 将该列下方的所有元素消为0
            // 对于第i行 (i > pivot_row)：
            //   如果 augmented[i][col] = k，则执行 Row[i] -= k * Row[pivot_row]
            //   这样 augmented[i][col] - k*1 = 0
            
            for (size_t i = pivot_row + 1; i < m; i++) {
                uint64_t factor = augmented[i][col];  // 消元因子
                
                if (factor == 0) continue;  // 已经是0，无需消元
                
                // 对该行的所有元素执行操作：augmented[i][j] -= factor * augmented[pivot_row][j]
                for (size_t j = col; j <= d1; j++) {
                    uint64_t term = Utils::mul_mod(factor, augmented[pivot_row][j], q);
                    augmented[i][j] = Utils::sub_mod(augmented[i][j], term, q);
                }
                // 现在 augmented[i][col] = 0
            }
            
            pivot_row++;  // 移动到下一行
        }
        
        // ============ 第三步：回代求解（Back Substitution） ============
        // 利用上三角矩阵从下往上逐行求解
        
        VectorType result(d1, 0);
        
        // 从最后一个有主元的行开始向上回代
        for (int i = std::min((int)pivot_row - 1, (int)d1 - 1); i >= 0; i--) {
            // --- 3.1 找到该行的主元所在列 ---
            int leading_col = -1;
            for (size_t j = 0; j < d1; j++) {
                if (augmented[i][j] != 0) {
                    leading_col = j;
                    break;
                }
            }
            
            if (leading_col == -1) {
                // 该行全为0，方程形如 0 = augmented[i][d1]
                // 如果 augmented[i][d1] != 0 则无解
                // 否则该方程自动满足，不约束任何变量
                continue;
            }
            
            // --- 3.2 计算右侧常数项 ---
            // 原方程为：augmented[i][leading_col] * x[leading_col] + sum(...) = augmented[i][d1]
            // 其中 sum(...) 是已经求出的变量的贡献
            uint64_t sum = augmented[i][d1];  // 初始为右侧常数
            
            for (size_t j = leading_col + 1; j < d1; j++) {
                // 减去已求解变量的贡献
                // augmented[i][j] * result[j]
                uint64_t term = Utils::mul_mod(augmented[i][j], result[j], q);
                sum = Utils::sub_mod(sum, term, q);
            }
            
            // 现在 sum = augmented[i][leading_col] * result[leading_col]
            
            // --- 3.3 求解该变量 ---
            // 由于前向消元已将主元归一化为1，
            // 若leading_col在主对角线位置（i == leading_col），则主元为1
            // 但一般情况下可能不是，所以需要除以主元（乘以模逆）
            uint64_t pivot = augmented[i][leading_col];
            
            if (pivot != 0) {
                uint64_t pivot_inv = Utils::mod_inverse(pivot, q);
                result[leading_col] = Utils::mul_mod(sum, pivot_inv, q);
            }
        }
        
        return result;
    }
    
    /**
     * 矩阵加法：C = A + B (mod q)
     * 逐元素相加
     */
    static MatrixType matrix_add(
        const MatrixType& A,
        const MatrixType& B,
        uint64_t q
    ) {
        size_t rows = A.size();
        size_t cols = A[0].size();
        
        MatrixType C(rows, VectorType(cols));
        
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < cols; j++) {
                C[i][j] = Utils::add_mod(A[i][j], B[i][j], q);
            }
        }
        
        return C;
    }
    
    /**
     * 矩阵减法：C = A - B (mod q)
     */
    static MatrixType matrix_sub(
        const MatrixType& A,
        const MatrixType& B,
        uint64_t q
    ) {
        size_t rows = A.size();
        size_t cols = A[0].size();
        
        MatrixType C(rows, VectorType(cols));
        
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < cols; j++) {
                C[i][j] = Utils::sub_mod(A[i][j], B[i][j], q);
            }
        }
        
        return C;
    }
    
    /**
     * 矩阵乘法：C = A * B (mod q)
     * 用于Query阶段PIR服务器处理（同态扩展）
     * 复杂度：O(n * m * k)
     */
    static MatrixType matrix_multiply(
        const MatrixType& A,
        const MatrixType& B,
        uint64_t q
    ) {
        size_t n = A.size();
        size_t m = A[0].size();
        size_t k = B[0].size();
        
        MatrixType C(n, VectorType(k, 0));
        
        for (size_t i = 0; i < n; i++) {
            for (size_t j = 0; j < k; j++) {
                uint64_t sum = 0;
                for (size_t l = 0; l < m; l++) {
                    uint64_t prod = Utils::mul_mod(A[i][l], B[l][j], q);
                    sum = Utils::add_mod(sum, prod, q);
                }
                C[i][j] = sum;
            }
        }
        
        return C;
    }
    
    /**
     * 向量-矩阵乘法：v^T * M = v' (行向量乘矩阵)
     * 用于Query阶段
     */
    static VectorType vector_matrix_multiply(
        const VectorType& v,
        const MatrixType& M,
        uint64_t q
    ) {
        size_t m = M.size();
        size_t n = M[0].size();
        
        if (v.size() != m) {
            return VectorType();
        }
        
        VectorType result(n, 0);
        
        for (size_t j = 0; j < n; j++) {
            uint64_t sum = 0;
            for (size_t i = 0; i < m; i++) {
                uint64_t prod = Utils::mul_mod(v[i], M[i][j], q);
                sum = Utils::add_mod(sum, prod, q);
            }
            result[j] = sum;
        }
        
        return result;
    }
    
    /**
     * 创建随机矩阵
     */
    static MatrixType random_matrix(size_t rows, size_t cols, uint64_t q) {
        MatrixType M(rows, VectorType(cols));
        
        std::mt19937_64 rng(std::random_device{}());
        std::uniform_int_distribution<uint64_t> dist(0, q - 1);
        
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < cols; j++) {
                M[i][j] = dist(rng) % q;
            }
        }
        
        return M;
    }
    
    /**
     * 创建零矩阵
     */
    static MatrixType zero_matrix(size_t rows, size_t cols) {
        return MatrixType(rows, VectorType(cols, 0));
    }
    
    /**
     * 矩阵转置
     */
    static MatrixType transpose(const MatrixType& M) {
        size_t rows = M.size();
        size_t cols = M[0].size();
        
        MatrixType MT(cols, VectorType(rows));
        
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < cols; j++) {
                MT[j][i] = M[i][j];
            }
        }
        
        return MT;
    }
};

#endif // MATRIX_H
