/*
 * SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include <gtest/gtest.h>

#include "linglong/repo/client_factory.h"

#include <QCoreApplication>
#include <QString>
#include <QTimer>

using namespace linglong::repo;

class ClientFactoryTest : public ::testing::Test
{
protected:
    void SetUp() override { }

    void TearDown() override { }
};

TEST_F(ClientFactoryTest, CreateWithStdString)
{
    ClientFactory factory(std::string("http://localhost:8081"));
    auto client = factory.createClientV2();
    EXPECT_NE(client, nullptr);
}

TEST_F(ClientFactoryTest, ImmediateRequestsCannotLoseCompletionBeforeEventLoopStarts)
{
    int argc = 1;
    char name[] = "client-factory-test";
    char *argv[] = { name, nullptr };
    std::unique_ptr<QCoreApplication> application;
    if (!QCoreApplication::instance()) {
        application = std::make_unique<QCoreApplication>(argc, argv);
    }
    ClientFactory factory("http://unused.invalid");
    auto client = factory.createClientV2();
    // Immediate failure/cached responses can finish during thread startup.
    // The watchdog also makes a lost wakeup fail instead of hanging ll-tests.
    for (int i = 0; i < 2000; ++i) {
        bool timedOut = false;
        QTimer watchdog;
        watchdog.setSingleShot(true);
        QObject::connect(&watchdog, &QTimer::timeout, [&] {
            timedOut = true;
            QCoreApplication::exit(1);
        });
        watchdog.start(1000);
        auto result = client->syncRun<int>(
          [](apiClient_t *) {
              return new int(42);
          },
          [](int *value) {
              delete value;
          });
        ASSERT_FALSE(timedOut) << "lost completion on iteration " << i;
        ASSERT_TRUE(result);
        EXPECT_EQ(*result, 42);
    }
}
