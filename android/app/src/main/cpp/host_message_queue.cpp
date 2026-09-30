#include "host_message_queue.h"

void mk64_mq_create(Mk64HostMessageQueue* q, void** storage, int capacity) {
    if (!q) return;
    q->messages=storage; q->capacity=capacity; q->first=0; q->count=0;
}
int mk64_mq_send(Mk64HostMessageQueue* q, void* message) {
    if (!q || !q->messages || q->capacity<=0 || q->count>=q->capacity) return -1;
    q->messages[(q->first+q->count)%q->capacity]=message; ++q->count; return 0;
}
int mk64_mq_recv(Mk64HostMessageQueue* q, void** message) {
    if (!q || q->count<=0) return -1;
    if (message) *message=q->messages[q->first];
    q->first=(q->first+1)%q->capacity; --q->count; return 0;
}
