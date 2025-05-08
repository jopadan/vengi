/**
 * @file
 */

#include "app/App.h"
#include "core/TimeProvider.h"
#include "core/ArrayLength.h"
#include "io/Filesystem.h"
#include <gtest/gtest.h>
#include "core/tests/TestHelper.h"

namespace app {

class TestApp : public App {
public:
	TestApp(int argc = 0, const char *args[] = nullptr)
		: App(core::make_shared<io::Filesystem>(),
			  core::make_shared<core::TimeProvider>()) {
		setArgs(argc, (char **)args);
	}
	bool createPid() override {
		return false;
	}
};

TEST(AppTest, testLifecycleManual) {
	TestApp app;
	ASSERT_EQ(app::AppState::Init, app.onConstruct());
	ASSERT_EQ(app::AppState::Running, app.onInit());
	ASSERT_EQ(app::AppState::Cleanup, app.onRunning());
	ASSERT_EQ(app::AppState::Destroy, app.onCleanup());
}

TEST(AppTest, testLifecycleOnFrame) {
	TestApp app;
	app.onFrame();
	ASSERT_EQ(app::AppState::Construct, app.state());
	app.onFrame();
	ASSERT_EQ(app::AppState::Init, app.state());
	app.onFrame();
	ASSERT_EQ(app::AppState::Running, app.state());
	app.onFrame();
	ASSERT_EQ(app::AppState::Cleanup, app.state());
	app.onFrame();
	ASSERT_EQ(app::AppState::InvalidAppState, app.state());
}

TEST(AppTest, testArguments) {
	const char *args[] = {"testbinary", "-t", "value"};
	TestApp app(lengthof(args), args);
	app.registerArg("--test").setDescription("test").setShort("-t");
	EXPECT_TRUE(app.hasArg("--test"));
	EXPECT_TRUE(app.hasArg("-t"));
	EXPECT_FALSE(app.hasArg("-te"));
	EXPECT_EQ("value", app.getArgVal("--test"));
	EXPECT_EQ("value", app.getArgVal("-t"));
	EXPECT_EQ("", app.getArgVal("-te"));
}

TEST(AppTest, testArgumentString) {
	const char *args[] = {"testbinary", "-t", "\"value1 value2\""};
	TestApp app(lengthof(args), args);
	app.registerArg("--test").setDescription("test").setShort("-t");
	EXPECT_TRUE(app.hasArg("--test"));
	EXPECT_TRUE(app.hasArg("-t"));
	EXPECT_FALSE(app.hasArg("-te"));
	EXPECT_EQ("value1 value2", app.getArgVal("--test"));
	EXPECT_EQ("value1 value2", app.getArgVal("-t"));
	EXPECT_EQ("", app.getArgVal("-te"));
}

TEST(AppTest, testMultiArguments) {
	const char *args[] = {"testbinary", "-t", "value", "-t", "value2"};
	TestApp app(lengthof(args), args);
	app.registerArg("--test").setDescription("test").setShort("-t");
	EXPECT_TRUE(app.hasArg("--test"));
	EXPECT_TRUE(app.hasArg("-t"));
	EXPECT_FALSE(app.hasArg("-te"));
	int arg = 0;
	EXPECT_EQ("value", app.getArgVal("--test", "", &arg));
	EXPECT_EQ("value2", app.getArgVal("-t", "", &arg));
	EXPECT_EQ("", app.getArgVal("-te"));
}

TEST(AppTest, testArgumentDefaultValue) {
	const char *args[] = {"testbinary"};
	TestApp app(lengthof(args), args);
	app.registerArg("--test").setDefaultValue("defaultval");
	EXPECT_EQ("defaultval", app.getArgVal("--test"));
}

} // namespace app
