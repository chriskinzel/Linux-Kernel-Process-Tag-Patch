# Linux-Kernel-Process-Tag-Patch
Linux kernel patch that adds the ability to give processes an arbitrary list of string tags. Child processes inherit parent tags. Processes are tagged using the ptag command line tool. Once tagged processes with a specific tag can be killed or inspected by referencing their tags with the tagkill and tagstat command line tools respectively.

# Compilation & Running
Each of the command line tools ptag, tagkill, and tagstat can be compiled by using the respective makefile and a make all command in the associated directory. The Linux kernel code is given as a patch file that can be applied to a linux kernel source tree after which compilation and running of the compiled kernel allows the command line tools to be used.

# ptag usage
User level program to add and remove tags to any given process owned by the calling user. Interacts with PTAG system call.

ptag <pid> -a <tag> [tag2 ...]  
            OR  
            ptag <pid> -r [tag1 ...]  

            Using -r with no tags removes all  
            tags from the specified process.  

# tagkill usage
Utillity that kills each process (kill -9) who's ptags match a given boolean expression  

tagkill <tag> OR tagkill '<expr>'  

   Where <expr> is a boolean expression of the form:  
       [operator2] <expr> <operator1> [operator2] <expr>  
   OR  
       <tag>  

   <operator1> can be any of the following boolean operators:  
       '&&', 'and', '||', 'or', '^^', 'xor'  

   [operator2] is an optional NOT operator:  
       '!', 'not'  

   Operators must be lowercase, <operator1> must contain
   a space on either side or the expression will be treated
   as a tag. In the case of 'not' for [operator2] the 'not'
   must be suffixed with a space otherwise the expression
   will be treated as a tag. No space is needed for '!'.

   The NOT operator has highest preecendence, all other
   operators have equal preecendecnce. Thus expressions
   containing more than one type of operator (excluding NOT)
   should be contained in parenthesis so that the order of
   operations is clear. Without parenthesis, leftmost
   expressions will be evaluated first.

   Any parenthesized expression prefixed with a '%' will be
   treated as a tag literal. This is so tags containing
   operators and whitespace can be specified.

   e.g. %(tagwith || and !!)

   Tags cannot contain percent signs or parenthesis unless
   escaped.


# tagstat usage
Utillity that prints a table to stdout listing all processID-tag mappings for processes that the user currently owns. Information is scraped from /proc/ptags, formatting of /proc/ptags is preserved i.e. lines of the form

 <pid> : <tag> : <process_state>  

that is a process ID number followed by a space followed by a colon followed by another space followed by the tag string followed by a space another colon another space followed by the process state and finally ending with a newline character and a null terminator. Only processes that are associated with at least one tag have entries in the proc file. A process ID may show up in more than one line if a process is associated with multiple tags. Lines are ordered by ascending process ID.

tagstat <tag> OR tagstat '<expr>'  

   passing --help will print this usage information, thus if  
   you wish to use --help as tag it must be encased in either  
   parenthesis or escaped, see below. 

   Where <expr> is a boolean expression of the form:  
       [operator2] <expr> <operator1> [operator2] <expr>  
   OR  
       <tag>  

   <operator1> can be any of the following boolean operators:  
       '&&', 'and', '||', 'or', '^^', 'xor'  

   [operator2] is an optional NOT operator:  
       '!', 'not'  

   Operators must be lowercase, <operator1> must contain  
   a space on either side or the expression will be treated  
   as a tag. In the case of 'not' for [operator2] the 'not'  
   must be suffixed with a space otherwise the expression   
   will be treated as a tag. No space is needed for '!'.   

   The NOT operator has highest preecendence, all other  
   operators have equal preecendecnce. Thus expressions  
   containing more than one type of operator (excluding NOT)  
   should be contained in parenthesis so that the order of  
   operations is clear. Without parenthesis, leftmost  
   expressions will be evaluated first.  

   Any parenthesized expression prefixed with a '%' will be  
   treated as a tag literal. This is so tags containing  
   operators and whitespace can be specified.  

   e.g. %(tagwith || and !!)  

   Tags cannot contain percent signs or parenthesis unless  
   escaped.  