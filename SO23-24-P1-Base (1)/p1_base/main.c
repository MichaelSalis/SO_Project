#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "constants.h"
#include "operations.h"
#include "parser.h"

int main(int argc, char *argv[]) {
  unsigned int state_access_delay_ms = STATE_ACCESS_DELAY_MS;

  if (argc > 1) {
    char *endptr;
    unsigned long int delay = strtoul(argv[1], &endptr, 10);

    if (*endptr != '\0' || delay > UINT_MAX) {
      fprintf(stderr, "Invalid delay value or value too large\n");
      return 1;
    }

    state_access_delay_ms = (unsigned int)delay;
  }

  if (ems_init(state_access_delay_ms)) {
    fprintf(stderr, "Failed to initialize EMS\n");
    return 1;
  }

  DIR *dirp;
    struct dirent *dp;
    dirp = opendir(argv[1]);

    char *files[100];
    char *files_output[100];
    int amount_of_files = 0;

    if (dirp == NULL) {
        perror("opendir failed");
        return;
    }

    for (;;) {
        errno = 0; 
        dp = readdir(dirp);
        if (dp == NULL)
            break;
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
            continue; /* Skip . and .. */
        

        char *file = malloc(128);
      // Find the position of the last dot (.) in the input file name
    const char *dotPosition = strrchr(file, '.');

    // Check if a dot was found and it is not the first character in the file name
    if (dotPosition != NULL && dotPosition != file) {
        // Calculate the length of the file name excluding the extension
        size_t fileNameLength = dotPosition - file;

        // Allocate memory for the output file name
        char *outputFileName = (char *)malloc(fileNameLength + 5); // 5 for ".out\0"

        if (outputFileName != NULL) {
            // Copy the file name to the output file name
            strncpy(outputFileName, file, fileNameLength);

            // Add the ".out" extension
            strcpy(outputFileName + fileNameLength, ".out");

            // Create and open the output file
            FILE *outputFile = fopen(outputFileName, "w");

            if (outputFile != NULL) {
                // Close the output file
                fclose(outputFile);
                printf("Output file '%s' created successfully.\n", outputFileName);
            } else {
                printf("Error creating output file '%s'.\n", outputFileName);
            }

            // Free the allocated memory for the output file name
            free(outputFileName);
        } else {
            printf("Memory allocation error.\n");
        }
    } else {
        printf("Invalid file name format. Unable to determine extension.\n");
    }
        files[amount_of_files] = strdup(file);
        files_output[amount_of_files] = strdup(outputFile);
        amount_of_files++;
    }

    closedir(dirp);
int file_num = 0
  set_output_file(files_output[file_num]);

  while (1) {
    unsigned int event_id, delay;
    size_t num_rows, num_columns, num_coords;
    size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];

    printf("> ");
    fflush(stdout);

    switch (get_next(file[file_num])) {
      case CMD_CREATE:
        if (parse_create(file[file_num], &event_id, &num_rows, &num_columns) != 0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (ems_create(event_id, num_rows, num_columns)) {
          fprintf(stderr, "Failed to create event\n");
        }

        break;

      case CMD_RESERVE:
        num_coords = parse_reserve(file[file_num], MAX_RESERVATION_SIZE, &event_id, xs, ys);

        if (num_coords == 0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (ems_reserve(event_id, num_coords, xs, ys)) {
          fprintf(stderr, "Failed to reserve seats\n");
        }

        break;

      case CMD_SHOW:
        if (parse_show(file[file_num], &event_id) != 0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (ems_show(event_id)) {
          fprintf(stderr, "Failed to show event\n");
        }

        break;

      case CMD_LIST_EVENTS:
        if (ems_list_events()) {
          fprintf(stderr, "Failed to list events\n");
        }

        break;

      case CMD_WAIT:
        if (parse_wait(file[file_num], &delay, NULL) == -1) {  // thread_id is not implemented
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (delay > 0) {
          printf("Waiting...\n");
          ems_wait(delay);
        }

        break;

      case CMD_INVALID:
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        break;

      case CMD_HELP:
        printf(
            "Available commands:\n"
            "  CREATE <event_id> <num_rows> <num_columns>\n"
            "  RESERVE <event_id> [(<x1>,<y1>) (<x2>,<y2>) ...]\n"
            "  SHOW <event_id>\n"
            "  LIST\n"
            "  WAIT <delay_ms> [thread_id]\n"  // thread_id is not implemented
            "  BARRIER\n"                      // Not implemented
            "  HELP\n");

        break;

      case CMD_BARRIER:  // Not implemented
      case CMD_EMPTY:
        break;

      case EOC:
        if (file_num < amount_of_files) {
            file_num++;
            set_output_file(files_output[file_num]);
        }
        else{
          close_output_file();
          ems_terminate();
          return 0;
        }
    }
  }
}
