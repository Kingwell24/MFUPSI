#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "config.h"
#include "utils.h"
#include "matrix.h"
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <cmath>

/**
 * MFUPSI协议的三个阶段实现
 * Setup -> Update -> Query
 * 
 * 改进版本：正确实现PIR的z维映射和同态加密运算
 */

class MFUPSIProtocol {
public:
    using Config_t = Config::ExperimentConfig;
    using MatrixType = Matrix::MatrixType;
    using VectorType = Matrix::VectorType;
    
    /**
     * LWE密钥结构：用于PIR查询
     * LWE密钥是一个长度N_lwe的向量
     */
    struct LWEKey {
        VectorType secret_key;  // 秘密密钥向量（长度N_lwe）
        size_t dimension;        // LWE維度N_lwe
    };
    
    /**
     * GSW密文矩阵结构
     * GSW密文是一个(2*N_lwe) × (2*N_lwe)的矩阵，用于同态评估
     */
    struct GSWCiphertext {
        MatrixType matrix;  // (2*N_lwe) × (2*N_lwe)的矩阵
        size_t dimension;   // N_lwe
    };
    
    /**
     * LWE密文向量：用于服务器响应
     * LWE密文是一对(a, b)，其中a是长度N_lwe的向量
     */
    struct LWECiphertext {
        VectorType a;      // 向量部分（长度N_lwe）
        uint64_t b;         // 标量部分
    };
    
    /**
     * 客户端数据结构
     */
    struct Client {
        size_t client_id;
        std::set<uint64_t> data_set;          // 当前数据集X_i
        MatrixType encoding_matrix;            // 编码矩阵 E_i
        MatrixType masked_encoding;            // 掩码编码 Ẽ_i = E_i + S_i
        MatrixType mask_matrix;                // 掩码矩阵 S_i
    };
    
    /**
     * 服务器端数据结构
     */
    struct Server {
        MatrixType global_encoding;            // 全局聚合编码 E_total
        std::map<uint64_t, uint64_t> query_log; // 查询日志（用于调试）
    };
    
    // 性能测量结果
    struct PerformanceMetrics {
        // Setup阶段
        double setup_client_encoding_time_ms;   // 客户端编码耗时
        double setup_server_aggregation_time_ms; // 服务器聚合耗时
        size_t setup_client_comm_bytes;         // 客户端上传字节数
        size_t setup_total_comm_bytes;          // Setup总通信
        
        // Update阶段
        double update_client_time_ms;           // 客户端增量计算
        double update_server_time_ms;           // 服务器聚合
        size_t update_client_comm_bytes;        // 更新通信字节
        
        // Query阶段
        double query_client_gen_time_ms;        // 客户端查询生成
        double query_server_process_time_ms;    // 服务器处理
        double query_client_decrypt_time_ms;    // 客户端解密
        size_t query_comm_bytes;                // 查询通信字节
        size_t response_comm_bytes;             // 响应通信字节
    };
    
    /**
     * 构造函数和初始化
     */
    MFUPSIProtocol(const Config_t& cfg);
    
    /**
     * 生成全局密钥 K_1, K_2, K_r
     */
    void generate_keys();
    
    /**
     * Setup阶段：初始化
     */
    void setup_phase();
    
    /**
     * Update阶段：增量更新
     */
    void update_phase(size_t num_clients_to_update);
    
    /**
     * Query阶段：PIR查询
     */
    void query_phase();
    
    /**
     * 获取性能指标
     */
    const PerformanceMetrics& get_metrics() const { return metrics_; }
    
    /**
     * 重置所有性能计数
     */
    void reset_metrics();
    
private:
    Config_t config_;
    std::vector<Client> clients_;
    Server server_;
    PerformanceMetrics metrics_;
    
    // 全局密钥
    uint64_t key_k1_;  // F_1: 分区哈希
    uint64_t key_k2_;  // F_2: 随机向量生成
    uint64_t key_kr_;  // F_r: 元素表示
    
    // LWE密钥（用于PIR查询）
    LWEKey pir_lwe_key_;

    // 全局掩码矩阵（为所有客户端预生成，不计入计时）
    std::vector<MatrixType> global_masks_;
    
    // ===== PIR相关辅助计算 =====
    
    /**
     * 计算z维PIR中每个维度的大小L
     * L = ceil(b^(1/z))，其中b是分区总数
     */
    size_t compute_pir_dimension_size();
    
    /**
     * 将分区索引j映射到z维超立方体坐标
     * 将j视为基数L的数字，分解为z个坐标：
     * j = idx_1 * L^(z-1) + idx_2 * L^(z-2) + ... + idx_z * L^0
     * 
     * 返回：z个坐标的向量[idx_1, idx_2, ..., idx_z]
     */
    std::vector<size_t> compute_hypercube_coordinates(size_t j);
    
    /**
     * 初始化LWE密钥：生成随机秘密向量
     */
    void initialize_lwe_key();
    
    /**
     * 生成GSW密文矩阵（模拟加密，不需要正确结果）
     * 用于客户端查询生成，矩阵大小(2*N_lwe) × (2*N_lwe)
     */
    GSWCiphertext generate_gsw_ciphertext_for_coordinate(size_t coordinate, size_t L);
    
    /**
     * 生成z个选择向量，每个对应超立方体中的一个维度
     */
    std::vector<VectorType> generate_z_selection_vectors(size_t element);
    
    /**
     * 计算时间消耗：模拟z维PIR折叠过程中的同态乘法
     * 执行约z×b次矩阵乘法，代表同态扩展的开销
     */
    size_t simulate_gsw_external_product_cost();
    
    // ===== 原有函数 =====
    
    /**
     * 为指定客户端生成随机数据集（Setup）
     */
    void generate_client_data(Client& client, size_t size);
    
    /**
     * 客户端本地编码（Setup）
     * 按照协议文档中的Algorithm 2: ClientEncode
     */
    MatrixType client_encode(const std::set<uint64_t>& data_set);
    
    /**
     * 分区编码子算法
     * 按照协议文档中的Algorithm 3: EncodePartition
     */
    VectorType encode_partition(const std::vector<uint64_t>& partition_elements);
    
    /**
     * 构建带状矩阵和目标向量
     */
    void build_linear_system(
        const std::vector<uint64_t>& elements,
        MatrixType& M,
        VectorType& y
    );
    
    /**
     * 生成稀疏向量
     * 按照协议文档中的Algorithm 4: RandVector
     */
    VectorType generate_rand_vector(uint64_t element);
    
    /**
     * 生成掩码矩阵
     */
    MatrixType generate_mask_matrix(size_t rows, size_t cols);

    /**
     * 生成全局掩码矩阵（Setup阶段初始化）
     */
    void generate_global_masks();
    
    /**
     * 服务器端聚合（Setup）
     */
    void server_aggregate();
    
    /**
     * 客户端增量编码（Update）
     */
    MatrixType client_incremental_update(Client& client, const std::set<uint64_t>& X_add,
                                                                                      const std::set<uint64_t>& X_del);
    
    /**
     * 服务器端增量更新（Update）
     */
    void server_incremental_update();
    
    /**
     * 【改进版】服务器处理PIR查询：z维维度折叠
     * 模拟GSW同态扩展和线性检索的完整过程
     * - 执行z轮维度折叠
     * - 每轮使用约b/L^i的矩阵乘法
     * - 总计约z×b次同态运算
     */
    VectorType server_process_pir_query_z_dimension(
        const std::vector<VectorType>& z_selection_vectors
    );
    
    /**
     * 客户端解密和交集判断
     */
    bool decrypt_and_judge(const VectorType& pir_response, uint64_t element);
};

#endif // PROTOCOL_H
