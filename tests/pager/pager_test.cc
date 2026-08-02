#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include "pager.h"

static constexpr std::string testDbName = "pagertest.db";

class PagerTest : public ::testing::Test
{
protected:
    void TearDown() override
    {
        std::filesystem::remove(testDbName);
    }
};

void append_page(std::ofstream &file, char value)
{
    for (int i = 0; i < PAGE_SIZE; i++)
    {
        file.put(value);
    }
}

TEST_F(PagerTest, CorrectInitializationWithNonexistingFile)
{
    Pager pager = Pager::pager_open(testDbName);
    EXPECT_EQ(pager.get_num_pages(), 0);
    ASSERT_TRUE(std::filesystem::exists(testDbName));
}

TEST_F(PagerTest, CorrectInitializationWithExistingEmptyFile)
{
    std::ofstream(testDbName).close();
    ASSERT_TRUE(std::filesystem::exists(testDbName));
    Pager pager = Pager::pager_open(testDbName);
    EXPECT_EQ(pager.get_num_pages(), 0);
}

TEST_F(PagerTest, CorrectInitializationWithExistingNonEmptyFile)
{
    std::ofstream(testDbName).close();
    std::filesystem::resize_file(testDbName, PAGE_SIZE * 3);
    Pager pager = Pager::pager_open(testDbName);
    EXPECT_EQ(pager.get_num_pages(), 3);
}

TEST_F(PagerTest, AllocatePage)
{
    std::ofstream(testDbName).close();
    std::filesystem::resize_file(testDbName, PAGE_SIZE * 3);
    Pager pager = Pager::pager_open(testDbName);
    EXPECT_EQ(pager.get_num_pages(), 3);
    PageId pageId = pager.allocate_page();
    EXPECT_EQ(pageId, 3);
    EXPECT_EQ(pager.get_num_pages(), 4);
    EXPECT_EQ(std::filesystem::file_size(testDbName), 4 * PAGE_SIZE);
}

TEST_F(PagerTest, GetExistingPage)
{
    std::ofstream file(testDbName, std::ios::binary);
    append_page(file, 0xAA); // Page 0: all 0xAA
    append_page(file, 0xBB); // Page 1: all 0xBB
    append_page(file, 0xCC); // Page 2: all 0xCC
    file.close();

    Pager pager = Pager::pager_open(testDbName);
    EXPECT_EQ(pager.get_num_pages(), 3);

    Pager::Page &page = pager.get_page(0);

    EXPECT_TRUE(std::all_of(
        page.data().begin(),
        page.data().end(),
        [](std::byte byte)
        {
            return byte == std::byte{0xAA};
        }));

    page = pager.get_page(1);

    EXPECT_TRUE(std::all_of(
        page.data().begin(),
        page.data().end(),
        [](std::byte byte)
        {
            return byte == std::byte{0xBB};
        }));

    page = pager.get_page(2);

    EXPECT_TRUE(std::all_of(
        page.data().begin(),
        page.data().end(),
        [](std::byte byte)
        {
            return byte == std::byte{0xCC};
        }));
}

TEST_F(PagerTest, GetNonExistingPage)
{
    std::ofstream(testDbName).close();
    std::filesystem::resize_file(testDbName, PAGE_SIZE * 3);
    Pager pager = Pager::pager_open(testDbName);
    EXPECT_EQ(pager.get_num_pages(), 3);
    EXPECT_THROW(pager.get_page(3), std::out_of_range);
}

TEST_F(PagerTest, MarkNonexistentPageDirtyFail)
{
    std::ofstream file(testDbName, std::ios::binary);
    append_page(file, 0xAA); // Page 0: all 0xAA
    file.close();
    Pager pager = Pager::pager_open(testDbName);
    EXPECT_EQ(pager.get_num_pages(), 1);
    EXPECT_THROW(pager.mark_dirty(2), std::logic_error);
}

TEST_F(PagerTest, MarkUnaccessedDirtyFail)
{
    std::ofstream file(testDbName, std::ios::binary);
    append_page(file, 0xAA); // Page 0: all 0xAA
    file.close();
    Pager pager = Pager::pager_open(testDbName);
    EXPECT_EQ(pager.get_num_pages(), 1);
    EXPECT_THROW(pager.mark_dirty(0), std::logic_error);
}

TEST_F(PagerTest, MarkDirty)
{
    std::ofstream file(testDbName, std::ios::binary);
    append_page(file, 0xAA); // Page 0: all 0xAA
    append_page(file, 0xBB); // Page 1: all 0xBB
    append_page(file, 0xCC); // Page 2: all 0xCC
    append_page(file, 0xDD);
    append_page(file, 0xEE);
    file.close();

    Pager pager = Pager::pager_open(testDbName);
    EXPECT_EQ(pager.get_num_pages(), 5);

    std::vector<bool> pages_to_mark_dirty = {false, true, true, false, true};

    for (PageId pageId = 0; pageId < pager.get_num_pages(); pageId++)
    {
        if (pages_to_mark_dirty[pageId])
        {
            Pager::Page &page = pager.get_page(pageId); // get the page so it's in cache
            page.data()[0] = std::byte{0x00};           // modify this page
            pager.mark_dirty(pageId);
        }
    }

    for (PageId pageId = 0; pageId < pager.get_num_pages(); pageId++)
    {
        EXPECT_EQ(pager.get_page(pageId).is_dirty(), pages_to_mark_dirty[pageId]);
    }
}

TEST_F(PagerTest, FlushOnlyOnDirtyPages)
{
    std::ofstream file(testDbName, std::ios::binary);
    append_page(file, 0xAA); // Page 0: all 0xAA
    append_page(file, 0xBB); // Page 1: all 0xBB
    append_page(file, 0xCC); // Page 2: all 0xCC
    append_page(file, 0xDD);
    append_page(file, 0xEE);
    file.close();

    Pager pager = Pager::pager_open(testDbName);
    EXPECT_EQ(pager.get_num_pages(), 5);

    std::vector<bool> pages_to_mark_dirty = {true, false, true, false, false};

    for (PageId pageId = 0; pageId < pager.get_num_pages(); pageId++)
    {
        Pager::Page &page = pager.get_page(pageId); // modify every single page
        page.data()[0] = std::byte{0x00};
        if (pages_to_mark_dirty[pageId])
        {
            pager.mark_dirty(pageId);
        }
    }

    // flush
    pager.pager_close();

    std::array<std::byte, 5> original_values = {
        std::byte{0xAA},
        std::byte{0xBB},
        std::byte{0xCC},
        std::byte{0xDD},
        std::byte{0xEE}};

    // reopen
    Pager pager2 = Pager::pager_open(testDbName);
    EXPECT_EQ(pager2.get_num_pages(), 5);

    for (PageId pageId = 0; pageId < pager2.get_num_pages(); pageId++)
    {
        // all pages must not be dirty
        Pager::Page &page = pager2.get_page(pageId);
        EXPECT_FALSE(page.is_dirty());
        if (pages_to_mark_dirty[pageId])
        {
            EXPECT_EQ(page.data()[0], std::byte{0x00});
            EXPECT_TRUE(std::all_of(
                page.data().begin() + 1,
                page.data().end(),
                [original_values, pageId](std::byte byte)
                {
                    return byte == original_values[pageId];
                }));
        }
        else
        {
            EXPECT_TRUE(std::all_of(
                page.data().begin(),
                page.data().end(),
                [original_values, pageId](std::byte byte)
                {
                    return byte == original_values[pageId];
                }));
        }
    }
}

TEST_F(PagerTest, FlushDirtyPagesOnlyAndEnsurePersistence)
{
    std::ofstream file(testDbName, std::ios::binary);
    append_page(file, 0xAA); // Page 0: all 0xAA
    append_page(file, 0xBB); // Page 1: all 0xBB
    append_page(file, 0xCC); // Page 2: all 0xCC
    append_page(file, 0xDD);
    append_page(file, 0xEE);
    file.close();

    std::array<std::byte, 5> original_values = {
        std::byte{0xAA},
        std::byte{0xBB},
        std::byte{0xCC},
        std::byte{0xDD},
        std::byte{0xEE}};

    Pager pager = Pager::pager_open(testDbName);
    EXPECT_EQ(pager.get_num_pages(), 5);

    std::vector<bool> pages_to_mark_dirty = {false, true, true, false, true};

    for (PageId pageId = 0; pageId < pager.get_num_pages(); pageId++)
    {
        if (pages_to_mark_dirty[pageId])
        {
            Pager::Page &page = pager.get_page(pageId); // get the page so it's in cache
            page.data()[0] = std::byte{0x00};           // modify this page
            pager.mark_dirty(pageId);
        }
    }

    for (PageId pageId = 0; pageId < pager.get_num_pages(); pageId++)
    {
        Pager::Page &page = pager.get_page(pageId);
        EXPECT_EQ(page.is_dirty(), pages_to_mark_dirty[pageId]);
        if (pages_to_mark_dirty[pageId])
        {
            EXPECT_EQ(page.data()[0], std::byte{0x00});
        }
        else
        {
            EXPECT_EQ(page.data()[0], original_values[pageId]);
        }
    }

    // flush
    pager.pager_close();

    // reopen
    Pager pager2 = Pager::pager_open(testDbName);
    EXPECT_EQ(pager2.get_num_pages(), 5);

    for (PageId pageId = 0; pageId < pager2.get_num_pages(); pageId++)
    {
        // all pages must not be dirty
        Pager::Page &page = pager2.get_page(pageId);
        EXPECT_FALSE(page.is_dirty());
        if (pages_to_mark_dirty[pageId])
        {
            EXPECT_EQ(pager2.get_page(pageId).data()[0], std::byte{0x00});
            EXPECT_TRUE(std::all_of(
                page.data().begin() + 1,
                page.data().end(),
                [original_values, pageId](std::byte byte)
                {
                    return byte == original_values[pageId];
                }));
        }
        else
        {
            EXPECT_TRUE(std::all_of(
                page.data().begin(),
                page.data().end(),
                [original_values, pageId](std::byte byte)
                {
                    return byte == original_values[pageId];
                }));
        }
    }
}