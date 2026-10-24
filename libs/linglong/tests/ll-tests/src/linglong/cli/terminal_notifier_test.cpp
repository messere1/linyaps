// SPDX-FileCopyrightText: 2026 messere1
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <gtest/gtest.h>

#include "linglong/cli/terminal_notifier.h"

#include <iostream>
#include <sstream>

namespace {
class TerminalInput
{
public:
    explicit TerminalInput(const std::string &text)
        : input(text)
        , output()
        , savedIn(std::cin.rdbuf(input.rdbuf()))
        , savedOut(std::cout.rdbuf(output.rdbuf()))
    {
        std::cin.clear();
    }

    ~TerminalInput()
    {
        std::cin.rdbuf(savedIn);
        std::cout.rdbuf(savedOut);
        std::cin.clear();
    }

private:
    std::istringstream input;
    std::ostringstream output;
    std::streambuf *savedIn;
    std::streambuf *savedOut;
};
} // namespace

TEST(TerminalNotifierTest, EnterConsumesOneLineWithoutSwallowingFollowingInput)
{
    TerminalInput input("\nnext-command\n");
    linglong::cli::TerminalNotifier notifier;
    linglong::api::types::v1::InteractionRequest request;
    request.summary = "Continue";
    auto reply = notifier.request(request);
    ASSERT_TRUE(reply);
    EXPECT_FALSE(reply->action);
    std::string remaining;
    ASSERT_TRUE(static_cast<bool>(std::getline(std::cin, remaining)));
    EXPECT_EQ(remaining, "next-command");
}

TEST(TerminalNotifierTest, ActionDoesNotLeaveANewlineForTheNextPrompt)
{
    TerminalInput input("\n  Y  \n\nnext-command\n");
    linglong::cli::TerminalNotifier notifier;
    linglong::api::types::v1::InteractionRequest request;
    request.summary = "Confirm";
    request.actions = std::vector<std::string>{ "Y", "Yes", "N", "No" };
    auto choice = notifier.request(request);
    ASSERT_TRUE(choice);
    ASSERT_TRUE(choice->action);
    EXPECT_EQ(*choice->action, "Y");
    request.actions.reset();
    ASSERT_TRUE(notifier.request(request));
    std::string remaining;
    ASSERT_TRUE(static_cast<bool>(std::getline(std::cin, remaining)));
    EXPECT_EQ(remaining, "next-command");
}

TEST(TerminalNotifierTest, EndOfInputIsAnError)
{
    TerminalInput input("");
    linglong::cli::TerminalNotifier notifier;
    linglong::api::types::v1::InteractionRequest request;
    request.summary = "Continue";
    EXPECT_FALSE(notifier.request(request));
}
