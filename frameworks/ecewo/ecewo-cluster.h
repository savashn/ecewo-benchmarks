#ifndef ECEWO_CLUSTER_H
#define ECEWO_CLUSTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

typedef struct {
  uint8_t cpus;
  uint16_t port;
  bool respawn; // Default: false
  uint32_t shutdown_timeout_ms; // Default: 15s
  uint32_t respawn_window_sec; // Default 5s
  uint8_t respawn_max_crashes; // Default 3
  uint32_t worker_startup_delay_ms; // Default 100ms
  uint32_t worker_respawn_delay_ms; // Default 100ms
  void (*on_start)(uint8_t worker_id);
  void (*on_exit)(uint8_t worker_id, int exit_status, bool is_crash);
} Cluster;

bool cluster_init(const Cluster *options, int argc, char **argv);
uint16_t cluster_get_port(void);
bool cluster_is_master(void);
bool cluster_is_worker(void);
uint8_t cluster_worker_id(void);
uint8_t cluster_worker_count(void);
uint8_t cluster_cpus(void);
uint8_t cluster_cpus_physical(void);
void cluster_signal_workers(int signum);
void cluster_wait_workers(void);
void cluster_graceful_restart(void);

typedef enum {
  WORKER_STARTING, // Process spawned, not yet active
  WORKER_ACTIVE, // Running normally
  WORKER_STOPPING, // Shutdown requested
  WORKER_CRASHED, // Exited unexpectedly
  WORKER_RESPAWNING, // Crashed, waiting to respawn
  WORKER_DISABLED // Respawn disabled (too many crashes)
} worker_status_t;

typedef struct {
  uint8_t worker_id; // Worker ID (0-based)
  int pid; // Process ID (0 if not running)
  uint16_t port; // Port worker is bound to
  worker_status_t status; // Current status
  time_t start_time; // When worker started (0 if not running)
  time_t uptime_seconds; // How long worker has been running
  int exit_status; // Last exit code (0 if still running)
  uint8_t crash_count; // Crashes within respawn window
  bool respawn_disabled; // Respawn disabled due to crash loop
} worker_stats_t;

typedef struct {
  uint8_t total_workers; // Total configured workers
  uint8_t active_workers; // Currently running workers
  uint8_t crashed_workers; // Workers that crashed
  uint8_t disabled_workers; // Workers with respawn disabled
  bool shutdown_requested; // Shutdown in progress
  bool restart_requested; // Graceful restart in progress
  uint64_t total_restarts; // Total worker restarts since start
} cluster_stats_t;

int cluster_get_worker_stats(uint8_t worker_id, worker_stats_t *stats);
int cluster_get_stats(cluster_stats_t *stats);
int cluster_get_all_workers(worker_stats_t *stats_array, int array_size);

#ifdef __cplusplus
}
#endif

#endif
