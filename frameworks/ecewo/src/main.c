#include "server.h"
#include "ecewo.h"
#include "cJSON.h"

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
    init_router();
    get("/", hello_world);
    ecewo(3000);
    reset_router();
    return 0;
}
