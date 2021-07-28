#include<iostream>

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

    char *start = line;
    int startIdx;
    for( startIdx = 0; line[startIdx] != '\0' && isspace( line[startIdx] ); ++startIdx ) 
        ;

    if( line[startIdx] == '\0' ) {
        std::cout << "Empty input line.\n";
        return 0;
    }

    // start with the next character to find either a space-character
    // for the end of the string.  
    int endIdx;
    for( endIdx = startIdx + 1;  line[endIdx] != '\0' && ! isspace( line[endIdx] ); ++endIdx ) 
        ;
    line[endIdx] = '\0';   // a c-string that begins with "line[startIdx]" and ends at "line[endIdx]".
    argsForExec[0] = line + startIdx;  // argsForExec[0] by convention contains the name of the command.
    std::cout << "The first word on the line is: --" << line + startIdx << "--\n";   

    // At this point, characters line[0] .. line[endIdx] have been
    // processed. The task of finding the next argument or
    // command-option on this line begins by inspecting the remaining
    // characters, starting with line[endIdx+1].

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
