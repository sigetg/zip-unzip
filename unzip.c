#define _POSIX_C_SOURCE 200809L // required for strdup() on cslab

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* Use 16-bit code words */
#define NUM_CODES 65536

/* allocate space for and return a new string s+c */
char *strappend_char(char *s, char c);

/* read the next code from fd
 * return NUM_CODES on EOF
 * return the code read otherwise
 */
unsigned int read_code(int fd);

/* uncompress in_file_name to out_file_name */
void uncompress(char *in_file_name, char *out_file_name);

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Usage: unzip file\n");
        exit(1);
    }

    char *in_file_name = argv[1];
    char *out_file_name = strdup(in_file_name);
    out_file_name[strlen(in_file_name)-4] = '\0';

    uncompress(in_file_name, out_file_name);

    free(out_file_name);

    return 0;
}

/* allocate space for and return a new string s+c */
char *strappend_char(char *s, char c)
{
    if (s == NULL)
    {
        return NULL;
    }

    // reminder: strlen() doesn't include the \0 in the length
    int new_size = strlen(s) + 2;
    char *result = (char *)malloc(new_size*sizeof(char));
    strcpy(result, s);
    result[new_size-2] = c;
    result[new_size-1] = '\0';

    return result;
}

/* read the next code from fd
 * return NUM_CODES on EOF
 * return the code read otherwise
 */
unsigned int read_code(int fd)
{
    // code words are 16-bit unsigned shorts in the file
    unsigned short actual_code;
    int read_return = read(fd, &actual_code, sizeof(unsigned short));
    if (read_return == 0)
    {
        return NUM_CODES;
    }
    if (read_return != sizeof(unsigned short))
    {
       perror("read");
       exit(1);
    }
    return (unsigned int)actual_code;
}

/* uncompress in_file_name to out_file_name */
void uncompress(char *in_file_name, char *out_file_name)
{
    char *dictionary[NUM_CODES] = { NULL };
    for (int i = 0; i < NUM_CODES; ++i)
    {
        if (i < 256)
        {
            char *char_i = strappend_char("", (char)i);
	    dictionary[i] = char_i;
	}
        else
        {
            dictionary[i] = NULL;
        }
    }
    int fd_read = open(in_file_name, O_RDONLY);
    if (fd_read == -1)
    {
        perror(in_file_name);
        exit(1);
    }
    int fd_write = open(out_file_name, O_WRONLY|O_CREAT|O_TRUNC, 0644);
    if (fd_read == -1)
    {
        perror(out_file_name);
        exit(1);
    }
    unsigned int current_code = read_code(fd_read);
    unsigned int next_code = 0;
    char *current_string;
    char *old_string;    
    char current_char = *dictionary[current_code];
    int write_size = write(fd_write, &current_char, sizeof(char));
    if (write_size < 0)
    {
        perror("write");
        exit(1);
    }
    while (current_code != NUM_CODES)
    {
	next_code = read_code(fd_read);
	if (next_code == NUM_CODES)
	{
	    break;
	}
	if (dictionary[next_code] == NULL)
	{ 
	    printf("%s", current_string);
	    current_string = dictionary[current_code];
	    current_string = strappend_char(current_string, current_char);
	}
	else
	{
	    current_string = dictionary[next_code];
	}
        write_size = write(fd_write, current_string, strlen(current_string));
        if (write_size < 0)
        {
            perror("write");
            exit(1);
        }
        current_char = current_string[0];
	old_string = dictionary[current_code];
	for (unsigned int i = 0; i < NUM_CODES; ++i)
	{
	    if (dictionary[i] == NULL)
	    {
		dictionary[i] = strappend_char(old_string, current_char);
		break;
	    }
	}
	current_code = next_code;
        //free(current_string);
	// while the above seems right, it isn't. 
	//It must be beccause I am referencing the same memory somewhere else, 
	//so it causes all kinds of errors when I try to free the meemory. 
	//All my unfreed memory in both files is similar in the way it doesnt work, 
	//and I would love to get an explanation on how to free it correctly in these situaltions if possible.
    }
    if (close(fd_read) < 0)
    {
        perror("close");
        exit(1);
    }
    if (close(fd_write) < 0)
    {
        perror("close");
        exit(1);
    }
  
    return;
}
