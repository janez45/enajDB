#include "table.h"

// Table definitions
Table::Table(Pager pager) : num_rows{static_cast<uint32_t>(pager.file_length / ROW_SIZE)}, pager{std::move(pager)} {};

Table::Cursor Table::table_start()
{
    return Table::Cursor{0, num_rows == 0, this};
}

Table::Cursor Table::table_end()
{
    return Table::Cursor{num_rows, true, this};
}

void Table::db_close()
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

MetaCommandResult Table::do_meta_command(const InputBuffer &input_buffer)
{
    if (input_buffer.buffer == ".exit")
    {
        db_close();
        exit(EXIT_SUCCESS);
    }
    else
    {
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}

ExecuteResult Table::execute_insert(const Statement &statement)
{
    if (num_rows >= TABLE_MAX_ROWS)
    {
        return EXECUTE_TABLE_FULL;
    }

    Table::Cursor cursor = table_end(); // write to the end of the table

    serialize_row(statement.row_to_insert, cursor.cursor_value());
    num_rows++;

    return EXECUTE_SUCCESS;
}

ExecuteResult Table::execute_select(const Statement &statement)
{
    Table::Cursor cursor = table_start();
    Row row;
    while (!(cursor.end_of_table))
    {
        deserialize_row(cursor.cursor_value(), row);
        print_row(row);
        cursor++;
    }
    return EXECUTE_SUCCESS;
}

ExecuteResult Table::execute_statement(const Statement &statement)
{
    switch (statement.type)
    {
    case (STATEMENT_INSERT):
        return execute_insert(statement);
    case (STATEMENT_SELECT):
        return execute_select(statement);
    }
}

// Cursor defintions

Table::Cursor::Cursor(uint32_t row_num, bool end_of_table, Table *table) : row_num{row_num}, end_of_table{end_of_table}, table{table} {}

Table::Cursor &Table::Cursor::operator++(int)
{
    row_num++;
    if (row_num >= table->num_rows)
    {
        end_of_table = true;
    }
    return *this;
}

std::byte *Table::Cursor::cursor_value()
{
    uint32_t page_num = row_num / ROWS_PER_PAGE;
    std::byte *page = table->pager.get_page(page_num);
    uint32_t row_offset_on_page = row_num % ROWS_PER_PAGE; // this is the x*th row
    uint32_t byte_offset = row_offset_on_page * ROW_SIZE;
    return page + byte_offset;
}

Table db_open(const std::string &filename)
{
    Pager pager = pager_open(filename);
    return Table{pager};
}
