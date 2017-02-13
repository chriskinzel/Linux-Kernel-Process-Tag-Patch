//
// Assignment 2 - Part A - tagstat
// ---------------------------------------------------------------------------------------------------
//
// Name:            Chris Kinzel
// Tutorial:                 T03
// ID:                  10160447
//
// tagstat.c
//
// Description:
// ---------------------------------------------------------------------------------------------------
//
// Utillity that prints a table to stdout listing all processID-tag mappings for processes that the
// user currently owns. Information is scraped from /proc/ptags, formatting of /proc/ptags is
// preserved i.e. lines of the form
//
// <pid> : <tag> : <process_state>
//
// that is a process ID number followed by a space followed by a colon followed by another space
// followed by the tag string followed by a space another colon another space followed by the process
// state and finally ending with a newline character and a null terminator. Only processes that are
// associated with at least one tag have entries in the proc file. A process ID may show up in more
// than one line if a process is associated with multiple tags. Lines are ordered by ascending process
// ID.
//
// BONUS IMPLEMENTED
//   tagstat can handle arbitrary boolean expressions using the logical operators and,or,xor and not as
//   a command line argument and will only print tag mappings for processes that match the given
//   expression. Usage information is given below and can be requested anytime be running tagstat with
//   the --help argument.
//
// USAGE
//   tagstat <tag> OR tagstat '<expr>'
//
//   passing --help will print this usage information, thus if
//   you wish to use --help as tag it must be encased in either
//   parenthesis or escaped, see below.
//
//   Where <expr> is a boolean expression of the form:
//       [operator2] <expr> <operator1> [operator2] <expr>
//   OR
//       <tag>
//
//   <operator1> can be any of the following boolean operators:
//       '&&', 'and', '||', 'or', '^^', 'xor'
//
//   [operator2] is an optional NOT operator:
//       '!', 'not'
//
//   Operators must be lowercase, <operator1> must contain
//   a space on either side or the expression will be treated
//   as a tag. In the case of 'not' for [operator2] the 'not'
//   must be suffixed with a space otherwise the expression
//   will be treated as a tag. No space is needed for '!'.
//
//   The NOT operator has highest preecendence, all other
//   operators have equal preecendecnce. Thus expressions
//   containing more than one type of operator (excluding NOT)
//   should be contained in parenthesis so that the order of
//   operations is clear. Without parenthesis, leftmost
//   expressions will be evaluated first.
//
//   Any parenthesized expression prefixed with a '%' will be
//   treated as a tag literal. This is so tags containing
//   operators and whitespace can be specified.
//
//   e.g. %(tagwith || and !!)
//
//   Tags cannot contain percent signs or parenthesis unless
//   escaped.
//
// COMPILE WITH
//   gcc -Wall -O2 tagstat.c -o tagstat
//
//  The -O2 is for tail call optimization
//
// EXIT CODES
//   0 - Exit success
//
//   1 - Incorrect usage:           incorrect number of arguments
//
//   2 - Invalid expression:        syntax error, see usage
//
//   3 - Out of memory:             malloc failed
//
//   4 - Assertion error:           grammar parsing did something unexpected
//
//   5 - IO error:                  IO operations with /proc/ptags failed
//
// Citations:
// ---------------------------------------------------------------------------------------------------
//   -  The following source was used to aid in the implementation of a CYK parser
//
//      https://en.wikipedia.org/wiki/CYK_algorithm
//
//   -  Linux man pages
//
//      http://man7.org/linux/man-pages/
//
//

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define GRAMMAR_NUM_NT 31                               // Number of nonterminals in the grammar

#define index_of(i, j, k, n) ((k)*n*n + (j)*n + i)      // Macro to get element from parse table given
                                                        // sequence length, starting index, nonterminal #
                                                        // and number of characters in the expression


// This macro generates all ASCII characters that are not control characters/whitespace
#define ASCII_NO_WHITESPACE 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, \
47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, \
61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, \
75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, \
89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, \
103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, \
115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, \
127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, \
139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, \
151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, \
163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, \
175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, \
187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, \
199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, \
211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, \
223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, \
235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, \
247, 248, 249, 250, 251, 252, 253, 254, 255

// This macro is for all ASCII characters excluding '\0'
#define ASCII_ALL 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,ASCII_NO_WHITESPACE


/*
 * Given below is a chomsky normal form context-free grammar for the expression parsing.
 * The spaces in the rules are not actually part of the grammar they have been added
 * for clarity, any spaces that are meant to be in the grammar are represented with
 * the nonterminal V_sp.
 *
 *  E0   --> EL | V_( E1 | V_! E | V_n E2 | V_% E3 | T1 T1 | all ascii > 33         // Start rule
 *  E    --> EL | V_( E1 | V_! E | V_n E2 | V_% E3 | T1 T1 | all ascii > 33         // Expression
 *  L    --> V_sp L1                                                                // The L variables make sure operators are surronded by spaces
 *  L1   --> O L2
 *  L2   --> V_sp E
 *  E1   --> E V_)                                                                  // Expression in brackets
 *  E2   --> V_o N                                                                  // Expression with 'not' operator
 *  E3   --> V_( X                                                                  // Escaped expression
 *  N    --> V_t N1                                                                 // Part of the 'not' operator expression
 *  N1   --> V_sp E                                                                 // Makes sure 'not' is suffixed by space
 *  X    --> T2 V_)                                                                 // Part of the escaped expression
 *  O    --> V_x O1 | V_^ V_^ | V_o V_r | V_| V_| | V_a O2 | V_& V_&                // Operator
 *  O1   --> V_o V_r                                                                // For 'xor' operator
 *  O2   --> V_n V_d                                                                // For 'and' operator
 *  T1   --> T1 T1 | all ascii > 33                                                 // Unescaped tag
 *  T2   --> T2 T2 | any ascii (except '\0')                                        // Escaped tag
 *  V_(  --> (
 *  V_)  --> )
 *  V_!  --> !
 *  V_%  --> %
 *  V_|  --> |
 *  V_&  --> &
 *  V_^  --> ^
 *  V_x  --> x
 *  V_o  --> o
 *  V_r  --> r
 *  V_a  --> a
 *  V_n  --> n
 *  V_d  --> d
 *  V_t  --> t
 *  V_sp --> space
 *
 * Note that in the encoded form of the grammar given below, nonterminals that
 * have single unit productions of the form V_* --> * where * is not an
 * alphanumeric character have their representing symbol name subsititued for
 * the first two letters of that symbols name as naming instead. So for example
 * V_! gets renamed to V_em and V_( to V_lb etc.
 *
 * The grammar is encoded as follows, each nonterminal is encoded as an array of
 * 2-byte unsigned integers. Since rules are either of the form P -> QR or P -> a
 * each byte in the 2-byte entry can be used to store an index into an array holding
 * all the nonterminal symbols. If the most significant byte (MSB) is zero then
 * the rule represents a unit production rule of the form P -> a (since in chomsky
 * the start symbol can never be producded by any rule) where the value of the
 * least significant byte (LSB) represents a char value of the terminal production.
 * If the MSB is non-zero then the rule represents a production of the form P -> QR,
 * and by extracting the MSB and LSB as seperate bytes the indicies of the nonterminals
 * P and Q can be computed. The 'rules' array further down stores the address for each
 * nonterminal and can be used to get the production rules of any nonterminal given its
 * index.
 */
static const uint16_t E0[]    = {(1 << 8) | 2, (16 << 8) | 5, (18 << 8) | 1, (27 << 8) | 6, (19 << 8) | 7, (14 << 8) | 14, ASCII_NO_WHITESPACE};
static const uint16_t E[]     = {(1 << 8) | 2, (16 << 8) | 5, (18 << 8) | 1, (27 << 8) | 6, (19 << 8) | 7, (14 << 8) | 14, ASCII_NO_WHITESPACE};
static const uint16_t L[]     = {(30 << 8) | 3};
static const uint16_t L1[]    = {(11 << 8) | 4};
static const uint16_t L2[]    = {(30 << 8) | 1};
static const uint16_t E1[]    = {(1 << 8) | 17};
static const uint16_t E2[]    = {(24 << 8) | 8};
static const uint16_t E3[]    = {(16 << 8) | 10};
static const uint16_t N[]     = {(29 << 8) | 9};
static const uint16_t N1[]    = {(30 << 8) | 1};
static const uint16_t X[]     = {(15 << 8) | 17};
static const uint16_t O[]     = {(23 << 8) | 12, (22 << 8) | 22, (24 << 8) | 25, (20 << 8) | 20, (26 << 8) | 13, (21 << 8) | 21};
static const uint16_t O1[]    = {(24 << 8) | 25};
static const uint16_t O2[]    = {(27 << 8) | 28};
static const uint16_t T1[]    = {(14 << 8) | 14, ASCII_NO_WHITESPACE};
static const uint16_t T2[]    = {(15 << 8) | 15, ASCII_ALL};
static const uint16_t V_lb[]  = {'('};
static const uint16_t V_rb[]  = {')'};
static const uint16_t V_em[]  = {'!'};
static const uint16_t V_pe[]  = {'%'};
static const uint16_t V_pi[]  = {'|'};
static const uint16_t V_am[]  = {'&'};
static const uint16_t V_ca[]  = {'^'};
static const uint16_t V_x[]   = {'x'};
static const uint16_t V_o[]   = {'o'};
static const uint16_t V_r[]   = {'r'};
static const uint16_t V_a[]   = {'a'};
static const uint16_t V_n[]   = {'n'};
static const uint16_t V_d[]   = {'d'};
static const uint16_t V_t[]   = {'t'};
static const uint16_t V_sp[]  = {' '};

/*
 * Store pointers to all rules in the grammar as well as the production count,
 * note that the rule count does not include unit productions for E0,E,T1,T2
 * this is because the main purpose of these lengths is to make cycling through
 * productions of the form P -> QR easier.
 */
static const uint16_t* const rules[] = {E0,  E,   L, L1, L2, E1, E2, E3, N, N1, X, O, O1, O2, T1,   T2,   V_lb, V_rb, V_em, V_pe, V_pi, V_am, V_ca, V_x, V_o, V_r, V_a, V_n, V_d, V_t, V_sp};
static const int rule_lens[]         = {6,   6,   1, 1,  1,  1,  1,  1,  1, 1,  1, 6, 1,  1,  1,    1,    1,    1,    1,    1,    1,    1,    1,    1,   1,   1,   1,   1,   1,   1,   1};


/*
 * This struct holds all the information nessecary to descend a parse tree
 * from any given node. i.e. rules of the form P -> QR
 */
struct parse_node {
    int nt1;        // nt1 is the index in the 'rules' array of the left nonterminal
    int nt2;        // nt2 is the index in the 'rules' array of the right nonterminal
    
    int start1;     // start1 is the index in the expression string where the left nonterminal starts
    int start2;     // start2 is the index in the expression string where the right nonterminal starts
    
    int len1;       // len1 is the number of terminals in the expression string that nt1 covers
    int len2;       // len1 is the number of terminals in the expression string that nt2 covers
};


static struct parse_node* parse_table;  // Pointer to root of entire parse tree/table of given expression
static char* expr;                      // The expression that was parsed (equivalent to argv[1])
static size_t n;                        // The number of characters in the expression


/*
 * Free's memory used by the parse table, to be used
 * as an exit handler.
 */
static void free_parse_table() {
    free(parse_table);
}



/*
 * Recursively descends a parse tree starting with a specified root
 * node and determines whether or not the expression specified by
 * the parse tree evaluates to true or false given a set of tags
 *
 *  PARAMETERS
 *      root - a pointer to the start node in the parse table
 *      tags - a set of tag strings to be matched by the expression
 *
 *  RETURN VALUE
 *      1 if the provided set of tags matched the parse table of the
 *      expression otherwise 0
 *
 *  NOTE
 *      Although this function is recursive the approximate with tail
 *      call optimization the memory used each function call can be
 *      0 for most calls with the exception of operator parsing where
 *      approximately 112 bytes of stack space will be used. This is
 *      not a serious limit since with a default 8MB stack roughly
 *      74,000 operators can be parsed before stack overflow.
 */
static int evaluate(struct parse_node* root, char** tags) {
    if(root->nt1 == 14 && root->nt2 == 14) {
        // Expression is tag
        char* tag;
        while( (tag = *(tags++)) != NULL ) {
            // Iterate all tags if there are any matches return true
            if(strlen(tag) == root->len1 + root->len2 && strncmp(tag, expr + root->start1 - 1, root->len1 + root->len2) == 0) {
                return 1;
            }
        }
        
        return 0;
    } else if( (root->nt1 == 18 && root->nt2 == 1) || (root->nt1 == 27 && root->nt2 == 6) ) {
        // Expression is negated
        return !evaluate(&parse_table[index_of(root->len2, root->start2, root->nt2, n)], tags);
    } else if(root->nt1 == 15) {
        // Expression is escaped tag
        char* tag;
        while( (tag = *(tags++)) != NULL ) {
            // Iterate all tags if there are any matches return true
            if(strlen(tag) == root->len1 && strncmp(tag, expr + root->start1 - 1, root->len1) == 0) {
                return 1;
            }
        }
        
        return 0;
    } else if(root->nt1 == 1 && root->nt2 == 2) {
        /*
         * Expression has operator, follow the chain of nonterminals down
         * until both expressions are found as well as the operator
         */
        struct parse_node* L_node  = &parse_table[index_of(root->len2, root->start2, root->nt2, n)];
        struct parse_node* L1_node = &parse_table[index_of(L_node->len2, L_node->start2, L_node->nt2, n)];
        struct parse_node* L2_node = &parse_table[index_of(L1_node->len2, L1_node->start2, L1_node->nt2, n)];
        struct parse_node* O_node  = &parse_table[index_of(L1_node->len1, L1_node->start1, L1_node->nt1, n)];
        
        struct parse_node* E_node_1  = &parse_table[index_of(root->len1, root->start1, root->nt1, n)];
        struct parse_node* E_node_2  = &parse_table[index_of(L2_node->len2, L2_node->start2, L2_node->nt2, n)];
        
        if( (O_node->nt1 == 23 && O_node->nt2 == 12) || (O_node->nt1 == 22 && O_node->nt2 == 22) ) {
            // XOR
            return ( evaluate(E_node_1, tags) != evaluate(E_node_2, tags) );
        } else if( (O_node->nt1 == 24 && O_node->nt2 == 25) || (O_node->nt1 == 20 && O_node->nt2 == 20) ) {
            // OR
            return ( evaluate(E_node_1, tags) || evaluate(E_node_2, tags) );
        } else {
            // AND
            return ( evaluate(E_node_1, tags) && evaluate(E_node_2, tags) );
        }
    } else if(root->nt1 == -1) {
        // Expression was tag but single character
        char* tag;
        while( (tag = *(tags++)) != NULL ) {
            // Iterate all tags if there are any matches return true
            if(tag[1] == '\0' && tag[0] == expr[root->start1-1]) {
                return 1;
            }
        }
        
        return 0;
    }
    
    /*
     * Some intermediate stage in the tree, keep following
     * nonterminals with rules of the form P -> QR until
     * an expression is encountered. This can be tail
     * optimizied.
     */
    if(root->nt1 < 16) {
        return evaluate(&parse_table[index_of(root->len1, root->start1, root->nt1, n)], tags);
    } else if(root->nt2 < 16) {
        return evaluate(&parse_table[index_of(root->len2, root->start2, root->nt2, n)], tags);
    } else {
        // This shouldn't happen
        fprintf(stderr, "tagstat: assertion error: two child unit productions encountered %d %d. quitting...\n", root->nt1, root->nt2);
        exit(4);
    }
}


/*
 * Builds a parse tree for the given expression using the
 * bottom-up CYK context-free grammar parsing algorithm
 * with dynamic programming.
 *
 *  LINK TO ALGORITHM
 *      https://en.wikipedia.org/wiki/CYK_algorithm
 *
 *  PARAMETERS
 *      arg - the expression string to be parsed
 */
static void build_parse_table(char* arg) {
    expr = arg;
    n   = strlen(expr);
    
    /*
     * Allocate space for the parse table, n+1 is used since the CYK algorithm I
     * used from wikipedia started indices at 1 instead of 0 and I was too lazy
     * to change it.
     */
    parse_table = calloc((n+1)*(n+1)*GRAMMAR_NUM_NT, sizeof(struct parse_node));
    if(parse_table == NULL) {
        fprintf(stderr, "tagstat: out of memory. qutting...\n");
        exit(3);
    }
    
    /*
     * Install this exit handler so I can be lazy and not be
     * constantly writing free(parse_table) for every error
     */
    atexit(free_parse_table);
    
    
    /*
     * The rest of the code in this function implements the CYK
     * bottom-up context-free grammar parsing algorithm. The
     * implementation is my own based on psuedocode from
     *
     * https://en.wikipedia.org/wiki/CYK_algorithm
     */
    
    int i;
    for(i = 1; i <= n ; i++) {
        /*
         * This loop determines which unit productions, that
         * is rules of the form A -> a could have produced
         * the terminals in the expression string
         */
        struct parse_node node;
        node.nt1 = -1;
        node.nt2 = -1;
        node.start1 = i;
        node.len1 = 1;
        
        // Scans all nonterminals that represent a single terminal symbol
        int r;
        for(r = 16; r < GRAMMAR_NUM_NT ; r++) {
            if(*rules[r] == expr[i-1]) {
                memcpy(&parse_table[index_of(1, i, r, n)], &node, sizeof(struct parse_node));
            }
        }
        
        /*
         * If the character is not whitespace, a percent sign, an
         * exclamation mark, or parenthesis then it could either
         * be a single symbol tag generated by either E0 or E or
         * it could be a multi symbol tag generated by T1
         */
        if(expr[i-1] >= 33 && expr[i-1] != '%' && expr[i-1] != '!' && expr[i-1] != '(' && expr[i-1] != ')') {
            memcpy(&parse_table[index_of(1, i, 0, n)], &node, sizeof(struct parse_node));
            memcpy(&parse_table[index_of(1, i, 1, n)], &node, sizeof(struct parse_node));
            memcpy(&parse_table[index_of(1, i, 14, n)], &node, sizeof(struct parse_node));
        }
        
        // T2 can be produce any nonterminal so this is always set
        memcpy(&parse_table[index_of(1, i, 15, n)], &node, sizeof(struct parse_node));
    }
    
    /*
     * This quintuple for loop searches through possible subsequences
     * and paritions them in to two halves. Then determining if any
     * rules of the form P -> QR such that Q matches the left half
     * of the subsequence and R matches the right half of the
     * subsequence. In order to make this process efficient dynamic
     * programming is used so that stored entries for P -> QR can
     * be used to determine the applicability of other rules.
     */
    for(i = 2; i <= n ; i++) {
        int j;
        for(j = 1; j <= n-i+1; j++) {
            int k;
            for(k = 1; k <= i-1; k++) {
                
                int r;
                for(r = 0; r < 16; r++) {
                    int p;
                    for(p = 0; p < rule_lens[r]; p++) {
                        int b = (rules[r][p] >> 8) & 255;
                        int c =  rules[r][p]       & 255;
                        
                        if(parse_table[index_of(k, j, b, n)].len1 != 0 && parse_table[index_of(i-k, j+k, c, n)].len1 != 0) {
                            // Store information pertaining to this nonterminals children
                            struct parse_node node;
                            node.nt1 = b;
                            node.nt2 = c;
                            node.start1 = j;
                            node.len1 = k;
                            node.start2 = j+k;
                            node.len2 = i-k;
                            
                            memcpy(&parse_table[index_of(i, j, r, n)], &node, sizeof(struct parse_node));
                        }
                    }
                }
                
            }
        }
    }
    
}


/*
 * Proc parsing helper function, counts the number of ptags and number
 * of bytes required to store the tags, from a buffered proc read for
 * a process specified by 'cur_pid'
 *
 *  PARAMETERS
 *      line       - A pointer to the start of the first line of the
 *                   in the proc entry buffer
 *
 *      end        - A pointer to the end of the proc entry buffer
 *
 *      tag_count  - pointer to a location holding a long to store
 *                   the number of tags for the given process
 *
 *      tags_bytes - pointer to a location holding a long to store
 *                   the number of bytes required to store the
 *                   specified processes tags
 *
 *      cur_pid    - the pid of the process to count the tags of
 *
 *  RETURN VALUE
 *      A pointer to the line that would've been counted next had
 *      cur_pid matched the pid value in the next line or NULL
 *      if the end of the proc entry buffer was reached
 */
static char* count_ptags(char* line, char* end, long* tag_count, long* tags_bytes, pid_t cur_pid) {
    line -= 1;
    
    *tag_count  = 0;
    *tags_bytes = 0;
    
    do {
        // Move to start of line
        line += 1;
        
        // This condition is basically an EOF
        if(line >= end) {
            return NULL;
        }
        
        pid_t pid = (pid_t)strtoul(line, NULL, 10);
        if(pid != cur_pid) {
            /*
             * This tag is not part of the process
             * currently being scanned, this means
             * that the end of the current tag set
             * has been reached.
             */
            
            break;
        }
        
        // Find colon seperating pid and tag
        char* sep1 = strchr(line, ':');
        if(sep1 == NULL) {
            // Formatting error, shouldn't happen, ignore tag
            continue;
        }
        
        // Find colon seperating tag and state
        char* sep2 = strrchr(line, ':');
        if(sep2 == NULL) {
            // Formatting error, shouldn't happen, ignore tag
            continue;
        }
        
        (*tag_count)++;
        (*tags_bytes) += (sep2 - sep1) - 2;
    } while( (line = strchr(line, '\0')) != NULL);
    
    return line;
}


/*
 * Proc parsing helper function, copies the ptags from a buffered
 * proc read for a process specified by 'cur_pid'
 *
 *  PARAMETERS
 *      line     - A pointer to the start of the first line of the
 *                 in the proc entry buffer
 *
 *      end      - A pointer to the end of the proc entry buffer
 *
 *      tags     - A pointer to memory to hold pointers to the copied tags
 *
 *      tag_pool - A pointer to an area in memory big enough to store all tags
 *
 *      cur_pid  - the pid of the process to copy the tags of
 *
 *  RETURN VALUE
 *      A pointer to the line that would've been copied next had
 *      cur_pid matched the pid value in the next line or NULL
 *      if the end of the proc entry buffer was reached
 */
static char* copy_ptags(char* line, char* end, char** tags, char* tag_pool, pid_t cur_pid) {
    line -= 1;
    
    do {
        // Move to start of line
        line += 1;
        
        // This condition is basically an EOF
        if(line >= end) {
            return NULL;
        }
        
        pid_t pid = (pid_t)strtoul(line, NULL, 10);
        if(pid != cur_pid) {
            /*
             * This tag is not part of the process
             * currently being scanned, this means
             * that the end of the current tag set
             * has been reached.
             */
            
            break;
        }
        
        // Find colon seperating pid and tag
        char* sep1 = strchr(line, ':');
        if(sep1 == NULL) {
            // Formatting error, shouldn't happen, ignore tag
            continue;
        }
        
        // Find colon seperating tag and state
        char* sep2 = strrchr(line, ':');
        if(sep2 == NULL) {
            // Formatting error, shouldn't happen, ignore tag
            continue;
        }
        
        long tag_len = (sep2 - sep1) - 3;
        
        // Copy tag and add null terminator
        memcpy(tag_pool, sep1+2, tag_len);
        tag_pool[tag_len] = '\0';
        
        // Update pointers
        *(tags++) = tag_pool;
        tag_pool += tag_len+1;
    } while( (line = strchr(line, '\0')) != NULL);
    
    return line;
}


/*
 * Proc parsing helper function, prints the ptags from a buffered
 * proc read for a process specified by 'cur_pid'
 *
 *  PARAMETERS
 *      line     - A pointer to the start of the first line of the
 *                 in the proc entry buffer
 *
 *      end      - A pointer to the end of the proc entry buffer
 *
 *      cur_pid  - the pid of the process to print the tags of
 *
 *  RETURN VALUE
 *      A pointer to the line that would've been printed next had
 *      cur_pid matched the pid value in the next line or NULL
 *      if the end of the proc entry buffer was reached
 */
static char* print_ptags(char* line, char* end, pid_t cur_pid) {
    line -= 1;
    
    do {
        // Move to start of line
        line += 1;
        
        // This condition is basically an EOF
        if(line >= end) {
            return NULL;
        }
        
        pid_t pid = (pid_t)strtoul(line, NULL, 10);
        if(pid != cur_pid) {
            /*
             * This tag is not part of the process
             * currently being scanned, this means
             * that the end of the current tag set
             * has been reached.
             */
            
            break;
        }
        
        // Find colon seperating pid and tag
        char* sep1 = strchr(line, ':');
        if(sep1 == NULL) {
            // Formatting error, shouldn't happen, ignore tag
            continue;
        }
        
        // Find colon seperating tag and state
        char* sep2 = strrchr(line, ':');
        if(sep2 == NULL) {
            // Formatting error, shouldn't happen, ignore tag
            continue;
        }
        
        // Copy line from /proc/ptags to stdout
        printf("%s", line);
    } while( (line = strchr(line, '\0')) != NULL);
    
    return line;
}

const char* const usage_str = "Usage:\n"
                                "\ttagstat <tag> OR tagstat '<expr>'\n\n"

                                "\tpassing --help will print this usage information, thus if\n"
                                "\tyou wish to use --help as tag it must be encased in either\n"
                                "\tparenthesis or escaped, see below.\n\n"

                                "\tWhere <expr> is a boolean expression of the form:\n"
                                    "\t\t[operator2] <expr> <operator1> [operator2] <expr>\n"
                                "\tOR\n"
                                    "\t\t<tag>\n\n"

                                "\t<operator1> can be any of the following boolean operators:\n"
                                    "\t\t'&&', 'and', '||', 'or', '^^', 'xor'\n\n"

                                "\t[operator2] is an optional NOT operator:\n"
                                    "\t\t'!', 'not'\n\n"

                                "\tOperators must be lowercase, <operator1> must contain\n"
                                "\ta space on either side or the expression will be treated\n"
                                "\tas a tag. In the case of 'not' for [operator2] the 'not'\n"
                                "\tmust be suffixed with a space otherwise the expression\n"
                                "\twill be treated as a tag. No space is needed for '!'.\n\n"

                                "\tThe NOT operator has highest preecendence, all other\n"
                                "\toperators have equal preecendecnce. Thus expressions\n"
                                "\tcontaining more than one type of operator (excluding NOT)\n"
                                "\tshould be contained in parenthesis so that the order of\n"
                                "\toperations is clear. Without parenthesis, leftmost\n"
                                "\texpressions will be evaluated first.\n\n"

                                "\tAny parenthesized expression prefixed with a '%%' will be\n"
                                "\ttreated as a tag literal. This is so tags containing\n"
                                "\toperators and whitespace can be specified.\n\n"

                                "\te.g. %%(tagwith || and !!)\n\n"

                                "\tTags cannot contain percent signs or parenthesis unless\n"
                                "\tescaped.\n\n";


int main(int argc, const char * argv[]) {
    if(argc != 2) {
        if(argc == 1) {     // No arguments will print all tags users owns
            // Open ptag proc entry for reading
            int ptags_pfd = open("/proc/ptags", O_RDONLY);
            if(ptags_pfd < 0) {
                fprintf(stderr, "tagstat: error accessing /proc/ptags: %s\n", strerror(errno));
                
                return 5;
            }
            
            /*
             * proc entry reads are confined to PAGESIZE bytes,
             * apparently there is some sort of limit on single
             * proc entry reads? I'm not sure if the information
             * I found was only for old kernels, the web was
             * very confusing so I decided to just stick with
             * PAGESIZE reads
             */
            long pagesize = sysconf(_SC_PAGESIZE);
            
            // Make some space for proc entry contents
            char* ptags = malloc(pagesize);
            if(ptags == NULL) {
                fprintf(stderr, "tagstat: out of memory. qutting...\n");
                close(ptags_pfd);
                
                return 3;
            }
            
            /*
             * Copy contents of /proc/ptags to ptags buffer, this is done so that
             * tagstat operation is atomic.
             */
            long proc_len;
            if( (proc_len = read(ptags_pfd, ptags, pagesize)) < 0) {
                fprintf(stderr, "tagstat: error reading /proc/ptags: %s\n", strerror(errno));
                
                free(ptags);
                close(ptags_pfd);
                
                return 5;
            }
            
            close(ptags_pfd);
            
            if(proc_len > 0) {
                write(STDOUT_FILENO, ptags, proc_len);
            } else {
                printf("You do not currently own any tagged processes.\n");
            }
            
            free(ptags);
            
            return 0;
        } else {            // Anything else is incorrect usage
            fprintf(stderr, "tagstat: Incorrect usage.\n");
            fprintf(stderr, "Try tagstat --help for more info.\n");
            
            return 1;
        }
    }
    
    // Check if argument was --help
    if(strncmp(argv[1], "--help", sizeof("--help")) == 0) {
        // If argument was --help print usage information and exit
        printf(usage_str);
        return 0;
    }
    
    // Build parse table and check if expression is valid
    build_parse_table((char*)argv[1]);
    
    struct parse_node* root = &parse_table[index_of(n, 1, 0, n)];
    if(root->nt1 == 0) {
        fprintf(stderr, "tagstat: Syntax error: invalid expression.\n");
        fprintf(stderr, "Try tagstat --help for more info.\n");
        
        return 2;
    }
    
    // Open ptag proc entry for reading
    int ptags_pfd = open("/proc/ptags", O_RDONLY);
    if(ptags_pfd < 0) {
        fprintf(stderr, "tagstat: error accessing /proc/ptags: %s\n", strerror(errno));
        
        return 5;
    }
    
    /*
     * proc entry reads are confined to PAGESIZE bytes,
     * apparently there is some sort of limit on single
     * proc entry reads? I'm not sure if the information
     * I found was only for old kernels, the web was
     * very confusing so I decided to just stick with
     * PAGESIZE reads
     */
    long pagesize = sysconf(_SC_PAGESIZE);
    
    // Make some space for proc entry contents
    char* ptags = malloc(pagesize);
    if(ptags == NULL) {
        fprintf(stderr, "tagstat: out of memory. qutting...\n");
        close(ptags_pfd);
        
        return 3;
    }
    
    /*
     * Copy contents of /proc/ptags to ptags buffer, this is done so that
     * tagstat operation is atomic.
     */
    long proc_len;
    if( (proc_len = read(ptags_pfd, ptags, pagesize)) < 0) {
        fprintf(stderr, "tagstat: error reading /proc/ptags: %s\n", strerror(errno));
        
        free(ptags);
        close(ptags_pfd);
        
        return 5;
    }
    
    close(ptags_pfd);
    
    if(proc_len > 0) {
        int found_match = 0;
        
        char* cur_line = ptags;
        char* proc_end = ptags + proc_len;
        
        /*
         * This loop scans all tags for each process and
         * determines whether or not the processes tags
         * match the given expression and if so copies
         * the lines in the proc entry to stdout.
         */
        do {
            // Get the pid of the current process to scan
            pid_t cur_pid = (pid_t)strtoul(cur_line, NULL, 10);
            
            // Count its ptags and the number of bytes needed to store them
            long tag_count;
            long tags_bytes;
            count_ptags(cur_line, proc_end, &tag_count, &tags_bytes, cur_pid);
            
            // Allocate some memory to store the ptags
            char** tags     = malloc(sizeof(char*)*(tag_count+1));
            char*  tag_pool = malloc(tags_bytes);
            tags[tag_count] = NULL; // NULL terminate tag pointers
            
            if(tags == NULL || tag_pool == NULL) {
                fprintf(stderr, "tagstat: out of memory. qutting...\n");
                free(ptags);
                
                // Clean up memory
                if(tags) {
                    free(tags);
                }
                if(tag_pool) {
                    free(tag_pool);
                }
                
                return 3;
            }
            
            // Extract the tags out of the proc entry contents
            char* tmp_line;
            tmp_line = copy_ptags(cur_line, proc_end, tags, tag_pool, cur_pid);
            
            /*
             * Test expression against the current set of tags and
             * print them if there's a match
             */
            if(evaluate(root, tags)) {
                print_ptags(cur_line, proc_end, cur_pid);
                found_match = 1;
            }
            
            free(tag_pool); // Free the tag pool
            free(tags);     // Free tag pointers
            
            cur_line = tmp_line;
        } while(cur_line != NULL);
        
        if(!found_match) {
            printf("No matching tagged processes found.\n");
        }
    } else {
        printf("You do not currently own any tagged processes.\n");
    }
    
    // Free proc entry contents
    free(ptags);
    
    return 0;
}
