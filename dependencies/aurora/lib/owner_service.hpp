#pragma once
namespace aurora {
/* The host owns callback lifetime; calls are synchronous, owner-only and
 * reentrancy guarded. Recursive host waits do not redispatch callbacks. */
struct OwnerService {
  int (*callback)()=nullptr;bool busy=false;
  bool install(bool owner,int (*provider)()){
    if(!owner || !provider || callback || busy)return false;
    callback=provider;return true;
  }
  bool pump(bool owner){
    if(!owner)return false;
    if(!callback || busy)return true;
    busy=true;const bool result=callback()!=0;busy=false;return result;
  }
  void reset(){callback=nullptr;busy=false;}
};
}
