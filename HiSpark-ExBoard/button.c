#include "button.h"
#include "gpio.h"
#include "pinctrl.h"

void button_init(void){
    uapi_pin_set_mode(BUTTON_PIN, HAL_PIO_FUNC_GPIO);
uapi_pin_set_pull(BUTTON_PIN, PIN_PULL_TYPE_UP);
uapi_gpio_set_dir(BUTTON_PIN, GPIO_DIRECTION_INPUT);
}
