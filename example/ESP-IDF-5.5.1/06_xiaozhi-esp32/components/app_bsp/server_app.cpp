#include <stdio.h>
#include <esp_check.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include "server_app.h"
#include "sdcard_bsp.h"
#include "button_bsp.h"
#include "mdns.h"
#include "user_app.h"

#include "lwip/udp.h"
#include "lwip/ip_addr.h"
#include "lwip/pbuf.h"


static const char *TAG = "server_bsp";

#define ServerPort_MIN(x, y) ((x < y) ? (x) : (y))
#define READ_LEN_MAX (10 * 1024) // Buffer area for receiving data
#define SEND_LEN_MAX (64 * 1024)  // Data for sending response

#define BSP_ESP_WIFI_SSID "esp_network"
#define BSP_ESP_WIFI_PASS "1234567890"
#define BSP_ESP_WIFI_CHANNEL 1
#define BSP_MAX_STA_CONN 4

#define DNS_PORT 53

EventGroupHandle_t ServerPortGroups;
static CustomSDPort *SDPort_ = NULL;
static uint8_t netMode = 0;   //Default AP mode
const char staresp[] = "1";
const char apresp[] = "0";

/*callback fun*/
esp_err_t static_resource_unified_handler(httpd_req_t *req);
esp_err_t receive_data_redirect_handler(httpd_req_t *req);
esp_err_t unknown_uri_handler(httpd_req_t *req);
void sta_wifi_event_callback(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void ap_wifi_event_callback(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

static void dns_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                     const ip_addr_t *addr, u16_t port)
{
    if (!p) return;

    uint8_t *query = (uint8_t *)p->payload;

    struct pbuf *resp = pbuf_alloc(PBUF_TRANSPORT, p->tot_len + 16, PBUF_RAM);
    memcpy(resp->payload, query, p->tot_len);

    uint8_t *dns = (uint8_t *)resp->payload;

    // 设置响应标志
    dns[2] = 0x81;
    dns[3] = 0x80;

    // answer 数量 = 1
    dns[7] = 1;

    int len = p->tot_len;

    // Answer section
    dns[len++] = 0xC0; dns[len++] = 0x0C; // name pointer
    dns[len++] = 0x00; dns[len++] = 0x01; // type A
    dns[len++] = 0x00; dns[len++] = 0x01; // class IN
    dns[len++] = 0x00; dns[len++] = 0x00;
    dns[len++] = 0x00; dns[len++] = 0x3C;
    dns[len++] = 0x00; dns[len++] = 0x04;

    // 返回 192.168.4.1
    dns[len++] = 192;
    dns[len++] = 168;
    dns[len++] = 4;
    dns[len++] = 1;

    resp->len = resp->tot_len = len;

    udp_sendto(pcb, resp, addr, port);

    pbuf_free(resp);
    pbuf_free(p);
}

void dns_server_start(void)
{
    struct udp_pcb *pcb = udp_new();
    udp_bind(pcb, IP_ADDR_ANY, DNS_PORT);
    udp_recv(pcb, dns_recv, NULL);
}

static void customfree(char *res) {
    if(NULL != res) {
        heap_caps_free(res);
        res = NULL;
    }
}

void ap_wifi_event_callback(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        xEventGroupSetBits(ServerPortGroups, (0x01UL << 4));
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        xEventGroupSetBits(ServerPortGroups, (0x01UL << 5));
    }
}

void sta_wifi_event_callback(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_id == WIFI_EVENT_STA_START) {
        ESP_ERROR_CHECK(esp_wifi_connect());
    } else if (event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(ServerPortGroups, GroupBit6); 
    } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGE("wifi", "WiFi disconnected, trying to reconnect...");
        xEventGroupSetBits(ServerPortGroups, GroupBit5); 
    }
}

esp_err_t static_resource_unified_handler(httpd_req_t *req) {
    char  *resp_str      = NULL;
    size_t str_respLen   = 0;
    size_t str_len       = 0;
    const char *uri = req->uri;                                     // The desired URI
    ESP_LOGI(TAG, "Return directly URL:%s",uri);

    if(strstr(uri,"index.html")) { // /index.html
        resp_str      = (char *) heap_caps_malloc(SEND_LEN_MAX + 1, MALLOC_CAP_SPIRAM);
        httpd_resp_set_type(req, "text/html");
        while(1) {
            if(NetWorkMode == 1){
                str_len = SDPort_->SDPort_ReadOffset("/sdcard/HTML/STA/index.html",resp_str,SEND_LEN_MAX,str_respLen);
            } else {
                str_len = SDPort_->SDPort_ReadOffset("/sdcard/HTML/AP/index.html",resp_str,SEND_LEN_MAX,str_respLen);
            }
            if (str_len) {
                httpd_resp_send_chunk(req, resp_str, str_len); 
                str_respLen += str_len;
            } else {
                break;
            }
        }
    } else if(strstr(uri,"bootstrap.min.css")) { // /bootstrap.min.css
        resp_str      = (char *) heap_caps_malloc(SEND_LEN_MAX + 1, MALLOC_CAP_SPIRAM);
        httpd_resp_set_type(req, "text/css");
        while(1) {
            if(NetWorkMode == 1){
                str_len = SDPort_->SDPort_ReadOffset("/sdcard/HTML/STA/bootstrap.min.css",resp_str,SEND_LEN_MAX,str_respLen);
            } else {
                str_len = SDPort_->SDPort_ReadOffset("/sdcard/HTML/AP/bootstrap.min.css",resp_str,SEND_LEN_MAX,str_respLen);
            }
            // str_len = SDPort_->SDPort_ReadOffset("/sdcard/03_sys_ap_html/bootstrap.min.css",resp_str,SEND_LEN_MAX,str_respLen);
            if (str_len) {
                httpd_resp_send_chunk(req, resp_str, str_len); 
                str_respLen += str_len;
            } else {
                break;
            }
        }
    } else if(strstr(uri,"styles.min.css")) { // /styles.min.css
        resp_str      = (char *) heap_caps_malloc(SEND_LEN_MAX + 1, MALLOC_CAP_SPIRAM);
        httpd_resp_set_type(req, "text/css");
        while(1) {
            if(NetWorkMode == 1){
                str_len = SDPort_->SDPort_ReadOffset("/sdcard/HTML/STA/styles.min.css",resp_str,SEND_LEN_MAX,str_respLen);
            } else {
                str_len = SDPort_->SDPort_ReadOffset("/sdcard/HTML/AP/styles.min.css",resp_str,SEND_LEN_MAX,str_respLen);
            }
            // str_len = SDPort_->SDPort_ReadOffset("/sdcard/03_sys_ap_html/styles.min.css",resp_str,SEND_LEN_MAX,str_respLen);
            if (str_len) {
                httpd_resp_send_chunk(req, resp_str, str_len); 
                str_respLen += str_len;
            } else {
                break;
            }
        }
    } else if(strstr(uri,"placeholder.svg")) { // /placeholder.svg
        resp_str      = (char *) heap_caps_malloc(SEND_LEN_MAX + 1, MALLOC_CAP_SPIRAM);
        httpd_resp_set_type(req, "image/svg+xml");
        while(1) {
            if(NetWorkMode == 1){
                str_len = SDPort_->SDPort_ReadOffset("/sdcard/HTML/STA/placeholder.svg",resp_str,SEND_LEN_MAX,str_respLen);
            } else {
                str_len = SDPort_->SDPort_ReadOffset("/sdcard/HTML/AP/placeholder.svg",resp_str,SEND_LEN_MAX,str_respLen);
            }
            // str_len = SDPort_->SDPort_ReadOffset("/sdcard/03_sys_ap_html/placeholder.svg",resp_str,SEND_LEN_MAX,str_respLen);
            if (str_len) {
                httpd_resp_send_chunk(req, resp_str, str_len); 
                str_respLen += str_len;
            } else {
                break;
            }
        }
    } else if(strstr(uri,"bootstrap.min.js")) { // /bootstrap.min.js
        resp_str      = (char *) heap_caps_malloc(SEND_LEN_MAX + 1, MALLOC_CAP_SPIRAM);
        httpd_resp_set_type(req, "text/javascript");
        while(1) {
            if(NetWorkMode == 1){
                str_len = SDPort_->SDPort_ReadOffset("/sdcard/HTML/STA/bootstrap.min.js",resp_str,SEND_LEN_MAX,str_respLen);
            } else {
                str_len = SDPort_->SDPort_ReadOffset("/sdcard/HTML/AP/bootstrap.min.js",resp_str,SEND_LEN_MAX,str_respLen);
            }
            // str_len = SDPort_->SDPort_ReadOffset("/sdcard/03_sys_ap_html/bootstrap.min.js",resp_str,SEND_LEN_MAX,str_respLen);
            if (str_len) {
                httpd_resp_send_chunk(req, resp_str, str_len); 
                str_respLen += str_len;
            } else {
                break;
            }
        }
    } else if(strstr(uri,"script.min.js")) { // /script.min.js
        resp_str      = (char *) heap_caps_malloc(SEND_LEN_MAX + 1, MALLOC_CAP_SPIRAM);
        httpd_resp_set_type(req, "text/javascript");
        while(1) {
            if(NetWorkMode == 1){
                str_len = SDPort_->SDPort_ReadOffset("/sdcard/HTML/STA/script.min.js",resp_str,SEND_LEN_MAX,str_respLen);
            } else {
                str_len = SDPort_->SDPort_ReadOffset("/sdcard/HTML/AP/script.min.js",resp_str,SEND_LEN_MAX,str_respLen);
            }
            // str_len = SDPort_->SDPort_ReadOffset("/sdcard/03_sys_ap_html/script.min.js",resp_str,SEND_LEN_MAX,str_respLen);
            if (str_len) {
                httpd_resp_send_chunk(req, resp_str, str_len); 
                str_respLen += str_len;
            } else {
                break;
            }
        }
    } else if(strstr(uri,"/NetWorkStatus")) {
        if(Get_CurrentlyNetworkMode()) {
            httpd_resp_send_chunk(req, staresp, HTTPD_RESP_USE_STRLEN);
        } else {
            httpd_resp_send_chunk(req, apresp, HTTPD_RESP_USE_STRLEN);
        }
    } else {     /*留给unknown_uri_handler处理*/
        customfree(resp_str);
        return ESP_FAIL;
    }

    httpd_resp_send_chunk(req, NULL, 0);                    // Send empty data to indicate completion of transmission
    customfree(resp_str);
    return ESP_OK;
}

esp_err_t receive_data_redirect_handler(httpd_req_t *req) {
    size_t total = req->content_len;
    uint8_t  status_flag = 0;
    if (total > IMG_BYTES+1) total = IMG_BYTES+1; 
    ESP_LOGI(TAG, "The URI of the user's POST is:%s,byte:%d", req->uri, total);
    xEventGroupSetBits(ServerPortGroups, (0x1UL << 0)); 

    int ret = httpd_req_recv(req, (char *)&status_flag, 1);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Transmission failed. Please resend");
        return ESP_FAIL;
    }

    ret = 0;
    size_t received = 0;
    total = total - 1;
    while (received < total) {
        ret = httpd_req_recv(req, (char *)img_buf + received, total - received);
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) continue;
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Transmission failed. Please resend");
            return ESP_FAIL;
        }
        received += ret;
    }
    xEventGroupSetBits(ServerPortGroups, (0x1UL << 1)); 
    
    if(status_flag & 0x01){
        AP_OR_STA_flag = 1;
        netMode = 1;
    } else {
        AP_OR_STA_flag = 0;
        netMode = 0;
    }

    if(status_flag & 0x10){
        rotary_flag = 1;
    } else {
        rotary_flag = 0;
    }
    // 0= vertical graph (1200*1600), 1= horizontal graph (1600*1200)
    // ESP_LOGI(TAG, "Image orientation flag: %d", flag);

    httpd_resp_send(req, "successfully upload", strlen("successfully upload"));
    xEventGroupSetBits(ServerPortGroups, (0x1UL << 2));

    // heap_caps_free(img_buf);
    return ESP_OK;
}

// esp_err_t receive_data_redirect_handler(httpd_req_t *req) {
//     char       *buf        = (char *) heap_caps_malloc(READ_LEN_MAX + 1, MALLOC_CAP_SPIRAM);
//     size_t      sdcard_len = 0;
//     size_t      remaining  = req->content_len;
//     const char *uri        = req->uri;
//     size_t      ret;
//     uint8_t     timeoutive = 0;      /*Expiry timeout and automatic logout*/
//     bool        is_NetworkMode = 1;  /*Handling the flag bits for the ESP32 mode*/
//     ESP_LOGW("TAG", "Receive url:%s,byte:%d", uri, remaining);
//     xEventGroupSetBits(ServerPortGroups, (0x1UL << 0)); 
//     SDPort_->SDPort_WriteOffset("/sdcard/02_sys_ap_img/user_send.bmp", NULL, 0, 0);
//     while (remaining > 0) {
//         if ((ret = httpd_req_recv(req, buf, ServerPort_MIN(remaining, READ_LEN_MAX))) <= 0) {
//             if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
//                 timeoutive++;
//                 if(timeoutive == 10) {
//                     httpd_resp_send_408(req);
//                     customfree(buf);
//                     return ESP_FAIL;
//                 }
//                 continue;
//             }
//             customfree(buf);
//             return ESP_FAIL;
//         }
//         size_t req_len = 0;
//         if(is_NetworkMode) {
//             is_NetworkMode = 0;
//             netMode = buf[0];
//             req_len = SDPort_->SDPort_WriteOffset("/sdcard/02_sys_ap_img/user_send.bmp", (buf + 1), (ret - 1), 1);
//             sdcard_len += req_len; // Final comparison result
//             remaining -= ret;      // Subtract the data that has already been received
//         } else {
//             req_len = SDPort_->SDPort_WriteOffset("/sdcard/02_sys_ap_img/user_send.bmp", buf, ret, 1);
//             sdcard_len += req_len; // Final comparison result
//             remaining -= ret;      // Subtract the data that has already been received
//         }
//     }
//     xEventGroupSetBits(ServerPortGroups, (0x1UL << 1)); 
//     if ((sdcard_len + 1) == req->content_len) {
//         httpd_resp_send(req, "Data verification successful", strlen("Data verification successful"));
//         xEventGroupSetBits(ServerPortGroups, (0x1UL << 2));
//     } else {
//         httpd_resp_send_408(req);
//         xEventGroupSetBits(ServerPortGroups, (0x1UL << 3));
//     } 
//     ESP_LOGW(TAG,"netMode:%d",netMode);
//     customfree(buf);
//     return ESP_OK;
// }

esp_err_t unknown_uri_handler(httpd_req_t *req) {
    const char *uri = req->uri; 
    httpd_method_t req_method = (httpd_method_t)req->method;
    if(req_method == HTTP_GET) {
        ESP_LOGW("err", "Request interface : GET,Return directly URL:%s", uri);
    } else if(req_method == HTTP_POST) {
        ESP_LOGW("err", "Request interface : POST,Return directly URL:%s", uri);
    }
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Resources do not exist");
    return ESP_OK;
}

void ServerPort_NetworkAPInit(void) {
    ServerPortGroups         = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    assert(esp_netif_create_default_wifi_ap());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &ap_wifi_event_callback,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_AP_STAIPASSIGNED,
                                                        &ap_wifi_event_callback,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {};
    snprintf((char *) wifi_config.ap.ssid, sizeof(wifi_config.ap.ssid), "%s", BSP_ESP_WIFI_SSID);
    snprintf((char *) wifi_config.ap.password, sizeof(wifi_config.ap.password), "%s", BSP_ESP_WIFI_PASS);
    wifi_config.ap.channel        = BSP_ESP_WIFI_CHANNEL;
    wifi_config.ap.max_connection = BSP_MAX_STA_CONN;
    wifi_config.ap.authmode       = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI("network", "wifi_init_softap finished. SSID:%s password:%s channel:%d", BSP_ESP_WIFI_SSID, BSP_ESP_WIFI_PASS, BSP_ESP_WIFI_CHANNEL);
}

uint8_t ServerPort_NetworkSTAInit(wifi_credential_t creden) {
    ServerPortGroups         = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    assert(esp_netif_create_default_wifi_sta());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t Instance_WIFI_IP;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &sta_wifi_event_callback, NULL, &Instance_WIFI_IP);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &sta_wifi_event_callback, NULL, &Instance_WIFI_IP);
    
    wifi_config_t wifi_config = {};
    strcpy((char *) wifi_config.sta.ssid, creden.ssid);
    strcpy((char *) wifi_config.sta.password, creden.password);
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    EventBits_t even = xEventGroupWaitBits(ServerPortGroups, (GroupBit6), pdTRUE, pdFALSE, pdMS_TO_TICKS(8000));
    if(even & GroupBit6) {
        ESP_LOGW(TAG, "WiFi connected successfully");
        return 1;
    } else {
        ESP_LOGE(TAG, "WiFi connection timed out");
        return 0;
    }
}

void ServerPort_init(CustomSDPort *SDPort) {
    if(SDPort_ == NULL) {
        SDPort_ = SDPort;
    }
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn   = httpd_uri_match_wildcard; /*Wildcard enabling*/
    ESP_ERROR_CHECK(httpd_start(&server, &config));

    /*Event callback function*/
    httpd_uri_t uri_config = {};
    uri_config.uri         = "/*";
    uri_config.method      = HTTP_GET;
    uri_config.handler     = static_resource_unified_handler;
    uri_config.user_ctx    = NULL;
    httpd_register_uri_handler(server, &uri_config);
    
    uri_config.uri         = "/dataUP";
    uri_config.method      = HTTP_POST;
    uri_config.handler     = receive_data_redirect_handler;
    uri_config.user_ctx    = NULL;
    httpd_register_uri_handler(server, &uri_config);
    
    uri_config.uri         = "/*"; // Match all URLs that have not been handled by other handlers
    uri_config.method      = (httpd_method_t)(HTTP_GET | HTTP_POST);
    uri_config.handler     = unknown_uri_handler; // Callback for returning a 404 response
    uri_config.user_ctx    = NULL;
    httpd_register_uri_handler(server, &uri_config);
}

void ServerPort_SetNetworkSleep(void) {
    esp_wifi_stop();
    esp_wifi_deinit();
    vTaskDelay(pdMS_TO_TICKS(500));
}

uint8_t Get_NetworkMode(void) {
    return netMode;
}

void Mdns_init_config(void) {
    esp_err_t err = mdns_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "MDNS Init failed: %s", esp_err_to_name(err));
        return;
    }

    mdns_hostname_set("esp32-s3-photopainter");
    mdns_instance_name_set("ESP32-S3 WebServer");
    mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);

    ESP_LOGW(TAG, "mDns配置完成,可通过 http://esp32-s3-photopainter.local/index.html 访问");
}