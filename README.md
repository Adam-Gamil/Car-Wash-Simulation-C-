# Service Station Simulation (C++) ⛽🚗

A multi-threaded C++ application that simulates the operation of a service station (gas station) using the **Producer-Consumer** design pattern. This project demonstrates low-level synchronization primitives, thread management, and resource sharing in an Operating System context.

![C++](https://img.shields.io/badge/Language-C++11-blue.svg)
![Type](https://img.shields.io/badge/Type-Simulation-green.svg)
![Topic](https://img.shields.io/badge/Topic-Multithreading-orange.svg)

## 📋 Table of Contents
- [Overview](#overview)
- [Key Features](#key-features)
- [Technical Concepts](#technical-concepts)
- [How It Works](#how-it-works)
- [Installation & Usage](#installation--usage)
- [Example Output](#example-output)

## 📖 Overview
This program models a service station with a limited number of **Pumps** (resources) and a limited **Waiting Queue** size. 
- **Cars (Producers):** Arrive at random intervals and attempt to enter the station.
- **Pumps (Consumers):** Take cars from the queue and service them for a random duration.

The simulation ensures that cars wait if the queue is full and pumps wait if the queue is empty, preventing race conditions and deadlocks.

## ✨ Key Features
* **Custom Semaphore Implementation:** A hand-written Semaphore class using `std::mutex` and `std::condition_variable` (mimicking Java's `wait`/`notify`).
* **Thread-Safe Logging:** a custom `safe_print` function ensures console output from multiple threads doesn't get jumbled.
* **Dynamic Configuration:** Users can input the queue size, number of pumps, and total cars at runtime.
* **Randomized Event Timing:** Arrival times and service durations are randomized to simulate real-world variance.

## 🧠 Technical Concepts
This project serves as a practical implementation of several Operating System concepts:
1.  **Concurrency:** Managing multiple threads (Car threads vs. Pump threads) running simultaneously.
2.  **Synchronization:** Using Mutex locks to protect Critical Sections (Shared Queue).
3.  **Inter-Process Communication:** Using Condition Variables to signal state changes (Full/Empty buffers).
4.  **Resource Management:** Tracking available semaphore permits to manage queue slots and pump availability.

## ⚙️ How It Works
The system consists of three main components:
1.  **SharedQueue:** A thread-safe FIFO queue guarded by Semaphores (`empty`, `full`, and `mutex`).
2.  **Pump Threads:** Infinite loops that wait for the `full` semaphore, remove a car, and simulate service time.
3.  **Car Threads:** Created dynamically; they attempt to acquire an `empty` slot in the semaphore to enter the queue.

