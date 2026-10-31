#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "tkjhat/sdk.h"

// Default stack size for the tasks. It can be reduced to 1024 if task is not using lot of memory.
#define DEFAULT_STACK_SIZE 2048 

//Add here necessary states
enum state { IDLE=1, READ_POS, TRANSMIT};
enum state programState = IDLE;
struct flags{
    uint8_t button1;
    uint8_t button2;
} flag = {false, false};




static void btn_fxn(uint gpio, uint32_t eventMask){
    if(gpio == BUTTON1){
        flag.button1 = true;

    }else if (gpio == BUTTON2){
        flag.button2 = true;
    }
}

static void read_position(void *arg){
    (void) arg;
    char buff[100];
    uint8_t buffCount = 0;
    uint8_t spaceCount = 0;
    float ax, ay, az, gx, gy, gz, t;
    if (init_ICM42670() == 0) {
        printf("IMU initialized successfully!\n");
        if (ICM42670_start_with_default_values() != 0){
            printf("IMU could not initialize accelerometer or gyroscope");
        }
    } else {
        printf("Failed to initialize IMU.\n");
    }
    while (1)
    {
        if (programState == IDLE){
            if(flag.button1){
                if (ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t) == 0) {
                    if((0.85 < az) && (az < 1.15)){
                        buff[buffCount] = "-";
                        buffCount += 1;
                    }
                    else if ((0.85 < ay) && (ay < 1.15)){
                        buff[buffCount] = ".";
                        buffCount += 1;
                    }
                    spaceCount = 0;
                    flag.button1 = false;
        
            } else if(flag.button2){
                buff[buffCount] = " ";
                buffCount += 1;
                spaceCount += 1;
                flag.button2 = false;
            }
        if (spaceCount > 2){
            programState = TRANSMIT;
        }
            printf("%c", buff[buffCount]);
            vTaskDelay(pdMS_TO_TICKS(500));
            }
        }
    }
}


static void example_task(void *arg){
    (void)arg;

    for(;;){
        tight_loop_contents(); // Modify with application code here.
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

int main() {
    stdio_init_all();
    // Uncomment this lines if you want to wait till the serial monitor is connected
    /*while (!stdio_usb_connected()){
        sleep_ms(10);
    }*/ 
    init_hat_sdk();
    init_button1();
    init_button2();
    gpio_set_irq_enabled_with_callback(SW1_PIN, GPIO_IRQ_EDGE_FALL, true, btn_fxn);
    sleep_ms(300); //Wait some time so initialization of USB and hat is done.

    TaskHandle_t readPosition = NULL;
    // Create the tasks with xTaskCreate
    BaseType_t result = xTaskCreate(read_position,       // (en) Task function
                "position",              // (en) Name of the task 
                DEFAULT_STACK_SIZE, // (en) Size of the stack for this task (in words). Generally 1024 or 2048
                NULL,               // (en) Arguments of the task 
                2,                  // (en) Priority of this task
                &readPosition);    // (en) A handle to control the execution of this task

    if(result != pdPASS) {
        printf("Example Task creation failed\n");
        return 0;
    }

    // Start the scheduler (never returns)
    vTaskStartScheduler();

    // Never reach this line.
    return 0;
}

