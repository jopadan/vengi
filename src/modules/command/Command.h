/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/Common.h"
#include "core/collection/StringMap.h"
#include "core/collection/DynamicArray.h"
#include "core/concurrent/Lock.h"
#include "command/ActionButton.h"
#include <functional>

namespace command {

typedef core::DynamicArray<core::String> CmdArgs;
#define COMMAND_PRESSED "+"
#define COMMAND_RELEASED "-"

struct ActionButtonCommands {
	const core::String first;
	const core::String second;

	inline ActionButtonCommands(core::String&& _first, core::String&& _second) :
			first(_first), second(_second) {
	}

	ActionButtonCommands& setHelp(const char* help);
};

#define MAX_COMMANDS 4096

/**
 * @brief A command is a string bound c++ function/method/lambda. You can bind
 * this to keys or execute on the console.
 */
class Command {
private:
	typedef core::StringMap<Command> CommandMap;
	typedef std::function<void(const CmdArgs&)> FunctionType;

	static CommandMap _cmds core_thread_guarded_by(_lock);
	static core::Lock _lock;
	static size_t _sortedCommandListSize;
	static Command* _sortedCommandList[MAX_COMMANDS];

	static double _delaySeconds;
	static core::DynamicArray<core::String> _delayedTokens;

	core::String _name;
	core::String _help;
	FunctionType _func;
	typedef std::function<int(const core::String&, core::DynamicArray<core::String>& matches)> CompleteFunctionType;
	mutable CompleteFunctionType _completer;

	Command() :
		_func() {
	}

	Command(const core::String& name, FunctionType&& func) :
		_name(name), _func(core::move(func)) {
	}

	static void updateSortedList();

public:
	static Command& registerCommand(const core::String &name, std::function<void(void)>& func) {
		return registerCommand(name, FunctionType([&] (const CmdArgs&) {func();}));
	}
	static Command& registerCommand(const core::String &name, FunctionType&& func);
	static bool unregisterCommand(const core::String &name);

	/**
	 * @brief Registers two commands prefixed with @c + and @c - (for pressed and released)
	 * for commands that are bound to keys.
	 * @param[in] name The name of the command. It will automatically be prefixed with
	 * a @c + and @c -
	 * @param[in,out] button The @c ActionButton instance.
	 * @note This class is not taking ownership of the button instance. You have to ensure
	 * that the instance given here is alive as long as the commands are bound.
	 */
	static ActionButtonCommands registerActionButton(const core::String& name, ActionButton& button, const core::String &help = "");
	static bool unregisterActionButton(const core::String& name);

	static void shutdown();

	/**
	 * @brief Executes delayed (by wait command e.g.) commands that are still in the command buffer
	 */
	static int update(double deltaFrameSeconds);

	static int execute(const core::String& command);

	static int execute(CORE_FORMAT_STRING const char* msg, ...) CORE_PRINTF_VARARG_FUNC(1);

	static bool execute(const core::String& command, const CmdArgs& args);

	static Command* getCommand(const core::String& name) {
		core::ScopedLock lock(_lock);
		auto i = _cmds.find(name);
		if (i == _cmds.end()) {
			return nullptr;
		}
		return (Command*)&i->value;
	}

	template<class Functor>
	static void visit(Functor&& func) {
		core::ScopedLock lock(_lock);
		for (auto i = _cmds.begin(); i != _cmds.end(); ++i) {
			func(i->value);
		}
	}

	template<class Functor>
	static void visitSorted(Functor&& func) {
		updateSortedList();
		core::ScopedLock lock(_lock);
		for (size_t i = 0; i < _sortedCommandListSize; ++i) {
			func(*_sortedCommandList[i]);
		}
	}

	int complete(const core::String& str, core::DynamicArray<core::String>& matches) const;

	/**
	 * @param func A functor or lambda that accepts the following parameters: @code const core::String& str, core::DynamicArray<core::String>& matches @endcode
	 */
	template<class Functor>
	Command& setArgumentCompleter(Functor&& func) {
		_completer = func;
		return *this;
	}

	Command& setBoolCompleter();

	const core::String &name() const;

	Command& setHelp(const core::String &help);
	const core::String &help() const;

	bool operator==(const Command& rhs) const;
};

inline bool Command::operator==(const Command& rhs) const {
	return rhs._name == _name;
}

inline const core::String &Command::name() const {
	return _name;
}

inline const core::String &Command::help() const {
	return _help;
}

inline core::String help(const core::String &cmd) {
	Command* command = Command::getCommand(cmd);
	if (!command) {
		return core::String::Empty;
	}
	return command->help();
}

}
