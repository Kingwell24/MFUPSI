#include "protocol.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <ctime>

/**
 * 输出性能指标到控制台和文件
 */
void print_metrics(const MFUPSIProtocol::PerformanceMetrics& metrics,
                   const Config::ExperimentConfig& config,
                   std::ofstream& output_file) {
    std::cout << "\n========== 性能指标汇总 ==========" << std::endl;
    std::cout << std::fixed << std::setprecision(4);
    
    // Setup阶段
    std::cout << "\n【Setup阶段】" << std::endl;
    std::cout << "  客户端编码耗时: " << metrics.setup_client_encoding_time_ms << " ms" << std::endl;
    std::cout << "  服务器聚合耗时: " << metrics.setup_server_aggregation_time_ms << " ms" << std::endl;
    double setup_total_time = metrics.setup_client_encoding_time_ms + metrics.setup_server_aggregation_time_ms;
    std::cout << "  Setup总耗时: " << setup_total_time << " ms" << std::endl;
    std::cout << "  客户端上传通信: " << metrics.setup_client_comm_bytes / (1024.0 * 1024.0) << " MB" << std::endl;
    
    // Update阶段
    std::cout << "\n【Update阶段】" << std::endl;
    std::cout << "  客户端更新耗时: " << metrics.update_client_time_ms << " ms" << std::endl;
    std::cout << "  服务器更新耗时: " << metrics.update_server_time_ms << " ms" << std::endl;
    double update_total_time = metrics.update_client_time_ms + metrics.update_server_time_ms;
    std::cout << "  Update总耗时: " << update_total_time << " ms" << std::endl;
    std::cout << "  更新通信: " << metrics.update_client_comm_bytes / (1024.0 * 1024.0) << " MB" << std::endl;
    
    // Query阶段
    std::cout << "\n【Query阶段】" << std::endl;
    std::cout << "  客户端查询生成耗时: " << metrics.query_client_gen_time_ms << " ms" << std::endl;
    std::cout << "  服务器处理耗时: " << metrics.query_server_process_time_ms << " ms" << std::endl;
    std::cout << "  客户端解密耗时: " << metrics.query_client_decrypt_time_ms << " ms" << std::endl;
    double query_total_time = metrics.query_client_gen_time_ms + 
                              metrics.query_server_process_time_ms + 
                              metrics.query_client_decrypt_time_ms;
    std::cout << "  Query总耗时: " << query_total_time << " ms" << std::endl;
    std::cout << "  查询通信: " << metrics.query_comm_bytes / 1024.0 << " KB" << std::endl;
    std::cout << "  响应通信: " << metrics.response_comm_bytes / 1024.0 << " KB" << std::endl;
    
    // 参数信息
    std::cout << "\n【参数配置】" << std::endl;
    std::cout << "  客户端数: " << config.num_clients << std::endl;
    std::cout << "  数据集大小: " << config.dataset_size << std::endl;
    std::cout << "  分区容量: " << config.partition_size << std::endl;
    std::cout << "  分区总数: " << config.num_partitions << std::endl;
    std::cout << "  扩展因子: " << config.expansion_factor << std::endl;
    std::cout << "  带宽(w): " << config.band_width << std::endl;
    std::cout << "  PIR维度: " << config.pir_dimension << std::endl;
    std::cout << "  LWE维度: " << config.lwe_dimension << std::endl;
    
    // 输出到文件
    if (output_file.is_open()) {
        output_file << "n,dataset_size,d_1,b,epsilon,w,z,N_lwe,q," 
                   << "setup_client_time_ms,setup_server_time_ms,setup_comm_MB,"
                   << "update_client_time_ms,update_server_time_ms,update_comm_MB,"
                   << "query_client_gen_ms,query_server_ms,query_decrypt_ms,query_comm_KB" << std::endl;
        
        output_file << config.num_clients << ","
                   << config.dataset_size << ","
                   << config.partition_size << ","
                   << config.num_partitions << ","
                   << config.expansion_factor << ","
                   << config.band_width << ","
                   << config.pir_dimension << ","
                   << config.lwe_dimension << ","
                   << config.modulus << ","
                   << metrics.setup_client_encoding_time_ms << ","
                   << metrics.setup_server_aggregation_time_ms << ","
                   << (metrics.setup_client_comm_bytes / (1024.0 * 1024.0)) << ","
                   << metrics.update_client_time_ms << ","
                   << metrics.update_server_time_ms << ","
                   << (metrics.update_client_comm_bytes / (1024.0 * 1024.0)) << ","
                   << metrics.query_client_gen_time_ms << ","
                   << metrics.query_server_process_time_ms << ","
                   << metrics.query_client_decrypt_time_ms << ","
                   << (metrics.query_comm_bytes / 1024.0) << std::endl;
        
        output_file.flush();
    }
}

/**
 * 主实验驱动程序
 */
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  MFUPSI协议性能评估实验" << std::endl;
    std::cout << "  版本: 1.0 (严谨科研实现)" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 打开结果文件
    std::time_t now = std::time(nullptr);
    char filename[256];
    std::strftime(filename, sizeof(filename), "results_%Y%m%d_%H%M%S.csv", std::localtime(&now));
    
    std::ofstream results_file(filename);
    if (!results_file.is_open()) {
        std::cerr << "错误：无法打开结果文件" << std::endl;
        return 1;
    }
    
    // 定义多个实验配置（用于测试不同的参数组合）
    std::vector<Config::ExperimentConfig> configs;
    
    // 小规模测试配置
    {
        auto cfg = Config::get_test_config();
        configs.push_back(cfg);
    }
    
    // 默认配置
    {
        auto cfg = Config::get_default_config();
        configs.push_back(cfg);
    }
    
    // 可以添加更多配置进行不同规模的测试
    // {
    //     auto cfg = Config::get_performance_config();
    //     configs.push_back(cfg);
    // }
    
    // 执行每个配置的实验
    for (size_t config_idx = 0; config_idx < configs.size(); config_idx++) {
        auto& config = configs[config_idx];
        
        std::cout << "\n\n" << std::string(50, '=') << std::endl;
        std::cout << "实验配置 " << (config_idx + 1) << "/" << configs.size() << std::endl;
        std::cout << "  客户端数: " << config.num_clients << std::endl;
        std::cout << "  数据集大小: " << config.dataset_size << std::endl;
        std::cout << "  分区容量: " << config.partition_size << std::endl;
        std::cout << "  更新数据量: " << config.num_updates << std::endl;
        std::cout << "  查询数量: " << config.num_queries << std::endl;
        std::cout << std::string(50, '=') << std::endl;
        
        // 初始化协议
        MFUPSIProtocol protocol(config);
        
        try {
            // 阶段一：Setup
            std::cout << "\n【阶段一】执行Setup..." << std::endl;
            protocol.setup_phase();
            
            // 阶段二：Update（可选，取决于要求）
            std::cout << "\n【阶段二】执行Update..." << std::endl;
            size_t num_update_clients = std::min(
                (size_t)3,  // 更新3个客户端
                config.num_clients
            );
            protocol.update_phase(num_update_clients);
            
            // 阶段三：Query
            std::cout << "\n【阶段三】执行Query..." << std::endl;
            protocol.query_phase();
            
            // 输出性能指标
            print_metrics(protocol.get_metrics(), config, results_file);
            
        } catch (const std::exception& e) {
            std::cerr << "错误：" << e.what() << std::endl;
            return 1;
        }
        
        std::cout << std::endl;
    }
    
    results_file.close();
    
    std::cout << "\n\n" << std::string(50, '=') << std::endl;
    std::cout << "所有实验完成" << std::endl;
    std::cout << "结果已保存至: " << filename << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    
    return 0;
}
