#include "ecewo.h"
#include "ecewo-cluster.h"
#include <stdio.h>

void hello_world(Req *req, Res *res) {
  (void)req;
  send_json(res, OK, "{\"message\":\"Hello World!\"}");
}

int main(int argc, char *argv[]) {
  // uint8_t cpu_count = cluster_cpus_physical();
  uint8_t cpu_count = cluster_cpus();
  Cluster config = {
      .cpus = cpu_count,
      .respawn = true,
      .port = 3000
  };

  int cluster_result = cluster_init(&config, argc, argv);
  
  // If master process (returns 0), wait for workers
  if (cluster_result == 0) {
      cluster_wait_workers();
      return 0;
  }
  
  // If error (returns -1 and NOT a worker), exit
  if (cluster_result != -1 || !cluster_is_worker()) {
      return 1;
  }

  // Worker process continues here
  server_init();
  get("/", hello_world);
  server_listen(cluster_get_port());
  server_run();
  
  return 0;
}