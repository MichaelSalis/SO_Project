#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "eventlist.h"

int CURRENT_OUTPUT_FILE = -1;

void set_output_file(char* file){
    if (CURRENT_OUTPUT_FILE != -1){
        close(CURRENT_OUTPUT_FILE);
    }
    CURRENT_OUTPUT_FILE = open(file, O_WRONLY | O_APPEND);
}

void close_output_file(){
    if (CURRENT_OUTPUT_FILE != -1)
        close(CURRENT_OUTPUT_FILE);
}

void write_output(const char* text) {
    if (CURRENT_OUTPUT_FILE != -1) {
         write(CURRENT_OUTPUT_FILE, text, strlen(text));
    } else {
        fprintf(stderr, "Error: Output file not set\n");
    }
}

static struct EventList* event_list = NULL;
static unsigned int state_access_delay_ms = 0;

static struct timespec delay_to_timespec(unsigned int delay_ms) {
    return (struct timespec){delay_ms / 1000, (delay_ms % 1000) * 1000000};
}

static struct Event* get_event_with_delay(unsigned int event_id) {
    struct timespec delay = delay_to_timespec(state_access_delay_ms);
    nanosleep(&delay, NULL);

    return get_event(event_list, event_id);
}

static unsigned int* get_seat_with_delay(struct Event* event, size_t index) {
    struct timespec delay = delay_to_timespec(state_access_delay_ms);
    nanosleep(&delay, NULL);

    return &event->data[index];
}

static size_t seat_index(struct Event* event, size_t row, size_t col) {
    return (row - 1) * event->cols + col - 1;
}

int ems_init(unsigned int delay_ms) {
    if (event_list != NULL) {
        fprintf(stderr, "EMS state has already been initialized\n");
        return 1;
    }

    event_list = create_list();
    state_access_delay_ms = delay_ms;

    return event_list == NULL;
}

int ems_terminate() {
    if (event_list == NULL) {
        fprintf(stderr, "EMS state must be initialized\n");
        return 1;
    }

    free_list(event_list);
    return 0;
}

int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols) {
    if (event_list == NULL) {
        fprintf(stderr, "EMS state must be initialized\n");
        return 1;
    }

    if (get_event_with_delay(event_id) != NULL) {
        fprintf(stderr, "Event already exists\n");
        return 1;
    }

    struct Event* event = malloc(sizeof(struct Event));

    if (event == NULL) {
        fprintf(stderr, "Error allocating memory for event\n");
        return 1;
    }

    event->id = event_id;
    event->rows = num_rows;
    event->cols = num_cols;
    event->reservations = 0;
    event->data = malloc(num_rows * num_cols * sizeof(unsigned int));

    if (event->data == NULL) {
        fprintf(stderr, "Error allocating memory for event data\n");
        free(event);
        return 1;
    }

    for (size_t i = 0; i < num_rows * num_cols; i++) {
        event->data[i] = 0;
    }

    if (append_to_list(event_list, event) != 0) {
        fprintf(stderr, "Error appending event to list\n");
        free(event->data);
        free(event);
        return 1;
    }

    return 0;
}

int ems_reserve(unsigned int event_id, size_t num_seats, size_t* xs, size_t* ys) {
    if (event_list == NULL) {
        fprintf(stderr, "EMS state must be initialized\n");
        return 1;
    }

    struct Event* event = get_event_with_delay(event_id);

    if (event == NULL) {
        fprintf(stderr, "Event not found\n");
        return 1;
    }

    unsigned int reservation_id = ++event->reservations;

    size_t i = 0;
    for (; i < num_seats; i++) {
        size_t row = xs[i];
        size_t col = ys[i];

        if (row <= 0 || row > event->rows || col <= 0 || col > event->cols) {
            fprintf(stderr, "Invalid seat\n");
            break;
        }

        if (*get_seat_with_delay(event, seat_index(event, row, col)) != 0) {
            fprintf(stderr, "Seat already reserved\n");
            break;
        }

        *get_seat_with_delay(event, seat_index(event, row, col)) = reservation_id;
    }

    if (i < num_seats) {
        event->reservations--;
        for (size_t j = 0; j < i; j++) {
            *get_seat_with_delay(event, seat_index(event, xs[j], ys[j])) = 0;
        }
        return 1;
    }

    return 0;
}

int ems_show(unsigned int event_id) {
    if (event_list == NULL) {
        fprintf(stderr, "EMS state must be initialized\n");
        return 1;
    }

    struct Event* event = get_event_with_delay(event_id);

    if (event == NULL) {
        fprintf(stderr, "Event not found\n");
        return 1;
    }

    for (size_t i = 1; i <= event->rows; i++) {
        for (size_t j = 1; j <= event->cols; j++) {
            unsigned int* seat = get_seat_with_delay(event, seat_index(event, i, j));
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%u", *seat);
            write_output(buffer);

            if (j < event->cols) {
                write_output(" ");
            }
        }

        write_output("\n");
    }

    return 0;
}

int ems_list_events() {
    if (event_list == NULL) {
        fprintf(stderr, "EMS state must be initialized\n");
        return 1;
    }

    if (event_list->head == NULL) {
        write_output("No events\n");
        return 0;
    }

    struct ListNode* current = event_list->head;
    while (current != NULL) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "Event: %u\n", (current->event)->id);
        write_output(buffer);
        current = current->next;
    }

    return 0;
}

void ems_wait(unsigned int delay_ms) {
    struct timespec delay = delay_to_timespec(delay_ms);
    nanosleep(&delay, NULL);
}
