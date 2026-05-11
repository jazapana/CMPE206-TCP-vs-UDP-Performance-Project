# TCP vs UDP Performance Comparison

## Overview
This project compares the performance of TCP and UDP under different network conditions using the NS-3 network simulator.

## Experiments
The following experiments were implemented:

1. Transfer Size Experiment
   - Compared TCP and UDP under different transfer sizes
   - `.cc` and `.csv` files are found in `/transfer-size`

2. Latency Experiment
   - Compared TCP and UDP under different latency values
   - `.cc` and `.csv` files are found in `/high-latency`

3. Low Latency Congestion Experiment
   - Tested congestion behavior under low latency
   - `.cc` and `.csv` files are found in `/high-congestion-low-latency`

4. High Latency Congestion Experiment
   - Tested congestion behavior under high latency
   - `.cc` and `.csv` files are found in `/high-congestion-high-latency`

The graphs and tables are found in:
- `TCP_vs_UDP_Transfer_Size.ipynb`
- `LatencyCongestionCharts.ipynb`

## Metrics Collected
The following metrics were collected:

- Throughput (Kbps)
- Mean Delay (ms)
- Packet Loss
- Mean Jitter (ms)
- Fairness

## Technologies Used
- GitHub
- NS-3:
   - Main network simulation framework
- PointToPointHelper:
   - Creates links between nodes
- OnOffHelper:
   - Generates TCP/UDP traffic
- PacketSinkHelper:
   - Creates TCP/UDP receivers
- FlowMonitor:
   - Collects metrics
- Google Colab:
   - Graphs and tables

## Authors
- Tyler Jones
- Joel Zapana