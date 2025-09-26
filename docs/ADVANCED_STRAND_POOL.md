# Advanced Strand Pool 设计文档

## 概述

`AdvancedStrandPool` 是一个高性能的异步执行器池，专为 FoxHTTP 服务器设计，提供多种负载均衡策略、实时监控和动态扩缩容功能。

## 主要特性

### 1. 多种负载均衡策略

- **Round Robin (轮询)**: 按顺序分配请求到各个 strand
- **Least Connections (最少连接)**: 将请求分配给当前活跃连接数最少的 strand
- **Consistent Hash (一致性哈希)**: 基于哈希值的一致性分配，支持会话亲和性
- **Random (随机)**: 随机分配请求
- **Weighted Round Robin (加权轮询)**: 根据负载因子进行加权分配

### 2. 线程安全设计

- 使用 `std::shared_mutex` 实现读写锁，提高并发性能
- 原子操作确保统计数据的线程安全
- 无锁的负载均衡算法减少锁竞争

### 3. 实时监控和统计

- 每个 strand 的详细统计信息：
  - 总请求数
  - 活跃请求数
  - 完成请求数
  - 失败请求数
  - 平均执行时间
  - 负载因子
  - 健康状态

### 4. 动态扩缩容

- 支持运行时调整池大小
- 智能的扩容和缩容策略
- 可配置的负载阈值和空闲阈值

### 5. 健康检查

- 定期健康检查机制
- 可自定义的健康检查回调
- 自动故障转移

### 6. 会话亲和性

- 基于会话ID的一致性哈希分配
- 确保同一会话的请求分配到同一个 strand
- 提高缓存命中率和性能

## 使用示例

### 基本使用

```cpp
#include <foxhttp/server/strand_pool.hpp>

// 创建IO上下文
boost::asio::io_context io_context;

// 配置Strand池
AdvancedStrandPool::Config config;
config.initial_size = 4;
config.strategy = LoadBalanceStrategy::LeastConnections;
config.health_check_interval = std::chrono::milliseconds(5000);

// 创建并启动池
AdvancedStrandPool strand_pool(io_context, config);
strand_pool.start();

// 获取strand
auto strand = strand_pool.getNextStrand();
if (strand) {
    boost::asio::post(*strand, []() {
        // 执行异步任务
    });
}
```

### 会话亲和性

```cpp
// 为特定会话获取固定的strand
auto strand = strand_pool.getStrandForSession("user123");
```

### 监控和统计

```cpp
// 设置指标回调
strand_pool.setMetricsCallback([](const std::string& metric, 
                                 const std::string& strand_id, 
                                 double value) {
    std::cout << metric << " for strand " << strand_id << ": " << value << std::endl;
});

// 获取统计信息
auto stats = strand_pool.getStatistics();
for (size_t i = 0; i < stats.size(); ++i) {
    std::cout << "Strand " << i << " load: " 
              << stats[i].getLoadFactor() << std::endl;
}
```

### 动态调整

```cpp
// 扩容
strand_pool.resize(8);

// 缩容
strand_pool.resize(4);

// 更改负载均衡策略
strand_pool.setLoadBalanceStrategy(LoadBalanceStrategy::ConsistentHash);
```

## 配置选项

```cpp
struct Config {
    std::size_t initial_size = std::thread::hardware_concurrency();
    std::size_t min_size = 1;
    std::size_t max_size = 64;
    LoadBalanceStrategy strategy = LoadBalanceStrategy::RoundRobin;
    std::chrono::milliseconds health_check_interval{5000};
    std::chrono::milliseconds metrics_report_interval{10000};
    bool enable_auto_scaling = true;
    double load_threshold = 0.8;  // 负载阈值
    double idle_threshold = 0.2;  // 空闲阈值
};
```

## 性能优势

### 相比原版本的改进

1. **更好的并发性能**
   - 使用读写锁替代互斥锁
   - 减少锁竞争和等待时间

2. **更智能的负载均衡**
   - 多种策略适应不同场景
   - 实时负载监控和调整

3. **更好的可观测性**
   - 详细的统计信息
   - 实时指标报告
   - 健康状态监控

4. **更强的扩展性**
   - 动态扩缩容
   - 可配置的策略
   - 插件化的健康检查

5. **更好的错误处理**
   - 健康检查和故障转移
   - 优雅的降级机制

## 最佳实践

### 1. 选择合适的负载均衡策略

- **Round Robin**: 适用于负载相对均匀的场景
- **Least Connections**: 适用于请求处理时间差异较大的场景
- **Consistent Hash**: 适用于需要会话亲和性的场景
- **Random**: 适用于简单的负载分散
- **Weighted Round Robin**: 适用于需要精细负载控制的场景

### 2. 合理配置池大小

```cpp
// 根据CPU核心数设置初始大小
config.initial_size = std::thread::hardware_concurrency();

// 设置合理的最大大小，避免过度创建线程
config.max_size = std::thread::hardware_concurrency() * 2;
```

### 3. 监控和调优

```cpp
// 定期检查统计信息
auto stats = strand_pool.getStatistics();
for (const auto& stat : stats) {
    if (stat.getLoadFactor() > 0.8) {
        // 考虑扩容或优化
    }
}
```

### 4. 健康检查

```cpp
// 实现自定义健康检查
strand_pool.setHealthCheckCallback([](std::size_t strand_index) -> bool {
    // 检查strand是否健康
    return checkStrandHealth(strand_index);
});
```

## 故障排除

### 常见问题

1. **性能问题**
   - 检查负载均衡策略是否合适
   - 监控各strand的负载分布
   - 考虑调整池大小

2. **内存泄漏**
   - 确保正确调用 `stop()` 方法
   - 检查回调函数是否正确释放资源

3. **健康检查失败**
   - 检查健康检查回调的实现
   - 确认strand是否真的有问题

## 总结

新的 `AdvancedStrandPool` 设计提供了更强大、更灵活、更可观测的异步执行器池功能，能够更好地适应高并发、高可用的服务器场景。通过合理的配置和使用，可以显著提升服务器的性能和稳定性。
