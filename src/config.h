#ifndef CONFIG_H
#define CONFIG_H

#include <cstdint>
#include <vector>
#include <cmath>
#include <numeric>
#include <limits>

/**
 * 全局参数配置
 * 遵循实验指导文档中的推荐参数
 */

class Config {
public:
    // 灵活参数：支持多组配置
    struct ExperimentConfig {
        size_t num_clients;      // n: 客户端数量
        size_t dataset_size;     // N_size: 单个客户端数据集大小
        size_t num_updates;      // N_upd: 更新数据量
        size_t num_queries;      // N_query: 查询数量
        size_t partition_size;   // d_1: 分区容量
        double expansion_factor; // epsilon: 扩展因子
        size_t pir_dimension;    // z: PIR维度
        size_t lwe_dimension;    // N_lwe: LWE维度
        uint64_t modulus;        // q: 有限域模数
        size_t band_width;       // w: 带宽参数
        
        // 派生参数（自动计算）
        size_t num_partitions;   // b: 分区总数 = ceil((1+epsilon)*n_size*n/d_1)
        double pir_fold_size;    // d_pir: PIR每维折叠大小 = b^(1/z)
        
        void compute_derived_params() {
            // b = ceil((1 + epsilon) * dataset_size * num_clients / partition_size)
            num_partitions = static_cast<size_t>(
                std::ceil((1.0 + expansion_factor) * dataset_size * num_clients / partition_size)
            );
            // d_pir = b^(1/z)
            pir_fold_size = std::pow(static_cast<double>(num_partitions), 1.0 / pir_dimension);
        }
    };
    
    // 推荐参数集合（从FUPSI和SparsePIR继承）
    static ExperimentConfig get_default_config() {
        ExperimentConfig cfg;
        cfg.num_clients = 10;
        cfg.dataset_size = (1 << 16);  // 2^16 = 65536
        cfg.num_updates = cfg.dataset_size / 100;  // 1% of dataset
        cfg.num_queries = 100;
        cfg.partition_size = 512;
        cfg.expansion_factor = 0.2;
        cfg.pir_dimension = 2;
        cfg.lwe_dimension = 1024;
        cfg.modulus = std::numeric_limits<uint64_t>::max() - 58;  // 接近2^64的最大值 - 59
        cfg.band_width = 80;  // w: 带状矩阵带宽
        cfg.compute_derived_params();
        return cfg;
    }
    
    // 获取小规模测试配置
    static ExperimentConfig get_test_config() {
        ExperimentConfig cfg;
        cfg.num_clients = 3;
        cfg.dataset_size = 1024;  // 较小规模用于测试
        cfg.num_updates = 50;
        cfg.num_queries = 10;
        cfg.partition_size = 128;
        cfg.expansion_factor = 0.2;
        cfg.pir_dimension = 2;
        cfg.lwe_dimension = 512;
        cfg.modulus = (1ULL << 32) - 5;  // 较小模数以加快测试
        cfg.band_width = 30;
        cfg.compute_derived_params();
        return cfg;
    }
    
    // 获取性能测试配置（大规模）
    static ExperimentConfig get_performance_config() {
        ExperimentConfig cfg;
        cfg.num_clients = 50;
        cfg.dataset_size = (1 << 20);  // 2^20 = 1MB
        cfg.num_updates = static_cast<size_t>(cfg.dataset_size * 0.05);  // 5%
        cfg.num_queries = 1000;
        cfg.partition_size = 1024;
        cfg.expansion_factor = 0.2;
        cfg.pir_dimension = 3;
        cfg.lwe_dimension = 2048;
        cfg.modulus = std::numeric_limits<uint64_t>::max() - 58;
        cfg.band_width = 100;
        cfg.compute_derived_params();
        return cfg;
    }
};

#endif // CONFIG_H
