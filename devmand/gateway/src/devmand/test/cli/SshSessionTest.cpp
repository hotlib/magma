// Copyright (c) 2019-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#define LOG_WITH_GLOG
#include <magma_logging.h>

#include <devmand/channels/cli/SshSessionAsync.h>
#include <devmand/channels/cli/SshSocketReader.h>
#include <devmand/test/cli/utils/Log.h>
#include <devmand/test/cli/utils/Ssh.h>
#include <folly/Singleton.h>
#include <folly/executors/IOThreadPoolExecutor.h>
#include <folly/futures/Future.h>
#include <gtest/gtest.h>
#include <chrono>
#include <ctime>
#include <thread>

namespace devmand {
namespace test {
namespace cli {

using namespace devmand::channels::cli;
using namespace devmand::test::utils::ssh;
using namespace std;
using namespace folly;
using devmand::channels::cli::sshsession::readCallback;
using devmand::channels::cli::sshsession::SshSession;
using devmand::channels::cli::sshsession::SshSessionAsync;
using folly::IOThreadPoolExecutor;

static const shared_ptr<IOThreadPoolExecutor> executor =
    std::make_shared<IOThreadPoolExecutor>(2);
static const shared_ptr<IOThreadPoolExecutor> testExecutor =
    std::make_shared<IOThreadPoolExecutor>(2);

class SshSessionTest : public ::testing::Test {
 protected:
  shared_ptr<server> ssh;

  void SetUp() override {
    devmand::test::utils::log::initLog();
    devmand::test::utils::ssh::initSsh();
    ssh = startSshServer(testExecutor);
  }

  void TearDown() override {
    ssh->close();
  }
};

TEST_F(SshSessionTest, sessionStopReading) {
  atomic_bool exceptionCaught(false);
  {
    const std::shared_ptr<SshSessionAsync>& session =
        std::make_shared<SshSessionAsync>(executor);

    session->openShell("127.0.0.1", 9999, "cisco", "cisco").get();

    event* sessionEvent = SshSocketReader::getInstance().addSshReader(
        readCallback, session->getSshFd(), session.get());
    session->setEvent(sessionEvent);

    MLOG(MDEBUG) << "Starting infinite read";
    session->readUntilOutput("never").thenError(
        tag_t<std::runtime_error>{},
        [&exceptionCaught](runtime_error const& e) {
          MLOG(MDEBUG) << "Read completed with error: " << e.what();
          exceptionCaught.store(true);
          return Future<string>(e);
        });
    this_thread::sleep_for(chrono::seconds(5));
  }

  // Assert the future completed exceptionally
  for (int i = 0; i < 10; i++) {
    if (exceptionCaught.load()) {
      return;
    } else {
      this_thread::sleep_for(chrono::seconds(1));
    }
  }
  FAIL();
}

TEST_F(SshSessionTest, sessionReadingStopServer) {
  atomic_bool exceptionCaught(false);
  const std::shared_ptr<SshSessionAsync>& session =
      std::make_shared<SshSessionAsync>(executor);

  session->openShell("127.0.0.1", 9999, "cisco", "cisco").get();

  event* sessionEvent = SshSocketReader::getInstance().addSshReader(
      readCallback, session->getSshFd(), session.get());
  session->setEvent(sessionEvent);

  MLOG(MDEBUG) << "Starting infinite read";
  session->readUntilOutput("never").thenError(
      tag_t<std::runtime_error>{}, [&exceptionCaught](runtime_error const& e) {
        MLOG(MDEBUG) << "Read completed with error: " << e.what();
        exceptionCaught.store(true);
        return Future<string>(e);
      });
  this_thread::sleep_for(chrono::seconds(5));

  MLOG(MDEBUG) << "Sending ctrl C";
  string ctrlC;
  ctrlC = (char)3;
  session->write(ctrlC).get();

  // Assert the future completed exceptionally
  for (int i = 0; i < 10; i++) {
    if (exceptionCaught.load()) {
      return;
    } else {
      this_thread::sleep_for(chrono::seconds(1));
    }
  }
  FAIL();
}

TEST_F(SshSessionTest, sessionStop) {
  const std::shared_ptr<SshSessionAsync>& session =
      std::make_shared<SshSessionAsync>(executor);

  session->openShell("127.0.0.1", 9999, "cisco", "cisco").get();

  event* sessionEvent = SshSocketReader::getInstance().addSshReader(
      readCallback, session->getSshFd(), session.get());
  session->setEvent(sessionEvent);
}

} // namespace cli
} // namespace test
} // namespace devmand