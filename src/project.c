#include <stdio.h>
#include <string.h>

#include <stdlib.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "tkjhat/sdk.h"



// Default stack size for the tasks. It can be reduced to 1024 if task is not using lot of memory.
#define DEFAULT_STACK_SIZE 2048
#define BUFFER_LENGTH 128 

// https://github.com/robsoncouto/arduino-songs/blob/master/pacman/pacman.ino
#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978
#define REST      0


enum state { IDLE=1, READ_POS, TRANSMIT, RECEIVE, DISPLAY, LED, MUSIC};
enum state programState = IDLE;
struct flags{
    uint8_t button1;
    uint8_t button2;
} flag = {false, false};
char message[BUFFER_LENGTH];
char receive[BUFFER_LENGTH];
char translated[BUFFER_LENGTH];
char empty[BUFFER_LENGTH] = {0};
uint8_t messageCount = 0;
int8_t c;

int melody[] = { // https://github.com/robsoncouto/arduino-songs/blob/master/pacman/pacman.ino

    // Pacman
    // Score available at https://musescore.com/user/85429/scores/107109
    NOTE_B4, 16, NOTE_B5, 16, NOTE_FS5, 16, NOTE_DS5, 16, //1
    NOTE_B5, 32, NOTE_FS5, -16, NOTE_DS5, 8, NOTE_C5, 16,
    NOTE_C6, 16, NOTE_G6, 16, NOTE_E6, 16, NOTE_C6, 32, NOTE_G6, -16, NOTE_E6, 8,

    NOTE_B4, 16,  NOTE_B5, 16,  NOTE_FS5, 16,   NOTE_DS5, 16,  NOTE_B5, 32,  //2
    NOTE_FS5, -16, NOTE_DS5, 8,  NOTE_DS5, 32, NOTE_E5, 32,  NOTE_F5, 32,
    NOTE_F5, 32,  NOTE_FS5, 32,  NOTE_G5, 32,  NOTE_G5, 32, NOTE_GS5, 32,  NOTE_A5, 16, NOTE_B5, 8
    };

void morse_char(char morse, uint8_t *space){
    if (messageCount < BUFFER_LENGTH){
        message[messageCount++] = morse;
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
                                "...", "-", "..-", "...-", ".--", "-..-", "-.--", "--.."}; //array of pointers (internal string arrays decay into pointer to first element.)

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

// https://github.com/robsoncouto/arduino-songs/blob/master/pacman/pacman.ino

// change this to make the song slower or faster
//int tempo = 105;

// notes of the moledy followed by the duration.
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
void pacman(uint8_t y, uint8_t x1, uint8_t x2){
    for(int i = y; i<(y+4); i++){
            draw_line(x1, i, x2, i);
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
            //flag.button2 = false; //temporary, otherwise program may get stuck
            if (flag.button1 && !flag.button2){
                flag.button1 = false;
                programState = READ_POS; //comment to force DISPLAY mode
                //programState = DISPLAY; //uncomment to force into DISPLAY mode
            } else if (flag.button1 && flag.button2){
                flag.button1 = false;
                flag.button2 = false;
                programState = MUSIC;
            }
            c = getchar_timeout_us(0); //uncomment for RECEIVE mode
            if (c != PICO_ERROR_TIMEOUT){
                receive[0] = (char)c;
                programState = RECEIVE;
            }
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
                        printf("%d", spaceCount);
                    } else if (((0.85 < ay) && (ay < 1.15)) || ((ay < -0.85) && (-1 < ay))){
                        morse_char('.', &spaceCount);
                        printf("%d", spaceCount);
                    }
                }
            } else if((flag.button2) && (messageCount < BUFFER_LENGTH)){
                if (spaceCount == 2){ //will Transmit after 2x button2 or 3x button2
                    message[messageCount] = '\n';
                    spaceCount = 0;
                    printf("%d", spaceCount);
                    programState = TRANSMIT;
                } else {
                    message[messageCount++] = ' ';
                    spaceCount++;
                    printf("%d", spaceCount);
                }
                flag.button2 = false;
            } else if(messageCount == BUFFER_LENGTH){
                programState = TRANSMIT;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void transmit_task(void *arg){
    (void) arg;

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
            // char display_buf[11];
            while (c != PICO_ERROR_TIMEOUT){
                if (c == '\n'){
                    receive[i++] = '\n';
                    receive[i] = '\0';
                    i = 0;
                    printf("%s", receive);
                    while (1){
                        if(flag.button1){
                            translate(receive);
                            printf("%s", translated);
                            flag.button1 = false;
                            programState = DISPLAY;
                            break;
                        }
                        else if (flag.button2){
                            flag.button2 = false;
                            programState = LED;
                            break;
                        }
                    }
                }
                else if (i < BUFFER_LENGTH -1){
                    receive[i++] = (char)c;
                    //printf("%c", c);
                }
                //add buffer overflow
                c = getchar_timeout_us(0);
            } 
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }

}

static void display_task(void *arg) {
    (void)arg;

    //char translated[BUFFER_LENGTH] = "abcdefghijklmnopqrstuvwxyz";
    char display_buf[11];
    uint8_t n;

    while(1) {

        if (programState == DISPLAY){
            rgb_led_write(255, 0, 255);
            if (flag.button1){
                flag.button1 = false;
                clear_display();
                if (strlen(translated) == 0){
                    strcpy(translated, empty); 
                    programState = IDLE;
                }
                snprintf(display_buf,11,"%s",translated);
                n = strlen(display_buf);
                write_text(display_buf);
                for (int i = (n); i <= strlen(translated); i++){
                    translated[i-(n)] = translated[i];
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));

    }

}
    
static void music_task(void *arg){
    (void)arg;

    while (1){
        if (programState == MUSIC){

            rgb_led_write(0, 0, 0);
            pacman(2, 52, 76);
            pacman(6, 44, 84);
            pacman(10, 40, 88);
            pacman(14, 36, 92);
            pacman(18, 36, 92);
            pacman(22, 32, 84);
            pacman(26, 32, 72);
            pacman(30, 32, 60);
            pacman(34, 32, 72);
            pacman(38, 32, 84);
            pacman(42, 36, 92);
            pacman(46, 36, 92);
            pacman(50, 40, 88);
            pacman(54, 44, 84);
            pacman(58, 52, 76);
            musicPlayer();
            clear_display();
            programState = IDLE;
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
    //while (!stdio_usb_connected()){
    //    sleep_ms(10);
    //} 

    TaskHandle_t readPosition, transfer, idling, receive, lcd, song = NULL;
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

    BaseType_t music = xTaskCreate(music_task,
            "song",
            DEFAULT_STACK_SIZE,
            NULL,
            2,
            &song);
    if(music != pdPASS) {
        printf("Task creation failed\n");
        return 0;
    }
    
    // Start the scheduler (never returns)
    vTaskStartScheduler();

    // Never reach this line.
    return 0;
}

