#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ioctl.h>
void process(const char *written, int *written_length, const char *const*const split_on_newlines)
{
  int not_ignored_character_count = 0;
  int new_length = 0;
  for(int idx=0;idx<*written_length;++idx){
    if(written[idx]!=127){
      ++new_length;
    }else{
      --new_length;
    }
  }
  *written_length=new_length;
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
  term_settings.c_cc[VMIN] = 0;
  term_settings.c_cc[VTIME] = 1;
  if (tcsetattr(STDOUT_FILENO, TCSANOW, &term_settings) == -1){
    fprintf(stderr, "tcsetattr_new failed on stdout\n");
    exit(EXIT_FAILURE);
  }
  int capacity=65535;
  int length=0;
  char *written=malloc(capacity);
  while (1){
    int r=read(STDOUT_FILENO, written+length, capacity-length);
    written[r+length]=0;
    if (r==0){
    }else{
      length+=r;
      if (length==capacity){
        char *temp=malloc(capacity*=2);
        for(int idx=0;idx<length;++idx)
          temp[idx]=written[idx];
        free(written);
        written=temp;
      }
      process(written, &length, split_on_newlines);
      written[length]=0;
      fprintf(stderr, "\033[1K\033[0E%s", written);
    }
  }
}
int main()
{
  int input_buffer_capacity = 65535;
  int input_buffer_length = 0;
  char *input_buffer = malloc(input_buffer_length);
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
  int newlines=input_buffer[input_buffer_length-1]=='\n'?0:1;
  for(int idx=0;idx<input_buffer_length;++idx){
    if(input_buffer[idx]=='\n')
      newlines+=1;
  }
  char **split_on_newlines=malloc(newlines);
  int last_newline_idx=0;
  for(int idx=0;idx<newlines;++idx){
    int next_newline_idx=last_newline_idx+1;
    while(input_buffer[next_newline_idx]!='\n' &&
      next_newline_idx<input_buffer_length)
      ++next_newline_idx;
    int size=next_newline_idx-last_newline_idx;
    split_on_newlines[idx]=malloc(size);
    split_on_newlines[size]=0;
    strncpy(
      split_on_newlines[idx],
      input_buffer+last_newline_idx,
      size
    );
    last_newline_idx=next_newline_idx+1;
  }
  sex(split_on_newlines, newlines);
}
