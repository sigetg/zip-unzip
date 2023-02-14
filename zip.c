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

/* allocate space for and return a new string s+t */
char *strappend_str(char *s, char *t);

/* allocate space for and return a new string s+c */
char *strappend_char(char *s, char c);

/* look for string s in the dictionary
 * return the code if found
 * return NUM_CODES if not found 
 */
unsigned int find_encoding(char *dictionary[], char *s);

/* write the code for string s to file */
void write_code(int fd, char *dictionary[], char *s);

/* compress in_file_name to out_file_name */
void compress(/* char *in_file_name, char *out_file_name */);

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Usage: zip file\n");
        exit(1);
    }

    char *in_file_name = argv[1];
    char *out_file_name = strappend_str(in_file_name, ".zip");

    compress(in_file_name, out_file_name);

    /* have to free the memory for out_file_name since strappend_str malloc()'ed it */
    free(out_file_name);
    return 0;
}

/* allocate space for and return a new string s+t */
char *strappend_str(char *s, char *t)
{
    if (s == NULL || t == NULL)
    {
        return NULL;
    }

    // reminder: strlen() doesn't include the \0 in the length
    int new_size = strlen(s) + strlen(t) + 1;
    char *result = (char *)malloc(new_size*sizeof(char));
    strcpy(result, s);
    strcat(result, t);

    return result;
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

/* look for string s in the dictionary
 * return the code if found
 * return NUM_CODES if not found 
 */
unsigned int find_encoding(char *dictionary[], char *s)
{
    if (dictionary == NULL || s == NULL)
    {
        return NUM_CODES;
    }

    for (unsigned int i=0; i<NUM_CODES; ++i)
    {
        /* code words are added in order, so if we get to a NULL value 
         * we can stop searching */
        if (dictionary[i] == NULL)
        {
            break;
        }

        if (strcmp(dictionary[i], s) == 0)
        {
            return i;
        }
    }
    return NUM_CODES;
}

/* write the code for string s to file */
void write_code(int fd, char *dictionary[], char *s)
{
    if (dictionary == NULL || s == NULL)
    {
        return;
    }

    unsigned int code = find_encoding(dictionary, s);
    // should never call write_code() unless s is in the dictionary 
    if (code == NUM_CODES)
    {
        printf("Algorithm error!");
        exit(1);
    }

    // cast the code to an unsigned short to only use 16 bits per code word in the output file
    unsigned short actual_code = (unsigned short)code;
    if (write(fd, &actual_code, sizeof(unsigned short)) != sizeof(unsigned short))
    {
        perror("write");
        exit(1);
    }
}

/* compress in_file_name to out_file_name */
void compress(char *in_file_name, char *out_file_name)
{
    char **dictionary = (char **)malloc(NUM_CODES * sizeof(char *));
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

    char current_char;
    int size = read(fd_read, &current_char, sizeof(char));
    if (size < 0)
    {
	perror("read");
        exit(1);
    }
    char *current_string = strappend_char("", current_char);
    char *string_char;
    char *string_char_copy;
    int current_index = 256;
    while (size != 0)
    { 
        size = read(fd_read, &current_char, sizeof(char));
        if (size == 0)
	{
	    break;
	}
	if (size < 0)
        {
            perror("read");
            exit(1);
        }		
        string_char = strappend_char(current_string, current_char);	
        if (find_encoding(dictionary, string_char) != NUM_CODES)
        {
	    current_string = string_char;
        }
	else
        {
	    write_code(fd_write, dictionary, current_string);
	    string_char_copy = strdup(string_char);
	    dictionary[current_index] = string_char_copy;    
	    current_string[0] = current_char;
	    current_string[1] = '\0';
	    current_index++;
        
	}
    }
    for (int i = 0; i < NUM_CODES; ++i)
    {
	if (dictionary[i] == NULL)
	{
	    break;
	}
    }
    if (close(fd_read) < 0)
    {
        perror("close");
	exit(1);
    }
    write_code(fd_write, dictionary, current_string);
    free(dictionary);
    free(current_string);
    if (close(fd_write) < 0)
    {
        perror("close");
	exit(1);
    }
		 
    return;
}
