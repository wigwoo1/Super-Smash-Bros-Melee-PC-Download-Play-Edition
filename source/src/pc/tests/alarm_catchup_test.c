#include <pc/alarm_queue.h>
#include <pc/os_alarm.h>
#include <pc/callback_gate.h>
#include <assert.h>
#include <stdio.h>

typedef struct { unsigned queued, delivered, consumed, capacity; } Consumer;
static int capacity(void* data) {
    Consumer* c = data;
    return c->queued < c->capacity;
}
static void produce(void* data) {
    Consumer* c = data;
    assert(c->queued < c->capacity);
    ++c->queued; ++c->delivered;
}
static void consume(Consumer* c) {
    assert(c->queued); --c->queued; ++c->consumed;
}
static void increment(void* data) { ++*(unsigned*)data; }
static void queue_debt(void) {
    MeleePc_AlarmQueue queue;
    Consumer c = {0,0,0,2};
    unsigned other = 0;
    int64_t next;
    MeleePc_AlarmQueueInit(&queue);
    assert(MeleePc_AlarmSchedule(&queue,1,10,10,produce,&c));
    assert(MeleePc_AlarmSetCatchUp(&queue,1,capacity,&c));
    assert(MeleePc_AlarmSchedule(&queue,2,25,0,increment,&other));
    // A half-second owner stall must keep all 50 boundaries. Full consumer
    // stops this producer without blocking an unrelated timer or losing debt.
    assert(MeleePc_AlarmPump(&queue,500,64)==3 && c.delivered==2 && other==1);
    assert(MeleePc_AlarmNextDeadline(&queue,&next) && next==30);
    for (unsigned n=0;n<48;++n) {
        assert(MeleePc_AlarmPump(&queue,500,64)==0); // Full, no overwrite.
        consume(&c);
        assert(MeleePc_AlarmPump(&queue,500,1)==1);
    }
    assert(c.delivered==50 && c.queued==2);
    while(c.queued)consume(&c);
    assert(c.consumed==50 && MeleePc_AlarmPump(&queue,500,64)==0);
    assert(MeleePc_AlarmNextDeadline(&queue,&next) && next==510);
    assert(MeleePc_AlarmPump(&queue,510,0)==0 && c.delivered==50);
    assert(MeleePc_AlarmPump(&queue,510,1)==1 && c.delivered==51);
    consume(&c);
    assert(MeleePc_AlarmSetCatchUp(&queue,1,NULL,NULL));
    assert(MeleePc_AlarmPump(&queue,1000,64)==1 && c.delivered==52);
    consume(&c);
    assert(MeleePc_AlarmNextDeadline(&queue,&next) && next==1010);
    assert(!MeleePc_AlarmSetCatchUp(NULL,1,capacity,&c));
    assert(!MeleePc_AlarmSetCatchUp(&queue,99,capacity,&c));
    assert(MeleePc_AlarmCancel(&queue,1));
    assert(!MeleePc_AlarmSetCatchUp(&queue,1,capacity,&c));
    assert(MeleePc_AlarmSchedule(&queue,1,INT64_MAX,1,increment,&other));
    assert(MeleePc_AlarmSetCatchUp(&queue,1,capacity,&c));
    assert(MeleePc_AlarmPump(&queue,INT64_MAX,64)==1 && other==2);
    assert(MeleePc_AlarmPump(&queue,INT64_MAX,64)==0);
    // Rescheduling the same key clears catch-up policy.
    assert(MeleePc_AlarmSchedule(&queue,1,10,10,increment,&other));
    assert(MeleePc_AlarmSetCatchUp(&queue,1,capacity,&c));
    assert(MeleePc_AlarmSchedule(&queue,1,10,10,increment,&other));
    assert(MeleePc_AlarmPump(&queue,500,64)==1 && other==3);
}
static OSTime tick;
static int owned=1;
static int owner(void) { return owned; }
static OSTime now(void* data) { assert(data==&tick);return tick; }
static Consumer adapter_consumer={0,0,0,2};
static OSContext context;
static void sample(OSAlarm* alarm,OSContext* actual) {
    assert(actual==&context && alarm->handler==sample);
    produce(&adapter_consumer);
}
static void adapter_debt(void) {
    OSAlarm alarm;
    assert(MeleePc_InstallCallbackOwner(owner));
    assert(MeleePc_InstallAlarmClock(now,&tick));
    OSInitAlarm(); OSCreateAlarm(&alarm);
    OSSetPeriodicAlarm(&alarm,10,10,sample);
    assert(MeleePc_SetAlarmCatchUp(&alarm,capacity,&adapter_consumer));
    owned=0;assert(!MeleePc_SetAlarmCatchUp(&alarm,NULL,NULL));owned=1;
    tick=500;
    BOOL prior=OSDisableInterrupts();
    assert(MeleePc_DispatchAlarms(&context,64)==0);
    OSRestoreInterrupts(prior);
    assert(MeleePc_DispatchAlarms(&context,64)==2 && alarm.fire==30);
    while(adapter_consumer.delivered<50) {
        consume(&adapter_consumer);
        assert(MeleePc_DispatchAlarms(&context,1)==1);
    }
    assert(alarm.fire==510 && alarm.handler==sample && OSCheckAlarmQueue());
    while(adapter_consumer.queued)consume(&adapter_consumer);
    assert(adapter_consumer.consumed==50);
    OSCancelAlarm(&alarm);
    assert(!MeleePc_SetAlarmCatchUp(&alarm,capacity,&adapter_consumer));
    assert(MeleePc_DispatchAlarms(&context,64)==0);
}
int main(void) {
    queue_debt(); adapter_debt();
    puts("PASS: retained timer debt, bounded consumer backpressure, unrelated alarm progress, default coalescing, owner gate and cancellation");
    return 0;
}
