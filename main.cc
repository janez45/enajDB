// Note to self: I'm following the tutorial.
// This is in C++, but it's gonna look very C-ish. I'm only applying synctactic sugar where needed
#include <iostream>
#include <string>

struct InputBuffer
{
    std::string buffer;
    size_t buffer_length;
    ssize_t input_length; // ssize is typed
    InputBuffer() : buffer(), buffer_length{0}, input_length{0} {};
};

enum MetaCommandResult
{
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
};

enum PrepareResult
{
    PREPARE_SUCCESS,
    PREPARE_UNRECOGNIZED_STATEMENT
};

enum StatementType
{
    STATEMENT_INSERT,
    STATEMENT_SELECT
};

struct Statement
{
    StatementType type;
};

InputBuffer new_input_buffer()
{
    return InputBuffer{};
}

MetaCommandResult do_meta_command(const InputBuffer &input_buffer)
{
    if (input_buffer.buffer == ".exit")
    {
        exit(EXIT_SUCCESS);
    }
    else
    {
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}

PrepareResult prepare_statement(const InputBuffer &input_buffer, Statement &statement)
{
    if (input_buffer.buffer.starts_with("insert"))
    {
        statement.type = STATEMENT_INSERT;
        return PREPARE_SUCCESS;
    }
    else if (input_buffer.buffer.starts_with("select"))
    {
        statement.type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }

    return PREPARE_UNRECOGNIZED_STATEMENT;
}

void execute_statement(const Statement &statement)
{
    switch (statement.type)
    {
    case (STATEMENT_INSERT):
        std::cout << "We'd do an insert here" << std::endl;
        break;
    case (STATEMENT_SELECT):
        std::cout << "We'd do a select here" << std::endl;
        break;
    }
}

void print_prompt()
{
    std::cout << "db > ";
}

void read_input(InputBuffer &input_buffer)
{
    if (!std::getline(std::cin, input_buffer.buffer))
    {
        std::cerr << "Error reading input" << std::endl;
        exit(EXIT_FAILURE);
    }

    input_buffer.input_length = static_cast<ssize_t>(input_buffer.buffer.size());
    input_buffer.buffer_length = input_buffer.buffer.size();
}

void close_input_buffer(InputBuffer *input_buffer)
{
    delete input_buffer;
}

int main(int argc, char *argv[])
{
    InputBuffer input_buffer = new_input_buffer();
    while (true)
    {
        print_prompt();
        read_input(input_buffer);

        if (input_buffer.buffer[0] == '.')
        {
            switch (do_meta_command(input_buffer))
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
        case (PREPARE_UNRECOGNIZED_STATEMENT):
            std::cout << "Unrecognized keyword at start of '" << input_buffer.buffer << "'" << std::endl;
            continue;
        }

        execute_statement(statement);
        std::cout << "Executed." << std::endl;
    }
}