#include "web.h"
#include <stdio.h>






void WEBSERVER::addHandler(const char *rota, httpd_method_t codigo, esp_err_t (*f)(httpd_req_t*) )
{
   //strcpy(vet_handlers[cnt].uri,(char *)rota);
    vet_handlers[cnt].uri=rota;
    vet_handlers[cnt].method = codigo;
    vet_handlers[cnt].handler = f;
    cnt++;
}

void WEBSERVER::start(void)
{
    //start webserver
        httpd_config_t config = HTTPD_DEFAULT_CONFIG();
        config.uri_match_fn = httpd_uri_match_wildcard;

    httpd_handle_t server = NULL;



    if (httpd_start(&server, &config) == ESP_OK) {

        for (int x=0;x<cnt;x++ )
        {
            httpd_register_uri_handler(server,&vet_handlers[x]);
        }



    }else printf("Erro criando rotas\n");
    

}