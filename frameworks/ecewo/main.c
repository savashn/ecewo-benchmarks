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

  if (cluster_init(&config, argc, argv)) {
    cluster_wait_workers();
    return 0;
  }

  server_init();
  get("/", hello_world);
  server_listen(cluster_get_port());
  server_run();
  return 0;
}