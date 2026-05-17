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

InputBuffer new_input_buffer()
{
    return InputBuffer{};
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

        if (input_buffer.buffer == ".exit")
        {
            // close_input_buffer(input_buffer);
            exit(EXIT_SUCCESS);
        }
        else
        {
            std::cout << "Unrecognized command " << input_buffer.buffer << std::endl;
        }
    }
}