#include <stdio.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_timer.h>
#include "button_bsp.h"
#include "multi_button.h"

EventGroupHandle_t UserButtonGroups;

static Button UserButton; 
#define USER_KEY_PIN    0 
#define USER_ID         2         
#define USER_Active     0 

   

/*******************Callback event declaration***************/
/*Click continuously*/
static void on_user_press_repeat(Button* btn_handle)
{
    xEventGroupSetBits(UserButtonGroups,set_bit_button(0));
}
/*click*/
static void on_user_single_click(Button* btn_handle)
{
    xEventGroupSetBits(UserButtonGroups,set_bit_button(1));
}
/*double-click*/
static void on_user_double_click(Button* btn_handle)
{
    xEventGroupSetBits(UserButtonGroups,set_bit_button(2));
}
/*Long press*/
static void on_user_long_press_start(Button* btn_handle)
{
    xEventGroupSetBits(UserButtonGroups,set_bit_button(3));
}
/*Long press to hold*/
static void on_user_long_press_hold(Button* btn_handle)
{
    xEventGroupSetBits(UserButtonGroups,set_bit_button(4));
}
/*press*/
static void on_user_press_down(Button* btn_handle)
{
    xEventGroupSetBits(UserButtonGroups,set_bit_button(5));
}
/*upspring*/
static void on_user_press_up(Button* btn_handle)
{
    xEventGroupSetBits(UserButtonGroups,set_bit_button(6));
}

/*********************************************/

static void clock_task_callback(void *arg) {
    button_ticks();
}

static uint8_t read_button_GPIO(uint8_t Button_ID) {
    switch (Button_ID) {
    case USER_ID:
        return gpio_get_level(USER_KEY_PIN);
    default:
        break;
    }
    return 1;
}

static void gpio_init(void) {
    gpio_config_t gpio_conf = {};
    gpio_conf.intr_type     = GPIO_INTR_DISABLE;
    gpio_conf.mode          = GPIO_MODE_INPUT;
    gpio_conf.pin_bit_mask  = (0x1ULL << USER_KEY_PIN);
    gpio_conf.pull_down_en  = GPIO_PULLDOWN_DISABLE;
    gpio_conf.pull_up_en    = GPIO_PULLUP_ENABLE;

    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf));
}

void Custom_ButtonInit(void) {
    UserButtonGroups = xEventGroupCreate();
    gpio_init();
    
    button_init(&UserButton, read_button_GPIO, USER_Active, USER_ID);           
    button_attach(&UserButton, BTN_PRESS_REPEAT, on_user_press_repeat);         // Repeated press event
    button_attach(&UserButton, BTN_SINGLE_CLICK, on_user_single_click);         // Single click event
    button_attach(&UserButton, BTN_DOUBLE_CLICK, on_user_double_click);         // Double click event
    button_attach(&UserButton, BTN_LONG_PRESS_START, on_user_long_press_start); // Long press event
    button_attach(&UserButton, BTN_PRESS_DOWN, on_user_press_down);             // Press event
    button_attach(&UserButton, BTN_PRESS_UP, on_user_press_up);                 // Release event
    button_attach(&UserButton, BTN_LONG_PRESS_HOLD, on_user_long_press_hold);   // Long press hold event

    esp_timer_create_args_t clock_tick_timer_args = {};
    clock_tick_timer_args.callback                = &clock_task_callback;
    clock_tick_timer_args.name                    = "clock_task";
    clock_tick_timer_args.arg                     = NULL;
    esp_timer_handle_t clock_tick_timer           = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&clock_tick_timer_args, &clock_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(clock_tick_timer, 1000 * 5)); // 5ms
    button_start(&UserButton);
}

uint8_t user_user_get_repeat_count(void) {
    return (button_get_repeat_count(&UserButton));
}
