#pragma once
#include <iostream>
#include <cstring>
#include "constants.h"

// enums
enum StatementType
{
    STATEMENT_INSERT,
    STATEMENT_SELECT
};

// structs
struct Statement
{
    StatementType type;
    Row row_to_insert;
};

struct InputBuffer
{
    std::string buffer;
    size_t buffer_length;
    ssize_t input_length; // ssize is typed
    InputBuffer();
};

// functions
void serialize_row(const Row &row, std::byte *destination);
void deserialize_row(std::byte *source, Row &row);
void print_row(const Row &row);

InputBuffer new_input_buffer();

void read_input(InputBuffer &input_buffer);
