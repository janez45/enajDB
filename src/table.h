#pragma once
#include "constants.h"
#include "pager.h"
#include "utils.h"

// a table that holds the rows
class Table
{

private:
    uint32_t num_rows;
    Pager &pager;

    ExecuteResult execute_insert(const Statement &statement);
    ExecuteResult execute_select(const Statement &statement);

public:
    class Cursor
    {
        uint32_t row_num;
        Table &table;
        Cursor(uint32_t row_num, Table &table);

        // Allows Table to construct Cursors.
        friend class Table;

    public:
        Cursor &operator++(int);
        void read_row(Row &row);        // read row row_num into the object
        void write_row(const Row &row); // write the object at row_num
        bool end() const;
    };

    // TODO: I'm removing this. db_open should be part of a higher level Database.
    // When that appears, I'm making a table_open() to open a table instead of the constructor
    Table(Pager &pager);
    static constexpr size_t num_rows_size()
    {
        return sizeof(num_rows);
    }

    Cursor table_start();
    Cursor table_end();

    MetaCommandResult do_meta_command(const InputBuffer &input_buffer);
    ExecuteResult execute_statement(const Statement &statement);

    // TODO: This should also be part of a higher level Database
    // Here until I start multi-table
    void db_close();
};
