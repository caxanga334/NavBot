#if defined _navbot_interfaces_dynamic_priority_included
 #endinput
#endif
#define _navbot_interfaces_dynamic_priority_included

#define MAX_DYNAMIC_PRIO_INSTANCE_ARGS 8 // maximum number of arguments that plugins can store on an DynamicWeaponInstance

methodmap DynamicWeaponInstance < Handle
{
	// Dynamic priority value set in the config file.
	property int Priority 
	{
		public native get();
	}
	/**
	 * Stores an interger value in the instance's internal memory.
	 * 
	 * @param value			Value to store.
	 * @param index			Index in the internal array. Range: 0 - (MAX_DYNAMIC_PRIO_INSTANCE_ARGS - 1)
	 * @error				Invalid handle.
	 */
	public native void StoreInt(int value, int index);
	/**
	 * Stores a float value in the instance's internal memory.
	 * 
	 * @param value			Value to store.
	 * @param index			Index in the internal array. Range: 0 - (MAX_DYNAMIC_PRIO_INSTANCE_ARGS - 1)
	 * @error				Invalid handle.
	 */
	public native void StoreFloat(int value, int index);
	/**
	 * Stores a string value in the instance's internal memory.
	 * 
	 * @param value			Value to store.
	 * @param index			Index in the internal array. Range: 0 - (MAX_DYNAMIC_PRIO_INSTANCE_ARGS - 1)
	 * @error				Invalid handle.
	 */
	public native void StoreString(const char[] value, int index);
	/**
	 * Reads an integer value from the instance internal memory.
	 * 
	 * @param index			Index in the internal array. Range: 0 - (MAX_DYNAMIC_PRIO_INSTANCE_ARGS - 1)
	 * @error				Bad type cast. Invalid Handle.
	 */
	public native int ReadInt(int index);
	/**
	 * Reads a float value from the instance internal memory.
	 * 
	 * @param index			Index in the internal array. Range: 0 - (MAX_DYNAMIC_PRIO_INSTANCE_ARGS - 1)
	 * @error				Bad type cast. Invalid Handle.
	 */
	public native int ReadFloat(int index);
	/**
	 * Reads a string value from the instance internal memory.
	 * 
	 * @param str			Char array to store the string.
	 * @param size			Size of the 'str' array.
	 * @param index			Index in the internal array. Range: 0 - (MAX_DYNAMIC_PRIO_INSTANCE_ARGS - 1)
	 * @error				Bad type cast. Invalid Handle.
	 */
	public native int ReadString(char[] str, int size, int index);
}

/**
 * Callback for Configure.
 * 
 * @param instance	Dynamic priority instance handle. This handle is managed by the extension.
 * @param arg       String argument from the config file.
 * @param index     Index of the argument.
 * @return          Return true to continue, false to stop and report an error in the configuration.
 */
typedef DynPrioConfigureCB = function bool (DynamicWeaponInstance instance, const char[] arg, int index);
/**
 * Callback for GetPriority.
 * 
 * @param instance	Dynamic priority instance handle. This handle is managed by the extension.
 * @param bot       Bot currently using the weapon.
 * @param weapon	Entity index of the weapon being used.
 * @param threat	Entity index of the current threat (enemy).
 * @return			Priority value to add to the weapon. 0 for no change.
 */
typedef DynPrioGetPriorityCB = function int (DynamicWeaponInstance instance, NavBot bot, int weapon, int threat);

methodmap DynamicWeaponPriority < Handle
{
	/**
	 * Creates and tries to register a new dynamic weapon priority handler.
	 * 
	 * @return Handle to the priority handler. NULL if registration failed.
	 * @error Duplicate handler.
	 */
	public native DynamicWeaponPriority(const char[] name, DynPrioConfigureCB configure_callback, DynPrioGetPriorityCB getpriority_callback);
}
