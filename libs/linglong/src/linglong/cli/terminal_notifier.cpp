// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "terminal_notifier.h"

#include <iostream>
#include <sstream>

namespace linglong::cli {

utils::error::Result<void>
TerminalNotifier::notify(const api::types::v1::InteractionRequest &request)
{
    auto &stdout = std::cout;
    stdout << request.summary << "\n";

    if (request.body) {
        stdout << "  " << request.body.value() << "\n";
    }

    stdout.flush();
    return LINGLONG_OK;
}

utils::error::Result<api::types::v1::InteractionReply>
TerminalNotifier::request(const api::types::v1::InteractionRequest &request)
{
    LINGLONG_TRACE("notify message through terminal")
    std::string msg;
    msg.append("\n");
    msg.append(request.summary + "\n");

    if (request.body) {
        msg.append(request.body.value() + "\n");
    }

    bool needReply{ false };
    if (request.actions) {
        if (request.actions->empty()) {
            return LINGLONG_ERR("empty action");
        }

        if (request.actions->size() % 2 != 0) {
            return LINGLONG_ERR("invalid action");
        }

        needReply = true;
        msg.append("available actions:\n");

        for (std::size_t index = 1; index < request.actions->size(); index += 2) {
            msg.append(request.actions.value()[index] + "\t");
        }

        msg.append("\nyour choice:");
    } else {
        msg.append("press enter to continue");
    }
    std::cout << msg << std::endl;

    std::string action;
    auto ret = api::types::v1::InteractionReply{};
    if (needReply) {
        // Consume the complete line so the next Enter prompt cannot reuse a
        // newline left behind by formatted extraction.
        do {
            std::string line;
            if (!std::getline(std::cin, line)) {
                return LINGLONG_ERR("failed to read action from terminal");
            }
            std::istringstream input(line);
            input >> action;
        } while (action.empty());
        ret.action = std::move(action);
    } else if (!std::getline(std::cin, action)) {
        return LINGLONG_ERR("failed to read Enter key from terminal");
    }

    return ret;
}

} // namespace linglong::cli
