#pragma once
#include "constants.h"
#include "pager.h"
#include "utils.h"

// a table that holds the rows
class Table
{

private:
    uint32_t num_rows;
    Pager pager;

    Table(Pager pager);
    ExecuteResult execute_insert(const Statement &statement);
    ExecuteResult execute_select(const Statement &statement);

    friend Table db_open(const std::string &filename);

public:
    class Cursor
    {
        uint32_t row_num;
        bool end_of_table;
        Table *table;

        Cursor(uint32_t row_num, bool end_of_table, Table *table);

        // Allows Table to construct Cursors.
        friend class Table;

    public:
        Cursor &operator++(int);
        // returns pointer to where row row_num exists on this table
        std::byte *cursor_value();
    };

    Cursor table_start();
    Cursor table_end();
    MetaCommandResult do_meta_command(const InputBuffer &input_buffer);
    ExecuteResult execute_statement(const Statement &statement);

    void db_close();
};

Table db_open(const std::string &filename);