#ifndef __WEB__
#define __WEB__

#include "esp_http_server.h"
#define ESP_VFS_PATH_MAX  128
#define MIN(A,B)  ((A<B)?A:B)

class WEBSERVER {
    private:
        int cnt=0;
        httpd_uri_t  vet_handlers[10];
    public:

        void addHandler(const char *rota, httpd_method_t codigo, esp_err_t (*f)(httpd_req_t*) );
        void start(void);


};


#endif