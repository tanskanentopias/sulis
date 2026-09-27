/*
Viikkotehtävä 5. Liikennevalojen yksikkötestaus
1p suoritus
Ohjelma suorittaa timer_handler() funktiossa olevan taskin UART-luetun ajan jälkeen (HHMMSS).
Testeissä vaaditut testit ja vähän muutakin.
*/

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/timing/timing.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/****************************
 * Remember to add line:
 * CONFIG_HEAP_MEM_POOL_SIZE=1024
 * to prj.conf
 ****************************/
#define TIME_LEN_ERROR      -1
#define TIME_ARRAY_ERROR    -2
#define TIME_VALUE_ERROR    -3

// Timer initializations
struct k_timer timer;
void timer_handler(struct k_timer *timer_id);
bool red_state = false;

int time_parse(char *time);

// Thread initializations
volatile int current_led_time = 1000;

#define PRIORITY 5

// UART initialization
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

// Create dispatcher FIFO buffer
K_FIFO_DEFINE(dispatcher_fifo);
K_FIFO_DEFINE(data_fifo);

static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

// Red led thread initialization
#define STACKSIZE 1024
#define PRIORITY 5
void dispatcher_task(void *, void *, void*);
void uart_task(void *, void *, void*);
void red_led_task(void *, void *, void*);
void yellow_led_task(void *, void *, void*);
void green_led_task(void *, void *, void*);
void debug_task(void *, void *, void*);
K_THREAD_DEFINE(dis_thread,STACKSIZE,dispatcher_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(uart_thread,STACKSIZE,uart_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(red_thread,STACKSIZE,red_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(yellow_thread,STACKSIZE,yellow_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(green_thread,STACKSIZE,green_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(debug_thread,STACKSIZE,debug_task,NULL,NULL,NULL,PRIORITY,0,0);


int init_led(void);

K_MUTEX_DEFINE(red_mutex);
K_CONDVAR_DEFINE(red_signal);
K_MUTEX_DEFINE(yellow_mutex);
K_CONDVAR_DEFINE(yellow_signal);
K_MUTEX_DEFINE(green_mutex);
K_CONDVAR_DEFINE(green_signal);
K_MUTEX_DEFINE(release_mutex);
K_CONDVAR_DEFINE(release_signal);


// FIFO dispatcher data type
struct data_t {
	/*************************
	// Add fifo_reserved below
	*************************/
	void *fifo_reserved;
	char msg[20];
    uint64_t time;
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

/********************
 * Main task
 */
int main(void)
{

    
	int ret = init_uart();
	if (ret != 0) {
		printk("UART initialization failed!\n");
		return ret;
	}

    init_led();
    timing_init();

	k_timer_init(&timer, timer_handler, NULL);

    printk("Main thread runs once\n");

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
		if (uart_poll_in(uart_dev,&rc) == 0) {
			// printk("Received: %c\n",rc);
			// If character is not newline, add to UART message buffer
			if (rc != '\r') {
				uart_msg[uart_msg_cnt] = rc;
				uart_msg_cnt++;
			// Character is newline, copy dispatcher data and put to FIFO buffer
			} else {
				printk("UART msg: %s\n", uart_msg);
                
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

				// Clear UART message buffer
				uart_msg_cnt = 0;
				memset(uart_msg,0,20);
			}
		}
		k_msleep(10);
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
		printk("Dispatcher: %s\n", sequence);
		
		char time_buf[7];
		strncpy(time_buf, sequence, 6);
		time_buf[6] ='\0';

		int parsed_seconds = time_parse(time_buf);
        
        if (parsed_seconds >= 0) {
            printk("Parsed delay: %d seconds\n", parsed_seconds);
            
            k_timer_start(&timer, K_SECONDS(parsed_seconds), K_NO_WAIT);

        } else {
            printk("Time parse error");
            continue;
        }
        int cnt = 6; 
        while (sequence[cnt] != 0 && sequence[cnt] != '\r' && sequence[cnt] != '\n') {
            bool valid_color = false;

            if (sequence[cnt] == 'R') {
                printk("RED\n");
                k_condvar_broadcast(&red_signal);
                valid_color = true;
            }
            else if (sequence[cnt] == 'Y') {
                printk("YELLOW\n");
                k_condvar_broadcast(&yellow_signal);
                valid_color = true;
            }
            else if (sequence[cnt] == 'G') {
                printk("GREEN\n");
                k_condvar_broadcast(&green_signal);
                valid_color = true;
            }
            
            cnt++;

            if (valid_color) {
                k_mutex_lock(&release_mutex, K_FOREVER);
                k_condvar_wait(&release_signal, &release_mutex, K_FOREVER);
                k_mutex_unlock(&release_mutex);
            }
        }

	}
}


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
        
			k_mutex_lock(&red_mutex, K_FOREVER);

            k_condvar_wait(&red_signal, &red_mutex, K_FOREVER);

			k_mutex_unlock(&red_mutex);

            timing_start();
            timing_t start_time = timing_counter_get();

            gpio_pin_set_dt(&red,1);
            printk("Red on\n");
            k_sleep(K_MSEC(current_led_time));
            
            gpio_pin_set_dt(&red,0);
            printk("Red off\n");
            k_sleep(K_MSEC(current_led_time));
            
            struct data_t *buf = k_malloc(sizeof(struct data_t));
		    if (buf == NULL) {
			    return;
		    }

            timing_stop();
            timing_t end_time = timing_counter_get();
            uint16_t diff = timing_cycles_to_ns(timing_cycles_get(&start_time, &end_time));

            buf->time = diff;
		    k_fifo_put(&data_fifo, buf);
            printk("Red added to fifo: %lld\n",buf->time);

            k_condvar_broadcast(&release_signal);

    }
}

void yellow_led_task(void *, void *, void*) {
    printk("Yellow led thread started\n");
    while (true) {

			k_mutex_lock(&yellow_mutex, K_FOREVER);

            k_condvar_wait(&yellow_signal, &yellow_mutex, K_FOREVER);
        
			k_mutex_unlock(&yellow_mutex);

            timing_start();
            timing_t start_time = timing_counter_get();

            gpio_pin_set_dt(&red,1);
            gpio_pin_set_dt(&green,1);
            printk("Yellow on\n");
            k_sleep(K_MSEC(current_led_time));
            
            gpio_pin_set_dt(&red,0);
            gpio_pin_set_dt(&green,0);
            printk("Yellow off\n");
            k_sleep(K_MSEC(current_led_time));
            
            struct data_t *buf = k_malloc(sizeof(struct data_t));
		    if (buf == NULL) {
			    return;
		    }

            timing_stop();
            timing_t end_time = timing_counter_get();
            uint16_t diff = timing_cycles_to_ns(timing_cycles_get(&start_time, &end_time));

            buf->time = diff;
		    k_fifo_put(&data_fifo, buf);
            printk("Yellow added to fifo: %lld\n",buf->time);

            k_condvar_broadcast(&release_signal);
    }
}

void green_led_task(void *, void *, void*) {
    printk("Green led thread started\n");
    while (true) {

			k_mutex_lock(&green_mutex, K_FOREVER);

            k_condvar_wait(&green_signal, &green_mutex, K_FOREVER);
            
			k_mutex_unlock(&green_mutex);

            timing_start();
            timing_t start_time = timing_counter_get();

            gpio_pin_set_dt(&green,1);
            printk("Green on\n");
            k_sleep(K_MSEC(current_led_time));
            
            gpio_pin_set_dt(&green,0);
            printk("Green off\n");
            k_sleep(K_MSEC(current_led_time));

            struct data_t *buf = k_malloc(sizeof(struct data_t));
		    if (buf == NULL) {
			    return;
		    }

            timing_stop();
            timing_t end_time = timing_counter_get();
            uint16_t diff = timing_cycles_to_ns(timing_cycles_get(&start_time, &end_time));

            buf->time = diff;
		    k_fifo_put(&data_fifo, buf);
            printk("Green added to fifo: %lld\n",buf->time);
            
            k_condvar_broadcast(&release_signal);    
   }
}

void debug_task(void *, void *, void*) {

	// Store received data
	struct data_t *received;
    uint64_t total_time = 0;
	while (true) {

		received = k_fifo_get(&data_fifo, K_FOREVER);

        total_time += received->time;
		printk("Debug received: %lld ns| Total time: %lld ns\n", received->time, total_time);
		k_free(received);
		k_yield();
    }
}

int time_parse(char *time) {


	// TODO: Check that string is not null

	// Parse values from time string
	if (time == NULL) {
		return TIME_VALUE_ERROR;
	}
			// For example: 124033 -> 12hour 40min 33sec
			int values[3];
			values[2] = atoi(time+4); // seconds
			time[4] = 0;
			values[1] = atoi(time+2); // minutes
			time[2] = 0;
			values[0] = atoi(time); // hours
			// Now you have:
			// values[0] hour
			// values[1] minute
			// values[2] second
			if (values[2] > 59 || values[2] < 0) {
				
				return TIME_VALUE_ERROR;
			}
			if (values[1] > 59 || values[1] < 0) {
				
				return TIME_VALUE_ERROR;
			}
			if (values[0] > 23 || values[0] < 0) {

				
				return TIME_VALUE_ERROR;

			}
			
		
		return ((values[2])+(values[1]*60)+(values[0]*3600));

	
	
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