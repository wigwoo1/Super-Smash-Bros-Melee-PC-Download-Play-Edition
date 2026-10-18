#include "../lib/keyboard_press_latch.hpp"
#include <cassert>
#include <cstdio>
#include <climits>
#include <initializer_list>
int main(){
  aurora::input::KeyboardPressLatch<512> latch;
  assert(latch.take().none());
  latch.press(40); // A release does not erase an unconsumed press.
  auto sample=latch.take();assert(sample.test(40) && sample.count()==1);
  assert(latch.take().none()); // Exactly one sample, no ghost held state.
  latch.press(40,true);assert(latch.take().none());
  latch.press(-1);latch.press(512);latch.press(9000);assert(latch.take().none());
  latch.press(0);latch.press(511);latch.press(40);latch.press(40);
  sample=latch.take();assert(sample.count()==3 && sample[0] && sample[511] && sample[40]);
  latch.press(40);latch.clear();assert(latch.take().none());
  latch.press(40);sample=latch.take();
  assert(sample[40] && latch.take().none()); // A later physical press is new.
  bool held[512]{};
  for(int scan=0;scan<512;++scan){
    latch.press(scan);sample=latch.take();
    assert(aurora::input::keyboard_sample_pressed(held,512,scan,sample));
    assert(!aurora::input::keyboard_sample_pressed(held,512,scan,latch.take()));
    held[scan]=true;
    assert(aurora::input::keyboard_sample_pressed(held,512,scan,latch.take()));
    held[scan]=false;
    assert(!aurora::input::keyboard_sample_pressed(held,512,scan,latch.take()));
  }
  sample.reset();
  for(int scan:{INT_MIN,-6,-1,512,INT_MAX})
    assert(!aurora::input::keyboard_sample_pressed(held,512,scan,sample));
  assert(!aurora::input::keyboard_sample_pressed(nullptr,0,0,sample));
  held[511]=true;
  assert(!aurora::input::keyboard_sample_pressed(held,511,511,sample));
  assert(aurora::input::keyboard_sample_pressed(held,512,511,sample));
  puts("PASS: short-tap retention, one-sample consumption, repeat filtering, bounds, coalescing and clear");
  puts("PASS:2048 keyboard button/axis sampled/held/released cases plus bounds/null/short-array guards");
}
