#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#define MAX_QUES_LEN 300
#define MAX_OPTION_LEN 100

volatile int timeout_happened = 0;

const char* COLOR_END = "\033[0m";
const char* RED = "\033[1;31m";
const char* GREEN = "\033[1;32m";
const char* YELLOW = "\033[1;33m";
const char* BLUE = "\033[1;34m";
const char* PINK = "\033[1;35m";
const char* AQUA = "\033[1;36m";

typedef struct {
    char text[MAX_QUES_LEN];
    char options[4][MAX_OPTION_LEN];
    char correct_option;
    int timeout;
    int prize_money;
} Question;

int read_questions(char* file_name, Question** questions);
void print_formatted_question(Question question);
void play_game(Question* questions, int no_of_questions);
int use_lifeline(Question* question, int* lifeline);

struct termios old_props;
void reset_terminal_attributes();
void set_terminal_attributes();

void timeout_handler();

int main() {
    srand(time(NULL));
    set_terminal_attributes();
    printf("\t\t%sChalo Kehlte hain KAUN BANEGA CROREPATI !!!%s\n", PINK, COLOR_END);

    Question* questions;
    int no_of_questions = read_questions("questions.txt", &questions);

    play_game(questions, no_of_questions);

    reset_terminal_attributes(); // ensure terminal resets before exit
    free(questions);
    return 0;
}

void timeout_handler() {
    timeout_happened = 1;
    printf("%s\n\nTime out!!!!! Press Any Key...%s\n", RED, COLOR_END);
    fflush(stdout);
    tcflush(STDIN_FILENO, TCIFLUSH); // flush pending input
}

void play_game(Question* questions, int no_of_questions) {
    int money_won = 0;
    int lifeline[] = {1, 1}; // 50-50, Skip

    signal(SIGALRM, timeout_handler);

    for (int i = 0; i < no_of_questions; i++) {
        timeout_happened = 0;
        print_formatted_question(questions[i]);

        alarm(questions[i].timeout);

        char ch = getchar();
        while (ch == '\n') ch = getchar(); // ignore newline
        alarm(0);

        printf("%c\n", ch);
        ch = toupper(ch);

        if (timeout_happened == 1) {
            break;
        }

        if (ch == 'L') {
            int value = use_lifeline(&questions[i], lifeline);
            if (value != 2) {
                i--; // ask the same question again if lifeline not skip
            }
            continue;
        }

        if (ch == questions[i].correct_option) {
            printf("%s\nCorrect!%s\n", GREEN, COLOR_END);
            money_won = questions[i].prize_money;
            printf("%sYou have won: Rs %d%s\n", BLUE, money_won, COLOR_END);
        } else {
            printf("%s\nWrong! Correct answer is %c.%s\n", RED, questions[i].correct_option, COLOR_END);
            break;
        }
    }
    printf("\n\n%sGame Over! Your total winnings are: Rs %d%s\n", BLUE, money_won, COLOR_END);
}

int use_lifeline(Question* question, int* lifeline) {
    printf("\n\n%sAvailable Lifelines:%s", PINK, COLOR_END);
    if (lifeline[0]) printf("\n%s1. Fifty-Fifty (50/50)%s", PINK, COLOR_END);
    if (lifeline[1]) printf("\n%s2. Skip the Question%s", PINK, COLOR_END);
    printf("\n%sChoose a lifeline or 0 to return: %s", PINK, COLOR_END);

    char ch = getchar();
    while (ch == '\n') ch = getchar(); // ignore newline
    printf("%c\n", ch);

    switch (ch) {
        case '1':
            if (lifeline[0]) {
                lifeline[0] = 0;
                int removed = 0;
                while (removed < 2) {
                    int num = rand() % 4;
                    if ((num + 'A') != question->correct_option && question->options[num][0] != '\0') {
                        question->options[num][0] = '\0';
                        removed++;
                    }
                }
                return 1;
            }
            break;

        case '2':
            if (lifeline[1]) {
                lifeline[1] = 0;
                return 2; // skip question
            }
            break;

        default:
            printf("\n%sReturning to the Question.%s\n", PINK, COLOR_END);
            break;
    }
    return 0;
}

void print_formatted_question(Question question) {
    printf("\n\n%s%s%s\n", YELLOW, question.text, COLOR_END);
    for (int i = 0; i < 4; i++) {
        if (question.options[i][0] != '\0') {
            printf("%s%c. %s%s\n", AQUA, ('A' + i), question.options[i], COLOR_END);
        }
    }
    printf("%sHurry!! You have only %d Seconds to answer..%s\n", YELLOW, question.timeout, COLOR_END);
    printf("%sEnter your answer (A, B, C, or D) or L for lifeline: %s", GREEN, COLOR_END);
}

int read_questions(char* file_name, Question** questions) {
    FILE *file = fopen(file_name, "r");
    if (file == NULL) {
        printf("\nUnable to open the questions bank.\n");
        reset_terminal_attributes();
        exit(1);
    }

    char str[MAX_QUES_LEN];
    int no_of_lines = 0;
    while (fgets(str, MAX_QUES_LEN, file)) {
        no_of_lines++;
    }
    int no_of_questions = no_of_lines / 8;

    *questions = (Question *) malloc(no_of_questions * sizeof(Question));
    rewind(file);

    for (int i = 0; i < no_of_questions; i++) {
        fgets((*questions)[i].text, MAX_QUES_LEN, file);
        for (int j = 0; j < 4; j++) {
            fgets((*questions)[i].options[j], MAX_OPTION_LEN, file);
            (*questions)[i].options[j][strcspn((*questions)[i].options[j], "\n")] = '\0'; // remove newline
        }

        char option[10];
        fgets(option, 10, file);
        (*questions)[i].correct_option = toupper(option[0]);

        char timeout[10];
        fgets(timeout, 10, file);
        (*questions)[i].timeout = atoi(timeout);

        char prize_money[10];
        fgets(prize_money, 10, file);
        (*questions)[i].prize_money = atoi(prize_money);
    }

    fclose(file);
    return no_of_questions;
}

void set_terminal_attributes() {
    tcgetattr(STDIN_FILENO, &old_props);
    atexit(reset_terminal_attributes);
    struct termios new_props = old_props;
    new_props.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_props);
}

void reset_terminal_attributes() {
    tcsetattr(STDIN_FILENO, TCSANOW, &old_props);
}
