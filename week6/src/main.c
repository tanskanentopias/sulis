/* 2p suoritus, time_parser() käyttämällä robotin testeistä tulee aikasekvenssi UARTiin, 
time_parser funktiossa käsitellään muutamia erroreita, Kuten: sekvenssi ei voi olla 0, vain numeroita ja vain 6 merkkiä pitkä.
Ja kaikki ne testataan robot frameworkillä*/

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/timing/timing.h>
#include <inttypes.h>
#include <zephyr/sys/util.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define TIME_VALUE_ERROR    -1
#define TIME_LEN_ERROR      -2
#define TIME_NOT_DIGIT      -3
#define SEQ_COLOR_ERROR		-4

#define STACKSIZE 1024
#define PRIORITY 5

int led_state = 0;
int old_state = 0;
int current_led_time = 1000;
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
void red_led_task(void *, void *, void*);
void green_led_task(void *, void *, void*);
void yellow_led_task(void *, void *, void*);
K_THREAD_DEFINE(red_thread,STACKSIZE,red_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(green_thread,STACKSIZE,green_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(yellow_thread,STACKSIZE,yellow_led_task,NULL,NULL,NULL,PRIORITY,0,0);

// UART initialization
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

// Create dispatcher FIFO buffer
K_FIFO_DEFINE(dispatcher_fifo);

int time_parse(char *str);
void dispatcher_task(void *, void *, void*);
void uart_task(void *, void *, void*);
K_THREAD_DEFINE(dis_thread,STACKSIZE,dispatcher_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(uart_thread,STACKSIZE,uart_task,NULL,NULL,NULL,PRIORITY,0,0);

// Timer initializations
struct k_timer timer;
void timer_handler(struct k_timer *timer_id);
bool red_state = false;

K_MUTEX_DEFINE(red_mutex);
K_CONDVAR_DEFINE(red_signal);
K_MUTEX_DEFINE(yellow_mutex);
K_CONDVAR_DEFINE(yellow_signal);
K_MUTEX_DEFINE(green_mutex);
K_CONDVAR_DEFINE(green_signal);
K_MUTEX_DEFINE(release_mutex);
K_CONDVAR_DEFINE(release_signal);

struct data_t {
	/*************************
	// Add fifo_reserved below
	*************************/
	void *fifo_reserved;
	char msg[20];
};

/********************
 * init UART
 */
int init_uart(void) {
	// UART initialization
	if (!device_is_ready(uart_dev)) {
		return 1;
	} 
	return 0;
}



int init_button(void);
int init_led(void);

// Main program
int main(void)
{
	init_led();
	

	int ret = init_button();
	if (ret < 0) {
		return 0;
	}
  
    int ret1 = init_uart();
	if (ret1 != 0) {
		printk("UART initialization failed!\n");
		return ret1;
	}

	while (1) {
		k_msleep(10);
	}

	return 0;
}

/********************
 * UART task
 */
void uart_task(void *unused1, void *unused2, void *unused3)
{
	// Received character from UART
	char rc=0;
	// Message from UART
	char uart_msg[20];
	memset(uart_msg,0,20);
	int uart_msg_cnt = 0;

	while (true) {
		// Ask UART if data available
		while (uart_poll_in(uart_dev,&rc) == 0) {
			// printk("Received: %c\n",rc);
			// If character is not newline, add to UART message buffer
			if (rc != 'X') {
				uart_msg[uart_msg_cnt] = rc;
				uart_msg_cnt++;
			// Character is newline, copy dispatcher data and put to FIFO buffer
			} else {
                
				int time_delay = time_parse(uart_msg);
				printk("%dX", time_delay);

				struct data_t *buf = k_malloc(sizeof(struct data_t));
				if (buf == NULL) {
					return;
				}
				// Copy UART message to dispatcher data
				// strncpy(buf->msg, 20, uart_msg); // mitä ihmettä, miksi kaatuu!!
				snprintf(buf->msg, 20, "%s", uart_msg);

				// You need to:
				// Put dispatcher data to FIFO buffer
                k_fifo_put(&dispatcher_fifo, buf);

                // Clear UART receive buffer
				uart_msg_cnt = 0;
				memset(uart_msg,0,20);

			}
		}
		k_msleep(1);
	}
}

/********************
 * Dispatcher task
*/
void dispatcher_task(void *unused1, void *unused2, void *unused3)
{
	while (true) {
		// Receive dispatcher data from uart_task fifo
		struct data_t *rec_item = k_fifo_get(&dispatcher_fifo, K_FOREVER);
		char sequence[20];
		memcpy(sequence,rec_item->msg,20);
		k_free(rec_item);
		
		
        int cnt=0;

        while (sequence[cnt] != 0) {

            struct data_t *new_item = k_fifo_get(&dispatcher_fifo, K_NO_WAIT);
            if (new_item != NULL) {
                printk("Interrupting current sequence\n");
                memcpy(sequence, new_item->msg, 20);
                k_free(new_item);
                cnt = 0;
                if (sequence[0] == 'B') {
                    printk("Sequence stopped.\n");
                    break; 
                }
                continue; 
            }

            if (sequence[cnt] == 'T') {
                if (cnt > 0) {
                    printk("Repeating %s\n", sequence);
                    cnt = 0;
                    continue;
                }
                else {
                    break;
                }
            }

            if (sequence[cnt] == 'R') {
                printk("RED\n");
                k_condvar_broadcast(&red_signal);
            }
            
            if (sequence[cnt] == 'Y') {
                printk("YELLOW\n");
                k_condvar_broadcast(&yellow_signal);
            }
            
            if (sequence[cnt] == 'G') {
                printk("GREEN\n");
                k_condvar_broadcast(&green_signal);

            }
            
		    cnt++;

            k_condvar_wait(&release_signal, &release_mutex, K_FOREVER);
            
        }
	}
}

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
        
            k_condvar_wait(&red_signal, &red_mutex, K_FOREVER);

            gpio_pin_set_dt(&red,1);
            printk("Red on\n");
            k_sleep(K_MSEC(current_led_time));
            
            gpio_pin_set_dt(&red,0);
            printk("Red off\n");
            k_sleep(K_MSEC(current_led_time));
            

            k_condvar_broadcast(&release_signal);   

    }
}

// Task to handle yellow led
void yellow_led_task(void *, void *, void*) {
    printk("Yellow led thread started\n");
    while (true) {

            k_condvar_wait(&yellow_signal, &yellow_mutex, K_FOREVER);
        
            gpio_pin_set_dt(&red,1);
            gpio_pin_set_dt(&green,1);
            printk("Yellow on\n");
            k_sleep(K_MSEC(current_led_time));
            
            gpio_pin_set_dt(&red,0);
            gpio_pin_set_dt(&green,0);
            printk("Yellow off\n");
            k_sleep(K_MSEC(current_led_time));
            
            k_condvar_broadcast(&release_signal);  
    }
}

// Task to handle green led
void green_led_task(void *, void *, void*) {
    printk("Green led thread started\n");
    while (true) {

            k_condvar_wait(&green_signal, &green_mutex, K_FOREVER);

            gpio_pin_set_dt(&green,1);
            printk("Green on\n");
            k_sleep(K_MSEC(current_led_time));
            
            gpio_pin_set_dt(&green,0);
            printk("Green off\n");
            k_sleep(K_MSEC(current_led_time));
            
            k_condvar_broadcast(&release_signal);  
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

int time_parse(char *time) {

    if (time == NULL) { 
		return TIME_VALUE_ERROR; // sequence acant be null
	}

    size_t len = strlen(time);
       
    if (len != 6) {
        return TIME_LEN_ERROR;  // seqeunce is exactly 6 cahracters long
    }

    for (int i = 0; i < 6; i++) {
        if (!isdigit((unsigned char)time[i])) {
            return TIME_NOT_DIGIT;
        }
    }

			int values[3];
			values[2] = atoi(time+4); // seconds
			time[4] = 0;
			values[1] = atoi(time+2); // minutes
			time[2] = 0;
			values[0] = atoi(time); // hours

			if (values[2] > 59 || values[2] < 0 || 
                values[1] > 59 || values[1] < 0 || 
                values[0] > 23 || values[0] < 0) { 

				return TIME_VALUE_ERROR;

			}
			
		
            int total_seconds = ((values[2])+(values[1]*60)+(values[0]*3600)); 

            if (total_seconds == 0) {
                return TIME_VALUE_ERROR;
            }
            
			return total_seconds;
                
        
	
	
}

void timer_handler(struct k_timer *timer_id) {
	if (red_state == false) {
		gpio_pin_set_dt(&red,1);
		red_state = true;
	} else {
		gpio_pin_set_dt(&red,0);
		red_state = false;
	}
}