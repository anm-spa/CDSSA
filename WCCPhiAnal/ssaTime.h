//
//  ssaTime.h
//  LLVM
//
//  Created by Abu Naser Masud on 2019-06-07.
//

#ifndef SSA_TIME_H
#define SSA_TIME_H

#include <type_traits>
#include <typeinfo>
#include <chrono>

using namespace std;

template<class Resolution = std::chrono::milliseconds>
class ExecTime {
public:
    typedef typename std::conditional<std::chrono::high_resolution_clock::is_steady,
    std::chrono::high_resolution_clock,
    std::chrono::steady_clock>::type Clock;
private:
    const Clock::time_point startClock=Clock::now();
    
public:
    ExecTime() = default;
    ~ExecTime() {
    }
    
    unsigned long stop() {
        const auto end = Clock::now();
        return std::chrono::duration_cast<Resolution>( end - startClock).count();
    }
    
}; 

#endif // EXECUTION_TIMER_H
