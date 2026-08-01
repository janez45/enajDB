#include "table.h"

// For now, the first bytes dictate the number of rows in this table
// TODO: What if this overflows? For now, make sure the TABLE_MAX_ROWS constant will not exceed this size
static constexpr size_t TABLE_METADATA_NUM_ROWS_OFFSET = 0;
static constexpr size_t ROW_START_OFFSET = Table::num_rows_size();

// Table definitions
Table::Table(Pager &pager) : pager{pager}
{
    PageId metadata_page_id = 0;
    // If there exists no data in the database yet, initialize the table with metadata
    if (!pager.get_num_pages())
    {
        metadata_page_id = pager.allocate_page();
        Pager::Page &page = pager.get_page(metadata_page_id);
        num_rows = 0;
        std::memcpy(page.data().data() + TABLE_METADATA_NUM_ROWS_OFFSET, &num_rows, sizeof(num_rows));
        pager.mark_dirty(metadata_page_id);
    }
    else
    {
        // figure out the number of rows
        Pager::Page &page = pager.get_page(metadata_page_id);
        std::memcpy(&num_rows, page.data().data(), sizeof(num_rows)); // read in the number of rows
    }
};

Table::Cursor Table::table_start()
{
    return Table::Cursor{0, *this};
}

Table::Cursor Table::table_end()
{
    return Table::Cursor{num_rows, *this};
}

// closing the pager when the db is closed
void Table::db_close()
{
    pager.pager_close();
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

    cursor.write_row(statement.row_to_insert);
    num_rows++;

    // need to write the number of rows to the disk
    Pager::Page &metadata_page = pager.get_page(0);

    std::memcpy(
        metadata_page.data().data() + TABLE_METADATA_NUM_ROWS_OFFSET,
        &num_rows,
        sizeof(num_rows));

    pager.mark_dirty(0);

    return EXECUTE_SUCCESS;
}

ExecuteResult Table::execute_select(const Statement &statement)
{
    Table::Cursor cursor = table_start();
    Row row;
    while (!(cursor.end()))
    {
        cursor.read_row(row);
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

Table::Cursor::Cursor(uint32_t row_num, Table &table) : row_num{row_num}, table{table} {}

bool Table::Cursor::end() const
{
    return row_num >= table.num_rows;
}

Table::Cursor &Table::Cursor::operator++(int)
{
    if (!end())
    {
        ++row_num;
    }
    return *this;
}

// read row row_num into the object
void Table::Cursor::read_row(Row &row)
{
    if (end())
    {
        throw std::runtime_error("Reached end, Cannot read row");
    }

    std::array<std::byte, ROW_SIZE> serialized_row;

    // calculate offset
    size_t start_offset = row_num * ROW_SIZE + ROW_START_OFFSET;
    uint32_t first_page_id = start_offset / PAGE_SIZE;
    size_t byte_offset_on_page = start_offset % PAGE_SIZE;

    Pager::Page &first_page = table.pager.get_page(first_page_id);
    size_t first_chunk_size = std::min(ROW_SIZE, PAGE_SIZE - byte_offset_on_page);

    memcpy(serialized_row.data(), first_page.data().data() + byte_offset_on_page, first_chunk_size);

    if (first_chunk_size < ROW_SIZE)
    {
        Pager::Page &second_page = table.pager.get_page(first_page_id + 1); // get the next page
        memcpy(serialized_row.data() + first_chunk_size, second_page.data().data(), ROW_SIZE - first_chunk_size);
    }

    deserialize_row(serialized_row.data(), row);
}

// write the object at row_num
void Table::Cursor::write_row(const Row &row)
{
    // serialize the row
    std::array<std::byte, ROW_SIZE> serialized_row;
    serialize_row(row, serialized_row.data());

    // calculate offset
    size_t start_offset = row_num * ROW_SIZE + ROW_START_OFFSET;
    uint32_t first_page_id = start_offset / PAGE_SIZE;
    size_t byte_offset_on_page = start_offset % PAGE_SIZE;

    if (first_page_id >= table.pager.get_num_pages())
    {
        first_page_id = table.pager.allocate_page();
    }

    Pager::Page &first_page = table.pager.get_page(first_page_id); // get the page this should belong on

    size_t first_chunk_size = std::min(ROW_SIZE, PAGE_SIZE - byte_offset_on_page);

    memcpy(first_page.data().data() + byte_offset_on_page, serialized_row.data(), first_chunk_size);

    table.pager.mark_dirty(first_page_id);

    if (first_chunk_size < ROW_SIZE)
    {
        uint32_t second_page_id = first_page_id + 1;
        if (second_page_id >= table.pager.get_num_pages())
        {
            second_page_id = table.pager.allocate_page();
        }
        Pager::Page &second_page = table.pager.get_page(second_page_id); // get the next page

        memcpy(second_page.data().data(), serialized_row.data() + first_chunk_size, ROW_SIZE - first_chunk_size);

        table.pager.mark_dirty(second_page_id);
    }
}
