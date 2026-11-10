#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "tkjhat/sdk.h"
//How do I commit changes to github, I seem to be doing smething wrong...

// Default stack size for the tasks. It can be reduced to 1024 if task is not using lot of memory.
#define DEFAULT_STACK_SIZE 2048
#define BUFFER_LENGTH 128 

//Add here necessary states
enum state { IDLE=1, READ_POS, TRANSMIT, RECEIVE, DISPLAY, LED};
enum state programState = IDLE;
struct flags{
    uint8_t button1;
    uint8_t button2;
} flag = {false, false};
char message[BUFFER_LENGTH];
char receive[BUFFER_LENGTH];
char translated[BUFFER_LENGTH];
uint8_t messageCount = 0;
uint8_t c;

void morse_char(char morse, uint8_t *space){
    if (messageCount < BUFFER_LENGTH){
        message[messageCount++] = morse; //increment by 1 happens after
        *space = 0;
        flag.button1 = false;
    }

}

void translate(char buf[]){
    char *token;
    uint8_t count = 0;
    uint8_t space = false;
    char *morse_translate[27] = {".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..",
                                "-", "-.-", ".-..", "--", "-.", "---", ".--.", "--.", ".-.", 
                                "...", "-", "..-", "...-", ".--", "-..-", "-.--", "--.."}; //array of pointers (ainternal string arrays decay into pointer to first element.)

    for(int i = 1; buf[i] != '\0'; i++){ 
        if ((buf[(i-1)] == ' ') && (buf[i] == ' ')){
            buf[(i-1)] = 's';
            printf("%s", buf);
        }
    }

    token = strtok(buf, " ");

    while (token != NULL){
        uint8_t len = strlen(token);
        if(token[(len-1)] == 's'){
            space = true;
            token[(len-1)] = '\0';

        } else if(token == "\n"){
            translated[count++] = '\n';
        }
        for (int i=0; i < 26; i++){
            if(strcmp(morse_translate[i], token) == 0){
                translated[count++] = 97 + i;
            }
        }
        if (space){
            translated[count++] = ' ';
            space = false;
        }
        token = strtok(NULL, " ");
    }
}

static void btn_fxn(uint gpio, uint32_t eventMask){
    if(gpio == BUTTON1){
        flag.button1 = true;

    }else if (gpio == BUTTON2){
        flag.button2 = true;
    }
}

static void idle_task(void *arg){
    (void) arg; //Why is this always at the start of a function?
    
    while (1) {

        if (programState == IDLE){
            rgb_led_write(0, 255, 0);
            flag.button2 = false; //otherwise program may get stuck
            if (flag.button1 && !flag.button2){
                flag.button1 = false;
                //programState = READ_POS;
                programState = DISPLAY;
            }
            //c = getchar_timeout_us(0);
            //if (c != PICO_ERROR_TIMEOUT){
                //programState = RECEIVE;
            //}
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void read_position_task(void *arg){
    (void) arg;

    uint8_t spaceCount = 0; //Doesn't always work correctly
    float ax, ay, az, gx, gy, gz, t;
    // if (init_ICM42670() == 0) {
    //     printf("IMU initialized successfully!\n");
    //     if (ICM42670_start_with_default_values() != 0){
    //         printf("IMU could not initialize accelerometer or gyroscope");
    //     }
    // } else {
    //     printf("Failed to initialize IMU.\n");
    // }

    init_ICM42670();
    ICM42670_start_with_default_values();

    while (1){

        if (programState == READ_POS){
            rgb_led_write(255, 0, 0);
            if(flag.button1){
                if (ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t) == 0) {//should I average multiple measurements?
                    if((0.85 < az) && (az < 1.15)){
                        morse_char('-', &spaceCount);
                    } else if ((0.85 < ay) && (ay < 1.15)){
                        morse_char('.', &spaceCount);
                    }
                }
            } else if((flag.button2) && (messageCount < BUFFER_LENGTH)){
                if (spaceCount == 2){
                    message[messageCount] = '\n';
                    spaceCount = 0;
                    flag.button2 = false;
                    programState = TRANSMIT;
                } else {
                    message[messageCount++] = ' ';
                    spaceCount++;
                    flag.button2 = false;
                }   
            } else if(messageCount == BUFFER_LENGTH){
                programState = TRANSMIT;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void transmit_task(void *arg){
    (void) arg;

    char empty[BUFFER_LENGTH] = {0};

    while (1){

        if (programState == TRANSMIT){
            //gpio_set_irq_enabled(SW1_PIN, GPIO_IRQ_EDGE_FALL, false); //disable innterrupts?
            //gpio_set_irq_enabled(SW2_PIN, GPIO_IRQ_EDGE_FALL, false);
            printf("%s", message);
            strcpy(message, empty);
            buzzer_play_tone(731, 100);
            messageCount = 0;
            programState = IDLE;
            
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void receive_task(void *arg){
    (void) arg;

    while (1){

        if (programState == RECEIVE){
            rgb_led_write(255, 255, 0);
            uint8_t i = 0;
            do {
                if (c == '\n'){
                    receive[i] = '\0';
                    i = 0;
                    printf("%s", receive);
                    programState = IDLE; //for now, later change to LED/print to display according to button.
                    // while (1){
                    //     if(flag.button1){
                    //         flag.button1 = false;
                    //         programState = DISPLAY;
                    //         break;
                    //     }
                    //     else if (flag.button2){
                    //         flag.button2 = false;
                    //         programState = LED;
                    //         break;
                    //     }
                    // }
                }
                else if (i < BUFFER_LENGTH -1){
                    receive[i++] = (char)c;
                    printf("%c", c);
                }
                //add buffer overflow
                c = getchar_timeout_us(0);
            } while (c != PICO_ERROR_TIMEOUT);
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }

}

static void display_task(void *arg) {
    (void)arg;

    char translated[BUFFER_LENGTH] = "abcdefghijklmnopqrstuvwxyz";
    char display_buf[11];
    uint8_t n;

    while(1) {

        if (programState == DISPLAY){
            rgb_led_write(255, 0, 255);
            if (flag.button1){
                flag.button1 = false;
                clear_display();
                
                snprintf(display_buf,11,"%s",translated);
                n = strlen(display_buf);
                printf("%s\n", translated);//the second iteration both prints are just newlines
                printf("%s\n", display_buf);
                for (int i = (n-1); i <= strlen(translated); i++){
                    translated[i-(n-1)] = translated[i];
                }
                printf("%s\n", translated);
                printf("%s\n", display_buf);
                write_text(display_buf);
                if (strlen(translated) == 1){
                    programState = IDLE;
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(500));
        

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
    init_rgb_led();
    init_buzzer();
    init_display();
    clear_display();
    gpio_set_irq_enabled_with_callback(SW1_PIN, GPIO_IRQ_EDGE_FALL, true, btn_fxn);
    gpio_set_irq_enabled(SW2_PIN, GPIO_IRQ_EDGE_FALL, true);
    sleep_ms(300); //Wait some time so initialization of USB and hat is done.

    TaskHandle_t readPosition, transfer, idling, receive, lcd = NULL;
    // Create the tasks with xTaskCreate
    BaseType_t result = xTaskCreate(read_position_task,       // (en) Task function
                "position",              // (en) Name of the task 
                DEFAULT_STACK_SIZE, // (en) Size of the stack for this task (in words). Generally 1024 or 2048
                NULL,               // (en) Arguments of the task 
                2,                  // (en) Priority of this task
                &readPosition);    // (en) A handle to control the execution of this task
    if(result != pdPASS) {
        printf("Task creation failed\n");
        return 0;
    }

    BaseType_t send = xTaskCreate(transmit_task,
            "transmit",
            DEFAULT_STACK_SIZE,
            NULL,
            2,
            &transfer);
    if(send != pdPASS) {
        printf("Task creation failed\n");
        return 0;
    }

    BaseType_t nothing = xTaskCreate(idle_task,
            "idling",
            DEFAULT_STACK_SIZE,
            NULL,
            2,
            &idling);
    if(nothing != pdPASS) {
        printf("Task creation failed\n");
        return 0;
    }

    BaseType_t incoming = xTaskCreate(receive_task,
            "receive",
            DEFAULT_STACK_SIZE,
            NULL,
            2,
            &receive);
    if(incoming != pdPASS) {
        printf("Task creation failed\n");
        return 0;
    }

    BaseType_t screen = xTaskCreate(display_task,
            "lcd",
            DEFAULT_STACK_SIZE,
            NULL,
            2,
            &lcd);
    if(screen != pdPASS) {
        printf("Task creation failed\n");
        return 0;
    }
    
    // Start the scheduler (never returns)
    vTaskStartScheduler();

    // Never reach this line.
    return 0;
}

