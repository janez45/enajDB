#pragma once
#include <iostream>
#include <unordered_map>

#include <fcntl.h>     // Required for open() and flags (like O_RDONLY, O_CREAT)
#include <sys/types.h> // Required for historical portability and type definitions
#include <sys/stat.h>  // Required if you use mode flags when creating files
#include <unistd.h>    // Required for lseek()
#include "constants.h"

// TODO: Move page size here

class Table;

// Page ids are 0 indexed
using PageId = uint32_t;

// pager accesses the page cache and the file
class Pager
{
public:
    // building block for the page cache
    class Page
    {
        bool is_dirty_;
        std::array<std::byte, PAGE_SIZE> data_;
        friend class Pager;

    public:
        Page();
        bool is_dirty() const;
        std::array<std::byte, PAGE_SIZE> &data();
    };

    static Pager pager_open(const std::string &filename); // only way to get a pager
    ~Pager();

    Pager(const Pager &) = delete;            // you cannot copy
    Pager &operator=(const Pager &) = delete; // you cannot copy construct

    Page &get_page(PageId page_id);
    PageId allocate_page();
    uint32_t get_num_pages() const;

    void mark_dirty(PageId page_id); // this is modified or outdated
    void flush(PageId page_id);      // Given the info above... yeah
    void pager_close();

private:
    int fd;
    uint32_t num_pages; // INVARIANT: file_length % PAGE_SIZE = 0
    std::unordered_map<PageId, Page> page_cache;
    Pager(int fd, off_t file_length);
};
