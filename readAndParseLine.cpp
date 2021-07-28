#include<iostream>
#include<string.h>

const int MAX_LINE = 1024;
const int MAX_ARGS = 20;


int main( void )
{
    char line[ MAX_LINE ];
    char *argsForExec[ MAX_ARGS ];
    int llen;

    std::cin.getline( line, MAX_LINE, '\n' );
    llen = strlen( line );
    std::cout << line << std::endl;

    // skip leading spaces.
    char *start;
    for( start = line; *start && isspace( *start ); ++start ) 
        ;

    if( start == line + llen ) {
        std::cout << "Empty input line.\n";
        return 0;
    }

    // start with the next character to find either a space-character
    // for the end of the string.  
    char *end = start + 1;
    while( *end && ! isspace( *end ) ) 
        end++;
    *end = '\0';   // a c-string that begins with "start" and ends at "end".
    argsForExec[0] = start;  // argsForExec[0] by convention contains the name of the command.
    std::cout << "The first word on the line is: --" << start << "--\n";   
    std::cout << "The last word on the line is: --" << end - 1 << "--\n";  

    // At this point, characters at line .. end (line[0] .. line[end-line]) 
    // have been processed. The task of finding the
    // next argument or command-option on this line begins by
    // inspecting the remaining characters, starting with
    // line[endIdx+1].

    // As we find each of the arguments or command-options, we turn
    // them into null-termianted c-strings (similar to above
    // algorithm) and store them into the successive elements of
    // argsForExec. Suppose the last argument get stored in
    // argsForExec[i]. Then we do: argsForExec[i+1] = NULL; This is
    // required by the execvp system-call.  Now, argsForExec is 
    // ready to be used as the first argument of execvp. 
    // execvp( argsForExec[0], argsForExec );
    
    return 0;
}
