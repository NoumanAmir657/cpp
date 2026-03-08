#ifndef DRIVER_H
#define DRIVER_H

#include "resource.h"

typedef struct driver_t driver_t;

void driver_retain(driver_t* d);
void driver_release(driver_t* d);
void driver_query_devices(driver_t* d);

typedef struct {
  void (*destroy)(driver_t*);
  void (*query_devices)(driver_t*);
} driver_vtable_t;

#define DISPATCH(d, method) \
  ((const driver_vtable_t*)(((resource_t*)(d))->vtable))->method

void driver_retain(driver_t* d)         { resource_retain((resource_t*)d); }
void driver_release(driver_t* d)        { resource_release((resource_t*)d); }
void driver_query_devices(driver_t* d)  { DISPATCH(d, query_devices)(d); }

#endif // DRIVER_H
