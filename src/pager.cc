#include <iostream>
#include <vector>

#include <fcntl.h>     // Required for open() and flags (like O_RDONLY, O_CREAT)
#include <sys/types.h> // Required for historical portability and type definitions
#include <sys/stat.h>  // Required if you use mode flags when creating files
#include <unistd.h>    // Required for lseek()

#include "constants.h"
#include "pager.h"

Pager::Pager(int fd, off_t file_length) : fd{fd}, num_pages{static_cast<uint32_t>(file_length / PAGE_SIZE)} {}

Pager::~Pager() {}

Pager::Page::Page() : is_dirty_{false} {}

bool Pager::Page::is_dirty() const
{
    return is_dirty_;
}

std::array<std::byte, PAGE_SIZE> &Pager::Page::data()
{
    return data_;
}

uint32_t Pager::get_num_pages() const
{
    return num_pages;
}

// static method that uses the constructor
Pager Pager::pager_open(const std::string &filename)
{
    // open the file in read/write mode. Create if not existing
    // Add read / write permissions for the user if creating this file
    int fd = open(filename.c_str(), O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);

    if (fd == -1)
    {
        throw std::system_error(
            errno,
            std::generic_category(),
            "Unable to open file");
    }

    // position right at the end. You add the zero from there
    // opposite of SEEK_END is SEEK_SET
    // in bytes
    // INVARIANT: file_length % PAGE_SIZE is always 0
    off_t file_length = lseek(fd, 0, SEEK_END);

    return Pager{fd, file_length};
}

PageId Pager::allocate_page()
{
    // allocate a new page in the actual file
    PageId new_page_id = num_pages;

    if (ftruncate(fd, (num_pages + 1) * PAGE_SIZE) == -1)
    {
        throw std::system_error(
            errno,
            std::generic_category(),
            "Failed to allocate page");
    }

    ++num_pages;

    return new_page_id;
}

void Pager::mark_dirty(PageId page_id)
{
    // throw error if this page does not exist in the cache.
    // because in that case it has no reason to be dirty
    if (!page_cache.contains(page_id))
    {
        throw std::logic_error("Cannot mark unaccessed page as dirty");
    }

    page_cache[page_id].is_dirty_ = true;
}

Pager::Page &Pager::get_page(PageId page_id)
{
    // Give me an existing page from the system. If it does not exist, throw an error
    if (page_id >= num_pages)
    {
        throw std::out_of_range("Page ID does not exist");
    }

    if (page_cache.contains(page_id))
    {
        return page_cache[page_id];
    }

    Page page{};
    page.is_dirty_ = false;

    off_t offset = static_cast<off_t>(page_id) * PAGE_SIZE;
    ssize_t bytes_read = pread(fd, page.data().data(), PAGE_SIZE, offset);

    if (bytes_read == -1)
    {
        throw std::system_error(
            errno,
            std::generic_category(),
            "Failed to read from database file");
    }

    page_cache[page_id] = page;
    return page_cache[page_id];
}

// Size is needed in case we need to flush a partial page
void Pager::flush(PageId page_id)
{
    if (!page_cache.contains(page_id) || !page_cache[page_id].is_dirty_)
    {
        return; // do nothing if the page is unaccessed or not dirty
    }

    off_t offset = static_cast<off_t>(page_id) * PAGE_SIZE;
    ssize_t bytes_written = pwrite(fd, page_cache[page_id].data().data(), PAGE_SIZE, offset);

    if (bytes_written == -1)
    {
        throw std::system_error(
            errno,
            std::generic_category(),
            "Error writing to database file");
    }

    if (bytes_written != PAGE_SIZE)
    {
        throw std::runtime_error("Partial page write");
    }

    page_cache[page_id].is_dirty_ = false;
}

void Pager::pager_close()
{
    for (auto &[page_id, page] : page_cache)
    {
        if (page.is_dirty_)
        {
            flush(page_id);
        }
    }

    if (::close(fd) == -1)
    {
        throw std::system_error(
            errno,
            std::generic_category(),
            "Failed to close database file");
    }
}
