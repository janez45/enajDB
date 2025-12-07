#include <iostream>
#include <string>

void print_prompt(std::ostream &o)
{
    o << "enajDB > ";
}

typedef enum
{
    STATEMENT_INSERT,
    STATEMENT_SELECT,
    STATEMENT_UNKNOWN
} StatementType;

typedef enum
{
    PREPARE_SUCCESS,
    PREPARE_UNRECOGNIZED_STATEMENT
} StatementPrepareResult;

typedef struct
{
    StatementType type;
} Statement;

StatementPrepareResult prepare_statement(std::string &line, Statement &s)
{
    if (line.rfind("select", 0) == 0)
    {
        s.type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }
    else if (line.rfind("insert", 0) == 0)
    {
        s.type = STATEMENT_INSERT;
        return PREPARE_SUCCESS;
    }
    return PREPARE_UNRECOGNIZED_STATEMENT;
}

void execute_statement(Statement *statement)
{
    switch (statement->type)
    {
    case (STATEMENT_INSERT):
        std::cout << "This is where we would do an insert" << std::endl;
        break;
    case (STATEMENT_SELECT):
        std::cout << "This is where we would do a select" << std::endl;
        break;
    default:
        std::cerr << "This should not be hit" << std::endl;
    }
}

int main()
{
    std::string line;
    std::cout << "Welcome to EnajDB!" << std::endl;

    while (true)
    {
        // Prompt
        print_prompt(std::cout);

        // Get input
        if (!std::getline(std::cin, line))
        {
            std::cout << "\nGoodbye!\n";
            return 0;
        }

        // If it's a meta command, run it
        if (line == "quit")
        {
            break;
        }

        // Validate the statement (compiler)
        Statement statement;
        switch (prepare_statement(line, statement))
        {
        case (PREPARE_SUCCESS):
            break;
        case (PREPARE_UNRECOGNIZED_STATEMENT):
            std::cout << "Unrecognized statement: " << line << std::endl;
            continue;
        }

        // Execute the statement
        execute_statement(&statement);
    }

    std::cout << "\nGoodbye!\n";
    return 0;
}