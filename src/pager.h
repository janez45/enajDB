#pragma once
#include <iostream>
#include <vector>

#include <fcntl.h>     // Required for open() and flags (like O_RDONLY, O_CREAT)
#include <sys/types.h> // Required for historical portability and type definitions
#include <sys/stat.h>  // Required if you use mode flags when creating files
#include <unistd.h>    // Required for lseek()

#include "constants.h"

class Table;

// pager accesses the page cache and the file
class Pager
{
    friend class Table;
    int fd;
    off_t file_length;
    std::vector<std::byte *> pages;

public:
    Pager(int fd, off_t file_length);
    ~Pager();
    std::byte *get_page(uint32_t page_num);
    void flush(uint32_t page_num, uint32_t size);
    int close_file();
};

Pager pager_open(const std::string &filename);