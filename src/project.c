// Marieke

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <stdlib.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "tkjhat/sdk.h"



// Default stack size for the tasks. It can be reduced to 1024 if task is not using lot of memory.
#define DEFAULT_STACK_SIZE 2048
#define BUFFER_LENGTH 1024

//--------------------------------------------------------------------------------------------------------------------------------------
// https://github.com/robsoncouto/arduino-songs/blob/master/pacman/pacman.ino

#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_E6  1319
#define NOTE_G6  1568
#define REST      0

//--------------------------------------------------------------------------------------------------------------------------------------


enum state { IDLE=1, READ_POS, READ_LIGHT, TRANSMIT, RECEIVE, DISPLAY, LED};
enum state programState = IDLE;

struct flags{
    uint8_t button1;
    uint8_t button2;
} flag = {false, false};

uint32_t past_time;

char message[BUFFER_LENGTH];
char receive[BUFFER_LENGTH];
char translated[BUFFER_LENGTH];
char empty[BUFFER_LENGTH] = {0};
uint8_t messageCount = 0;
int8_t c;

// function prototype
void musicPlayer();
uint8_t intercept(char buffer[]);
void translate(char buf[]);
void sym(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void song();
void flower();

//--------------------------------------------------------------------------------------------------------------------------------------
// https://github.com/robsoncouto/arduino-songs/blob/master/pacman/pacman.ino
int melody[] = { 

    // Pacman
    // Score available at https://musescore.com/user/85429/scores/107109
    NOTE_B4, 16, NOTE_B5, 16, NOTE_FS5, 16, NOTE_DS5, 16, //1
    NOTE_B5, 32, NOTE_FS5, -16, NOTE_DS5, 8, NOTE_C5, 16,
    NOTE_C6, 16, NOTE_G6, 16, NOTE_E6, 16, NOTE_C6, 32, NOTE_G6, -16, NOTE_E6, 8,

    NOTE_B4, 16,  NOTE_B5, 16,  NOTE_FS5, 16,   NOTE_DS5, 16,  NOTE_B5, 32,  //2
    NOTE_FS5, -16, NOTE_DS5, 8,  NOTE_DS5, 32, NOTE_E5, 32,  NOTE_F5, 32,
    NOTE_F5, 32,  NOTE_FS5, 32,  NOTE_G5, 32,  NOTE_G5, 32, NOTE_GS5, 32,  NOTE_A5, 16, NOTE_B5, 8
    };


// notes of the melody followed by the duration.
// a 4 means a quarter note, 8 an eighteenth , 16 sixteenth, so on
// !!negative numbers are used to represent dotted notes,
// so -4 means a dotted quarter note, that is, a quarter plus an eighteenth!!
    

// sizeof gives the number of bytes, each int value is composed of two bytes (16 bits)
// there are two values per note (pitch and duration), so for each note there are four bytes
int notes = sizeof(melody) / sizeof(melody[0]) / 2;

// this calculates the duration of a whole note in ms
int wholenote = (60000 * 4) / 105;

int divider = 0, noteDuration = 0;

void musicPlayer() {
    // iterate over the notes of the melody.
    // Remember, the array is twice the number of notes (notes + durations)
        for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2) {

            // calculates the duration of each note
            divider = melody[thisNote + 1];
            if (divider > 0) {
            // regular note, just proceed
                noteDuration = (wholenote) / divider;
            } else if (divider < 0) {
            // dotted notes are represented with negative durations!!
                noteDuration = (wholenote) / abs(divider);
                noteDuration *= 1.5; // increases the duration in half for dotted notes
            }

            // we only play the note for 90% of the duration, leaving 10% as a pause
            buzzer_play_tone(melody[thisNote], noteDuration * 0.9);

            // Wait for the specief duration before playing the next note.
            buzzer_play_tone(0, noteDuration);

            // stop the waveform generation before the next note.
            buzzer_turn_off();
        }
    }
//--------------------------------------------------------------------------------------------------------------------------------------

uint8_t intercept(char buffer[]){
    // intercept certain commands while sending/receiving morse
    // execute corresponding action
    if (strcmp(buffer,"----  \n") == 0){
        song();
        programState = IDLE;
        return 1;
    } else if (strcmp(buffer,"-----  \n") == 0){
        flower();
        programState = IDLE;
        return 1;
    } else if (strcmp(buffer, ".  \n") == 0){
        // end of message
        strcpy(buffer, ". -. -..  --- ..-.  -- . ... ... .- --. .");
    } else if (strcmp(buffer, "...  \n") == 0){
        // start of message
        strcpy(buffer, "... - .- .-. -  --- ..-.  -- . ... ... .- --. .  \n");
    }
    return 0;
}

void translate(char buf[]){
    // translate string of morse code to ascii characters.
    char *token;
    uint8_t count = 0;
    uint8_t space = false; // keeps track whether space is needed
    // alphabetically ordered morse
    char *morse_translate[27] = {".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..",
                                ".---", "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", 
                                "...", "-", "..-", "...-", ".--", "-..-", "-.--", "--.."};
    
    for (int i = 1; buf[i] != '\0'; i++){ 
        // use 's' to signal 2 consecutive spaces, means space between letters is required
        if ((buf[(i-1)] == ' ') && (buf[i] == ' ')){
            buf[(i-1)] = 's';
        }
    }
    // tokenise buffer
    token = strtok(buf, " ");

    while (token != NULL){

        uint8_t len = strlen(token);
        // 's' removed and space flag activated
        if (token[(len-1)] == 's'){
            space = true;
            token[(len-1)] = '\0';

        } else if (token == "\n"){
            translated[count++] = '\n';
        }
        // match token to a morse letter
        for (int i=0; i < 26; i++){
            if (strcmp(morse_translate[i], token) == 0){
                translated[count++] = 97 + i;// ascii value for a-z = 97-122
            }
        }

        if (space){ // space added in case of active flag
            translated[count++] = ' ';
            space = false;
        }

        token = strtok(NULL, " ");
    }
}


void sym(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1){
    // draw line and mirror accros y-axis
    uint8_t hold;

    if (y1 < y0){ // y1 and x1 must be the smallest values.
        hold = y0;
        y0 = y1;
        y1 = hold;
    }
    if (x1 < x0){
        hold = x0;
        x0 = x1;
        x1 = hold;
    }

    draw_line(x0, y0, x1, y1);
    draw_line(127-x1, y0, 127-x0, y1);
    
}

static void btn_fxn(uint gpio, uint32_t eventMask){
    // interrupt handler
    uint32_t current_time = time_us_32();
    // advised 10ms = 10000 µs
    if ((current_time - past_time) > 200000){// long time inbetween
        past_time = current_time;

        if (gpio == BUTTON1){
            flag.button1 = true;
        
        } else if (gpio == BUTTON2){
            flag.button2 = true;

        }
    }
}

static void idle_task(void *arg){
    (void) arg;
    // main task
    uint8_t i = 0;

    while (1) {

        if (programState == IDLE){
            rgb_led_write(0, 255, 0);

            if (flag.button1 && !flag.button2){
                flag.button1 = false;
                programState = READ_POS;
            } else if (!flag.button1 && flag.button2){
                flag.button2 = false;
                programState = READ_LIGHT;
            }
            c = getchar_timeout_us(0);
            
            // detect when character over USB
            while (c != PICO_ERROR_TIMEOUT){

                // message ended
                if (c == '\n'){
                    receive[i++] = '\n';
                    receive[i] = '\0';
                    i = 0;
                    // intercept commands
                    if (intercept(receive)){
                        break;
                    };
                    i = 0;
                    programState = RECEIVE;

                // collecting characters over usb
                } else if (i < BUFFER_LENGTH-1){
                    receive[i++] = (char)c;

                // in case of overflow
                } else if (i == BUFFER_LENGTH-1){  
                    receive [i++] = '\0';
                    buzzer_play_tone(731, 100);
                    programState = RECEIVE; 
                }

                c = getchar_timeout_us(0);
            } 
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
        
}

static void sensor_task(void *arg){
    (void)arg;
    // execute commands based on movement
    // neutral position = horizontal
    float ax, ay, az, gx, gy, gz, temp;
    float movement;
    uint8_t move;
    uint32_t start_time = 0;
    uint32_t end_time = 0;
    float ax_max, ax_min, az_max, az_min, gy_max, gy_min;
    ax_max = ax_min = az_max = az_min = gy_max = gy_min = 0;
    
    while(1){

        if (programState == IDLE){

            ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &temp);
            movement = sqrt(pow(ax, 2) + pow((az-1.0),2)); // calculate significant movement

            if (movement > 0.75){ // threshold for significant movement
                start_time = time_us_32();
            }

            // collect min/max of certain variables
            while (movement > 0.75){
                move = true;
                ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &temp);
                movement = sqrt(pow(ax, 2) + pow((az-1),2));
                if (ax < ax_min){
                    ax_min = ax;
                } else if (ax_max < ax){
                    ax_max = ax;
                }

                if (az < az_min){
                    az_min = az;
                } else if (az_max < az){
                    az_max = az;
                }

                if (gy < gy_min){
                    gy_min = gy;
                } else if (gy_max < gy){
                    gy_max = gy;
                }
            }

            // compare min/max of certain variables to thresholds
            if (move){
                end_time = time_us_32();
                move = false;

                // duration of movement between 0.6 and 1.5 seconds
                if((600000 < (end_time -start_time)) && ((end_time-start_time) < 1500000)){
                    //rotation of 90° around y-axis
                    if((gy_min < -140) && (140 < gy_max) && (az_min < 0.1) && (0.55 < az_max)){
                        // rotation towards left
                        if ((ax_min < -0.85) && (-0.15 < ax_max)){
                            flower();
                        // rotation towards right
                        } else if ((ax_min < 0.15) && (0.85 < ax_max)){
                            song();
                        }
                    }
                }
                ax_max = ax_min = az_max = az_min = gy_max = gy_min = end_time = start_time = 0;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


static void read_position_task(void *arg){
    (void) arg;
    // combination of position of device and buttons create morse message
    uint8_t space = 0; 
    float ax, ay, az, gx, gy, gz, t;

    while (1){

        if (programState == READ_POS){

            rgb_led_write(255, 0, 0);

            if (flag.button1){
                if (ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t) == 0) {
                    // horizontal/flat
                    if ((0.85 < az) && (az < 1.15)){
                        message[messageCount++] = '-';
                        space = 0;
                    // pointing up/down
                    } else if (((0.85 < ay) && (ay < 1.15)) || ((-1.15 < ay) && (ay < -0.85))){
                        message[messageCount++] = '.';
                        space = 0;
                    }
                }
                flag.button1 = false;

            } else if ((flag.button2) && (messageCount < BUFFER_LENGTH-4)){
                if (space >= 2){ 
                    // two consecutive spaces -> transmit message
                    message[messageCount] = '\n';
                    space = 0;
                    programState = TRANSMIT;
                } else if (space < 2){
                    // increase count of consecutive spaces
                    message[messageCount++] = ' ';
                    space++;
                }
                flag.button2 = false;
            }

            if (messageCount == BUFFER_LENGTH-4){
                // guarantuee proper termination of string when overflow
                message[messageCount++] = ' ';
                message[messageCount++] = ' ';
                message[messageCount++] = '\n';
                message[messageCount] = '\0';
                buzzer_play_tone(731, 100);
                programState = TRANSMIT;
            }

        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


static void read_light_task(void *arg){
    (void) arg;
    // combination of absence of light and buttons create morse message
    uint8_t space = 0;

    while (1){

        if (programState == READ_LIGHT){
            
            rgb_led_write(0, 0, 255);

            if(flag.button1){
                // normal lighting conditions
                if(veml6030_read_light() > 10){
                    message[messageCount++] = '-';
                    space = 0;
                } else {
                // light sensor covered
                    message[messageCount++] = '.';
                    space = 0;
                }
                flag.button1 = false;

            } else if ((flag.button2) && (messageCount < BUFFER_LENGTH-4)){

                if (space >= 2){
                    // two consecutive spaces -> transmit message
                    message[messageCount] = '\n';
                    space = 0;
                    programState = TRANSMIT;
                } else if (space < 2) {
                    // increase count of consecutive spaces
                    message[messageCount++] = ' ';
                    space++;
                }
                flag.button2 = false;
            }

            if(messageCount == BUFFER_LENGTH-4){
                // guarantuee proper termination of string when overflow
                message[messageCount++] = ' ';
                message[messageCount++] = ' ';
                message[messageCount++] = '\n';
                message[messageCount] = '\0';
                buzzer_play_tone(731, 100);
                programState = TRANSMIT;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


static void transmit_task(void *arg){
    (void) arg;
    // transmit message over usb
    while (1){

        if (programState == TRANSMIT){
            messageCount = 0;
            intercept(message);
            if (programState == TRANSMIT){ // re-evaluation of programState in case it changed during intercept

                printf("%s", message);
                buzzer_play_tone(731, 100);
                programState = IDLE;
            }
            memcpy(message, empty, BUFFER_LENGTH);
        }

        vTaskDelay(pdMS_TO_TICKS(300));
    }
}


static void receive_task(void *arg){
    (void) arg;
    // choose how to display received message
    while (1){

        if (programState == RECEIVE){
            
            rgb_led_write(255, 255, 0);

            if (flag.button1){ // show on display
                translate(receive);
                memcpy(receive, empty, BUFFER_LENGTH);
                flag.button1 = false;
                programState = DISPLAY;

            } else if(flag.button2){// blink using LED
                flag.button2 = false;
                programState = LED;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
                
    }

}


static void display_task(void *arg) {
    (void)arg;
    // display translated message on LCD
    char display_buf[11];
    char empty1[11] = {0};
    uint8_t n;

    while(1) {

        if (programState == DISPLAY){

            rgb_led_write(255, 0, 255);

            // use button 1 to 'scroll' through screen
            if (flag.button1){
                flag.button1 = false;
                clear_display();

                // in case of full message displayed
                if (strlen(translated) == 0){
                    strcpy(display_buf, empty1);
                    memcpy(translated, empty, BUFFER_LENGTH);
                    programState = IDLE;
                }
                // display at most 11 characters
                snprintf(display_buf,11,"%s",translated); //11 characters fit on display at a time
                n = strlen(display_buf);
                write_text(display_buf);

                // iterate through translated string and shift all characters n to the left
                // n = length of display_buf = amount of characters displayed during previous cycle
                for (int i = n; i <= strlen(translated); i++){
                    translated[i-(n)] = translated[i];
                }
            }

        }

        vTaskDelay(pdMS_TO_TICKS(200));

    }

}


static void led_task(void *arg){
    (void)arg;
    // display morse code through LED
    while (1){

        if (programState == LED){

            stop_rgb_led();
            sleep_ms(1023);

            for (uint8_t i = 0; i <= strlen(receive); i++){

                if (receive[i] == '-'){
                    set_led_status(true);
                    sleep_ms(541);
                    set_led_status(false);
                    sleep_ms(389);

                } else if (receive[i] == '.'){
                    set_led_status(true);
                    sleep_ms(127);
                    set_led_status(false);
                    sleep_ms(389);

                } else if (receive[i] == ' '){
                    sleep_ms(1000);

                } else if (receive[i] == '\n'){
                    memcpy(receive, empty, BUFFER_LENGTH);
                    init_rgb_led();
                    programState = IDLE;
                }
            }

        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

    
void song(){

    // draw picture
    draw_square(52, 2, 24, 4, true);
    draw_square(44, 6, 40, 4, true);
    draw_square(40, 10, 44, 4, true);
    draw_square(36, 14, 56, 4, true);
    draw_square(36, 18, 56, 4, true);
    draw_square(32, 22, 52, 4, true);
    draw_square(32, 26, 40, 4, true);
    draw_square(32, 30, 28, 4, true);
    draw_square(32, 34, 40, 4, true);
    draw_square(32, 38, 52, 4, true);
    draw_square(36, 42, 56, 4, true);
    draw_square(36, 46, 56, 4, true);
    draw_square(40, 50, 44, 4, true);
    draw_square(44, 54, 40, 4, true);
    draw_square(52, 58, 24, 4, true);

    // play song
    musicPlayer();
    sleep_ms(1000);
    clear_display();
}

void flower(){
    // draw a flower

    //STEM
    sym(63, 63, 63, 26);
    sym(62, 63, 62, 26);
    sym(61, 63, 61, 56);
    
    
    //LEAF
    sym(60, 57, 60, 54);
    sym(59, 54, 69, 53);
    sym(58, 53, 56, 53);
    sym(56, 52, 54, 52);
    sym(54, 51, 51, 51);
    sym(51, 50, 50, 50);
    sym(50, 49, 49, 49);
    sym(49, 48, 47, 48);
    sym(47, 47, 47, 46);
    sym(46, 46, 46, 43);
    sym(47, 43, 48, 43);
    sym(48, 44, 49, 44);
    sym(49, 45, 51, 45);
    sym(51, 46, 53, 46);
    sym(53, 47, 61, 47);
    sym(61, 48, 61, 48);
    
    //PETAL1
    sym(61, 28, 61, 37);
    sym(60, 38, 60, 39);
    sym(59, 39, 59, 40);
    sym(58, 40, 58, 40);
    sym(57, 41, 55, 41);
    sym(55, 42, 53, 42);
    sym(53, 41, 51, 41);
    sym(51, 40, 50, 40);
    sym(50, 39, 50, 39);
    sym(49, 38, 49, 35);
    sym(50, 35, 50, 30);
    sym(51, 31, 51, 29);
    sym(52, 30, 52, 28);
    sym(53, 29, 53, 28);
    sym(54, 28, 54, 27);
    sym(55, 27, 57, 27);
    sym(56, 26, 62, 26);  
    
    //PETAL2
    sym(58, 24, 58, 22);
    sym(57, 22, 56, 22);
    sym(58, 21, 44, 21);
    sym(47, 22, 45, 22);
    sym(45, 23, 43, 23);
    sym(43, 24, 43, 24);
    sym(42, 25, 42, 26);
    sym(41, 26, 41, 31);
    sym(42, 31, 42, 33);
    sym(43, 33, 43, 34);
    sym(44, 34, 47, 34);
    sym(47, 33, 48, 33);
    sym(48, 32, 49, 32);
    sym(49, 31, 49, 31);
    
    //PETAL3
    sym(58, 20, 58, 17);
    sym(59, 17, 60, 17);
    sym(59, 16, 59, 14);
    sym(58, 15, 57, 15);
    sym(58, 14, 55, 14);
    sym(55, 13, 52, 13);
    sym(52, 12, 48, 12);
    sym(48, 13, 46, 13);
    sym(46, 14, 45, 14);
    sym(45, 15, 44, 15);
    sym(44, 16, 43, 16);
    sym(43, 17, 43, 19);
    sym(44, 19, 44, 20);
    sym(45, 20, 45, 20);
    
    //PETAL4
    sym(63, 16, 60, 16);
    sym(58, 13, 58, 11);
    sym(59, 11, 59, 11);
    sym(60, 10, 61, 10);
    sym(61, 9, 63, 9);
    
    sleep_ms(3000);
    clear_display();

}

int main() {

    // initialise
    stdio_init_all();
    init_hat_sdk();
    init_button1();
    init_button2();
    init_rgb_led();
    init_ICM42670();
    ICM42670_start_with_default_values();
    init_buzzer();
    init_veml6030();
    init_display();
    clear_display();
    init_led();

    // interrupts
    gpio_set_irq_enabled_with_callback(SW1_PIN, GPIO_IRQ_EDGE_RISE, true, btn_fxn);
    gpio_set_irq_enabled(SW2_PIN, GPIO_IRQ_EDGE_RISE, true);
    sleep_ms(300); //wait some time so initialization of USB and hat is done

    TaskHandle_t readPosition, readLight, transferring, idling, receiving, display_lcd ,display_led, data = NULL;

    // task creation

    BaseType_t nothing = xTaskCreate(
            idle_task,
            "idle",
            DEFAULT_STACK_SIZE,
            NULL,
            2,
            &idling);
    if(nothing != pdPASS) {
        printf("Task creation failed\n");
        return 0;
    }

    BaseType_t sensor = xTaskCreate(
            sensor_task,
            "data",
            DEFAULT_STACK_SIZE,
            NULL,
            2,
            &data);
    if(sensor != pdPASS) {
        printf("Task creation failed\n");
        return 0;
    }

    BaseType_t read_pos = xTaskCreate(
                read_position_task,       // task function
                "position",              // name of the task 
                DEFAULT_STACK_SIZE, // size of the stack for this task (in words). Generally 1024 or 2048
                NULL,               // arguments of the task 
                2,                  // priority of this task
                &readPosition);    // handle to control the execution of this task
    if(read_pos != pdPASS) { // verify task creation
        printf("Task creation failed\n");
        return 0;
    }

    BaseType_t read_light = xTaskCreate(
            read_light_task,
            "light",
            DEFAULT_STACK_SIZE,
            NULL,
            2,
            &readLight);
    if(read_light != pdPASS) {
        printf("Task creation failed\n");
        return 0;
    }

    BaseType_t outgoing = xTaskCreate(
            transmit_task,
            "transmit",
            DEFAULT_STACK_SIZE,
            NULL,
            2,
            &transferring);
    if(outgoing != pdPASS) {
        printf("Task creation failed\n");
        return 0;
    }

    BaseType_t incoming = xTaskCreate(
            receive_task,
            "receive",
            DEFAULT_STACK_SIZE,
            NULL,
            2,
            &receiving);
    if(incoming != pdPASS) {
        printf("Task creation failed\n");
        return 0;
    }

    BaseType_t screen = xTaskCreate(
            display_task,
            "lcd",
            DEFAULT_STACK_SIZE,
            NULL,
            2,
            &display_lcd);
    if(screen != pdPASS) {
        printf("Task creation failed\n");
        return 0;
    }

    BaseType_t blink = xTaskCreate(
            led_task,
            "led",
            DEFAULT_STACK_SIZE,
            NULL,
            2,
            &display_led);
    if(blink != pdPASS) {
        printf("Task creation failed\n");
        return 0;
    }
    
    // start the scheduler (never returns)
    vTaskStartScheduler();

    // never reach this line.
    return 0;
}

