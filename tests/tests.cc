#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <cstdio>
#include <stdexcept>

enum EXIT_CODE
{
    SUCCESS,
    FAIL
};

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

        execl("./enajDB", "./enajDB", nullptr); // change this to include inputs

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