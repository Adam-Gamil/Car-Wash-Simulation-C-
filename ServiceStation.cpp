#include <iostream>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>
#include <chrono>
#include <random>
#include <atomic>

// -- Helper for thread-safe printing --
std::mutex cout_mutex;
void safe_print(const std::string& msg) {
    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout << msg << std::endl;
}

// -- Helper for Random Numbers --
int random_int(int min, int max) {
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}

// -- Data Structure for Car --
struct Car {
    std::string carName;
    int carId;

    Car(std::string name, int id) : carName(name), carId(id) {}
};

// -- Custom Semaphore Class --
class Semaphore {
private:
    std::mutex mtx;
    std::condition_variable cv;
    int slots;

public:
    Semaphore(int initialSlots) : slots(initialSlots) {
        if (initialSlots < 0) {
            throw std::invalid_argument("Initial slots cannot be negative");
        }
    }

    void acquire() {
        std::unique_lock<std::mutex> lock(mtx);
        // Equivalent to wait() logic in Java
        cv.wait(lock, [this] { return slots > 0; });
        slots--;
    }

    void release() {
        std::lock_guard<std::mutex> lock(mtx);
        slots++;
        cv.notify_one(); // or notify_all()
    }

    int availablePermits() {
        std::lock_guard<std::mutex> lock(mtx);
        return slots;
    }
};

// -- Shared Queue Class --
class SharedQueue {
private:
    std::queue<Car> queue;
    Semaphore empty;
    Semaphore full;
    Semaphore mutex;

public:
    SharedQueue(int capacity) 
        : empty(capacity), full(0), mutex(1) {}

    void addCar(const Car& car) {
        // 1. Wait for a slot to be 'empty'
        empty.acquire();

        // 2. Lock the queue
        mutex.acquire();

        // --- Critical Section ---
        queue.push(car);
        safe_print(car.carName + " entered the waiting queue. (Queue size: " + std::to_string(queue.size()) + ")");
        // --- End Critical Section ---

        // 3. Unlock the queue
        mutex.release();

        // 4. Signal that one slot is now 'full'
        full.release();
    }

    Car removeCar(int pumpId) {
        // 1. Wait for a car (wait for 'full')
        full.acquire();

        // 2. Lock the queue
        mutex.acquire();

        // --- Critical Section ---
        Car car = queue.front();
        queue.pop();
        
        safe_print("Pump " + std::to_string(pumpId) + ": " + car.carName + " taken from queue. (Queue size: " + std::to_string(queue.size()) + ")");
        // --- End Critical Section ---

        // 3. Unlock the queue
        mutex.release();

        // 4. Signal that one slot is now 'empty'
        empty.release();

        return car;
    }

    int getWaitingCarCount() {
        // In the original Java logic, full.availablePermits() represents items currently in queue
        return full.availablePermits();
    }
};

// -- Pump Thread Function --
void pump_routine(int pumpId, SharedQueue& queue, Semaphore& pumps) {
    try {
        while (true) {
            // 1) Wait until a car is available and remove it from queue
            // Note: In C++, accessing the queue is blocking based on semaphores, 
            // so we don't need to check for null explicitly like the Java defensive code.
            Car car = queue.removeCar(pumpId);

            // 2) Acquire a service bay (pump)
            pumps.acquire();
            
            safe_print("Pump " + std::to_string(pumpId) + ": " + car.carName + " begins service at Bay " + std::to_string(pumpId));
            
            // Sleep for 2000 - 6000 ms
            std::this_thread::sleep_for(std::chrono::milliseconds(random_int(2000, 6000)));

            safe_print("Pump " + std::to_string(pumpId) + ": " + car.carName + " finishes service");
            safe_print("Pump " + std::to_string(pumpId) + ": Bay " + std::to_string(pumpId) + " is now free");
            
            // 3) Signal that the pump bay is free
            pumps.release();
        }
    } catch (...) {
        safe_print("Pump " + std::to_string(pumpId) + " interrupted.");
    }
}

// -- Car Thread Function --
void car_routine(std::string name, int id, SharedQueue& queue) {
    safe_print(name + " arrived");
    queue.addCar(Car(name, id));
}

// -- Main --
int main() {
    int queueSize;
    int pumpCount;
    int maxCars;

    // Input validation loop
    do {
        std::cout << "Enter waiting area size (1 - 10): ";
        std::cin >> queueSize;
    } while (queueSize < 1 || queueSize > 10);

    std::cout << "Enter number of pumps: ";
    std::cin >> pumpCount;

    std::cout << "Enter total number of cars to generate: ";
    std::cin >> maxCars;

    // Initialize Resources
    Semaphore pumps(pumpCount);
    SharedQueue queue(queueSize);

    // ---- Start Pump Threads (Consumers) ----
    std::vector<std::thread> pumpThreads;
    for (int i = 0; i < pumpCount; i++) {
        // We pass arguments by reference using std::ref
        pumpThreads.emplace_back(pump_routine, i + 1, std::ref(queue), std::ref(pumps));
    }

    // ---- Start Car Stream (Producers) ----
    std::vector<std::thread> carThreads;
    for (int carId = 1; carId <= maxCars; carId++) {
        std::string name = "Car " + std::to_string(carId);
        
        // Launch car thread
        carThreads.emplace_back(car_routine, name, carId, std::ref(queue));

        // Sleep 1-2 seconds between arrivals
        std::this_thread::sleep_for(std::chrono::milliseconds(random_int(1000, 2000)));
    }

    // Join car threads ensures they have all finished adding themselves to the queue
    for (auto& t : carThreads) {
        if (t.joinable()) t.join();
    }

    // Wait until queue is empty and all pump bays are free
    while (true) {
        int waiting = queue.getWaitingCarCount();
        int freePumps = pumps.availablePermits();
        
        if (waiting == 0 && freePumps == pumpCount) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    std::cout << "\nAll cars serviced. Shutting down pumps.\n";

    // In C++, we cannot easily "interrupt" a thread like in Java.
    // Since the program is ending, we detach the pump threads and let the OS clean them up.
    // This effectively stops them when main returns.
    for (auto& t : pumpThreads) {
        t.detach(); 
    }

    std::cout << "ServiceStation finished." << std::endl;

    return 0;
}