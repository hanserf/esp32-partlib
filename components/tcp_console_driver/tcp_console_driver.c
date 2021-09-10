
/* Ported from ESP-IDF Console example
 * ESP-IDF Socket Console
 *
 * Copyright (C) 2021 Hans Erik Fjeld <hanse.fjeld@gmail.com>
 * EDIT Serf
 * BSD Licensed as described in the file LICENSE
 */

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <sys/unistd.h>
#include <sys/param.h>
#include <sys/select.h>
#include <string.h>
#include "esp_system.h"
#include "protocol_examples_common.h"
#include "esp_log.h"
#include "esp_console.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "cmd_decl.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "tcp_console_driver.h"
#include "app_console.h"
#include "esp_vfs.h"
#include "esp_vfs_dev.h"

#define CONSOLE_PORT 9170
#define MY_STDOUT_BUF_W 20
#define MY_LINEENDING "\r\n"
static const char* TAG = "tcp_console";

static struct {
    int socket_fd;
    int connection_fd;
    SemaphoreHandle_t new_tcp;
    QueueHandle_t new_client_queue;
    TaskHandle_t tcp_handler;
    TaskHandle_t console_task;
    log_callback_funtion_t log_callback;
}CONSOLE;

/* Console command history can be stored to and loaded from a file.
 * The easiest way to do this is to use FATFS filesystem on top of
 * wear_levelling library.
 */

static void initialize_console();
static int socket_init();
static void socket_maintainer_task(void *param);
/* Funtions for Redirecting STDOUT and STDIN to a TCP Socket stream */
static int console_writefn(void* cookie, const char* data, int size);
static int console_readfn(void* cookie, char* data, int size);
static void console_loop();
static void select_task(void *param);

#if CONFIG_STORE_HISTORY

#define MOUNT_PATH "/data"
#define HISTORY_PATH MOUNT_PATH "/history.txt"


static void initialize_filesystem()
{
    static wl_handle_t wl_handle;
    const esp_vfs_fat_mount_config_t mount_config = {
            .max_files = 4,
            .format_if_mount_failed = true
    };
    esp_err_t err = esp_vfs_fat_spiflash_mount(MOUNT_PATH, "storage", &mount_config, &wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
        return;
    }
}
#endif // CONFIG_STORE_HISTORY

static void initialize_nvs()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

static int console_writefn(void* cookie, const char* data, int size){
    int res = write(CONSOLE.connection_fd,data,size);
    if (res == -1) {
        /* Console connection closed */
        return 0;
    }
    return res;
}
static int console_readfn(void* cookie, char* data, int size)
{
    int res = read(CONSOLE.connection_fd,data,size);
    if (res == -1) {
        return 0;
    }
    return res;
}

void register_log_callback(log_callback_funtion_t a_callback){
    CONSOLE.log_callback = a_callback;
    ESP_ERROR_CHECK((CONSOLE.log_callback != NULL)?ESP_OK:ESP_FAIL);
}


static void initialize_console()
{   
    /* Redirect stdout and stdin to socket interface. Disable buffering on stdin */
    //setvbuf(fd, NULL, _IONBF, 0);
    /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
    /* Initialize the console */
    CONSOLE.new_tcp = xSemaphoreCreateBinary();
    ESP_ERROR_CHECK((CONSOLE.new_tcp != NULL)?ESP_OK:ESP_FAIL);    
    CONSOLE.new_client_queue = xQueueCreate(1, sizeof(CONSOLE.connection_fd)); 
    ESP_ERROR_CHECK((CONSOLE.log_callback != NULL)?ESP_OK:ESP_FAIL);
    CONSOLE.connection_fd = -1;
    CONSOLE.socket_fd = -1;
    CONSOLE.console_task = NULL;
    esp_console_config_t console_config = {
            .max_cmdline_args = 8,
            .max_cmdline_length = 256,
#if CONFIG_LOG_COLORS
            .hint_color = atoi(LOG_COLOR_CYAN)
#endif
    };
    ESP_ERROR_CHECK( esp_console_init(&console_config) );

    /* Configure linenoise line completion library */
    /* Enable multiline editing. If not set, long commands will scroll within
     * single line.
     */
    linenoiseSetMultiLine(1);

    /* Tell linenoise where to get command completions and hints */
    linenoiseSetCompletionCallback(&esp_console_get_completion);
    linenoiseSetHintsCallback((linenoiseHintsCallback*) &esp_console_get_hint);

    /* Set command history size */
    linenoiseHistorySetMaxLen(100);

#if CONFIG_STORE_HISTORY
    /* Load command history from filesystem */
    linenoiseHistoryLoad(HISTORY_PATH);
#endif
}

void start_console(my_bme280e_t *inner,my_bme280e_t *outer)
{
    initialize_nvs();
    
#if CONFIG_STORE_HISTORY
    initialize_filesystem();
#endif

    initialize_console();
    /* Register commands */
    esp_console_register_help_command();
    register_system();
    register_nvs();
    register_app_console(inner,outer);
    /* Register tasks */
    xTaskCreate( socket_maintainer_task, "ConsConn", configMINIMAL_STACK_SIZE*4, NULL, 1, &CONSOLE.tcp_handler );
    ESP_ERROR_CHECK((CONSOLE.tcp_handler!=NULL)?ESP_OK:ESP_FAIL);
    //xTaskCreate( console_loop, "ConsLoop", configMINIMAL_STACK_SIZE*4, NULL, 2, &CONSOLE.console_task );
    //ESP_ERROR_CHECK((CONSOLE.console_task!=NULL)?ESP_OK:ESP_FAIL);
    
}


static void socket_maintainer_task(void *param)
{
    int new_conn = -1;
    fd_set rfds;
    static const char* SA_MSG = "Console socket connection active";
    static const char* SNA_MSG = "Console socket connection closed";
    while(true){
        if(CONSOLE.socket_fd == -1){
            CONSOLE.log_callback(CB_WARNING, SNA_MSG);
            xTaskCreate( console_loop, "ConsLoop", configMINIMAL_STACK_SIZE*4, NULL, 2, &CONSOLE.console_task );
            ESP_ERROR_CHECK((CONSOLE.console_task != NULL)?ESP_OK:ESP_FAIL);
            ESP_ERROR_CHECK((xSemaphoreTake(CONSOLE.new_tcp,portMAX_DELAY) == pdPASS)?ESP_OK:ESP_FAIL);
            new_conn = socket_init();
            ESP_ERROR_CHECK((xQueueSend(CONSOLE.new_client_queue, &new_conn, portMAX_DELAY)==pdPASS)?ESP_OK:ESP_FAIL);      
        }
        else{
            CONSOLE.log_callback(CB_INFO,SA_MSG);
            vTaskDelay(pdMS_TO_TICKS(3000));            
        }
    }
}


static int socket_init()
{
    char addr_str[128];
    int addr_family;
    int ip_protocol;
    CONSOLE.socket_fd = -1;
    int conn_fd = -1;
    int flag = 1;
    char buf[100];
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(CONSOLE_PORT);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;
    inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);
    sprintf(buf,"Initializing Socket");
    CONSOLE.log_callback(CB_INFO, buf);
    CONSOLE.socket_fd = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (CONSOLE.socket_fd < 0) {
        sprintf(buf,"Unable to create socket: errno %d", errno);
        CONSOLE.log_callback(CB_ERROR, buf);
        return -1;
    }
    sprintf(buf,"Socket created");
    CONSOLE.log_callback(CB_INFO,buf);
    setsockopt(CONSOLE.socket_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    int err = bind(CONSOLE.socket_fd, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        sprintf(buf, "Socket unable to bind: errno %d", errno);
        CONSOLE.log_callback(CB_ERROR,buf);
        return -1;
    }
    sprintf(buf,"Socket bound, port %d", CONSOLE_PORT);
    CONSOLE.log_callback(CB_INFO,buf);
    err = listen(CONSOLE.socket_fd, 1);
    if (err != 0) {
        sprintf(buf, "Error occurred during listen: errno %d", errno);
        CONSOLE.log_callback(CB_ERROR,buf);
        return -1;
    }
    sprintf(buf, "Socket listening");
    CONSOLE.log_callback(CB_INFO,buf);

    // Large enough for both IPv4 or IPv6
    struct sockaddr_in6 source_addr;
    uint addr_len = sizeof(source_addr);
    conn_fd = accept(CONSOLE.socket_fd, (struct sockaddr *)&source_addr, &addr_len);
    if (conn_fd < 0) {
        sprintf(buf, "Unable to accept connection: errno %d", errno);
        CONSOLE.log_callback(CB_ERROR,buf);
        return -1;
    }
    sprintf(buf, "Socket connection accepted, port %d", CONSOLE_PORT);
    CONSOLE.log_callback(CB_INFO,buf);
    return conn_fd;
}

void tcp_console_socket_deinit()
{
    int i;
    for(i = 0; i<3;i++){
        printf("Killing Terminal in %d [s]\n",3-i);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    CONSOLE.log_callback(CB_WARNING,"Closing down socket");
    fclose(stdin);
    fclose(stdout);
    //shutdown(CONSOLE.socket_fd, 0);
    close(CONSOLE.socket_fd);
    CONSOLE.socket_fd = -1;
    shutdown(CONSOLE.connection_fd, 0);
    close(CONSOLE.connection_fd);
    CONSOLE.connection_fd = -1;
    vTaskDelete(CONSOLE.console_task);    
    /*Handle is just an opaque pointer copy of the real handle. The task will be cleaned up by next tick of idle task.*/
    
    
}

static void console_loop(void *pvArgs){
    ESP_LOGI(TAG, "Switch my console stdout and stdin to socket connection");
    stdout = fwopen(NULL, &console_writefn);
    setvbuf(stdout, NULL, _IONBF, 0);
    stdin = fropen(NULL,&console_readfn);
    /* Disable buffering on stdin */
    setvbuf(stdin, NULL, _IONBF, 0);    
    /*It is now ok to start socket.*/
    ESP_ERROR_CHECK((xSemaphoreGive(CONSOLE.new_tcp) == pdPASS)?ESP_OK:ESP_FAIL);
    while (true){ 
        /*Block until a connection has appeared on socket*/
        if(xQueueReceive(CONSOLE.new_client_queue, &CONSOLE.connection_fd, portMAX_DELAY)){
            /* Prompt to be printed before each line.
            * This can be customized, made dynamic, etc.
            */
            const char* prompt = LOG_COLOR_I "esp32> " LOG_RESET_COLOR;
            printf("\n"
                "Attent Dentalcam config console.\n"
                "Type 'help' to get the list of commands.\n"
                "Use UP/DOWN arrows to navigate through command history.\n"
                "Press TAB when typing command name to auto-complete.\n"
                "NB! \"exit\" before disconnecting socket, or you will not reconnect\n");
            /* Figure out if the terminal supports escape sequences */
            int probe_status = linenoiseProbe();
            if (probe_status) { /* zero indicates success */
                printf("\n"
                    "Your terminal application does not support escape sequences.\n"
                    "Line editing and history features are disabled.\n"
                    "On Windows, try using Putty instead.\n");
                linenoiseSetDumbMode(1);
#if CONFIG_LOG_COLORS
                /* Since the terminal doesn't support escape sequences,
                * don't use color codes in the prompt.
                */
                prompt = "esp32> ";
#endif //CONFIG_LOG_COLORS
            }
            /* Main loop */
            
            while(true) {
                if(CONSOLE.connection_fd == -1){
                    break;
                }
                /* Get a line using linenoise.
                * The line is returned when ENTER is pressed.
                */
                char* line = linenoise(prompt);
                if (line == NULL) { /* Ignore empty lines */
                    continue;
                }
                /* Add the command to the history */
                linenoiseHistoryAdd(line);
#if CONFIG_STORE_HISTORY
                /* Save command history to filesystem */
                linenoiseHistorySave(HISTORY_PATH);
#endif
                /* Try to run the command */
                int ret;
                esp_err_t err = esp_console_run(line, &ret);
                if (err == ESP_ERR_NOT_FOUND) {
                    printf("Unrecognized command\n");
                } else if (err == ESP_ERR_INVALID_ARG) {
                    // command was empty
                } else if (err == ESP_OK && ret != ESP_OK) {
                    printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(err));
                } else if (err != ESP_OK) {
                    printf("Internal error: %s\n", esp_err_to_name(err));
                }
                /* linenoise allocates line buffer on the heap, so need to free it */
                linenoiseFree(line);
            }
        }
    }
}
