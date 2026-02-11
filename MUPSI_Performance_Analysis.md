# MUPSI vs FUPSI 实验结果差异原因分析报告

## 1. 核心结论摘要

经过对您的MUPSI代码实现 (`protocol.cpp`, `config.h`) 与两方FUPSI实验结果 (`两方FUPSI实验结果分析.txt`) 的对比分析，导致通信开销和查询时间开销巨大差异的主要原因并非协议逻辑（Server Aggregation）本身的设计问题，而是**底层PIR及其密码原语的模拟方式存在显著差异**。

具体来说：
1.  **通信差异源头 (1000x差距)**：您当前的代码使用了**朴素的LWE (Naive LWE/GSW)** 矩阵大小来计算通信量 ($\approx 64$ MB/查询)，而FUPSI/OnionPIR 使用了高度优化的 **Ring-LWE** 结构 ($\approx 64$ KB/查询)。
2.  **计算差异源头**：代码中使用了稠密矩阵乘法 ($O(N^3)$ 或 $O(N^2)$) 来模拟同态运算，而实际高效PIR方案利用了 **NTT (数论变换)** 技术实现 $O(N \log N)$ 的多项式运算。

---

## 2. 详细原因分析

### 2.1 查询阶段通信开销 (Communication Overhead)

**对比数据：**
- **FUPSI (OnionPIR)**: 无论数据库大小，Query请求大小固定约为 **64 KB**。
- **您的模拟代码**: 单次查询请求约为 **67 MB** (以 $N_{lwe}=1024, z=2$ 为例)。

**代码问题分析：**
在 `protocol.cpp` 的 `query_phase` 函数中，通信量计算公式如下：
```cpp
// 每个GSW矩阵大小：(2*N_lwe)^2 * 8 bytes
size_t gsw_matrix_size = (2 * N_lwe) * (2 * N_lwe) * 8;
metrics_.query_comm_bytes += z * gsw_matrix_size;
```
- 您的实现假设每个查询维度传送一个完整的GSW密文矩阵。
- 矩阵维度为 $(2 \cdot N_{lwe}) \times (2 \cdot N_{lwe})$。当 $N_{lwe}=1024$ 时，单矩阵大小为 $2048 \times 2048 \times 8 \text{ bytes} \approx 32 \text{ MB}$。
- $z=2$ 时，总请求大小即为 $64$ MB。

**理论差异：**
- **FUPSI/OnionPIR** 使用的是 **Module-LWE** 或 **Ring-LWE**。在这种设置下，密文是多项式环上的元素，而非巨大的整数矩阵。
- 典型的OnionPIR请求包含少量的多项式，每个多项式系数可能是 $N=4096$ 或 $2048$，模数 $q$ 较小，且经过了种种打包和压缩技术。
- **结论**：您的模拟使用了“其最原始、未优化的数学形式”来估算大小，而非“工程优化后的PIR协议”大小。这导致了三个数量级（MB vs KB）的差异。

### 2.2 查询阶段计算开销 (Computational Overhead)

**代码问题分析：**
在 `server_process_pir_query_z_dimension` 函数中，您为了“模拟GSW计算开销”，执行了真实的随机矩阵乘法：
```cpp
// 模拟GSW同态操作的高复杂度
// 此处执行了额外的矩阵乘法
MatrixType small_gsw_matrix = Matrix::random_matrix(...);
auto _ = Matrix::vector_matrix_multiply(small_vec, small_gsw_matrix, ...);
```
- 这种模拟方式实际上是在进行 $O(N_{lwe}^3)$ 或 $O(N_{lwe}^2 \cdot \text{scale})$ 的整数矩阵运算。
- 对于 $N_{lwe}=1024$，这是一个非常繁重的CPU操作。

**理论差异：**
- 实际的高效PIR（如Spiral, OnionPIR）利用 NTT (Number Theoretic Transform) 将多项式乘法加速到 $O(N \log N)$。
- 它们并不进行朴素的矩阵乘法。
- 因此，您的模拟器在CPU上跑满负荷进行的计算，比实际使用了AVX2/AVX-512优化的NTT运算要慢得多。

### 2.3 数据库规模的影响 (Server Aggregation)

您提到的“服务器端聚合”确实会导致服务器端数据库变大，但这符合预期，且不是造成上述巨大数量级差异的主因。

- **FUPSI**: 服务器持有 1 个Client的数据集，分区总数 $b_{FUPSI} \approx \frac{(1+\epsilon)N_{size}}{d_1}$。
- **MUPSI**: 服务器持有 $n$ 个Client的聚合数据，分区总数 $b_{MUPSI} \approx \frac{(1+\epsilon)N_{size} \cdot n}{d_1} = n \cdot b_{FUPSI}$。

**影响**：
- 服务器的**运算量**确实会随 $n$ 线性增长（因为要处理 $n$ 倍的分区）。
- 但是，**查询请求的大小** (Communication) 应当基本保持不变（只取决于PIR参数 $N_{lwe}$ 和维度 $z$，与 $b$ 的关系是对数级的或几乎无关）。
- 所以，MUPSI的通信量**不应该**比FUPSI大很多。您观察到的通信量暴涨完全是由于上述 2.1 节中的模型参数设置不当导致的。

## 3. 修正建议

为了让实验结果更符合预期（即更接近两方FUPSI的先进性能），建议调整模拟策略：

1.  **修正通信模型**：
    不要使用 `(2*N_lwe)^2 * 8` 计算大小。直接使用常数或基于Ring-LWE的公式。
    *建议修改 `protocol.cpp` 中的 `gsw_matrix_size` 为 `64 * 1024` (即64KB) 来模拟 OnionPIR 的表现。*

2.  **修正计算模型**：
    移除 `server_process_pir_query_z_dimension` 中为了“模拟复杂度”而人为添加的 `Matrix::vector_matrix_multiply` 循环。
    或者，将其替换为更符合 NTT 运算次数的简单延时（基于 $N \log N$），而不是执行沉重的 $N^3$ 矩阵乘法。

3.  **参数对齐**：
    确保 `config.h` 中的 `N_lwe` 参数意义与文献一致。在Ring-LWE中，1024/2048 通常指多项式度数，而在标准LWE中指向量维度。这两者对应的矩阵大小天差地别。

通过以上调整，您的MUPSI复现结果应当能回归到与两方FUPSI同数量级的水平，并能正确反映出由 $n$ (客户端数量) 带来的线性计算增长，而非当前的爆炸式增长。
