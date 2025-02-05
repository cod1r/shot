#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#define DEBUG(X, F, FILENAME)                                                   \
  {int fd = open(FILENAME, O_RDWR | O_APPEND | O_CREAT);                        \
  if (fd == -1) exit(EXIT_FAILURE);                                             \
  dprintf(fd, F, X);                                                            \
  close(fd);}

// We are only allowing ascii characters for now.
// Unicode is a pain
const int32_t MIN_ALPHABET_VALUE = 32;
const int32_t MAX_ALPHABET_VALUE = 126;
const int32_t ALPHABET_LEN = 126 - 32 + 1;

struct termios set_up_terminal() {
  if (freopen("/dev/tty", "r", stdin) == NULL) {
    fprintf(stderr, "freopen failed");
    exit(EXIT_FAILURE);
  }
  struct termios term_settings;
  if (tcgetattr(STDIN_FILENO, &term_settings) == -1) {
    fprintf(stderr, "tcgetattr failed on stdin\n");
    exit(EXIT_FAILURE);
  }
  struct termios term_settings_copy = term_settings;
  term_settings.c_lflag &= ~ICANON;
  term_settings.c_lflag &= ~ECHO;
  term_settings.c_lflag &= ~ISIG;
  term_settings.c_cc[VMIN] = 1;
  term_settings.c_cc[VTIME] = 0;
  if (tcsetattr(STDIN_FILENO, TCSANOW, &term_settings) == -1) {
    fprintf(stderr, "tcsetattr_new failed on stdin\n");
    exit(EXIT_FAILURE);
  }
  dprintf(STDERR_FILENO, "\033[?1049h");
  return term_settings_copy;
}
void reset_terminal(struct termios term_settings) {
  if (tcsetattr(STDIN_FILENO, TCSANOW, &term_settings) == -1) {
    fprintf(stderr, "tcsetattr_new failed on stdin\n");
    exit(EXIT_FAILURE);
  }
}
int32_t min(int32_t a, int32_t b) { return a < b ? a : b; }
int32_t max(int32_t a, int32_t b) { return a > b ? a : b; }
int32_t **construct_boyer_moore_bad_character_table(char *pattern,
                                                    int pattern_length) {
  int **table = malloc(ALPHABET_LEN * sizeof(int *));
  for (int32_t i = 0; i < ALPHABET_LEN; ++i) {
    table[i] = malloc(pattern_length * sizeof(int32_t));
    // initial value should be -1 because there could not be
    // a next-highest i character that occurs at index j in pattern
    memset(table[i], -1, pattern_length * sizeof(int32_t));
  }

  for (int32_t i = 0; i < ALPHABET_LEN; ++i) {
    for (int32_t j = 0; j < pattern_length; ++j) {
      if (pattern[j] - MIN_ALPHABET_VALUE == i) {
        table[i][j] = j;
      } else if (j) {
        table[i][j] = table[i][j - 1];
      }
    }
  }
  return table;
}
int32_t boyer_moore_string_search(int32_t **boyer_moore_table_bad_character,
                                  char *pattern, int32_t pattern_length,
                                  char *text, int32_t text_length) {
  if (pattern_length == 0)
    return -1;
  int32_t *good_suffix_preprocess = malloc(pattern_length * sizeof(int32_t));
  memset(good_suffix_preprocess, 0, pattern_length * sizeof(int32_t));
  int32_t last_compare_index = pattern_length - 1;
  for (int32_t i = pattern_length - 2; i >= 0;) {
    int32_t last_compare_idx_cpy = last_compare_index;
    int32_t i_cpy = i;
    while (pattern[last_compare_idx_cpy] == pattern[i_cpy]) {
      --last_compare_idx_cpy;
      --i_cpy;
    }
    if (i - i_cpy > 1) {
      good_suffix_preprocess[last_compare_index] = i;
      i = i_cpy;
      last_compare_index = last_compare_idx_cpy;
    } else {
      --i;
    }
  }
  for (int32_t i = pattern_length - 1; i < text_length;) {
    int32_t pattern_character_compare_index = pattern_length - 1;
    int32_t text_character_compare_index = i;
    int32_t shift_bad_character_rule = 0;
    while (pattern_character_compare_index >= 0 &&
           text_character_compare_index >= 0 &&
           pattern[pattern_character_compare_index] ==
               text[text_character_compare_index]) {
      --pattern_character_compare_index;
      --text_character_compare_index;
    }
    if (pattern_character_compare_index >= 0) {
      int32_t j =
          boyer_moore_table_bad_character[text[text_character_compare_index] -
                                          MIN_ALPHABET_VALUE]
                                         [pattern_character_compare_index];
      if (j != -1)
        shift_bad_character_rule = pattern_character_compare_index - j;
      else
        shift_bad_character_rule =
            pattern_length -
            (pattern_length - 1 - pattern_character_compare_index);
    } else {
      return text_character_compare_index + 1;
    }

    int32_t shift_good_suffix_rule = 0;
    if (pattern_length - 1 - pattern_character_compare_index > 1) {
      int32_t t_prime = good_suffix_preprocess[pattern_length - 1];
      if (t_prime != 0) {
        shift_good_suffix_rule = pattern_length - 1 - t_prime;
      }
    }
    // shift is whichever is greatest between bad-character rule
    // and the good suffix rule
    i += max(shift_good_suffix_rule, shift_bad_character_rule);
  }
  free(good_suffix_preprocess);
  return -1;
}
int32_t levenshtein_dist(char *one, int32_t one_length, char *two,
                         int32_t two_length) {
  int32_t table_row_length = one_length + 1;
  int32_t table_col_length = two_length + 1;
  int32_t **table = malloc(table_row_length * sizeof(int32_t *));
  for (int32_t i = 0; i < table_row_length; ++i) {
    table[i] = malloc(table_col_length * sizeof(int32_t));
    for (int32_t j = 0; j < table_col_length; ++j) {
      table[i][j] = 0;
    }
  }
  table[0][0] = 0;
  for (int32_t i = 1; i < table_row_length; ++i) {
    table[i][0] = table[i - 1][0] + 1;
  }
  for (int32_t i = 1; i < table_col_length; ++i) {
    table[0][i] = table[0][i - 1] + 1;
  }
  for (int32_t i = 1; i < table_row_length; ++i) {
    for (int32_t j = 1; j < table_col_length; ++j) {
      if (one[i - 1] == two[i - 1]) {
        table[i][j] = table[i - 1][j - 1];
      } else {
        table[i][j] =
            min(table[i][j - 1], min(table[i - 1][j], table[i - 1][j - 1])) + 1;
      }
    }
  }
  int32_t answer = table[one_length][two_length];
  for (int32_t i = 0; i < table_row_length; ++i) {
    free(table[i]);
  }
  free(table);
  return answer;
}
void sort_on_edit_dist_output_to_indices(int32_t split_on_newlines_length,
                                         int32_t *indices, int32_t *distances) {
  // insertion sort
  for (int32_t i = 0; i < split_on_newlines_length - 1; ++i) {
    int32_t j = indices[i + 1];
    while (distances[indices[j - 1]] > distances[indices[j]] && j > 0) {
      int32_t temp = indices[j];
      indices[j] = indices[j - 1];
      indices[j - 1] = temp;
      --j;
    }
  }
}
void process(char *written, int32_t *written_length) {
  char *new_written = malloc(*written_length);
  int32_t new_written_length = 0;
  for (int32_t idx = 0; idx < *written_length; ++idx) {
    if (written[idx] >= 32) {
      new_written[new_written_length++] = written[idx];
    }
  }
  int32_t new_length = 0;
  for (int32_t idx = 0; idx < new_written_length; ++idx) {
    if (new_written[idx] != 127) {
      new_written[new_length] = new_written[idx];
      ++new_length;
    } else {
      new_length = max(0, new_length - 1);
    }
  }
  for (int32_t idx = 0; idx < new_length; ++idx) {
    written[idx] = new_written[idx];
  }
  free(new_written);
  *written_length = new_length;
}
char *big_right_arrow = "➜";
char *format_str =
    "\033[H"  // Setting to 0,0 position
    "\r"      // Clearing from cursor to start of line
    "\033[0J" // Clearing everything from cursing to the end of the screen
    "%s"      // search input
    "\0337"   // Saving cursor position
    "\n――――――――――――――――――――――――――――――――――――――――――――――\n" // Separator
    "%s"       // Entries that were piped
    "\033[%dA" // Moving cursor back to search input line
    "\0338"    // Moving cursor back to saved position
    ;
void update_match_print(char *results, char **split_on_newlines,
                        char *written,
                        int32_t written_length, int32_t *matches,
                        int32_t tab_index, int32_t lines_count,
                        int32_t found_amt, int32_t max_width_length_of_line) {
  int32_t currn_idx = 0;
  int32_t found_counter = 0;
  if (found_amt < lines_count)
    tab_index %= found_amt;
  for (int32_t idx = 0; idx < split_on_newlines_length; ++idx) {
    // if nothing is written, all entries should be printed
    // if something is written, some or all entries could be skipped
    if (matches[idx] == -1 && written_length > 0)
      continue;
    if (tab_index == found_counter) {
      strncpy(results + currn_idx, big_right_arrow, strlen(big_right_arrow));
      currn_idx += strlen(big_right_arrow);
    }
    // clamps entry to a value that doesn't go past the terminal screens width
    int32_t length_to_add =
        min(strlen(split_on_newlines[idx]),
            max_width_length_of_line -
                (tab_index == found_counter ? strlen(big_right_arrow) : 0));
    strncpy(results + currn_idx, split_on_newlines[idx], length_to_add);
    currn_idx += length_to_add;
    results[currn_idx] = '\n';
    currn_idx += 1;
    found_counter += 1;
    if (found_counter == lines_count) break;
  }
  results[currn_idx] = 0;
  dprintf(STDERR_FILENO, format_str, written, results, lines_count);
}
struct window_changes_info {
  struct winsize *winfo;
  int32_t *lines_count;
  int32_t split_on_newlines_length;
  int32_t *cancel_ptr;
};
void *poll_for_window_changes(void *arg) {
  struct window_changes_info wc = *(struct window_changes_info *)arg;
  int32_t prev_lines_count = wc.winfo->ws_row;
  while (!*wc.cancel_ptr) {
    if (ioctl(STDIN_FILENO, TIOCGWINSZ, wc.winfo) == -1) {
      dprintf(STDERR_FILENO, "\033[?1049lioctl has failed");
      exit(EXIT_FAILURE);
    }
    if (wc.winfo->ws_row != prev_lines_count) {
      prev_lines_count = wc.winfo->ws_row;
      *wc.lines_count = min(wc.winfo->ws_row - 2, wc.split_on_newlines_length);
      // read blocks so when the termina screen resizes, the only thing we
      // can do is to reset the screen output to empty in the window size
      // polling thread.
      dprintf(STDERR_FILENO,
              "\033[H\033[0JScreen resized. Type something to reload.");
    }
  }
  return NULL;
}
void shotgun(char **split_on_newlines, int32_t split_on_newlines_length) {
  struct termios term_settings_old = set_up_terminal();
  int32_t max_length_of_entries = 0;
  for (int32_t idx = 0; idx < split_on_newlines_length; ++idx) {
    max_length_of_entries =
        max(strlen(split_on_newlines[idx]), max_length_of_entries);
  }
  int32_t initial_results_length =
      split_on_newlines_length * (max_length_of_entries + 1) + 1;
  char *results = malloc(initial_results_length);

  struct winsize winfo;
  if (ioctl(STDIN_FILENO, TIOCGWINSZ, &winfo) == -1) {
    dprintf(STDERR_FILENO, "\033[?1049lioctl has failed");
    exit(EXIT_FAILURE);
  }
  int32_t lines_count = min(winfo.ws_row - 2, split_on_newlines_length);
  struct window_changes_info wc;
  int32_t cancel_thread = 0;
  wc.winfo = &winfo;
  wc.lines_count = &lines_count;
  wc.split_on_newlines_length = split_on_newlines_length;
  wc.cancel_ptr = &cancel_thread;
  pthread_t thread;
  if (pthread_create(&thread, NULL, &poll_for_window_changes, &wc) == -1) {
    dprintf(STDERR_FILENO, "pthread_create failed");
    exit(EXIT_FAILURE);
  }

  int32_t tab_index = 0;
  int32_t capacity = 65535;
  int32_t length = 0;
  char *written = malloc(capacity);
  written[0] = 0;
  int32_t **boyer_moore_bad_character_table = NULL;
  int32_t *matches = malloc(split_on_newlines_length * sizeof(int32_t));
  int32_t found_amt = lines_count;
  while (1) {
    update_match_print(results, split_on_newlines,
                       written, length, matches, tab_index, lines_count,
                       found_amt, winfo.ws_col);
    int32_t r = read(STDIN_FILENO, written + length, capacity - length);
    written[r + length] = 0;
    for (int32_t idx = length; idx < length + r;) {
      // 3 is the end of text character / control+c
      if (written[idx] == 3) {
        dprintf(STDERR_FILENO, "\033[?1049l");
        reset_terminal(term_settings_old);
        cancel_thread = 1;
        if (pthread_cancel(thread) != 0) {
          dprintf(STDERR_FILENO, "pthread_cancel failed");
          exit(EXIT_FAILURE);
        }
        return;
      }
      if (written[idx] == '\t' || written[idx] == '\n') {
        if (written[idx] == '\t')
          // min is used so that the correct wrapping occurs
          tab_index = (tab_index + 1) % min(found_amt, lines_count);
        if (written[idx] == '\n') {
          dprintf(STDERR_FILENO, "\033[?1049l");
          reset_terminal(term_settings_old);
          int32_t current_line_in_results = 0;
          int32_t prev_newline_index = 0;
          int32_t end_of_result_idx = 0;
          for (int32_t i = 0; i < (int32_t)strlen(results); ++i) {
            if (results[i] == '\n') {
              prev_newline_index = i;
              current_line_in_results++;
            }
            if (current_line_in_results == tab_index) {
              break;
            }
          }
          end_of_result_idx = prev_newline_index + 1;
          while (end_of_result_idx < (int32_t)strlen(results) &&
                 results[end_of_result_idx] != '\n') {
            end_of_result_idx++;
          }
          results[end_of_result_idx] = 0;
          dprintf(STDOUT_FILENO, "%s\n",
                  results + prev_newline_index + strlen(big_right_arrow) +
                      (tab_index > 0 ? 1 : 0));
          cancel_thread = 1;
          if (pthread_cancel(thread) != 0) {
            dprintf(STDERR_FILENO, "pthread_cancel failed");
            exit(EXIT_FAILURE);
          }
          return;
        }
        for (int32_t idx2 = idx; idx2 < length + r - 1; ++idx2) {
          written[idx2] = written[idx2 + 1];
        }
        --r;
        continue;
      }
      ++idx;
    }
    length += r;
    if (length == capacity) {
      char *temp = malloc(capacity *= 2);
      for (int32_t idx = 0; idx < length; ++idx)
        temp[idx] = written[idx];
      free(written);
      written = temp;
    }
    process(written, &length);
    written[length] = 0;
    // reconstructing the bad character table for every change. not the
    // most performant thing but eh
    if (boyer_moore_bad_character_table != NULL) {
      for (int32_t idx = 0; idx < ALPHABET_LEN; ++idx) {
        free(boyer_moore_bad_character_table[idx]);
      }
      free(boyer_moore_bad_character_table);
    }
    boyer_moore_bad_character_table =
        construct_boyer_moore_bad_character_table(written, length);
    found_amt = 0;
    // goes through each entry and tries to find a pattern match
    for (int32_t split_idx = 0; split_idx < split_on_newlines_length;
         ++split_idx) {
      int32_t found = boyer_moore_string_search(
          boyer_moore_bad_character_table, written,
          length, split_on_newlines[split_idx],
          strlen(split_on_newlines[split_idx]));
      if (found != -1) {
        found_amt += 1;
        matches[split_idx] = split_idx;
      } else {
        matches[split_idx] = -1;
      }
    }
    // ensures found_amt is never 0 and that tab_index is never divided by 0
    if (found_amt == 0) found_amt = lines_count;
  }
}
int32_t main() {
  int32_t input_buffer_capacity = 65535;
  int32_t input_buffer_length = 0;
  char *input_buffer = malloc(input_buffer_capacity);
  int32_t bytes_read;
  while ((bytes_read = read(STDIN_FILENO, input_buffer + input_buffer_length,
                            input_buffer_capacity - input_buffer_length)) > 0) {
    input_buffer_length += bytes_read;
    if (input_buffer_length == input_buffer_capacity) {
      char *temp = malloc(input_buffer_capacity *= 2);
      for (int32_t idx = 0; idx < input_buffer_length; ++idx) {
        temp[idx] = input_buffer[idx];
      }
      free(input_buffer);
      input_buffer = temp;
    }
  }
  input_buffer[input_buffer_length] = 0;
  int32_t newlines = 0;
  for (int32_t idx = 0; idx < input_buffer_length; ++idx) {
    if (input_buffer[idx] == '\n')
      newlines += 1;
  }
  char **split_on_newlines = malloc(newlines * sizeof(char *));
  int32_t last_newline_idx_plus_1 = 0;
  for (int32_t idx = 0; idx < newlines; ++idx) {
    int32_t next_newline_idx = last_newline_idx_plus_1 + 1;
    while (input_buffer[next_newline_idx] != '\n' &&
           next_newline_idx < input_buffer_length)
      ++next_newline_idx;
    int32_t size = next_newline_idx - last_newline_idx_plus_1;
    split_on_newlines[idx] = malloc(size + 1);
    strncpy(split_on_newlines[idx], input_buffer + last_newline_idx_plus_1,
            size);
    split_on_newlines[idx][size] = 0;
    last_newline_idx_plus_1 = next_newline_idx + 1;
  }
  if (input_buffer_length > 0) {
    free(input_buffer);
    shotgun(split_on_newlines, newlines);
  }
}
