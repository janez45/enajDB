#include "utils.h"

InputBuffer::InputBuffer() : buffer(), buffer_length{0}, input_length{0} {};

// functions
void serialize_row(const Row &row, std::byte *destination)
{
    memcpy(destination + ID_OFFSET, &(row.id), sizeof(row.id));
    memcpy(destination + USERNAME_OFFSET, &(row.username), sizeof(row.username));
    memcpy(destination + EMAIL_OFFSET, &(row.email), sizeof(row.email));
}

void deserialize_row(std::byte *source, Row &row)
{
    memcpy(&(row.id), source + ID_OFFSET, ID_SIZE);
    memcpy(&(row.username), source + USERNAME_OFFSET, USERNAME_SIZE);
    memcpy(&(row.email), source + EMAIL_OFFSET, EMAIL_SIZE);
}

void print_row(const Row &row)
{
    std::cout << "(" << row.id << ", " << row.username << ", " << row.email << ")" << std::endl;
}

InputBuffer new_input_buffer()
{
    return InputBuffer{};
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
