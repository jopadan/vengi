/**
 * @file
 */

#pragma once

#include <benchmark/benchmark.h>
#include "app/CommandlineApp.h"
#include "io/Filesystem.h"
#include "core/TimeProvider.h"

namespace app {

class AbstractBenchmark : public benchmark::Fixture {
private:
	int _threadPoolSize;
	class BenchmarkApp: public app::CommandlineApp {
		friend class AbstractBenchmark;
	protected:
		using Super = app::CommandlineApp;
		AbstractBenchmark* _benchmark = nullptr;
	public:
		BenchmarkApp(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider,
					 AbstractBenchmark *benchmark, size_t threadPoolSize);
		virtual ~BenchmarkApp();

		virtual app::AppState onInit() override;
		virtual app::AppState onCleanup() override;
	};

protected:
	BenchmarkApp *_benchmarkApp = nullptr;

	virtual void onCleanupApp() {
	}

	virtual bool onInitApp() {
		return true;
	}

public:
	AbstractBenchmark(size_t threadPoolSize = 1) : _threadPoolSize(threadPoolSize) {
	}
	virtual void SetUp(benchmark::State& st) override;

	virtual void TearDown(benchmark::State& st) override;
};

}
