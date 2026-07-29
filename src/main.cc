// Note to self: I'm following the tutorial.
#include <iostream>
#include <string>
#include <print>
#include <sstream>
#include <cstring>
#include <vector>
#include <fstream>

#include <fcntl.h>     // Required for open() and flags (like O_RDONLY, O_CREAT)
#include <sys/types.h> // Required for historical portability and type definitions
#include <sys/stat.h>  // Required if you use mode flags when creating files
#include <unistd.h>    // Required for lseek()

static constexpr int COLUMN_USERNAME_SIZE = 32;
static constexpr int COLUMN_EMAIL_SIZE = 255;

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

// pager accesses the page cache and the file
struct Pager
{
    int fd;
    off_t file_length;
    std::vector<std::byte *> pages;
    Pager(int fd, off_t file_length) : fd{fd}, file_length{file_length}, pages{std::vector<std::byte *>(TABLE_MAX_PAGES, nullptr)} {}
    ~Pager()
    {
        for (std::byte *page : pages)
        {
            delete[] page;
        }
    }
    std::byte *get_page(uint32_t page_num)
    {
        if (page_num > TABLE_MAX_PAGES)
        {
            std::cerr << "Tried to fetch page number out of bounds. " << page_num << " > " << TABLE_MAX_PAGES << std::endl;
            exit(EXIT_FAILURE);
        }

        // cache miss. Allocate memory and load from file
        if (pages[page_num] == nullptr)
        {
            std::byte *page = new std::byte[PAGE_SIZE];   // this could memory leak
            uint32_t num_pages = file_length / PAGE_SIZE; // how many pages are there in total in this table?

            // there is one more page, a partial one at the end of the file
            if (file_length % PAGE_SIZE)
            {
                num_pages++;
            }

            if (page_num <= num_pages)
            {
                lseek(fd, page_num * PAGE_SIZE, SEEK_SET);
                ssize_t bytes_read = read(fd, page, PAGE_SIZE); // read into the pager
                if (bytes_read == -1)
                {
                    std::cerr << "Error reading file: " << errno << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
            pages[page_num] = page;
        }

        return pages[page_num];
    }

    // Size is needed in case we need to flush a partial page
    void flush(uint32_t page_num, uint32_t size)
    {
        if (pages[page_num] == nullptr)
        {
            std::cerr << "Tried to flush null page" << std::endl;
            exit(EXIT_FAILURE);
        }

        off_t offset = lseek(fd, page_num * PAGE_SIZE, SEEK_SET);
        if (offset == -1)
        {
            std::cerr << "Error seeing: " << errno << std::endl;
            exit(EXIT_FAILURE);
        }

        ssize_t bytes_written = write(fd, pages[page_num], size);
        if (bytes_written == -1)
        {
            std::cerr << "Error writing: " << errno << std::endl;
            exit(EXIT_FAILURE);
        }
    }
};

// TODO: Should this be ptr or object?
Pager pager_open(const std::string &filename)
{
    // open the file in read/write mode. Create if not existing
    // Add read / write permissions for the user if creating this file
    int fd = open(filename.c_str(), O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);

    if (fd == -1)
    {
        std::cout << "Unable to open file" << std::endl;
        exit(EXIT_FAILURE);
    }

    // position right at the end. You add the zero from there
    // opposite of SEEK_END is SEEK_SET
    // in bytes
    off_t file_length = lseek(fd, 0, SEEK_END);
    Pager pager{fd, file_length};
    return pager;
}

// a table that holds the rows
struct Table
{
    struct Cursor
    {
        uint32_t row_num;
        bool end_of_table;
        Table *table;

        Cursor &operator++(int)
        {
            row_num++;
            if (row_num >= table->num_rows)
            {
                end_of_table = true;
            }
            return *this;
        }
    };
    uint32_t num_rows;
    Pager pager;
    // TODO: I casted this
    Table(Pager pager) : num_rows{static_cast<uint32_t>(pager.file_length / ROW_SIZE)}, pager{std::move(pager)} {};

    Cursor table_start()
    {
        return Cursor{0, num_rows == 0, this};
    }

    Cursor table_end()
    {
        return Cursor{num_rows, true, this};
    }

    void db_close()
    {
        uint32_t num_full_pages = num_rows / ROWS_PER_PAGE;

        for (uint32_t i = 0; i < num_full_pages; i++)
        {
            if (pager.pages[i] == nullptr)
            {
                continue;
            }
            pager.flush(i, PAGE_SIZE);
            delete[] pager.pages[i];
            pager.pages[i] = nullptr;
        }

        // is there an extra page?
        uint32_t num_additional_rows = num_rows % ROWS_PER_PAGE;
        if (num_additional_rows)
        {
            uint32_t page_num = num_full_pages;
            pager.flush(page_num, num_additional_rows * ROW_SIZE);
            delete[] pager.pages[page_num];
            pager.pages[page_num] = nullptr;
        }

        int result = close(pager.fd);
        if (result == -1)
        {
            std::cerr << "Error closing db file." << std::endl;
            exit(EXIT_FAILURE);
        }
    }
};

Table db_open(const std::string &filename)
{
    Pager pager = pager_open(filename);
    return Table{pager};
}

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

// returns pointer to where row row_num exists on this table
std::byte *cursor_value(const Table::Cursor &cursor)
{
    uint32_t row_num = cursor.row_num;
    uint32_t page_num = row_num / ROWS_PER_PAGE;
    std::byte *page = cursor.table->pager.get_page(page_num);
    uint32_t row_offset_on_page = row_num % ROWS_PER_PAGE; // this is the x*th row
    uint32_t byte_offset = row_offset_on_page * ROW_SIZE;
    return page + byte_offset;
}

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
    PREPARE_NEGATIVE_ID,
    PREPARE_STRING_TOO_LONG,
    PREPARE_UNRECOGNIZED_STATEMENT,
    PREPARE_SYNTAX_ERROR
};

enum StatementType
{
    STATEMENT_INSERT,
    STATEMENT_SELECT
};

enum ExecuteResult
{
    EXECUTE_SUCCESS,
    EXECUTE_TABLE_FULL
};

struct Statement
{
    StatementType type;
    Row row_to_insert;
};

InputBuffer new_input_buffer()
{
    return InputBuffer{};
}

MetaCommandResult do_meta_command(const InputBuffer &input_buffer, Table &table)
{
    if (input_buffer.buffer == ".exit")
    {
        table.db_close();
        exit(EXIT_SUCCESS);
    }
    else
    {
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}

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

ExecuteResult execute_insert(const Statement &statement, Table &table)
{
    if (table.num_rows >= TABLE_MAX_ROWS)
    {
        return EXECUTE_TABLE_FULL;
    }

    Table::Cursor cursor = table.table_end(); // write to the end of the table

    serialize_row(statement.row_to_insert, cursor_value(cursor));
    table.num_rows++;

    return EXECUTE_SUCCESS;
}

ExecuteResult execute_select(const Statement &statement, Table &table)
{
    Table::Cursor cursor = table.table_start();
    Row row;
    while (!(cursor.end_of_table))
    {
        deserialize_row(cursor_value(cursor), row);
        print_row(row);
        cursor++;
    }
    return EXECUTE_SUCCESS;
}

ExecuteResult execute_statement(const Statement &statement, Table &table)
{
    switch (statement.type)
    {
    case (STATEMENT_INSERT):
        return execute_insert(statement, table);
    case (STATEMENT_SELECT):
        return execute_select(statement, table);
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
            switch (do_meta_command(input_buffer, table))
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

        switch (execute_statement(statement, table))
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