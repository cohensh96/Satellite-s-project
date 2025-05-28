
# 🛰️ Space Situational Awareness (SSA) Algorithms Implementation

This project implements and evaluates **Space Situational Awareness (SSA)** algorithms for detecting potential collisions between satellites. The project is based on cutting-edge research and was developed as a final-year capstone project in Software Engineering.

👉 [📽️ Watch the project demo video](https://github.com/cohensh96/Satellite-s-project/blob/main/ProjectUserGuideVideo.MP4)  
👉 [📂 Project Repository](https://github.com/cohensh96/Satellite-s-project/tree/main)

---

## 📘 Project Overview

**Space debris is a growing concern.** With over 27,000 tracked objects orbiting Earth, autonomous satellites must detect and avoid potential collisions. This project addresses this challenge by implementing efficient collision detection algorithms on real satellite hardware.

### 🎯 Goals
- Implement and deploy **SSA algorithms** (ANCAS, SBO-ANCAS, CATCH) on a real embedded system.
- Test and optimize these algorithms for **real-time performance** under **satellite-like constraints**.
- Collaborate with **Dr. Elad Dannenberg**, author of CATCH & SBO-ANCAS, to bridge research and industry.

---

## 📌 Keywords

`Satellite`, `Collision Detection`, `CATCH`, `SBO-ANCAS`, `ANCAS`, `TCA`, `OBC`, `Embedded`, `Eigenvalues`, `Raspberry Pi`

---

## 🚀 Algorithms Implemented

### 1. ANCAS (Alfano-Negron Close Approach Software)
- Uses cubic polynomial interpolation of 4 points to approximate the distance function over short time windows.
- Solves 3rd degree polynomial to find candidate TCA values.
- Pros: Fast runtime.
- Cons: Low accuracy if time intervals are too wide.

### 2. SBO-ANCAS (Sampling-Based Optimization)
- Enhances ANCAS using iterative refinement and a satellite orbit propagator (SGP4).
- Refines results until distance and time tolerances are met.
- Pros: Highest accuracy.
- Cons: Computationally heavier than ANCAS.

### 3. CATCH (Conjunction Assessment Through Chebyshev Polynomials)
- Employs Chebyshev Proxy Polynomials for high-precision approximations and uses eigenvalue methods to find TCA.
- Configurable polynomial degree (e.g., N=16).
- Pros: High precision and deterministic runtime.
- Cons: Requires matrix eigensolver and more memory.

---

## 🖥️ Hardware & Technologies

### 🔧 **Tested OBC**: Raspberry Pi 5

A cost-effective alternative to space-grade OBCs used for algorithm deployment and testing.

![Raspberry Pi 5](https://raw.githubusercontent.com/cohensh96/Satellite-s-project/refs/heads/main/raspberry-pi-5-features.jpg.webp)

- **CPU**: ARM Cortex-A76, 2.4GHz
- **RAM**: 8GB LPDDR4X
- **Power**: ~7W
- **OS**: Linux
- **Cost**: $80 USD

### 🛰️ **Reference Board**: ISIS OBC
- 400 MHz ARM9, 32MB RAM, dual SD storage
- FreeRTOS for real-time operations

---

## 🧪 Testing System Architecture

### 🧩 System Parts
- **Testing Station App**: GUI built with Blazor (.NET C#) to manage tests and collect results.
- **Tested OBC App**: C++ app running on Raspberry Pi 5 performing algorithm execution.

### 🔄 Communication
- Cross-platform **TCP/IP** over Ethernet or Wi-Fi.
- Custom protocol with **CRC-based error detection**.

### 🖼️ Diagram
![System Architecture](https://github.com/cohensh96/Satellite-s-project/blob/main/diagram/system-overview.png)

---

## 📊 Feasibility & Performance Testing

- Compared algorithms by runtime, memory usage, and accuracy.
- Tested over varying input sizes, intervals, and tolerances.
- Developed simulation environments with:
  - TCP/IP or UDP communication
  - CRC-based error detection
  - Local Simulation Mode (runs both apps on the same machine)

---

## 📊 Performance Analysis

- **ANCAS**: Fastest runtime, lower accuracy.
- **SBO-ANCAS**: Best accuracy/runtime balance.
- **CATCH**: High accuracy, higher runtime and occasional numerical instability.

| Algorithm   | Avg. Runtime | Error Margin     |
|-------------|--------------|------------------|
| ANCAS       | ⚡ Fast       | ❌ High Error     |
| SBO-ANCAS   | ✅ Balanced  | ✅ Low Error       |
| CATCH       | 🧠 Accurate  | ⚠️ Unstable in edge cases |

> **Conclusion**: SBO-ANCAS offers the best trade-off between speed and accuracy for onboard execution.

---

## 🤝 Collaboration

This project was completed in collaboration with **Dr. Elad Dannenberg**, a leading researcher in SSA algorithms. His input ensured the scientific and practical validity of our implementations.

---

## 📌 Conclusions

- Satellite collision detection can be **autonomously and efficiently** handled on embedded hardware.
- **SBO-ANCAS** emerges as the most feasible algorithm for real-time use in constrained environments.
- Our system design supports **scalability and reproducibility** for future research and operational satellites.

---

## 🛠️ Tools & Languages 

- **Languages**: C++, Python, C#, Bash
- **Libraries**: Eigen, SQLite3, INIH, SGP4
- **Platforms**: Raspberry Pi OS, Visual Studio, GitHub

---

## 📁 Repository Structure

```
/Satellite-s-project/
├── TestStationApp/         # Blazor Web App
├── TestedOBCApp/           # C++ Algorithm Engine
├── Algorithms/             # Core logic (ANCAS, SBO-ANCAS, CATCH)
├── Docs/                   # Reports and Presentations
├── Data/                   # Test input/output samples
└── diagrams/               # System architecture and process diagrams
```

---

## 🧑‍💻 Authors

- Shir Cohen
- Yotam Aharon
- Supervisor: Mr. Ilya Zeldner
- Research Advisor: Dr. Elad Dannenberg

---

## 📂 Related Files

- 📄 Phase A Report  
- 📄 Phase B Report  
- 📽️ [Project Demo Video](https://github.com/cohensh96/Satellite-s-project/blob/main/ProjectUserGuideVideo.MP4)

---

> _"Real innovation happens when research meets hands-on engineering."_  
