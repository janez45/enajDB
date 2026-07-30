#include <iostream>
#include <vector>

#include <fcntl.h>     // Required for open() and flags (like O_RDONLY, O_CREAT)
#include <sys/types.h> // Required for historical portability and type definitions
#include <sys/stat.h>  // Required if you use mode flags when creating files
#include <unistd.h>    // Required for lseek()

#include "constants.h"
#include "pager.h"

Pager::Pager(int fd, off_t file_length) : fd{fd}, file_length{file_length}, pages{std::vector<std::byte *>(TABLE_MAX_PAGES, nullptr)} {}

Pager::~Pager()
{
    for (std::byte *page : pages)
    {
        delete[] page;
    }
}

std::byte *Pager::get_page(uint32_t page_num)
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
void Pager::flush(uint32_t page_num, uint32_t size)
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

int Pager::close_file()
{
    return close(fd);
}

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