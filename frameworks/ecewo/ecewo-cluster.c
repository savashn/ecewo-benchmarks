#ifndef __linux__
#error "Cluster plugin supported on Linux only."
#endif

#include "ecewo-cluster.h"
#include "ecewo.h"
#include <uv.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <signal.h>

#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#define WORKER_STOP_SIGNAL SIGTERM

#define DEFAULT_SHUTDOWN_TIMEOUT_MS 15000
#define DEFAULT_RESPAWN_WINDOW_SEC 5
#define DEFAULT_RESPAWN_MAX_CRASHES 3
#define DEFAULT_WORKER_STARTUP_DELAY_MS 100
#define DEFAULT_WORKER_RESPAWN_DELAY_MS 100

#ifdef ECEWO_DEBUG
#define LOG_DEBUG(fmt, ...) \
  fprintf(stderr, "[DEBUG] " fmt "\n", ##__VA_ARGS__)

#define LOG_INFO(fmt, ...) \
  printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...) ((void)0)
#define LOG_INFO(fmt, ...) ((void)0)
#endif

#define LOG_ERROR(fmt, ...) \
  fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__)

typedef struct {
  uv_process_t handle;
  uint8_t worker_id;
  uint16_t port;
  int pid;
  worker_status_t status;

  time_t restart_times[10]; // Circular buffer for crash times
  uint8_t restart_count;
  bool respawn_disabled;

  time_t start_time;
  int exit_status;
} worker_process_t;

typedef struct {
  bool is_master;
  uint8_t worker_id;
  uint8_t worker_count;
  uint16_t base_port;
  uint16_t worker_port;

  worker_process_t *workers;
  Cluster config;

  uv_signal_t sigterm;
  uv_signal_t sigint;
  uv_signal_t sigusr2;

  int original_argc;
  char **original_argv;
  char exe_path[1024];

  bool shutdown_requested;
  bool graceful_restart_requested;
  uint64_t total_restarts;

  bool initialized;
} cluster_state_t;

static cluster_state_t ecewo_cluster = { 0 };

static void on_exit_cb(uv_process_t *handle, int64_t exit_status, int term_signal);
static int spawn_worker(uint8_t worker_id, uint16_t port);

static Cluster cluster_default_options(void) {
  Cluster opts = {
    .cpus = 0, // 0 = use cluster_cpus()
    .port = 0, // Required
    .respawn = false,
    .shutdown_timeout_ms = DEFAULT_SHUTDOWN_TIMEOUT_MS,
    .respawn_window_sec = DEFAULT_RESPAWN_WINDOW_SEC,
    .respawn_max_crashes = DEFAULT_RESPAWN_MAX_CRASHES,
    .worker_startup_delay_ms = DEFAULT_WORKER_STARTUP_DELAY_MS,
    .worker_respawn_delay_ms = DEFAULT_WORKER_RESPAWN_DELAY_MS,
    .on_start = NULL,
    .on_exit = NULL
  };
  return opts;
}

static void save_original_args(int argc, char **argv) {
  if (ecewo_cluster.original_argv)
    return;

  ecewo_cluster.original_argc = argc;

  ecewo_cluster.original_argv = calloc(argc + 1, sizeof(char *));
  if (!ecewo_cluster.original_argv) {
    LOG_ERROR("Failed to allocate memory for argv");
    return;
  }

  for (int i = 0; i < argc; i++) {
    if (argv[i]) {
      ecewo_cluster.original_argv[i] = strdup(argv[i]);
      if (!ecewo_cluster.original_argv[i]) {
        LOG_ERROR("Failed to duplicate argv[%d]", i);
        // Cleanup what we allocated so far
        for (int j = 0; j < i; j++) {
          free(ecewo_cluster.original_argv[j]);
        }
        free(ecewo_cluster.original_argv);
        ecewo_cluster.original_argv = NULL;
        return;
      }
    }
  }
  ecewo_cluster.original_argv[argc] = NULL;

  size_t size = sizeof(ecewo_cluster.exe_path);
  if (uv_exepath(ecewo_cluster.exe_path, &size) != 0) {
    LOG_DEBUG("Failed to get executable path, using argv[0]");
    if (argv && argv[0]) {
      strncpy(ecewo_cluster.exe_path, argv[0], sizeof(ecewo_cluster.exe_path) - 1);
      ecewo_cluster.exe_path[sizeof(ecewo_cluster.exe_path) - 1] = '\0';
    }
  }
}

static void cleanup_original_args(void) {
  if (!ecewo_cluster.original_argv)
    return;

  for (int i = 0; ecewo_cluster.original_argv[i]; i++) {
    free(ecewo_cluster.original_argv[i]);
  }
  free(ecewo_cluster.original_argv);
  ecewo_cluster.original_argv = NULL;
  ecewo_cluster.original_argc = 0;
}

static char **build_worker_args(uint8_t worker_id, uint16_t port) {
  if (!ecewo_cluster.original_argv || ecewo_cluster.original_argc == 0) {
    LOG_ERROR("Original arguments not saved");
    return NULL;
  }

  // Count args excluding any existing --cluster-worker
  int filtered_count = 0;
  for (int i = 1; i < ecewo_cluster.original_argc; i++) {
    if (strcmp(ecewo_cluster.original_argv[i], "--cluster-worker") == 0) {
      i += 2; // Skip --cluster-worker and its two arguments
      continue;
    }
    filtered_count++;
  }

  // Allocate worker_id and port strings FIRST
  char *worker_id_str = malloc(16);
  char *port_str = malloc(16);

  if (!worker_id_str || !port_str) {
    free(worker_id_str);
    free(port_str);
    LOG_ERROR("Failed to allocate worker arg strings");
    return NULL;
  }

  snprintf(worker_id_str, 16, "%u", (unsigned int)worker_id);
  snprintf(port_str, 16, "%u", (unsigned int)port);

  // Total elements needed
  int total_size = 1 + filtered_count + 4;
  char **args = calloc(total_size, sizeof(char *));
  if (!args) {
    free(worker_id_str);
    free(port_str);
    LOG_ERROR("Failed to allocate worker args");
    return NULL;
  }

  int args_idx = 0;

  // Add executable path
  args[args_idx++] = ecewo_cluster.exe_path;

  // Add filtered original arguments
  for (int i = 1; i < ecewo_cluster.original_argc; i++) {
    if (strcmp(ecewo_cluster.original_argv[i], "--cluster-worker") == 0) {
      i += 2; // Skip --cluster-worker and its two arguments
      continue;
    }
    args[args_idx++] = ecewo_cluster.original_argv[i];
  }

  // Ensure we have space for 3 more items plus NULL
  if (args_idx + 4 > total_size) {
    LOG_ERROR("Buffer overflow detected: idx=%d, size=%d", args_idx, total_size);
    free(worker_id_str);
    free(port_str);
    free(args);
    return NULL;
  }

  // Add cluster worker arguments
  // NOTE: worker_id_str and port_str are heap-allocated and must be freed
  args[args_idx++] = "--cluster-worker";
  args[args_idx++] = worker_id_str; // HEAP ALLOCATED - must free
  args[args_idx++] = port_str; // HEAP ALLOCATED - must free
  args[args_idx] = NULL;

  return args;
}

static void free_worker_args(char **args) {
  if (!args)
    return;

  // Find the --cluster-worker argument
  for (int i = 0; args[i]; i++) {
    if (strcmp(args[i], "--cluster-worker") == 0) {
      // The next two arguments are ALWAYS heap-allocated strings
      // created by malloc() in build_worker_args
      // NOLINTNEXTLINE(clang-analyzer-unix.Malloc)
      free(args[i + 1]); // worker_id_str
      // NOLINTNEXTLINE(clang-analyzer-unix.Malloc)
      free(args[i + 2]); // port_str
      break;
    }
  }

  free(args);
}

static void apply_config(const Cluster *options) {
  Cluster defaults = cluster_default_options();

  if (options) {
    ecewo_cluster.config = *options;

    if (ecewo_cluster.config.cpus == 0)
      ecewo_cluster.config.cpus = cluster_cpus();

    if (ecewo_cluster.config.shutdown_timeout_ms == 0)
      ecewo_cluster.config.shutdown_timeout_ms = defaults.shutdown_timeout_ms;

    if (ecewo_cluster.config.respawn_window_sec == 0)
      ecewo_cluster.config.respawn_window_sec = defaults.respawn_window_sec;

    if (ecewo_cluster.config.respawn_max_crashes == 0)
      ecewo_cluster.config.respawn_max_crashes = defaults.respawn_max_crashes;

    if (ecewo_cluster.config.worker_startup_delay_ms == 0)
      ecewo_cluster.config.worker_startup_delay_ms = defaults.worker_startup_delay_ms;

    if (ecewo_cluster.config.worker_respawn_delay_ms == 0)
      ecewo_cluster.config.worker_respawn_delay_ms = defaults.worker_respawn_delay_ms;
  } else {
    ecewo_cluster.config = defaults;
    ecewo_cluster.config.cpus = cluster_cpus();
  }

  ecewo_cluster.worker_count = ecewo_cluster.config.cpus;

  if (ecewo_cluster.worker_count < 1) {
    LOG_ERROR("Invalid worker count: %" PRIu8 ", using 1", ecewo_cluster.worker_count);
    ecewo_cluster.worker_count = 1;
  }

  uint8_t cpu_count = cluster_cpus();
  if (ecewo_cluster.worker_count > cpu_count * 2) {
    LOG_INFO("Warning: %" PRIu8 " workers > 2x CPU count (%" PRIu8 ") - may cause contention",
             ecewo_cluster.worker_count, cpu_count);
  }
}

static bool should_respawn_worker(worker_process_t *worker) {
  if (!ecewo_cluster.config.respawn || worker->respawn_disabled)
    return false;

  time_t now = time(NULL);

  uint8_t max = ecewo_cluster.config.respawn_max_crashes;
  if (max > 10)
    max = 10;

  if (worker->restart_count >= max) {
    for (int i = 0; i < max - 1; i++) {
      worker->restart_times[i] = worker->restart_times[i + 1];
    }
    worker->restart_count = max - 1;
  }

  worker->restart_times[worker->restart_count++] = now;

  // Check if crashing too fast
  if (worker->restart_count >= max) {
    time_t window = now - worker->restart_times[0];
    if (window < (time_t)ecewo_cluster.config.respawn_window_sec) {
      LOG_ERROR("Worker %" PRIu8 " crashing too fast (%d times in %lds), disabling respawn",
                worker->worker_id, max, (long)window);

      worker->respawn_disabled = true;
      worker->status = WORKER_DISABLED;
      return false;
    }
  }

  return true;
}

typedef struct {
  uv_timer_t timer;
  void (*callback)(void);
} delay_ctx_t;

static void delay_callback(uv_timer_t *handle) {
  delay_ctx_t *ctx = (delay_ctx_t *)handle->data;
  if (ctx && ctx->callback) {
    ctx->callback();
  }
  uv_close((uv_handle_t *)handle, (uv_close_cb)free);
}

static void async_delay(uint64_t ms, void (*callback)(void)) {
  delay_ctx_t *ctx = malloc(sizeof(delay_ctx_t));
  if (!ctx) {
    LOG_ERROR("Failed to allocate delay context");
    if (callback)
      callback(); // Execute immediately on failure
    return;
  }

  ctx->callback = callback;
  uv_timer_init(uv_default_loop(), &ctx->timer);
  ctx->timer.data = ctx;
  uv_timer_start(&ctx->timer, delay_callback, ms, 0);
}

static void setup_worker_stdio(uv_process_options_t *options) {
  static uv_stdio_container_t stdio[3];

  stdio[0].flags = UV_IGNORE;

  stdio[1].flags = UV_INHERIT_FD;
  stdio[1].data.fd = 1;

  stdio[2].flags = UV_INHERIT_FD;
  stdio[2].data.fd = 2;

  options->stdio_count = 3;
  options->stdio = stdio;
}

static int spawn_worker(uint8_t worker_id, uint16_t port) {
  if (worker_id >= ecewo_cluster.worker_count) {
    LOG_ERROR("Invalid worker ID: %" PRIu8, worker_id);
    return -1;
  }

  if (!ecewo_cluster.original_argv) {
    LOG_ERROR("Original arguments not saved");
    return -1;
  }

  worker_process_t *worker = &ecewo_cluster.workers[worker_id];

  memset(worker, 0, sizeof(worker_process_t));
  worker->worker_id = worker_id;
  worker->port = port;
  worker->status = WORKER_STARTING;
  worker->respawn_disabled = false;
  worker->restart_count = 0;
  worker->start_time = time(NULL);
  worker->pid = 0;

  char **args = build_worker_args(worker_id, port);
  if (!args) {
    LOG_ERROR("Failed to build worker arguments");
    worker->status = WORKER_DISABLED;
    return -1;
  }

  uv_process_options_t options = { 0 };
  options.file = ecewo_cluster.exe_path;
  options.args = args;
  options.exit_cb = on_exit_cb;

  setup_worker_stdio(&options);

  // Set ECEWO_WORKER=1 env
  extern char **environ;
  int env_count = 0;
  while (environ[env_count])
    env_count++;

  char **new_env = malloc((env_count + 2) * sizeof(char *));
  if (!new_env) {
    free_worker_args(args);
    LOG_ERROR("Failed to allocate environment");
    worker->status = WORKER_DISABLED;
    return -1;
  }

  for (int i = 0; i < env_count; i++)
    new_env[i] = environ[i];

  new_env[env_count] = "ECEWO_WORKER=1";
  new_env[env_count + 1] = NULL;

  options.env = new_env;
  options.flags = UV_PROCESS_DETACHED;

  uv_process_t *handle = &worker->handle;
  handle->data = worker;

  int result = uv_spawn(uv_default_loop(), handle, &options);

  free(new_env);
  free_worker_args(args);

  if (result != 0) {
    LOG_ERROR("Failed to spawn worker %" PRIu8 ": %s", worker_id, uv_strerror(result));
    worker->status = WORKER_DISABLED;
    return -1;
  }

  worker->status = WORKER_ACTIVE;
  worker->pid = handle->pid;

  if (ecewo_cluster.config.on_start)
    ecewo_cluster.config.on_start(worker_id);

  return 0;
}

static void respawn_timer_cb(uv_timer_t *t) {
  worker_process_t *w = (worker_process_t *)t->data;
  if (w && ecewo_cluster.is_master) {
    if (spawn_worker(w->worker_id, w->port) != 0) {
      LOG_ERROR("Failed to respawn worker %" PRIu8, w->worker_id);
    }
  }
  uv_close((uv_handle_t *)t, (uv_close_cb)free);
}

static void on_exit_cb(uv_process_t *handle, int64_t exit_status, int term_signal) {
  worker_process_t *worker = (worker_process_t *)handle->data;

  if (!worker || !ecewo_cluster.is_master)
    return;

  uint8_t worker_id = worker->worker_id;
  time_t uptime = time(NULL) - worker->start_time;

  worker->status = WORKER_CRASHED;
  worker->exit_status = (int)exit_status;
  worker->pid = 0;

  bool is_crash = !ecewo_cluster.shutdown_requested && !ecewo_cluster.graceful_restart_requested && exit_status != 0;

  if (term_signal == SIGTERM || term_signal == SIGINT)
    is_crash = false;

  if (is_crash) {
    LOG_ERROR("Worker %" PRIu8 " crashed after %ld seconds (exit: %d, signal: %d)",
              worker_id, (long)uptime, (int)exit_status, term_signal);
  } else {
    LOG_DEBUG("Worker %" PRIu8 " exited after %ld seconds (exit: %d)",
              worker_id, (long)uptime, (int)exit_status);
  }

  if (ecewo_cluster.config.on_exit)
    ecewo_cluster.config.on_exit(worker_id, (int)exit_status, is_crash);

  uv_close((uv_handle_t *)handle, NULL);

  bool should_respawn = false;

  if (ecewo_cluster.graceful_restart_requested || (is_crash && should_respawn_worker(worker))) {
    should_respawn = true;
    worker->status = WORKER_RESPAWNING;
  } else {
    worker->status = worker->respawn_disabled ? WORKER_DISABLED : WORKER_CRASHED;
  }

  if (should_respawn) {
    ecewo_cluster.total_restarts++;

    uint64_t delay = ecewo_cluster.config.worker_respawn_delay_ms;

    uv_timer_t *timer = malloc(sizeof(uv_timer_t));
    if (timer) {
      uv_timer_init(uv_default_loop(), timer);
      timer->data = worker;
      uv_timer_start(timer, respawn_timer_cb, delay, 0);
    } else {
      if (spawn_worker(worker_id, worker->port) != 0) {
        LOG_ERROR("Failed to respawn worker %" PRIu8, worker_id);
      }
    }
  }

  if (ecewo_cluster.graceful_restart_requested) {
    bool all_active = true;
    for (uint8_t i = 0; i < ecewo_cluster.worker_count; i++) {
      if (ecewo_cluster.workers[i].status != WORKER_ACTIVE) {
        all_active = false;
        break;
      }
    }

    if (all_active) {
      ecewo_cluster.graceful_restart_requested = false;
      LOG_INFO("Graceful restart completed");
    }
  }
}

static void on_sigterm(uv_signal_t *handle, int signum) {
  (void)handle;
  (void)signum;

  if (!ecewo_cluster.is_master || ecewo_cluster.shutdown_requested)
    return;

  LOG_INFO("Shutdown requested (SIGTERM)");
  ecewo_cluster.shutdown_requested = true;

  for (uint8_t i = 0; i < ecewo_cluster.worker_count; i++) {
    if (ecewo_cluster.workers[i].status == WORKER_ACTIVE) {
      ecewo_cluster.workers[i].status = WORKER_STOPPING;
      uv_process_kill(&ecewo_cluster.workers[i].handle, WORKER_STOP_SIGNAL);
    }
  }
}

static void on_sigint(uv_signal_t *handle, int signum) {
  (void)handle;
  (void)signum;

  if (!ecewo_cluster.is_master || ecewo_cluster.shutdown_requested)
    return;

  LOG_INFO("Shutdown requested (SIGINT)");
  ecewo_cluster.shutdown_requested = true;

  for (uint8_t i = 0; i < ecewo_cluster.worker_count; i++) {
    if (ecewo_cluster.workers[i].status == WORKER_ACTIVE) {
      ecewo_cluster.workers[i].status = WORKER_STOPPING;
      uv_process_kill(&ecewo_cluster.workers[i].handle, WORKER_STOP_SIGNAL);
    }
  }
}

static void on_sigusr2(uv_signal_t *handle, int signum) {
  (void)handle;
  (void)signum;

  if (!ecewo_cluster.is_master)
    return;

  if (ecewo_cluster.graceful_restart_requested || ecewo_cluster.shutdown_requested) {
    LOG_INFO("Graceful restart already in progress");
    return;
  }

  LOG_INFO("Graceful restart requested (SIGUSR2)");
  cluster_graceful_restart();
}

static void setup_signal_handlers(void) {
  if (!ecewo_cluster.is_master)
    return;

  uv_signal_init(uv_default_loop(), &ecewo_cluster.sigterm);
  uv_signal_start(&ecewo_cluster.sigterm, on_sigterm, SIGTERM);

  uv_signal_init(uv_default_loop(), &ecewo_cluster.sigint);
  uv_signal_start(&ecewo_cluster.sigint, on_sigint, SIGINT);

  uv_signal_init(uv_default_loop(), &ecewo_cluster.sigusr2);
  uv_signal_start(&ecewo_cluster.sigusr2, on_sigusr2, SIGUSR2);
}

static void cleanup_signal_handlers(void) {
  if (!uv_is_closing((uv_handle_t *)&ecewo_cluster.sigterm)) {
    uv_signal_stop(&ecewo_cluster.sigterm);
    uv_close((uv_handle_t *)&ecewo_cluster.sigterm, NULL);
  }

  if (!uv_is_closing((uv_handle_t *)&ecewo_cluster.sigint)) {
    uv_signal_stop(&ecewo_cluster.sigint);
    uv_close((uv_handle_t *)&ecewo_cluster.sigint, NULL);
  }

  if (!uv_is_closing((uv_handle_t *)&ecewo_cluster.sigusr2)) {
    uv_signal_stop(&ecewo_cluster.sigusr2);
    uv_close((uv_handle_t *)&ecewo_cluster.sigusr2, NULL);
  }
}

static void close_handle_cb(uv_handle_t *handle, void *arg) {
  (void)arg;

  if (uv_is_closing(handle))
    return;

  if (handle->type == UV_SIGNAL) {
    uv_signal_stop((uv_signal_t *)handle);
  } else if (handle->type == UV_TIMER) {
    uv_timer_stop((uv_timer_t *)handle);
    uv_close(handle, (uv_close_cb)free);
    return;
  }

  uv_close(handle, NULL);
}

static void cluster_cleanup(void) {
  if (!ecewo_cluster.initialized)
    return;

  if (ecewo_cluster.workers) {
    free(ecewo_cluster.workers);
    ecewo_cluster.workers = NULL;
  }

  cleanup_original_args();

  uv_loop_t *loop = uv_default_loop();
  cleanup_signal_handlers();
  uv_walk(loop, close_handle_cb, NULL);

  // Run loop to process close callbacks
  int iterations = 0;
  while (uv_loop_alive(loop) && iterations < 50) {
    uv_run(loop, UV_RUN_NOWAIT);
    uv_sleep(10);
    iterations++;
  }

  int result = uv_loop_close(loop);
  if (result != 0) {
    uv_walk(loop, close_handle_cb, NULL);
    uv_run(loop, UV_RUN_NOWAIT);
    uv_loop_close(loop);
  }

  ecewo_cluster.initialized = false;
}

bool cluster_init(const Cluster *options, int argc, char **argv) {
  if (ecewo_cluster.initialized) {
    LOG_ERROR("Cluster already initialized");
    return false;
  }

  if (!options || options->port == 0 || !argv) {
    LOG_ERROR("Invalid cluster configuration (port required)");
    return false;
  }

  save_original_args(argc, argv);
  apply_config(options);

  ecewo_cluster.base_port = options->port;

  char **args = uv_setup_args(argc, argv);

  ecewo_cluster.is_master = true;
  ecewo_cluster.worker_id = 0;

  // Check if this is a worker process
  for (int i = 1; args && i < argc - 2; i++) {
    if (strcmp(args[i], "--cluster-worker") == 0) {
      ecewo_cluster.is_master = false;

      char *endptr;
      long worker_id_long = strtol(args[i + 1], &endptr, 10);
      long worker_port_long = strtol(args[i + 2], &endptr, 10);

      ecewo_cluster.worker_id = (uint8_t)worker_id_long;
      ecewo_cluster.worker_port = (uint16_t)worker_port_long;

      char title[64];
      snprintf(title, sizeof(title), "ecewo:worker-%" PRIu8, ecewo_cluster.worker_id);
      uv_set_process_title(title);

      ecewo_cluster.initialized = true;
      return false; // Worker process
    }
  }

  // Master process
  static bool cleanup_registered = false;
  if (!cleanup_registered) {
    atexit(cluster_cleanup);
    cleanup_registered = true;
  }

  uv_set_process_title("ecewo:master");

  setup_signal_handlers();

  ecewo_cluster.workers = calloc(ecewo_cluster.worker_count, sizeof(worker_process_t));
  if (!ecewo_cluster.workers) {
    LOG_ERROR("Failed to allocate worker array");
    cleanup_original_args();
    return false;
  }

  // Spawn workers with delays
  int failed_count = 0;
  for (uint8_t i = 0; i < ecewo_cluster.worker_count; i++) {
    uint16_t port = ecewo_cluster.base_port; // All workers use same port (SO_REUSEPORT)

    if (spawn_worker(i, port) != 0) {
      LOG_ERROR("Failed to spawn worker %" PRIu8, i);
      failed_count++;

      if (failed_count > ecewo_cluster.worker_count / 2) {
        LOG_ERROR("Too many spawn failures, aborting");
        cleanup_original_args();
        return false;
      }
    }

    if (i < ecewo_cluster.worker_count - 1) {
      uv_sleep(ecewo_cluster.config.worker_startup_delay_ms);
    }
  }

  uv_sleep(500); // Final delay for workers to fully start

  ecewo_cluster.initialized = true;

  printf("Server listening on http://localhost:%" PRIu16 " (Cluster: %d CPUs)\n",
         ecewo_cluster.base_port, ecewo_cluster.worker_count);

  return true; // Master process
}

uint16_t cluster_get_port(void) {
  if (!ecewo_cluster.initialized)
    return 0;

  if (ecewo_cluster.is_master)
    return ecewo_cluster.base_port;

  return ecewo_cluster.worker_port;
}

bool cluster_is_master(void) {
  return ecewo_cluster.initialized && ecewo_cluster.is_master;
}

bool cluster_is_worker(void) {
  return ecewo_cluster.initialized && !ecewo_cluster.is_master;
}

uint8_t cluster_worker_id(void) {
  return ecewo_cluster.worker_id;
}

uint8_t cluster_worker_count(void) {
  return ecewo_cluster.worker_count;
}

void cluster_signal_workers(int signum) {
  if (!ecewo_cluster.is_master || !ecewo_cluster.initialized) {
    LOG_ERROR("Only master can signal workers");
    return;
  }

  if (signum <= 0 || signum >= 32) {
    LOG_ERROR("Invalid signal number: %d", signum);
    return;
  }

  for (uint8_t i = 0; i < ecewo_cluster.worker_count; i++) {
    if (ecewo_cluster.workers[i].status == WORKER_ACTIVE) {
      uv_process_kill(&ecewo_cluster.workers[i].handle, signum);
    }
  }
}

void cluster_graceful_restart(void) {
  if (!ecewo_cluster.is_master || !ecewo_cluster.initialized) {
    LOG_ERROR("Only master can trigger graceful restart");
    return;
  }

  if (ecewo_cluster.graceful_restart_requested || ecewo_cluster.shutdown_requested) {
    LOG_INFO("Restart already in progress");
    return;
  }

  LOG_INFO("Starting graceful restart");
  ecewo_cluster.graceful_restart_requested = true;

  for (uint8_t i = 0; i < ecewo_cluster.worker_count; i++) {
    if (ecewo_cluster.workers[i].status == WORKER_ACTIVE) {
      uv_process_kill(&ecewo_cluster.workers[i].handle, SIGTERM);
    }
  }
}

void cluster_wait_workers(void) {
  if (!ecewo_cluster.is_master || !ecewo_cluster.initialized) {
    LOG_ERROR("Only master can wait for workers");
    return;
  }

  uv_loop_t *loop = uv_default_loop();
  uint64_t shutdown_start_time = 0;

  while (1) {
    bool any_active = false;
    for (uint8_t i = 0; i < ecewo_cluster.worker_count; i++) {
      if (ecewo_cluster.workers[i].status == WORKER_ACTIVE || ecewo_cluster.workers[i].status == WORKER_STOPPING || ecewo_cluster.workers[i].status == WORKER_RESPAWNING) {
        any_active = true;
        break;
      }
    }

    if (!any_active)
      break;

    if (ecewo_cluster.shutdown_requested) {
      if (shutdown_start_time == 0)
        shutdown_start_time = uv_now(loop);

      uint64_t elapsed = uv_now(loop) - shutdown_start_time;

      if (elapsed > ecewo_cluster.config.shutdown_timeout_ms) {
        LOG_INFO("Shutdown timeout - force killing remaining workers");
        for (uint8_t i = 0; i < ecewo_cluster.worker_count; i++) {
          if (ecewo_cluster.workers[i].status == WORKER_ACTIVE || ecewo_cluster.workers[i].status == WORKER_STOPPING) {
            uv_process_kill(&ecewo_cluster.workers[i].handle, SIGKILL);
          }
        }
        break;
      }
    }

    uv_run(loop, UV_RUN_ONCE);

    if (!uv_loop_alive(loop) && any_active)
      break;
  }

  cluster_cleanup();
}

static long count_physical_cores(void) {
  int max_cpu = sysconf(_SC_NPROCESSORS_ONLN);
  if (max_cpu <= 0 || max_cpu > 1024)
    return -1;

  bool core_seen[1024] = { 0 };
  int unique_cores = 0;

  for (int cpu = 0; cpu < max_cpu; cpu++) {
    char path[256];
    snprintf(path, sizeof(path),
             "/sys/devices/system/cpu/cpu%d/topology/core_id", cpu);

    uv_fs_t open_req;
    int fd = uv_fs_open(NULL, &open_req, path, O_RDONLY, 0, NULL);
    uv_fs_req_cleanup(&open_req);

    if (fd >= 0) {
      char buf[32];
      uv_buf_t uv_buf = uv_buf_init(buf, sizeof(buf) - 1);

      uv_fs_t read_req;
      int nread = uv_fs_read(NULL, &read_req, fd, &uv_buf, 1, 0, NULL);
      uv_fs_req_cleanup(&read_req);

      if (nread > 0) {
        buf[nread] = '\0';
        char *endptr;
        errno = 0;
        long core_id_long = strtol(buf, &endptr, 10);

        // Check for conversion errors
        if (errno == 0 && endptr != buf && *endptr == '\0') {
          int core_id = (int)core_id_long;
          if (core_id >= 0 && core_id < 1024 && !core_seen[core_id]) {
            core_seen[core_id] = true;
            unique_cores++;
          }
        }
      }

      uv_fs_t close_req;
      uv_fs_close(NULL, &close_req, fd, NULL);
      uv_fs_req_cleanup(&close_req);
    }
  }

  return unique_cores > 0 ? unique_cores : -1;
}

uint8_t cluster_cpus_physical(void) {
  long logical_cores = sysconf(_SC_NPROCESSORS_ONLN);
  if (logical_cores > 255)
    logical_cores = 255;
  if (logical_cores < 1)
    return 1;

  long physical_cores = count_physical_cores();

  if (physical_cores > 0) {
    return (uint8_t)physical_cores;
  }

  // Fallback: detect hyperthreading
  physical_cores = logical_cores;

  uv_fs_t open_req;
  int fd = uv_fs_open(NULL, &open_req,
                      "/sys/devices/system/cpu/cpu0/topology/thread_siblings_list",
                      O_RDONLY, 0, NULL);
  uv_fs_req_cleanup(&open_req);

  if (fd >= 0) {
    char line[256];
    uv_buf_t buf = uv_buf_init(line, sizeof(line) - 1);

    uv_fs_t read_req;
    int nread = uv_fs_read(NULL, &read_req, fd, &buf, 1, 0, NULL);
    uv_fs_req_cleanup(&read_req);

    if (nread > 0) {
      line[nread] = '\0';
      if (nread > 0 && line[nread - 1] == '\n')
        line[nread - 1] = '\0';

      int sibling_count = 1;
      bool has_comma = false;
      bool has_dash = false;

      for (char *p = line; *p; p++) {
        if (*p == ',') {
          sibling_count++;
          has_comma = true;
        } else if (*p == '-') {
          has_dash = true;
        }
      }

      if (has_dash && !has_comma) {
        char *endptr;
        errno = 0;
        long start_long = strtol(line, &endptr, 10);

        if (errno == 0 && endptr != line && *endptr == '-') {
          errno = 0;
          long end_long = strtol(endptr + 1, &endptr, 10);

          if (errno == 0 && (*endptr == '\0' || *endptr == '\n')) {
            int start = (int)start_long;
            int end = (int)end_long;
            if (start >= 0 && end >= start) {
              sibling_count = end - start + 1;
            }
          }
        }
      }

      if (sibling_count > 1) {
        physical_cores = logical_cores / sibling_count;
      }
    }

    uv_fs_t close_req;
    uv_fs_close(NULL, &close_req, fd, NULL);
    uv_fs_req_cleanup(&close_req);
  }

  if (physical_cores < 1)
    physical_cores = 1;
  return (uint8_t)physical_cores;
}

uint8_t cluster_cpus(void) {
  unsigned int parallelism = uv_available_parallelism();
  if (parallelism > 255)
    return 255;
  return (uint8_t)parallelism;
}

int cluster_get_worker_stats(uint8_t worker_id, worker_stats_t *stats) {
  if (!ecewo_cluster.is_master || !ecewo_cluster.initialized) {
    LOG_ERROR("Only master can get worker stats");
    return 0;
  }

  if (worker_id >= ecewo_cluster.worker_count || !stats) {
    return 0;
  }

  worker_process_t *worker = &ecewo_cluster.workers[worker_id];

  stats->worker_id = worker->worker_id;
  stats->pid = worker->pid;
  stats->port = worker->port;
  stats->status = worker->status;
  stats->start_time = worker->start_time;
  stats->uptime_seconds = worker->status == WORKER_ACTIVE
      ? (time(NULL) - worker->start_time)
      : 0;
  stats->exit_status = worker->exit_status;
  stats->crash_count = worker->restart_count;
  stats->respawn_disabled = worker->respawn_disabled;

  return 1;
}

int cluster_get_stats(cluster_stats_t *stats) {
  if (!ecewo_cluster.is_master || !ecewo_cluster.initialized) {
    LOG_ERROR("Only master can get cluster stats");
    return 0;
  }

  if (!stats)
    return 0;

  stats->total_workers = ecewo_cluster.worker_count;
  stats->active_workers = 0;
  stats->crashed_workers = 0;
  stats->disabled_workers = 0;
  stats->shutdown_requested = ecewo_cluster.shutdown_requested;
  stats->restart_requested = ecewo_cluster.graceful_restart_requested;
  stats->total_restarts = ecewo_cluster.total_restarts;

  for (uint8_t i = 0; i < ecewo_cluster.worker_count; i++) {
    worker_process_t *worker = &ecewo_cluster.workers[i];

    if (worker->status == WORKER_ACTIVE)
      stats->active_workers++;
    else if (worker->status == WORKER_CRASHED)
      stats->crashed_workers++;
    else if (worker->status == WORKER_DISABLED)
      stats->disabled_workers++;
  }

  return 1;
}

int cluster_get_all_workers(worker_stats_t *stats_array, int array_size) {
  if (!ecewo_cluster.is_master || !ecewo_cluster.initialized) {
    LOG_ERROR("Only master can get all worker stats");
    return 0;
  }

  if (!stats_array || array_size <= 0)
    return 0;

  int count = ecewo_cluster.worker_count < array_size
      ? ecewo_cluster.worker_count
      : array_size;

  for (int i = 0; i < count; i++) {
    cluster_get_worker_stats(i, &stats_array[i]);
  }

  return count;
}
