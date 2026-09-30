#pragma once
#include <cstdint>

struct Mk64HostMessageQueue {
    void** messages = nullptr;
    int capacity = 0;
    int first = 0;
    int count = 0;
};

void mk64_mq_create(Mk64HostMessageQueue* q, void** storage, int capacity);
int mk64_mq_send(Mk64HostMessageQueue* q, void* message);
int mk64_mq_recv(Mk64HostMessageQueue* q, void** message);
