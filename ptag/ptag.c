//
// Assignment 2 - Part A - ptag user-level tagging program
// ---------------------------------------------------------------------------------------------------
//
// Name:            Chris Kinzel
// Tutorial:                 T03
// ID:                  10160447
//
// ptag.c
//
// Description:
// ---------------------------------------------------------------------------------------------------
//
// User level program to add and remove tags to any given process owned by the calling user. Interacts
// with PTAG system call.
//
// Usage:   ptag <pid> -a <tag> [tag2 ...]
//          OR
//          ptag <pid> -r [tag1 ...]
//
//          Using -r with no tags removes all
//          tags from the specified process.
//
// COMPILATION
//  gcc -Wall ptag.c -o ptag
//
// NOTE
//  The empty string is considered a valid tag, i.e. a string consisting of a single '\0' character.
//
// EXIT CODES
//  same as PTAG system call
//
// Citations:
// ---------------------------------------------------------------------------------------------------
//   -  Linux man pages for info on how to use strtoul()
//
//      http://linux.die.net/man/3/strtoul
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

static const char* const usage_str = "Usage:\tptag <pid> -a <tag> [tag2 ...]\n"
                                     "\tOR\n"
                                     "\tptag <pid> -r [tag1 ...]\n\n"

                                     "\tUsing -r with no tags removes all\n"
                                     "\ttags from the specified process.\n";

int main(int argc, const char* argv[]) {
    if(argc < 4) {
        if(argc == 1) {     // No arguments will be interpreted as the user asking for usage
            printf(usage_str);
            
            return 0;
        } else if(argc == 3 && strncmp(argv[2], "-r", sizeof("-r")) == 0) {
            // ptag <pid> -r    * Remove all tags associated with process *
        } else {            // Anything else is incorrect usage
            fprintf(stderr, "ptag: Incorrect usage.\n");
            fprintf(stderr, usage_str);
            
            return 1;
        }
    }
    
    
    errno = 0;
    const char* pid_str = argv[1];
    char* tmp;
    unsigned long int pid_tmp = strtoul(pid_str, &tmp, 10);
    if(tmp == pid_str || *tmp != '\0' || (pid_tmp == LONG_MAX && errno == ERANGE)) {
        fprintf(stderr, "ptag: Invalid PID argument '%s'.\n", pid_str);
        fprintf(stderr, usage_str);
        
        return 2;
    }
    pid_t pid = (pid_t)pid_tmp;
    
    
    
    char mode = argv[2][1];
    if(strncmp(argv[2], "-a", sizeof("-a")) != 0 && strncmp(argv[2], "-r", sizeof("-r"))) {
        if(argv[2][0] == '-') {
            // Argument was an option but not -a or -r
            fprintf(stderr, "ptag: Unrecognized option '%s'. Use either -a or -r.\n", argv[2]);
            fprintf(stderr, usage_str);
        } else {
            // Argument was not an option
            fprintf(stderr, "ptag: Unexpected argument '%s'. Expecting either -a or -r.\n", argv[2]);
            fprintf(stderr, usage_str);
        }
        
        
        return 1;
    }
    
    /*
     * Loop through all remaining arguments treating them as tags
     * and add or remove them to the specified process
     */
    int i;
    for(i=3; i < argc; i++) {
        const char* tag = argv[i];
        
        // Trap to kernel and execute PTAG system call
        long retval = syscall(337, pid, tag, mode);
        
        switch (retval) {
            case 0:         // Process was tagged sucessfully
                break;
                
            case 1:         // Invalid mode argument but this should be handled above
                fprintf(stderr, "ptag: An unexpected error occured\n");
                return 5;
                
            case 2:         // Tag was NULL, someone was being crafty with argv
                fprintf(stderr, "ptag: Tag name was invalid\n");
                return 1;
                
            case 3:         // PID did not match any running processes
                fprintf(stderr, "ptag: No process existed with matching PID %d\n", pid);
                return 2;
                
            case 4:         // Calling user is not the owner of the specified process
                fprintf(stderr, "ptag: You do not own process %d\n", pid);
                return 3;
                
            case 5:         // Kernel memory allocation failed
                fprintf(stderr, "ptag: Memory allocation error\n");
                return 4;
                
            default:        // Unknown error
                fprintf(stderr, "ptag: Unknown error occured\n");
                return 5;
        }
    }
    
    // ptag <pid> -r  * Remove all tags associated with process *
    if(mode == 'r' && i == 3) {
        // Trap to kernel and execute PTAG system call
        long retval = syscall(337, pid, NULL, 'c');
        
        switch (retval) {
            case 0:         // Process was tagged sucessfully
                break;
                
            case 1:         // Invalid mode argument but this should be handled above
                fprintf(stderr, "ptag: An unexpected error occured\n");
                return 5;
                
            case 3:         // PID did not match any running processes
                fprintf(stderr, "ptag: No process existed with matching PID %d\n", pid);
                return 2;
                
            case 4:         // Calling user is not the owner of the specified process
                fprintf(stderr, "ptag: You do not own process %d\n", pid);
                return 3;
                
            case 5:         // Kernel memory allocation failed
                fprintf(stderr, "ptag: Memory allocation error\n");
                return 4;
                
            default:        // Unknown error
                fprintf(stderr, "ptag: Unknown error occured\n");
                return 5;
        }
    }
    
    return 0;
}
