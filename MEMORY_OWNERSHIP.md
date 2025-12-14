# Memory Ownership Documentation

## Overview

This document describes the memory ownership rules and lifetime guarantees for RF signal buffers in the M5-RF-Suite project. These rules prevent common memory bugs such as double-free, use-after-free, and memory leaks.

## Core Principles

### RAII (Resource Acquisition Is Initialization)

All dynamically allocated resources follow RAII principles:
- Resources are acquired in constructors
- Resources are released in destructors
- No manual memory management required by users
- Automatic cleanup prevents memory leaks

### Ownership Transfer

The codebase uses C++11 move semantics for efficient ownership transfer:
- Move operations transfer ownership without allocation
- Source object's pointers are nullified after move
- Prevents accidental double-free
- Improves performance by avoiding unnecessary copies

## Data Structures

### CapturedSignalData (rf_test_workflow.h)

**Memory Model:** Owns dynamically allocated pulse timing buffer

**Ownership Rules:**
```cpp
struct CapturedSignalData {
    uint16_t* pulseTimes;  // OWNED: This object owns the buffer
    uint16_t pulseCount;   // Number of elements in buffer
    // ... other fixed-size members
};
```

**Lifetime Guarantees:**
- `pulseTimes` buffer is valid as long as the object exists
- Buffer is automatically freed when object is destroyed
- After move, source object has `pulseTimes = nullptr`

**Operations:**

1. **Default Construction**
   ```cpp
   CapturedSignalData signal;  // pulseTimes = nullptr
   ```
   - No allocation performed
   - Safe to destroy immediately

2. **Buffer Allocation**
   ```cpp
   signal.allocatePulseBuffer(100);  // Allocates buffer for 100 pulses
   ```
   - Frees any existing buffer first
   - Returns false on allocation failure
   - Zero-initializes buffer for safety

3. **Deep Copy (Copy Constructor/Assignment)**
   ```cpp
   CapturedSignalData signal2 = signal1;  // Creates independent copy
   ```
   - Allocates NEW buffer
   - Copies all data from source
   - Both objects own independent buffers
   - Either can be destroyed without affecting the other

4. **Move (Move Constructor/Assignment)**
   ```cpp
   CapturedSignalData signal2 = std::move(signal1);  // Transfers ownership
   ```
   - NO allocation performed
   - Transfers buffer pointer to destination
   - Source pointer set to nullptr
   - Source object is safe to destroy (no double-free)

5. **Automatic Cleanup**
   ```cpp
   {
       CapturedSignalData signal;
       signal.allocatePulseBuffer(100);
   }  // Destructor automatically frees buffer here
   ```

**Thread Safety:** NOT thread-safe. External synchronization required.

### std::vector<CapturedSignalData> (rf_test_workflow.h)

**Memory Model:** Vector owns all CapturedSignalData objects

**Ownership Rules:**
```cpp
class RFTestWorkflow {
    std::vector<CapturedSignalData> captureBuffer;  // OWNS all signals
};
```

**Operations:**

1. **Adding Signals (Efficient - with move)**
   ```cpp
   CapturedSignalData signal;
   signal.allocatePulseBuffer(10);
   captureBuffer.push_back(std::move(signal));
   // signal.pulseTimes is now nullptr (ownership transferred)
   // No allocation or copy performed
   ```

2. **Adding Signals (Less Efficient - with copy)**
   ```cpp
   CapturedSignalData signal;
   signal.allocatePulseBuffer(10);
   captureBuffer.push_back(signal);
   // signal.pulseTimes still valid (independent copy made)
   // Buffer was allocated and copied
   ```

3. **Clearing Buffer**
   ```cpp
   captureBuffer.clear();
   // Calls destructor on each CapturedSignalData
   // All pulseTimes buffers automatically freed
   ```

4. **Accessing Signals**
   ```cpp
   const CapturedSignalData* signal = getCapturedSignal(0);
   // BORROWED: Pointer is valid only while captureBuffer exists
   // MUST NOT delete the returned pointer
   // MUST NOT hold pointer across operations that modify captureBuffer
   ```

### Fixed-Size Structures (No Dynamic Allocation)

The following structures use only fixed-size arrays and do not require special memory management:

#### RF433Signal (rf433_module.h)
```cpp
struct RF433Signal {
    unsigned long value;
    unsigned int bitLength;
    // ... all fixed-size members
    char description[64];  // Fixed array, not pointer
};
```
- Safe to pass by value
- Default copy/move is sufficient
- No memory leaks possible

#### WiFiNetworkInfo (rf24_module.h)
```cpp
struct WiFiNetworkInfo {
    char ssid[33];      // Fixed array
    uint8_t bssid[6];   // Fixed array
    // ... all fixed-size members
};
```
- Safe to store in vector
- No special ownership rules needed

#### ESPNowMessage (rf24_module.h)
```cpp
struct ESPNowMessage {
    uint8_t senderId[6];   // Fixed array
    uint8_t data[200];     // Fixed array
    size_t dataLen;
};
```
- Safe to copy
- No dynamic allocation

#### BLEDeviceInfo (rf24_module.h)
```cpp
struct BLEDeviceInfo {
    char name[64];      // Fixed array
    char address[18];   // Fixed array
    // ... all fixed-size members
};
```
- Safe to pass by value
- No memory management needed

## Best Practices

### DO ✅

1. **Use move semantics when transferring ownership**
   ```cpp
   captureBuffer.push_back(std::move(signal));  // Efficient
   ```

2. **Use helper methods for buffer management**
   ```cpp
   if (signal.allocatePulseBuffer(count)) {
       // Use buffer safely
   }
   if (signal.hasPulseBuffer()) {
       // Access pulseTimes
   }
   ```

3. **Let RAII handle cleanup**
   ```cpp
   // No need to manually free - destructor handles it
   {
       CapturedSignalData signal;
       signal.allocatePulseBuffer(100);
   }  // Automatic cleanup
   ```

4. **Check allocation success**
   ```cpp
   if (!signal.allocatePulseBuffer(size)) {
       // Handle allocation failure
   }
   ```

### DON'T ❌

1. **Don't manually delete pulseTimes**
   ```cpp
   // WRONG - destructor will double-free
   delete[] signal.pulseTimes;
   ```

2. **Don't access after move**
   ```cpp
   buffer.push_back(std::move(signal));
   signal.pulseTimes[0] = 100;  // WRONG - nullptr dereference
   ```

3. **Don't hold borrowed pointers across modifications**
   ```cpp
   const CapturedSignalData* ptr = getCapturedSignal(0);
   captureBuffer.push_back(newSignal);  // May invalidate ptr
   ptr->pulseCount;  // WRONG - ptr may be invalid
   ```

4. **Don't mix ownership models**
   ```cpp
   uint16_t* buffer = new uint16_t[10];
   signal.pulseTimes = buffer;  // WRONG - use allocatePulseBuffer()
   ```

## Common Patterns

### Pattern 1: Capture and Store Signal

```cpp
void capture433MHzSignals() {
    while (rf433Module->isSignalAvailable()) {
        RF433Signal rfSignal = rf433Module->receiveSignal();
        
        CapturedSignalData captured;
        convertRF433Signal(rfSignal, captured);  // Allocates buffer
        
        if (validateSignal433MHz(captured)) {
            // Move to vector for efficient transfer
            captureBuffer.push_back(std::move(captured));
        }
        // If not moved, destructor frees buffer automatically
    }
}
```

### Pattern 2: Process Signals in Buffer

```cpp
void processSignals() {
    for (auto& signal : captureBuffer) {
        // signal is reference to owned object
        if (signal.hasPulseBuffer()) {
            // Safe to access pulseTimes
            for (uint16_t i = 0; i < signal.pulseCount; i++) {
                processPulse(signal.pulseTimes[i]);
            }
        }
    }
}
```

### Pattern 3: Clean Up Resources

```cpp
void cleanup() {
    // Option 1: Clear entire buffer (automatic cleanup)
    captureBuffer.clear();
    
    // Option 2: Let vector destructor handle it
    // (when object goes out of scope)
}
```

## Memory Safety Guarantees

### No Double-Free
- Each buffer has exactly one owner
- Owner's destructor frees buffer exactly once
- Move operations nullify source pointer

### No Memory Leaks
- RAII ensures cleanup even on exception
- Vector destruction cleans up all elements
- Failed allocations leave object in valid state

### No Use-After-Free
- Moved-from objects have nullptr (safe to check)
- Borrowed pointers documented as temporary
- hasPulseBuffer() checks validity before access

### No Dangling Pointers
- No external references to internal buffers
- getCapturedSignal() returns borrowed pointer (documented)
- Buffer lifetime tied to object lifetime

## Testing

See `/tmp/test_memory_ownership.cpp` for comprehensive test suite that verifies:
- Default construction
- Buffer allocation
- Deep copy semantics
- Move semantics
- Vector operations
- RAII cleanup

All tests pass with no memory leaks (verified with Valgrind).

## Future Improvements

Potential enhancements for even stronger safety:

1. **Smart Pointers**: Replace raw pointer with `std::unique_ptr<uint16_t[]>`
   - Automatic cleanup (even with exceptions)
   - Cannot accidentally copy
   - Explicit move required

2. **Custom Allocator**: Pool allocator for pulse buffers
   - Reduce allocation overhead
   - Better cache locality
   - Deterministic allocation time

3. **Bounds Checking**: Add debug assertions
   - Verify buffer size on access
   - Detect out-of-bounds access
   - Catch bugs in development

## References

- [C++ Core Guidelines: Resource Management](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#r-resource-management)
- [RAII Pattern](https://en.cppreference.com/w/cpp/language/raii)
- [Move Semantics](https://en.cppreference.com/w/cpp/language/move_constructor)

---

**Last Updated:** December 14, 2025  
**Version:** 1.0.0
