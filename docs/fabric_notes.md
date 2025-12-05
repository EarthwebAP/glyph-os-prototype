# Fabric Network Profiling and RDMA Notes

## Overview

The fabric layer provides distributed communication between glyph nodes. This document covers:
1. Loopback baseline performance
2. RDMA availability and configuration
3. Network transport recommendations

## Test Environment

- **Platform:** Single-node WSL2 environment
- **Network:** Loopback interface (127.0.0.1)
- **RDMA:** Not available (no InfiniBand/RoCE hardware)
- **Kernel bypass:** Not applicable for loopback

## Loopback Baseline Performance

### Results (benchmarks/fabric_loopback.json)

| Metric | Value | Notes |
|--------|-------|-------|
| P50 | 0.0147 ms | 14.7 µs median latency |
| P95 | 0.0181 ms | 18.1 µs (95th percentile) |
| P99 | 0.0495 ms | 49.5 µs (99th percentile) |
| Mean | 0.0161 ms | 16.1 µs average |
| Transport | loopback | In-memory simulation |
| RDMA | false | Not available |

**Analysis:**
- Loopback latency represents best-case (no network overhead)
- P99/P50 ratio: 3.37x (tail latency spike)
- Suitable for single-node or testing scenarios
- Real network latency will be 10-1000x higher

### Latency Distribution

```
P50:  14.7 µs  ████████████████████████████████
P90:  18.0 µs  ██████████████████████████████████████
P95:  18.1 µs  ██████████████████████████████████████
P99:  49.5 µs  █████████████████████████████████████████████████████████████████████
P99.9: ~80 µs  ████████████████████████████████████████████████████████████████████████████████
```

Tail latency (P99) likely caused by:
- Kernel scheduler preemption
- GC pauses (Python runtime)
- System call overhead

## RDMA Availability

### Current Status

**RDMA is NOT available** in this environment:

```bash
$ lspci | grep -i infiniband
# No output - no InfiniBand hardware

$ ibstat
# ibstat: command not found

$ rdma link
# rdma: command not found
```

**Reasons:**
1. Running in WSL2 (Windows Subsystem for Linux)
2. No physical InfiniBand or RoCE NIC
3. Virtual network adapter doesn't support RDMA

### RDMA Configuration (for production deployments)

If RDMA hardware is available, configuration steps:

#### 1. Install RDMA Stack

```bash
# RHEL/CentOS
sudo yum install rdma-core libibverbs libmlx5

# Ubuntu/Debian
sudo apt-get install rdma-core libibverbs-dev libmlx5-1

# Verify installation
ibstat
rdma link show
```

#### 2. Configure Network Interface

```bash
# Check RDMA device
ibv_devices

# Example output:
#   device                 node GUID
#   ------              ----------------
#   mlx5_0              0002c9030051e800

# Configure IP over IB (if using IPoIB)
sudo ip addr add 192.168.10.1/24 dev ib0
sudo ip link set ib0 up

# Or configure RoCE (RDMA over Converged Ethernet)
sudo ip addr add 192.168.20.1/24 dev enp1s0
sudo ip link set enp1s0 up
```

#### 3. Tune RDMA Parameters

```bash
# Increase completion queue size
echo 4096 | sudo tee /sys/module/mlx5_core/parameters/cq_size

# Enable adaptive moderation
echo 1 | sudo tee /sys/class/infiniband/mlx5_0/ports/1/adaptive_moderation

# Set MTU (9000 for jumbo frames)
sudo ip link set ib0 mtu 9000
```

#### 4. Test RDMA Performance

```bash
# Server side (node 1)
ib_write_bw -d mlx5_0

# Client side (node 2)
ib_write_bw -d mlx5_0 192.168.10.1

# Expected results (40Gbps InfiniBand):
# Bandwidth: 35-40 Gbps
# Latency: 1-2 µs
```

## Performance Projections

### Loopback (Current)

- **Latency:** 14.7 µs (P50), 49.5 µs (P99)
- **Bandwidth:** N/A (in-memory)
- **Use case:** Single-node, testing, development

### TCP/IP over Ethernet (10GbE)

- **Latency:** 50-100 µs (P50), 200-500 µs (P99)
- **Bandwidth:** 8-9 Gbps (effective)
- **Kernel bypass:** No
- **Use case:** Basic multi-node, moderate scale

### RDMA over InfiniBand (40Gbps)

- **Latency:** 1-2 µs (P50), 3-5 µs (P99)
- **Bandwidth:** 35-40 Gbps
- **Kernel bypass:** Yes
- **Use case:** High-performance computing, low-latency

### RDMA over Converged Ethernet (RoCE)

- **Latency:** 2-4 µs (P50), 5-10 µs (P99)
- **Bandwidth:** 90-100 Gbps (100GbE)
- **Kernel bypass:** Yes
- **Use case:** Modern datacenter, cost-effective RDMA

### UDP with DPDK (Kernel Bypass)

- **Latency:** 10-20 µs (P50), 30-50 µs (P99)
- **Bandwidth:** 8-9 Gbps (10GbE)
- **Kernel bypass:** Yes (user-space networking)
- **Use case:** High packet rate, moderate latency

## Latency Comparison

```
Loopback:           14.7 µs  ██
InfiniBand (RDMA):   1.5 µs  █
RoCE (RDMA):         3.0 µs  █
DPDK (UDP):         15.0 µs  ██
10GbE TCP/IP:       75.0 µs  ██████████
1GbE TCP/IP:       250.0 µs  ████████████████████████████
```

## Recommendations

### For Current Environment (WSL2, No RDMA)

**Recommendation:** Use loopback for testing, TCP/IP for any multi-VM scenarios

- Loopback performance (14.7 µs) is sufficient for development
- No additional network configuration needed
- Benchmark results represent best-case latency

### For Production Deployment (Bare Metal)

**Recommendation:** RDMA (InfiniBand or RoCE) is **highly recommended** for multi-node

#### Why RDMA?

1. **Low latency:** 1-4 µs vs 50-100 µs for TCP/IP (25x improvement)
2. **Kernel bypass:** Zero-copy, no context switches
3. **CPU efficiency:** 90% less CPU overhead vs TCP/IP
4. **Predictable performance:** Lower jitter, better P99

#### When RDMA is NOT needed:

- Single-node deployments (use loopback)
- Write-heavy workloads (persistence is bottleneck, not network)
- Budget constraints (RDMA NICs cost $500-2000)
- Latency requirements >100 µs (TCP/IP sufficient)

### Fallback: TCP/IP with Optimizations

If RDMA is not available, optimize TCP/IP:

```bash
# Increase TCP buffer sizes
sudo sysctl -w net.core.rmem_max=134217728
sudo sysctl -w net.core.wmem_max=134217728
sudo sysctl -w net.ipv4.tcp_rmem="4096 87380 134217728"
sudo sysctl -w net.ipv4.tcp_wmem="4096 65536 134217728"

# Enable TCP low-latency mode
sudo sysctl -w net.ipv4.tcp_low_latency=1

# Disable Nagle's algorithm (application level)
# Use TCP_NODELAY socket option
```

**Expected improvement:** 20-30% latency reduction

## Jitter Analysis

### Loopback Jitter

```
P50: 14.7 µs
P90: 18.0 µs  (+22%)
P95: 18.1 µs  (+23%)
P99: 49.5 µs  (+237%)  ← High jitter
```

**Jitter ratio (P99/P50):** 3.37x

**Causes:**
- Python GC pauses
- Kernel scheduling
- No real-time priorities set

**Mitigation:**
- Use real-time scheduling (SCHED_FIFO)
- Pin processes to CPU cores
- Disable CPU frequency scaling

### Expected RDMA Jitter

```
P50: 1.5 µs
P90: 2.0 µs  (+33%)
P95: 2.5 µs  (+67%)
P99: 4.0 µs  (+167%)  ← Much lower jitter
```

**Jitter ratio (P99/P50):** 2.67x (better than loopback)

RDMA provides more consistent latency due to:
- Hardware-managed queues
- No kernel scheduling
- Dedicated network path

## Future Work

1. **Multi-node testing** - Deploy on 2-4 physical nodes with RDMA
2. **Kernel bypass (DPDK)** - Evaluate for TCP/IP environments
3. **Protocol comparison** - TCP vs UDP vs RDMA for glyph messaging
4. **Compression** - Test impact of compression on bandwidth/latency
5. **Load balancing** - Multi-path routing for throughput

## Testing Scripts

### Loopback Test (current)

```python
# benchmarks/fabric_loopback_test.py
import time
import socket

def test_loopback_latency(iterations=100000):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(('127.0.0.1', 9000))

    latencies = []
    for _ in range(iterations):
        start = time.perf_counter()
        sock.send(b'ping')
        sock.recv(4)
        latencies.append((time.perf_counter() - start) * 1000)  # ms

    return {
        'p50_ms': percentile(latencies, 50),
        'p95_ms': percentile(latencies, 95),
        'p99_ms': percentile(latencies, 99),
        'avg_ms': mean(latencies)
    }
```

### RDMA Test (future)

```python
# benchmarks/fabric_rdma_test.py
import rdma  # Requires pyverbs or similar

def test_rdma_latency(iterations=100000):
    # Create RDMA connection
    ctx = rdma.Context('mlx5_0')
    pd = ctx.alloc_pd()
    cq = ctx.create_cq(1000)
    qp = ctx.create_qp(pd, cq)

    # RDMA Write with immediate
    for _ in range(iterations):
        start = time.perf_counter()
        qp.rdma_write(remote_addr, data)
        cq.poll()  # Wait for completion
        latency = (time.perf_counter() - start) * 1000000  # µs
```

## Histogram Generation

```bash
# Requires matplotlib
python3 tools/plot_latency.py benchmarks/fabric_loopback.json \
    --out benchmarks/fabric_loopback_hist.png
```

Expected output: PNG histogram of latency distribution

## References

- Loopback results: `benchmarks/fabric_loopback.json`
- RDMA documentation: https://www.kernel.org/doc/Documentation/infiniband/
- RoCE configuration: https://community.mellanox.com/s/article/roce-configuration
- DPDK: https://www.dpdk.org/

---

**Document Version:** 1.0
**Last Updated:** 2025-12-04
**RDMA Status:** Not available (WSL2 environment)
**Recommendation:** RDMA is sufficient for production if latency <5µs required, otherwise TCP/IP acceptable
