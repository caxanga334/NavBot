#ifndef __NAVBOT_CONCMD_ARGS_H_
#define __NAVBOT_CONCMD_ARGS_H_

class SVCommandAutoCompletion
{
public:

	void AddAutoCompletionEntry(const char* entry) { m_entries.emplace_back(entry); }

	const std::vector<std::string>& GetEntries() const { return m_entries; }

private:
	std::vector<std::string> m_entries;
};

class CConCommandArgs
{
public:
	CConCommandArgs();

	/**
	 * @brief Returns the argument count. The first argument is the command itself so the actual argument count is this minus one.
	 * @return Argument count.
	 */
	std::size_t Count() const { return m_args.size(); }
	// Compatibility: Same as Count()
	std::size_t ArgC() const { return m_args.size(); }
	/**
	 * @brief Retrieves an argument. Arguments indexes starts at 1 (index 0 is the command itself) and goes up to Count() - 1.
	 * @param i Argument index to get.
	 * @return Argument C string.
	 */
	const char* operator[](std::size_t i) const { return m_args[i].c_str(); }
	/**
	 * @brief Retrieves an argument. Arguments indexes starts at 1 (index 0 is the command itself) and goes up to Count() - 1.
	 * @param i Argument index to get.
	 * @return Argument C string.
	 */
	const char* Arg(std::size_t i) const { return m_args[i].c_str(); }
	/**
	 * @brief Retrieves an argument. Arguments indexes starts at 1 (index 0 is the command itself) and goes up to Count() - 1.
	 * @param i Argument index to get.
	 * @return Argument string.
	 */
	const std::string& GetArgString(std::size_t i) const { return m_args[i]; }
	/**
	 * @brief Checks if the given arg is present. (Case insensitive)
	 * @param arg Argument to search for.
	 * @return True if present, false if not.
	 */
	bool HasArg(const char* arg) const;
	/**
	 * @brief Gets the value of the given argument.
	 * @param arg Argument to get the value of.
	 * @param def Default value if the argument is found but a value is not present.
	 * @return C string of the argument value or NULL if not present/missing.
	 */
	const char* FindArg(const char* arg, const char* def = nullptr) const;
	/**
	 * @brief Finds and returns an int value of a command argument.
	 * @param arg Argument to search for.
	 * @param defaultValue Default value to return if the argument cannot be found.
	 * @return Int value.
	 */
	int FindArgInt(const char* arg, int defaultValue = 0) const;
	/**
	 * @brief Finds and returns a float value of a command argument.
	 * @param arg Argument to search for.
	 * @param defaultValue Default value to return if the argument cannot be found.
	 * @return Float value.
	 */
	float FindArgFloat(const char* arg, float defaultValue = 0.0f) const;
	/**
	 * @brief Finds and returns an unsigned int64 value of a command argument.
	 * @param arg Argument to search for.
	 * @param defaultValue Default value to return if the argument cannot be found.
	 * @return Unsigned int64 value.
	 */
	std::uint64_t FindArgUint64(const char* arg, std::uint64_t defaultValue = 0) const;
	/**
	 * @brief Given an argument, tries to parse a 3D Vector.
	 * @param arg Argument to search for.
	 * @param out Vector to store the result.
	 * @return True if a vector was parsed, false on failure.
	 */
	bool ParseArgVector(const char* arg, Vector& out) const;
	/**
	 * @brief Gets the vector that stores the command arguments.
	 * @return Vector of strings.
	 */
	const std::vector<std::string>& GetArgVector() const { return m_args; }
	/**
	 * @brief Checks if the current command is the given command name.
	 * @param szCmdName Command name to check.
	 * @return True if the current command is the given command, false otherwise.
	 */
	bool IsCommand(const char* szCmdName) const;

	void PushArg(const char* arg) { m_args.emplace_back(arg); }

private:
	std::vector<std::string> m_args;

};

#endif // !__NAVBOT_CONCMD_ARGS_H_

