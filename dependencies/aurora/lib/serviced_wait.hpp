#pragma once
#include <chrono>
namespace aurora {
/* The predicate and CV state remain protected by the caller's lock. Service
 * callbacks run with that lock released, so callbacks can retire work and
 * safely take the same mutex. Ownership/reentrancy belongs to the provider. */
template<class Cv,class Lock,class Predicate,class Service>
bool serviced_wait(Cv& cv,Lock& lock,Predicate ready,Service service){
 while(!ready()){
  if(cv.wait_for(lock,std::chrono::milliseconds{1},ready))break;
  struct Unlocked {Lock& lock;explicit Unlocked(Lock& value):lock(value){lock.unlock();}~Unlocked(){lock.lock();}};
  bool success;{Unlocked released{lock};success=service();}
  if(!success)return false;
 }
 return true;
}
}
