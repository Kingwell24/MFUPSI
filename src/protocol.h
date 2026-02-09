#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "config.h"
#include "utils.h"
#include "matrix.h"
#include <vector>
#include <map>
#include <set>
#include <memory>

/**
 * MFUPSI协议的三个阶段实现
 * Setup -> Update -> Query
 */

class MFUPSIProtocol {
public:
    using Config_t = Config::ExperimentConfig;
    using MatrixType = Matrix::MatrixType;
    using VectorType = Matrix::VectorType;
    
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

    // 全局掩码矩阵（为所有客户端预生成，不计入计时）
    std::vector<MatrixType> global_masks_;
    
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
     * 查询向量生成
     * 按照协议文档中的Algorithm 9
     */
    std::vector<VectorType> generate_query_vectors(uint64_t element);
    
    /**
     * 服务器处理PIR查询（同态扩展和线性检索）
     * 模拟GSW外部积和矩阵乘法
     */
    VectorType server_process_pir_query(const std::vector<VectorType>& query_vectors);
    
    /**
     * 客户端解密和交集判断
     */
    bool decrypt_and_judge(const VectorType& pir_response, uint64_t element);
};

#endif // PROTOCOL_H
