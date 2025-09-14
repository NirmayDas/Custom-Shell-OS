#ifndef PARSER_H
#define PARSER_H

// Parses a command line into argv tokens, stopping at any special symbol (<, >, 2>, |, or &).
// Returns the number of arguments parsed (argc).
// On success, *argv_out points to a NULL-terminated array of strings.
// Caller must free each string and the array itself.
int parse_argv(const char *line, char ***argv_out);

#endif // PARSER_H