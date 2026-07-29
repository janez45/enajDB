#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <cstdio>
#include <stdexcept>
#include <format>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

struct ProcessResult
{
    int exitCode;
    std::string stdout;
};

ProcessResult run_script(const std::vector<std::string> &inputs)
{
    int stdinPipe[2];  // parent writes, child reads
    int stdoutPipe[2]; // child writes, parent out

    if (pipe(stdinPipe) == -1 || pipe(stdoutPipe) == -1)
    {
        throw std::runtime_error("pipe() failed");
    }

    pid_t pid = fork();

    if (pid == -1)
    {
        throw std::runtime_error("fork() failed");
    }

    if (pid == 0)
    {
        // you are the child
        dup2(stdinPipe[0], STDIN_FILENO);
        dup2(stdoutPipe[1], STDOUT_FILENO);

        close(stdinPipe[0]);
        close(stdinPipe[1]);
        close(stdoutPipe[0]);
        close(stdoutPipe[1]);

        execl("./build/enajDB", "./build/enajDB", nullptr); // change this to include inputs

        // this is only reached if exec fails
        exit(EXIT_FAILURE);
    }
    close(stdinPipe[0]);
    close(stdoutPipe[1]);

    // fire in the inputs
    std::vector<std::string> outputs;
    for (const std::string &input : inputs)
    {
        // OPTIMIZATION: Beware partial writes
        write(stdinPipe[1], input.data(), input.size());
        write(stdinPipe[1], "\n", 1);
    }

    // close to signal EOF
    close(stdinPipe[1]);

    std::string rawOutput;
    char buffer[4096];

    ssize_t bytesRead;
    while ((bytesRead = read(stdoutPipe[0], buffer, sizeof(buffer))) > 0)
    {
        rawOutput.append(buffer, bytesRead);
    }

    close(stdoutPipe[0]);

    int status;
    waitpid(pid, &status, 0); // wait for the child to exit

    int exitCode;
    // finished normally
    if (WIFEXITED(status))
    {
        exitCode = WEXITSTATUS(status);
    }
    else
    {
        // Process didn't exit normally.
        exitCode = EXIT_FAILURE;
    }
    return {exitCode, rawOutput};
}

TEST(EnajDB, InsertAndRetrievesRow)
{
    auto result = run_script({"insert 1 user1 person1@example.com",
                              "select",
                              ".exit"});
    EXPECT_EQ(
        result.stdout,
        "db > Executed.\n"
        "db > (1, user1, person1@example.com)\n"
        "Executed.\n"
        "db > ");
}

TEST(EnajDB, ErrorMessageTableFull)
{
    std::vector<std::string> commands;

    // 100 pages per table, 14 rows per page
    for (int i = 1; i <= 1401; i++)
    {
        std::string command = std::format("insert {} user{} person{}@example.com", i, i, i);
        commands.push_back(command);
    }
    commands.push_back(".exit");
    auto result = run_script(commands);

    std::vector<std::string> lines;
    std::stringstream ss(result.stdout);
    std::string line;

    while (std::getline(ss, line))
    {
        lines.push_back(line);
    }

    ASSERT_GE(lines.size(), 2);

    EXPECT_EQ(lines[lines.size() - 2],
              "db > Error: Table full.");
}

TEST(EnajDB, AllowMaxLengthStrings)
{
    std::string long_username(32, 'a');
    std::string long_email(255, 'a');
    std::vector<std::string> commands = {
        std::format("insert 1 {} {}", long_username, long_email),
        "select",
        ".exit"};
    auto result = run_script(commands);
    EXPECT_EQ(
        result.stdout,
        std::format("db > Executed.\n"
                    "db > (1, {}, {})\n"
                    "Executed.\n"
                    "db > ",
                    long_username, long_email));
}

TEST(EnajDB, ErrorUsernameTooLong)
{
    std::string long_username(33, 'a');
    std::string long_email(255, 'a');
    std::vector<std::string> commands = {
        std::format("insert 1 {} {}", long_username, long_email),
        "select",
        ".exit"};
    auto result = run_script(commands);
    EXPECT_EQ(
        result.stdout,
        "db > String is too long.\n"
        "db > Executed.\n"
        "db > ");
}

TEST(EnajDB, NoNegativeIds)
{
    std::vector<std::string> commands = {
        "insert -1 user1 user1@example.com",
        "select",
        ".exit"};
    auto result = run_script(commands);
    EXPECT_EQ(
        result.stdout,
        "db > ID must be positive.\n"
        "db > Executed.\n"
        "db > ");
}

TEST(EnajDB, KeepsDataAfterClosingConnection)
{
    std::vector<std::string> commands1 = {
        "insert 1 user1 user1@example.com",
        ".exit"};

    auto result = run_script(commands1);
    EXPECT_EQ(
        result.stdout,
        "db > Executed.\n"
        "db > ");

    std::vector<std::string> commands2 = {
        "select",
        ".exit"};

    result = run_script(commands2);
    EXPECT_EQ(
        result.stdout,
        "db > (1, user1, person1@example.com)\n"
        "Executed.\n"
        "db > ");
}
