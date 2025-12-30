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

class SVCommandArgs
{
public:

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

	void PushArg(const char* arg) { m_args.emplace_back(arg); }

private:
	std::vector<std::string> m_args;

};

class CLCommandArgs
{
public:


};

#endif // !__NAVBOT_CONCMD_ARGS_H_

