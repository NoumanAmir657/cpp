#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "resource.h"
#include "driver.h"

// -----------------------------------------------------------------------------
// CPU driver implementation
// -----------------------------------------------------------------------------
typedef struct {
    resource_t resource;
    int      core_count;
} cpu_driver_t;

static const driver_vtable_t cpu_driver_vtable;

static cpu_driver_t* cpu_driver_cast(driver_t* d) {
    if (((resource_t*)d)->vtable != &cpu_driver_vtable) abort();
    return (cpu_driver_t*)d;
}

static void cpu_driver_destroy(driver_t* d) {
    cpu_driver_t* self = cpu_driver_cast(d);
    printf("cpu_driver destroyed (cores=%d)\n", self->core_count);
  free(self);
}

static void cpu_driver_query_devices(driver_t* d) {
    cpu_driver_t* self = cpu_driver_cast(d);
    printf("cpu_driver: %d core(s)\n", self->core_count);
}

static const driver_vtable_t cpu_driver_vtable = {
  .destroy       = cpu_driver_destroy,
  .query_devices = cpu_driver_query_devices,
};

int cpu_driver_create(driver_t** out_driver, int core_count) {
    cpu_driver_t* driver = NULL;
    driver = (cpu_driver_t*)malloc(sizeof(cpu_driver_t));
    resource_init(&driver->resource, &cpu_driver_vtable);
    driver->core_count = core_count;
    *out_driver = (driver_t*)driver;
    return 1;
}
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// GPU driver implementation
// -----------------------------------------------------------------------------
typedef struct {
    resource_t resource;
    char     name[32];
} gpu_driver_t;

static const driver_vtable_t gpu_driver_vtable;

static gpu_driver_t* gpu_driver_cast(driver_t* d) {
    if (((resource_t*)d)->vtable != &gpu_driver_vtable) abort();
    return (gpu_driver_t*)d;
}

static void gpu_driver_destroy(driver_t* d) {
    gpu_driver_t* self = gpu_driver_cast(d);
    printf("gpu_driver destroyed (name=%s)\n", self->name);
    free(self);
}

static void gpu_driver_query_devices(driver_t* d) {
    gpu_driver_t* self = gpu_driver_cast(d);
    printf("gpu_driver: device \"%s\"\n", self->name);
}

static const driver_vtable_t gpu_driver_vtable = {
    .destroy       = gpu_driver_destroy,
    .query_devices = gpu_driver_query_devices,
};

int gpu_driver_create(driver_t** out_driver, const char* name) {
    gpu_driver_t* driver = NULL;
    driver = (gpu_driver_t*)malloc(sizeof(gpu_driver_t));
    resource_init(&driver->resource, &gpu_driver_vtable);
    strncpy(driver->name, name, sizeof(driver->name) - 1);
    *out_driver = (driver_t*)driver;
    return 1;
}
// -----------------------------------------------------------------------------

int main(void) {
    driver_t* driver_cpu = NULL;
    cpu_driver_create(&driver_cpu, 8);
    driver_query_devices(driver_cpu);
    driver_release(driver_cpu);

    driver_t* driver_gpu = NULL;
    gpu_driver_create(&driver_gpu, "NVIDIA");
    driver_query_devices(driver_gpu);
    driver_release(driver_gpu);
}
