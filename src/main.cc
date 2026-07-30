// Note to self: I'm following the tutorial.
#include <iostream>
#include <string>
#include <print>
#include <sstream>
#include <cstring>
#include <vector>
#include <fstream>

#include "constants.h"
#include "table.h"

enum PrepareResult
{
    PREPARE_SUCCESS,
    PREPARE_NEGATIVE_ID,
    PREPARE_STRING_TOO_LONG,
    PREPARE_UNRECOGNIZED_STATEMENT,
    PREPARE_SYNTAX_ERROR
};

PrepareResult prepare_insert(std::istream &inputStream, Statement &statement)
{
    statement.type = STATEMENT_INSERT;
    int id;
    std::string username;
    std::string email;
    if (!(inputStream >> id >> username >> email))
    {
        return PREPARE_SYNTAX_ERROR;
    }
    if (id < 0)
    {
        return PREPARE_NEGATIVE_ID;
    }
    if (username.size() > COLUMN_USERNAME_SIZE || username.size() > COLUMN_EMAIL_SIZE)
    {
        return PREPARE_STRING_TOO_LONG;
    }
    statement.row_to_insert.id = id;
    std::strcpy(statement.row_to_insert.username, username.c_str());
    std::strcpy(statement.row_to_insert.email, email.c_str());

    return PREPARE_SUCCESS;
}

PrepareResult prepare_statement(const InputBuffer &input_buffer, Statement &statement)
{
    std::istringstream inputStream(input_buffer.buffer);
    std::string command;

    inputStream >> command;
    if (command == "insert")
    {
        return prepare_insert(inputStream, statement);
    }
    else if (command == "select")
    {
        statement.type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }

    return PREPARE_UNRECOGNIZED_STATEMENT;
}

void print_prompt()
{
    std::cout << "db > ";
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Must supply a database filename" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string filename = argv[1];
    Table table = db_open(filename);
    InputBuffer input_buffer = new_input_buffer();
    while (true)
    {
        print_prompt();
        read_input(input_buffer);

        if (input_buffer.buffer[0] == '.')
        {
            switch (table.do_meta_command(input_buffer))
            {
            case (META_COMMAND_SUCCESS):
                continue;
            case (META_COMMAND_UNRECOGNIZED_COMMAND):
                std::cout << "Unrecognized command '" << input_buffer.buffer << "'" << std::endl;
                continue;
            }
        }

        Statement statement;
        switch (prepare_statement(input_buffer, statement))
        {
        case (PREPARE_SUCCESS):
            break;
        case (PREPARE_NEGATIVE_ID):
            std::cout << "ID must be positive." << std::endl;
            continue;
        case (PREPARE_STRING_TOO_LONG):
            std::cout << "String is too long." << std::endl;
            continue;
        case (PREPARE_SYNTAX_ERROR):
            std::cout << "Syntax error. Could not parse statement." << std::endl;
            continue;
        case (PREPARE_UNRECOGNIZED_STATEMENT):
            std::cout << "Unrecognized keyword at start of '" << input_buffer.buffer << "'" << std::endl;
            continue;
        }

        switch (table.execute_statement(statement))
        {
        case (EXECUTE_SUCCESS):
            std::cout << "Executed." << std::endl;
            break;
        case (EXECUTE_TABLE_FULL):
            std::cout << "Error: Table full." << std::endl;
            break;
        }
    }
}