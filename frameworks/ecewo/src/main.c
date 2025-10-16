#include "ecewo.h"
#include "cJSON.h"
#include <stdlib.h>
#include <stdio.h>

void hello_world(Req *req, Res *res)
{
    cJSON *json = cJSON_CreateObject();

    cJSON_AddStringToObject(json, "message", "Hello World!");

    char *json_string = cJSON_PrintUnformatted(json);

    send_json(res, 200, json_string);

    cJSON_Delete(json);
    free(json_string);
}

int main()
{
    if (server_init() != SERVER_OK)
    {
        fprintf(stderr, "Failed to initialize server\n");
        return 1;
    }

    get("/", hello_world);

    if (server_listen(3000) != SERVER_OK)
    {
        fprintf(stderr, "Failed to start server\n");
        return 1;
    }

    server_run();
    return 0;
}
