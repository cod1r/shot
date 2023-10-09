#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
void process(char *written, int *written_length)
{
  int new_length = 0;
  for(int idx=0;idx<*written_length;++idx){
    if(written[idx]!=127){
      ++new_length;
    }else{
      new_length = new_length - 1 >= 0 ? new_length - 1:0;
    }
  }
  *written_length=new_length;
}
int min(int a, int b)
{
  return a < b ? a:b;
}
int levenshtein_dist(char *one, int one_length, char *two, int two_length)
{
  int table_row_length = one_length+1;
  int table_col_length = two_length+1;
  int **table=malloc(table_row_length * sizeof(int*));
  for(int i=0;i<table_row_length;++i){
    table[i]=malloc(table_col_length * sizeof(int));
    for(int j=0;j<table_col_length;++j){
      table[i][j]=0;
    }
  }
  table[0][0]=0;
  for(int i=1;i<table_row_length;++i){
    table[i][0]=table[i-1][0] + 1;
  }
  for(int i=1;i<table_col_length;++i){
    table[0][i]=table[0][i-1] + 1;
  }
  for(int i=1;i<table_row_length;++i){
    for(int j=1;j<table_col_length;++j){
      if(one[i-1]==two[i-1]){
        table[i][j]=table[i-1][j-1];
      }else{
        table[i][j]=min(table[i][j-1], min(table[i-1][j], table[i-1][j-1])) + 1;
      }
    }
  }
  int answer = table[one_length][two_length];
  for(int i=0;i<table_row_length;++i){
    free(table[i]);
  }
  free(table);
  return answer;
}
void sort_on_edit_dist(char **sorted_p, char *written, int written_length, char **split_on_newlines, int split_on_newlines_length)
{
  int length_of_sorted = split_on_newlines_length;
  for(int idx=0;idx<split_on_newlines_length;++idx){
    length_of_sorted+=strlen(split_on_newlines[idx]);
  }
  *sorted_p=malloc(length_of_sorted);
  char *sorted = *sorted_p;
  sorted[length_of_sorted]=0;
  int *indices=malloc(split_on_newlines_length * sizeof(int));
  for(int i=0;i<split_on_newlines_length;++i){
    indices[i]=i;
  }
  //insertion sort
  for(int i=0;i<split_on_newlines_length-1;++i){
    int j = indices[i + 1];
    while(levenshtein_dist(written, written_length,
      split_on_newlines[indices[j-1]],
      strlen(split_on_newlines[indices[j-1]])) > levenshtein_dist(written, written_length,
      split_on_newlines[indices[j]],
      strlen(split_on_newlines[indices[j]])) && j > 0){
      int temp = indices[j];
      indices[j] = indices[j-1];
      indices[j-1]=temp;
      --j;
    }
  }
  int currn_idx=0;
  for(int idx=0;idx<split_on_newlines_length;++idx){
    strncpy(sorted+currn_idx, split_on_newlines[indices[idx]], strlen(split_on_newlines[indices[idx]]));
    currn_idx+=strlen(split_on_newlines[indices[idx]]);
    sorted[currn_idx]='\n';
    currn_idx++;
  }
  sorted[currn_idx]=0;
}
void sex(char **split_on_newlines, int split_on_newlines_length)
{
  struct termios term_settings;
  if (tcgetattr(STDOUT_FILENO, &term_settings) == -1){
    fprintf(stderr, "tcgetattr failed on stdout\n");
    exit(EXIT_FAILURE);
  }
  term_settings.c_lflag &= ~ICANON;
  term_settings.c_lflag &= ~ECHO;
  term_settings.c_cc[VMIN] = 1;
  term_settings.c_cc[VTIME] = 0;
  if (tcsetattr(STDOUT_FILENO, TCSANOW, &term_settings) == -1){
    fprintf(stderr, "tcsetattr_new failed on stdout\n");
    exit(EXIT_FAILURE);
  }
  int capacity=65535;
  int length=0;
  char *written=malloc(capacity);
  char *sorted = malloc(1);
  sorted[0]=0;
  fprintf(stderr, "\033[0J\033[1K\r%s\0337\n-----------------------------\n%s\033[%dA\0338", written, sorted, split_on_newlines_length + 2);
  while (1){
    int r=read(STDOUT_FILENO, written+length, capacity-length);
    written[r+length]=0;
    for(int idx=length;idx<length+r;){
      if(written[idx]=='\n'||written[idx]=='\t'||written[idx]=='\v'||written[idx]=='\f'){
        for(int idx2=idx;idx2<length+r-1;++idx2){
          written[idx2]=written[idx2+1];
        }
        --r;
        continue;
      }
      ++idx;
    }
    length+=r;
    if (length==capacity){
      char *temp=malloc(capacity*=2);
      for(int idx=0;idx<length;++idx)
        temp[idx]=written[idx];
      free(written);
      written=temp;
    }
    process(written, &length);
    written[length]=0;
    free(sorted);
    sort_on_edit_dist(&sorted, written, length, split_on_newlines, split_on_newlines_length);
    fprintf(stderr, "\033[0J\033[1K\r%s\0337\n-----------------------------\n%s\033[%dA\0338", written, sorted, split_on_newlines_length + 2);
  }
}
int main()
{
  int input_buffer_capacity = 65535;
  int input_buffer_length = 0;
  char *input_buffer = malloc(input_buffer_capacity);
  int bytes_read;
  while((bytes_read=read(STDIN_FILENO,
    input_buffer + input_buffer_length,
    input_buffer_capacity-input_buffer_length))){
    input_buffer_length += bytes_read;
    if(input_buffer_length==input_buffer_capacity){
      char *temp = malloc(input_buffer_capacity*=2);
      for(int idx=0;idx<input_buffer_length;++idx){
        temp[idx]=input_buffer[idx];
      }
      free(input_buffer);
      input_buffer=temp;
    }
  }
  input_buffer[input_buffer_length]=0;
  int newlines=0;
  for(int idx=0;idx<input_buffer_length;++idx){
    if(input_buffer[idx]=='\n')
      newlines+=1;
  }
  if(input_buffer[input_buffer_length]=='\n')
    newlines--;
  char **split_on_newlines=malloc(newlines * sizeof(char*));
  int last_newline_idx_plus_1=0;
  for(int idx=0;idx<newlines;++idx){
    int next_newline_idx=last_newline_idx_plus_1+1;
    while(input_buffer[next_newline_idx]!='\n' &&
      next_newline_idx<input_buffer_length)
      ++next_newline_idx;
    int size=next_newline_idx-last_newline_idx_plus_1;
    split_on_newlines[idx]=malloc(size+1);
    strncpy(
      split_on_newlines[idx],
      input_buffer+last_newline_idx_plus_1,
      size
    );
    split_on_newlines[idx][size]=0;
    last_newline_idx_plus_1=next_newline_idx+1;
  }
  sex(split_on_newlines, newlines);
}
