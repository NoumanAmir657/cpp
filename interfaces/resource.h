#ifndef RESOURCE_H
#define RESOURCE_H

typedef struct resource_t resource_t;

typedef struct {
  void (*destroy)(resource_t*);
} resource_vtable_t;

struct resource_t {
  int32_t      ref_count;
  const void*  vtable;
};

static void resource_init(resource_t* r, const void* vtable) {
  r->ref_count = 1;
  r->vtable    = vtable;
}

static void resource_retain(resource_t* r) {
  r->ref_count++;
}

static void resource_release(resource_t* r) {
  if (--r->ref_count == 0) {
    ((const resource_vtable_t*)r->vtable)->destroy(r);
  }
}

#endif // RESOURCE_H
