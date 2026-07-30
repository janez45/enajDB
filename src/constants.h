#pragma once
#include <cstddef>
#include <cstdint>

static constexpr int COLUMN_USERNAME_SIZE = 32;
static constexpr int COLUMN_EMAIL_SIZE = 255;

// gets the size of the type of some member
// Struct::* is a member of the class. Say id in row for example
// Attribute is thus the type. So Attribute = int, Struct:: = a member inside Struct::
// Then it returns the sizeof(int)
// Attribute Struct::* essentially is "pointer to a member of Struct whose type is Attribute"
// using pointers because we don't need the value of a concrete object, we just need the size
template <typename Struct, typename Attribute>
static constexpr size_t size_of_attribute(Attribute Struct::*)
{
    return sizeof(Attribute);
}

struct Row
{
    uint32_t id;
    char username[COLUMN_USERNAME_SIZE + 1];
    char email[COLUMN_EMAIL_SIZE + 1];
};

const uint32_t ID_SIZE = size_of_attribute(&Row::id);
const uint32_t USERNAME_SIZE = size_of_attribute(&Row::username);
const uint32_t EMAIL_SIZE = size_of_attribute(&Row::email);
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

// table has page, page has rows
const uint32_t PAGE_SIZE = 4096;            // same size as page in most OS systems
static constexpr int TABLE_MAX_PAGES = 100; // arbitrary limit for now

const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;

// enums
enum MetaCommandResult
{
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
};

enum ExecuteResult
{
    EXECUTE_SUCCESS,
    EXECUTE_TABLE_FULL
};