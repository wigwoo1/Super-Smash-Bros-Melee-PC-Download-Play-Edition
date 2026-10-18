#pragma once
#include <cstdint>
namespace aurora::vi {
enum class AcquireWaitDecision { Waiting, Ready, GraceGranted, TimedOut };
/* A wait deadline is not a presentation acknowledgement. Only an actual
 * in-flight surface acquisition may extend the ordinary deadline, once.
 * The extension stays attached to the same ticket and never resets time. */
struct AcquireWaitBudget {
  static constexpr uint64_t ordinaryNs=2000000000ULL;
  static constexpr uint64_t hardNs=10000000000ULL;
  bool extended=false;
  uint64_t frame=0;
  AcquireWaitDecision evaluate(uint64_t elapsed,bool ready,bool acquiring,uint64_t ticket) {
    if(ready)return AcquireWaitDecision::Ready;
    if(elapsed<ordinaryNs)return AcquireWaitDecision::Waiting;
    if(elapsed>=hardNs)return AcquireWaitDecision::TimedOut;
    if(!extended){
      if(!acquiring||!ticket)return AcquireWaitDecision::TimedOut;
      extended=true;frame=ticket;return AcquireWaitDecision::GraceGranted;
    }
    if(ticket!=frame)return AcquireWaitDecision::TimedOut;
    return AcquireWaitDecision::Waiting;
  }
};
}
