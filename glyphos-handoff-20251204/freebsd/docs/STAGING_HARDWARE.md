# GlyphOS Staging Hardware Reservation

**Project**: GlyphOS Production Hardening
**Phase**: 6 - Staging Soak Testing
**Reservation Window**: 2026-01-13 to 2026-01-24 (72-hour soak + setup/teardown)
**Status**: 📋 Pending Reservation

---

## Executive Summary

**Purpose**: 72-hour continuous soak test of GlyphOS under realistic production load

**Hardware Required**:
- 3x CPU servers (primary, secondary, test)
- 1x GPU server (optional, for future offload validation)
- 1x FPGA/NPU simulator (optional, for hardware integration testing)
- Network infrastructure (10 Gbps switch, isolated VLAN)

**Estimated Cost**: $15,000 (2-week reservation + engineer time)

**Critical Path**: Hardware must be reserved by 2026-01-06 to meet Phase 6 timeline

---

## Server Specifications

### Server 1: Primary Production Node

**Role**: Main GlyphOS instance under load

**CPU**: 2x AMD EPYC 7763 (64-core, 128-thread) @ 2.45 GHz
- **Alternative**: Intel Xeon Platinum 8380 (40-core, 80-thread)

**Memory**: 256 GB ECC DDR4-3200 (8x 32 GB DIMMs)
- **Minimum**: 128 GB ECC
- **ECC required**: Yes (data integrity critical)

**Storage**:
- OS: 2x 960 GB NVMe SSD (RAID 1 for redundancy)
- Data: 4x 1.92 TB NVMe SSD (ZFS RAID-Z for vault storage)
- **Total capacity**: 6 TB usable

**Network**:
- 2x 10 Gbps Ethernet (bonded for redundancy)
- 1x 1 Gbps management interface

**Operating System**: FreeBSD 13.2-RELEASE

**Hosted At**:
- **Preferred**: On-premises data center
- **Alternative**: AWS c6id.metal, Azure HBv3, GCP c2d-highmem-112

---

### Server 2: Secondary/Standby Node

**Role**: Failover testing, replication validation

**Specifications**: Same as Server 1

**Purpose**:
- Test ZFS replication
- Validate backup/recovery procedures
- Chaos testing (simulate primary failure)

---

### Server 3: Load Generator

**Role**: Generate realistic glyph activation load

**CPU**: 2x AMD EPYC 7543 (32-core, 64-thread)
- Lower core count acceptable (load generation not CPU-intensive)

**Memory**: 128 GB ECC DDR4

**Storage**: 2x 480 GB SATA SSD (RAID 1)

**Network**: 2x 10 Gbps Ethernet

**Operating System**: FreeBSD 13.2 or Linux (for load tools)

---

### Optional: GPU Server (Future-proofing)

**Role**: Validate GPU-accelerated proof generation (if implemented)

**GPU**: 4x NVIDIA A100 80GB or equivalent
- **Alternative**: 8x NVIDIA A40
- **Minimum**: 2x A100 40GB

**CPU**: 2x AMD EPYC 7543 (32-core)

**Memory**: 512 GB ECC DDR4

**Storage**: 4x 1.92 TB NVMe SSD

**Network**:
- 2x 100 Gbps InfiniBand or RoCE
- NVLink topology for GPU-GPU communication

**Operating System**: Ubuntu 22.04 LTS with CUDA 12.x

**Note**: GPU offload not in Phase 6 scope, but hardware allows validation if implemented

---

### Optional: FPGA/NPU Simulator

**Role**: Test hardware-accelerated field state computations

**FPGA**: Xilinx Alveo U280
- **Alternative**: Intel Stratix 10 GX, Xilinx VU9P

**Host Requirements**:
- PCIe 4.0 x16 slot
- 64 GB RAM minimum
- Ubuntu 22.04 LTS

**Software**:
- Xilinx Vitis 2023.2
- Vivado 2023.2
- Custom RTL for substrate simulation

**Note**: Can be software-simulated if FPGA hardware unavailable

---

## Network Infrastructure

### Isolated Test VLAN

**VLAN ID**: 100 (staging-glyphos)

**Subnet**: 10.100.0.0/24
- 10.100.0.10 - Primary production node
- 10.100.0.11 - Secondary/standby node
- 10.100.0.12 - Load generator
- 10.100.0.20 - GPU server (optional)
- 10.100.0.30 - FPGA host (optional)
- 10.100.0.100 - Prometheus/Grafana monitoring
- 10.100.0.1 - Gateway

**Switch**: 10 Gbps managed switch with VLAN support
- Minimum 8 ports
- QoS support (for traffic shaping)

**Firewall**: Isolated from production
- Allow inbound: SSH (22), Prometheus (9090-9103)
- Block all other inbound
- Allow outbound for package updates

---

## Monitoring Infrastructure

### Prometheus + Grafana Server

**Role**: Real-time metrics collection and visualization

**Specifications**:
- CPU: 8 cores
- Memory: 32 GB
- Storage: 500 GB SSD (for time-series data)
- Network: 1 Gbps

**Hosted At**: Same data center, VLAN 100

**Configuration**:
- Scrape interval: 15 seconds
- Retention: 30 days
- Dashboard: Production Overview + Substrate Deep Dive

---

## Reservation Timeline

### Week 1: Setup and Warmup (2026-01-13 to 2026-01-17)

**Monday 01-13**:
- [ ] Hardware provisioning complete
- [ ] FreeBSD installation and configuration
- [ ] Network VLAN setup
- [ ] ZFS pool creation

**Tuesday 01-14**:
- [ ] GlyphOS deployment (all servers)
- [ ] Monitoring infrastructure (Prometheus + Grafana)
- [ ] Load generator configuration
- [ ] Vault seeding (1000+ test glyphs)

**Wednesday 01-15**:
- [ ] Smoke tests
- [ ] Network connectivity validation
- [ ] ZFS replication setup
- [ ] Backup/recovery test

**Thursday 01-16**:
- [ ] 24-hour warmup load (light)
- [ ] Baseline metrics collection
- [ ] Alert tuning

**Friday 01-17**:
- [ ] Warmup complete
- [ ] Final pre-soak checklist
- [ ] Go/no-go decision for soak test

---

### Week 2: 72-Hour Soak + Testing (2026-01-20 to 2026-01-24)

**Monday 01-20** (Soak Start):
- 06:00 AM: Launch sustained load (100 activations/min)
- 10:00 AM: First checkpoint (4 hours stable)
- 06:00 PM: Second checkpoint (12 hours stable)
- **On-call**: Primary engineer + backup

**Tuesday 01-21** (Soak Day 2):
- Monitor continuously
- No interventions unless P0 alert
- Collect telemetry every hour
- **On-call**: Rotation

**Wednesday 01-22** (Soak Day 3):
- Continue monitoring
- 06:00 AM: 48-hour checkpoint
- Prepare for spike test
- **On-call**: Rotation

**Thursday 01-23** (Soak End + Load Testing):
- 06:00 AM: 72-hour soak complete ✅
- 10:00 AM: Spike test (burst to 1000/min for 1 hour)
- 02:00 PM: Chaos testing (process kills, network faults)
- 06:00 PM: Final data collection

**Friday 01-24** (Teardown and Analysis):
- 09:00 AM: Download all logs and metrics
- 11:00 AM: Begin teardown
- 02:00 PM: Hardware released
- 04:00 PM: Initial analysis meeting

---

## Load Test Scenarios

### Scenario 1: Sustained Load (72 hours)

**Configuration**:
- Activation rate: 100 glyphs/minute
- Glyph distribution: 80% simple, 15% complex, 5% deep inheritance
- Randomized activations from 1000-glyph corpus

**Expected Metrics**:
- P50 latency: < 50ms
- P99 latency: < 500ms
- Memory growth: < 1% per day
- CPU utilization: 40-60%

**Failure Conditions**:
- Crash or restart
- Memory leak (RSS growth > 10% per day)
- P99 latency > 2 seconds
- Data corruption (checksum/parity failures)

---

### Scenario 2: Spike Load (1 hour)

**Configuration**:
- Burst from 100/min → 1000/min
- Observe recovery time
- Monitor queue depths

**Expected Behavior**:
- Latency spike < 5 seconds
- Recovery to baseline < 1 minute
- No dropped activations
- No data loss

---

### Scenario 3: Chaos Engineering

**Tests**:

**Process Kill**:
```bash
# Kill substrate_core
kill -9 $(pgrep substrate_core)
# Expected: Restart within 10s, recover from checkpoint
```

**Network Partition**:
```bash
# Simulate network outage
iptables -A INPUT -s 10.100.0.11 -j DROP
# Wait 30 seconds
iptables -D INPUT -s 10.100.0.11 -j DROP
# Expected: Graceful reconnection, no data loss
```

**Disk Full**:
```bash
# Fill /var/db/glyphos
dd if=/dev/zero of=/var/db/glyphos/fill bs=1M count=1000
# Expected: Alert fires, service degrades gracefully
```

**Memory Pressure**:
```bash
# Consume RAM
stress-ng --vm 8 --vm-bytes 80G --timeout 60s
# Expected: OOM killer protects critical processes
```

---

## Data Collection

### Metrics to Capture

**System Metrics** (node_exporter):
- CPU usage (per-core and aggregate)
- Memory (RSS, swap, available)
- Disk I/O (reads, writes, latency)
- Network (throughput, errors, retransmits)

**GlyphOS Metrics** (Prometheus):
- All metrics from `docs/MONITORING.md`
- Activation rate and latency
- Cell operations
- Checksum/parity failures
- Queue depths

**Logs**:
- substrate.log (all levels)
- interpreter.log (all levels)
- System logs (/var/log/messages)
- ZFS events

**Frequency**:
- Metrics: Every 15 seconds
- Logs: Continuous (rotated hourly)
- Snapshots: Every 6 hours (ZFS)

---

## Success Criteria

**Phase 6 passes if**:
- [ ] 72-hour soak completes without crash
- [ ] P99 latency remains within SLA (< 500ms sustained)
- [ ] Memory growth < 1% per day (no leaks)
- [ ] All spike/chaos tests pass
- [ ] 0 data corruption events (checksum/parity)
- [ ] Proof verification 100% success OR software fallback validated
- [ ] ZFS replication and backup validated

**Phase 6 fails if**:
- Crash or hang during soak
- Memory leak detected
- Data corruption
- Unrecoverable error

**On failure**: Fix, re-deploy, re-run (add 1 week to Phase 6)

---

## Reservation Request

**To**: Data Center Operations / Cloud Provider
**From**: GlyphOS Project Team
**Date**: 2025-12-05

**Requested Resources**:
- 3x bare-metal servers (EPYC 7763 class, specs above)
- Network: 10 Gbps isolated VLAN
- Duration: 2 weeks (2026-01-13 to 2026-01-24)
- Optional: 1x GPU server, 1x FPGA host

**Justification**: Critical production readiness validation for GlyphOS release

**Budget Code**: GLYPHOS-PHASE6

**Contact**:
- Name: Dave
- Email: daveswo@earthwebap.com
- Phone: [TBD]
- Slack: @daveswo

**Approval Required From**:
- [ ] Engineering Manager
- [ ] Finance (budget approval)
- [ ] Data Center Ops (resource availability)

**Backup Plan**: If on-prem unavailable, provision AWS/Azure/GCP instances

---

## Post-Soak Deliverables

**Within 3 days of completion**:
- [ ] Soak test report (executive summary)
- [ ] Performance metrics (Grafana snapshots)
- [ ] Telemetry data archive (Prometheus export)
- [ ] Incident log (if any issues occurred)
- [ ] Lessons learned

**Within 1 week**:
- [ ] Detailed analysis report
- [ ] Recommendations for Phase 8 rollout
- [ ] Updated runbooks (based on chaos tests)

---

## Contacts

| Role | Name | Email | Phone |
|------|------|-------|-------|
| Ops Lead | TBD | ops@earthwebap.com | [TBD] |
| Hardware Engineer | TBD | hardware@earthwebap.com | [TBD] |
| SRE On-Call | Rotation | sre@earthwebap.com | [TBD] |
| Data Center Ops | TBD | dc-ops@example.com | [TBD] |

---

**Next Steps**:
1. Submit reservation request by 2026-01-06
2. Confirm hardware availability
3. Schedule kickoff meeting with Ops team
4. Begin deployment planning

---

**Document Owner**: Ops Lead
**Last Updated**: 2025-12-05
**Next Review**: 2026-01-06 (confirmation deadline)
