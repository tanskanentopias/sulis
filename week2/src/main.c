/* 3p suoritus, valot vilkkuu punainen, keltainen ja vihreä, 
sekunnin välein. Syklin pystyy pysäyttämään napista ja jatkmaan painamalla nappia uudestan.
Pasuen ollessa päällä napeilla 2-4 pystyy valitsemaan onko valittu ledi päällä vai ei.*/
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <inttypes.h>
#include <zephyr/sys/util.h>

int led_state = 0;
int old_state = 0;

// Configure buttons
#define BUTTON_0 DT_ALIAS(sw0)
#define BUTTON_2 DT_ALIAS(sw2)
#define BUTTON_3 DT_ALIAS(sw3)
#define BUTTON_4 DT_ALIAS(sw4)
static const struct gpio_dt_spec button_0 = GPIO_DT_SPEC_GET_OR(BUTTON_0, gpios, {0});
static const struct gpio_dt_spec button_2 = GPIO_DT_SPEC_GET_OR(BUTTON_2, gpios, {0});
static const struct gpio_dt_spec button_3 = GPIO_DT_SPEC_GET_OR(BUTTON_3, gpios, {0});
static const struct gpio_dt_spec button_4 = GPIO_DT_SPEC_GET_OR(BUTTON_4, gpios, {0});
static struct gpio_callback button_0_data;
static struct gpio_callback button_2_data;
static struct gpio_callback button_3_data;
static struct gpio_callback button_4_data;

// Led pin configurations
static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

// Led thread initialization
#define STACKSIZE 1024
#define PRIORITY 5
void red_led_task(void *, void *, void*);
K_THREAD_DEFINE(red_thread,STACKSIZE,red_led_task,NULL,NULL,NULL,PRIORITY,0,0);
void green_led_task(void *, void *, void*);
K_THREAD_DEFINE(green_thread,STACKSIZE,green_led_task,NULL,NULL,NULL,PRIORITY,0,0);
void yellow_led_task(void *, void *, void*);
K_THREAD_DEFINE(yellow_thread,STACKSIZE,yellow_led_task,NULL,NULL,NULL,PRIORITY,0,0);

int init_button(void);
int init_led(void);

// Button interrupt handler
void button_0_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{

    static int pressed = 0; 
    
    pressed = !pressed;
    
    if (pressed) {
        
        printk("Pause\n");
        old_state = led_state;
        led_state = 3;         
    }
    else {
        printk("Resume\n");
        led_state = old_state; 
        
    }

}

void button_2_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{

    if (led_state == 3) {
        static int toggle = 0; 

        toggle = !toggle;

        if (toggle) {
            
            printk("Red interrupts: ON\n");
            gpio_pin_set_dt(&red,1);
            gpio_pin_set_dt(&green,0);
            
            
        }
        else {
            printk("Red interrupt: OFF\n");
            gpio_pin_set_dt(&red,0);
	        gpio_pin_set_dt(&green,0);
            
        }
    }
}

void button_3_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{


    if (led_state == 3) {
        static int toggle = 0; 

        toggle = !toggle;

        if (toggle) {
            
            printk("Red interrupts: ON\n");
            gpio_pin_set_dt(&red,1);
            gpio_pin_set_dt(&green,1);
            
            
        }
        else {
            printk("Red interrupt: OFF\n");
            gpio_pin_set_dt(&red,0);
	        gpio_pin_set_dt(&green,0);
            
        }
    }

}

void button_4_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{


    if (led_state == 3) {
        static int toggle = 0; 

        toggle = !toggle;

        if (toggle) {
            
            printk("Red interrupts: ON\n");
            gpio_pin_set_dt(&red,0);
            gpio_pin_set_dt(&green,1);
            
            
        }
        else {
            printk("Red interrupt: OFF\n");
            gpio_pin_set_dt(&red,0);
	        gpio_pin_set_dt(&green,0);
            
        }
    }

}



// Main program
int main(void)
{
	init_led();
	

	int ret = init_button();
	if (ret < 0) {
		return 0;
	}

	while (1) {
		k_msleep(10);
	}

	return 0;
}

// Initialize leds
int  init_led() {

	// Led pin initialization
	int ret = gpio_pin_configure_dt(&red, GPIO_OUTPUT_ACTIVE);
	int ret1 = gpio_pin_configure_dt(&green, GPIO_OUTPUT_ACTIVE);

	
	if (ret < 0) {
		printk("Error: Led configure failed\n");		
		return ret;
	}

	if (ret1 < 0) {
		printk("Error: Led configure failed\n");		
		return ret1;
	}
	
	// set led off
	gpio_pin_set_dt(&red,0);
	gpio_pin_set_dt(&green,0);
	
	
	printk("Led initialized ok\n");
	
	return 0;
}


void red_led_task(void *, void *, void*) {
    printk("Red led thread started\n");
    while (true) {

        if (led_state == 0) {
            
            gpio_pin_set_dt(&red,1);
            printk("Red on\n");
            k_sleep(K_SECONDS(1));
            
            while (led_state == 3) {
                k_msleep(10); 
            }

            gpio_pin_set_dt(&red,0);
            printk("Red off\n");
            k_sleep(K_SECONDS(1));
            
            while (led_state == 3) {
                k_msleep(10);
            }

            led_state = 1; 
        }
        
        k_msleep(1);
    }
}

// Task to handle yellow led
void yellow_led_task(void *, void *, void*) {
    printk("Yellow led thread started\n");
    while (true) {

        if (led_state == 1) {
            
            gpio_pin_set_dt(&green, 1);
            gpio_pin_set_dt(&red, 1);
            printk("Yellow on\n");
            k_sleep(K_SECONDS(1));
            
            while (led_state == 3) {
                k_msleep(10);
            }

            gpio_pin_set_dt(&green, 0);
            gpio_pin_set_dt(&red, 0);
            printk("Yellow off\n");
            k_sleep(K_SECONDS(1));
            
            while (led_state == 3) {
                k_msleep(10);
            }

            led_state = 2; 
        }
        
        k_msleep(1);
    }
}

// Task to handle green led
void green_led_task(void *, void *, void*) {
    printk("Green led thread started\n");
    while (true) {

        if (led_state == 2) {

            gpio_pin_set_dt(&green, 1);
            printk("Green on\n");
            k_sleep(K_SECONDS(1));

            while (led_state == 3) {
                k_msleep(10);
            }

            gpio_pin_set_dt(&green, 0);
            printk("Green off\n");
            k_sleep(K_SECONDS(1));
            
            while (led_state == 3) {
                k_msleep(10);
            }

            led_state = 0; 
        }
        
        k_msleep(1);
    }
}

// Button initialization
int init_button() {

	int ret;
	if (!gpio_is_ready_dt(&button_0)) {
		printk("Error: button 0 is not ready\n");
		return -1;
	}

	ret = gpio_pin_configure_dt(&button_0, GPIO_INPUT);
	if (ret != 0) {
		printk("Error: failed to configure pin\n");
		return -1;
	}

	ret = gpio_pin_interrupt_configure_dt(&button_0, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error: failed to configure interrupt on pin\n");
		return -1;
	}

	gpio_init_callback(&button_0_data, button_0_handler, BIT(button_0.pin));
	gpio_add_callback(button_0.port, &button_0_data);
	printk("Set up button 0 ok\n");

    int ret2;
	if (!gpio_is_ready_dt(&button_2)) {
		printk("Error: button 2 is not ready\n");
		return -1;
	}

	ret2 = gpio_pin_configure_dt(&button_2, GPIO_INPUT);
	if (ret2 != 0) {
		printk("Error: failed to configure pin\n");
		return -1;
	}

	ret2 = gpio_pin_interrupt_configure_dt(&button_2, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret2 != 0) {
		printk("Error: failed to configure interrupt on pin\n");
		return -1;
	}

	gpio_init_callback(&button_2_data, button_2_handler, BIT(button_2.pin));
	gpio_add_callback(button_2.port, &button_2_data);
	printk("Set up button 2 ok\n");
	
    int ret3;
	if (!gpio_is_ready_dt(&button_3)) {
		printk("Error: button 0 is not ready\n");
		return -1;
	}

	ret3 = gpio_pin_configure_dt(&button_3, GPIO_INPUT);
	if (ret3 != 0) {
		printk("Error: failed to configure pin\n");
		return -1;
	}

	ret3 = gpio_pin_interrupt_configure_dt(&button_3, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret3 != 0) {
		printk("Error: failed to configure interrupt on pin\n");
		return -1;
	}

	gpio_init_callback(&button_3_data, button_3_handler, BIT(button_3.pin));
	gpio_add_callback(button_3.port, &button_3_data);
	printk("Set up button 3 ok\n");
	
	
    int ret4;
	if (!gpio_is_ready_dt(&button_4)) {
		printk("Error: button 4 is not ready\n");
		return -1;
	}

	ret4 = gpio_pin_configure_dt(&button_4, GPIO_INPUT);
	if (ret4 != 0) {
		printk("Error: failed to configure pin\n");
		return -1;
	}

	ret4 = gpio_pin_interrupt_configure_dt(&button_4, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret4 != 0) {
		printk("Error: failed to configure interrupt on pin\n");
		return -1;
	}

	gpio_init_callback(&button_4_data, button_4_handler, BIT(button_4.pin));
	gpio_add_callback(button_4.port, &button_4_data);
	printk("Set up button 4 ok\n");
	

	return 0;
}
