// COMP3511 Spring 2023
// PA3: Page Replacement Algorithms
//
// Your name:
// Your ITSC email:           @connect.ust.hk
//
// Declaration:
//
// I declare that I am not involved in plagiarism
// I understand that both parties (i.e., students providing the codes and students copying the codes) will receive 0 marks.

// ===
// Region: Header files
// Note: Necessary header files are included, do not include extra header files
// ===
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// ===
// Region: Constants
// ===

#define UNFILLED_FRAME -1
#define MAX_QUEUE_SIZE 10
#define MAX_FRAMES_AVAILABLE 10
#define MAX_REFERENCE_STRING 30

#define ALGORITHM_FIFO "FIFO"
#define ALGORITHM_OPT "OPT"
#define ALGORITHM_LRU "LRU"
#define ALGORITHM_CLOCK "CLOCK"

// Keywords (to be used when parsing the input)
#define KEYWORD_ALGORITHM "algorithm"
#define KEYWORD_FRAMES_AVAILABLE "frames_available"
#define KEYWORD_REFERENCE_STRING_LENGTH "reference_string_length"
#define KEYWORD_REFERENCE_STRING "reference_string"



// Assume that we only need to support 2 types of space characters:
// " " (space), "\t" (tab)
#define SPACE_CHARS " \t"

// ===
// Region: Global variables:
// For simplicity, let's make everything static without any dyanmic memory allocation
// In other words, we don't need to use malloc()/free()
// It will save you lots of time to debug if everything is static
// ===
char algorithm[10];
int reference_string[MAX_REFERENCE_STRING];
int reference_string_length;
int frames_available;
int frames[MAX_FRAMES_AVAILABLE];

// Helper function: Check whether the line is a blank line (for input parsing)
int is_blank(char *line)
{
    char *ch = line;
    while (*ch != '\0')
    {
        if (!isspace(*ch))
            return 0;
        ch++;
    }
    return 1;
}
// Helper function: Check whether the input line should be skipped
int is_skip(char *line)
{
    if (is_blank(line))
        return 1;
    char *ch = line;
    while (*ch != '\0')
    {
        if (!isspace(*ch) && *ch == '#')
            return 1;
        ch++;
    }
    return 0;
}
// Helper: parse_tokens function
void parse_tokens(char **argv, char *line, int *numTokens, char *delimiter)
{
    int argc = 0;
    char *token = strtok(line, delimiter);
    while (token != NULL)
    {
        argv[argc++] = token;
        token = strtok(NULL, delimiter);
    }
    *numTokens = argc;
}

// Helper: parse the input file
void parse_input()
{
    FILE *fp = stdin;
    char *line = NULL;
    ssize_t nread;
    size_t len = 0;

    char *two_tokens[2];                                 // buffer for 2 tokens
    char *reference_string_tokens[MAX_REFERENCE_STRING]; // buffer for the reference string
    int numTokens = 0, n = 0, i = 0;
    char equal_plus_spaces_delimiters[5] = "";

    strcpy(equal_plus_spaces_delimiters, "=");
    strcat(equal_plus_spaces_delimiters, SPACE_CHARS);

    while ((nread = getline(&line, &len, fp)) != -1)
    {
        if (is_skip(line) == 0)
        {
            line = strtok(line, "\n");
            if (strstr(line, KEYWORD_ALGORITHM))
            {
                parse_tokens(two_tokens, line, &numTokens, equal_plus_spaces_delimiters);
                if (numTokens == 2)
                {
                    strcpy(algorithm, two_tokens[1]);
                }
            }
            else if (strstr(line, KEYWORD_FRAMES_AVAILABLE))
            {
                parse_tokens(two_tokens, line, &numTokens, equal_plus_spaces_delimiters);
                if (numTokens == 2)
                {
                    sscanf(two_tokens[1], "%d", &frames_available);
                }
            }
            else if (strstr(line, KEYWORD_REFERENCE_STRING_LENGTH))
            {
                parse_tokens(two_tokens, line, &numTokens, equal_plus_spaces_delimiters);
                if (numTokens == 2)
                {
                    sscanf(two_tokens[1], "%d", &reference_string_length);
                }
            }
            else if (strstr(line, KEYWORD_REFERENCE_STRING))
            {
                parse_tokens(two_tokens, line, &numTokens, "=");
                if (numTokens == 2)
                {
                    parse_tokens(reference_string_tokens, two_tokens[1], &n, SPACE_CHARS);
                    for (i = 0; i < n; i++)
                    {
                        sscanf(reference_string_tokens[i], "%d", &reference_string[i]);
                    }
                }
            }
        }
    }
}
// Helper: Display the parsed values
void print_parsed_values()
{
    int i;
    printf("%s = %s\n", KEYWORD_ALGORITHM, algorithm);
    printf("%s = %d\n", KEYWORD_FRAMES_AVAILABLE, frames_available);
    printf("%s = %d\n", KEYWORD_REFERENCE_STRING_LENGTH, reference_string_length);
    printf("%s = ", KEYWORD_REFERENCE_STRING);
    for (i = 0; i < reference_string_length; i++)
        printf("%d ", reference_string[i]);
    printf("\n");
}

// Useful string template used in printf()
// We will use diff program to auto-grade the PA2 submissions
// Please use the following templates in printf to avoid formatting errors
//
// Example:
//
//   printf(template_total_page_fault, 0)    # Total Page Fault: 0 is printed on the screen
//   printf(template_no_page_fault, 0)       # 0: No Page Fault is printed on the screen

const char template_total_page_fault[] = "Total Page Fault: %d\n";
const char template_no_page_fault[] = "%d: No Page Fault\n";

// Helper function:
// This function is useful for printing the fault frames in this format:
// current_frame: f0 f1 ...
//
// For example: the following 4 lines can use this helper function to print
//
// 7: 7
// 0: 7 0
// 1: 7 0 1
// 2: 2 0 1
//
// For the non-fault frames, you should use template_no_page_fault (see above)
//
void display_fault_frame(int current_frame)
{
    int j;
    printf("%d: ", current_frame);
    for (j = 0; j < frames_available; j++)
    {
        if (frames[j] != UNFILLED_FRAME)
            printf("%d ", frames[j]);
        else
            printf("  ");
    }
    printf("\n");
}

// Helper function: initialize the frames
void frames_init()
{
    int i;
    for (i = 0; i < frames_available; i++)
        frames[i] = UNFILLED_FRAME;
}

// * Queue Implementation
typedef struct Queue {
    int q[MAX_FRAMES_AVAILABLE];
    int rear, capacity;
} Queue;

// void new_queue(Queue* queue, int capacity){
//     Queue queue;
//     queue.capacity = capacity;
//     queue.rear = 0;
//     return &queue;
// }

void qEnqueue(Queue* q, int data){
    if (q->capacity==q->rear){
        printf("error: queue full");
        return;
    }
    // printf("%d\n", q->q[0]);
    q->q[q->rear++] = data;
    // printf("%d\n", q->q[0]);
    fflush(stdout);
    return;
}

int qDequeue(Queue* q){
    if (q->rear == 0){
        printf("error: queue empty");
        return -1;
    }

    int data = q->q[0];
    for (int i=1; i < q->rear; ++i){
        q->q[i-1] = q->q[i];
    }
    q->rear--;
    return data;
}

void FIFO_replacement()
{
    // TODO: Implement FIFO replacement here
    // printf("hello\n");
    Queue queue;
    queue.capacity = frames_available;
    queue.rear = 0;
    int faults = 0;
    // printf("FIFO init\n");

    for (int ref = 0; ref < reference_string_length; ++ref){

        int cur_page = reference_string[ref];
        // Search if page already in memory
        int f = 0;
        for (; f<frames_available; f++){
            if (frames[f] == cur_page){
                printf(template_no_page_fault, cur_page);
                break;
            }
        }
        if (f<frames_available) continue;

        // printf("Page not in frames: %d\n", f);
        // * Page not in memory
        faults++;

        // Find empty frame
        f = 0;
        while (f < frames_available && frames[f]!=UNFILLED_FRAME) ++f;
        // printf("Empty Frame: %d\n", f);
        // fflush(stdout);

        // if empty frame not found, select page to remove
        if (f == frames_available){
            //pop first item in queue
            int page = qDequeue(&queue);
            // find which frame the page currently occupies
            f = 0;
            while (f < frames_available && frames[f]!=page) ++f;
            if (f == frames_available) printf("error: page not found in frame");
        }

        // load page
        frames[f] = cur_page;
        qEnqueue(&queue, cur_page);

        display_fault_frame(cur_page);
    }

    printf(template_total_page_fault, faults);
}

void OPT_replacement()
{
    // TODO: Implement OPT replacement here
    int faults = 0;

    for (int ref = 0; ref < reference_string_length; ++ref){
        
        int cur_page = reference_string[ref];
        // Search if page already in memory
        int f = 0;
        for (; f<frames_available; f++){
            if (frames[f] == cur_page){
                printf(template_no_page_fault, cur_page);
                break;
            }
        }
        if (f<frames_available) continue;

        // * Page not in memory
        faults++;

        // Find empty frame
        f = 0;
        while (f < frames_available && frames[f]!=UNFILLED_FRAME) ++f;

        // if empty frame not found, select page to remove
        if (f == frames_available){
            // ? iter through frames, find & store first occurance of each?
            int next_use[frames_available];
            for (int i=0; i<frames_available; i++){
                int j = ref+1;
                for (; j<reference_string_length; j++){
                    if (reference_string[j]==frames[i]){
                        next_use[i] = j;
                        break;
                    }
                }
                if (j==reference_string_length) next_use[i]=reference_string_length;
            }

            // ? find latest max occurance
            f = 0;
            for (int i=1; i<frames_available; i++){
                if (next_use[i] >= next_use[f]){
                    if(next_use[i]>next_use[f] || frames[i]<frames[f]){
                        f = i;
                    }
                }
            }
        }

        // load page
        frames[f] = cur_page;
        display_fault_frame(cur_page);
    }
    
    printf(template_total_page_fault, faults);
}

void LRU_replacement()
{
    // TODO: Implement LRU replacement here
    int counter[MAX_FRAMES_AVAILABLE];
    for (int i=0; i<MAX_FRAMES_AVAILABLE; i++){
        counter[i]=-1;
    }
    int faults = 0;

    for (int ref = 0; ref < reference_string_length; ++ref){

        for (int j=0; j<MAX_FRAMES_AVAILABLE; j++) printf("%d ", counter[j]);
        printf("\n");
        
        int cur_page = reference_string[ref];
        // Search if page already in memory
        int f = 0;
        for (; f<frames_available; f++){
            if (frames[f] == cur_page){
                printf(template_no_page_fault, cur_page);
                counter[cur_page] = ref;
                break;
            }
        }
        if (f<frames_available) continue;

        // * Page not in memory
        faults++;

        // Find empty frame
        f = 0;
        while (f < frames_available && frames[f]!=UNFILLED_FRAME) ++f;

        // if empty frame not found, select page to remove
        if (f == frames_available){
            // ? search counter for each frame and select lowest
            f = 0;
            for (int i = 1; i<frames_available; ++i){
                if (counter[frames[i]]<0) prinf("error: inccorect frame access\n");
                if (counter[frames[i]] < counter[frames[i]]) f = i;
            }
        }

        // load page
        frames[f] = cur_page;
        counter[cur_page] = ref;
        display_fault_frame(cur_page);
    }
    
    printf(template_total_page_fault, faults);
}

void CLOCK_replacement()
{
    // TODO: Implement CLOCK replacement here
    // ? if in frame: set sc=1
    // ? else cycle through frames, set all sc=0 until find sc==0, move pointer
}

int main()
{
    parse_input();              
    print_parsed_values();      
    frames_init();              

    if (strcmp(algorithm, ALGORITHM_FIFO) == 0)
    {
        FIFO_replacement();
    }
    else if (strcmp(algorithm, ALGORITHM_OPT) == 0)
    {
        OPT_replacement();
    }
    else if (strcmp(algorithm, ALGORITHM_LRU) == 0)
    {
        LRU_replacement();
    }
    else if (strcmp(algorithm, ALGORITHM_CLOCK) == 0)
    {
        CLOCK_replacement();
    }

    return 0;
}
