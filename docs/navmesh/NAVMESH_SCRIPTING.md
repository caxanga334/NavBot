# Toggle Condition

Toggle conditions is a scripting features that allows certain nav mesh features to be togggled on/off when certain conditions are met.    
Toggle conditions are currently used by nav volumes and prerequisites.    

## Condition Data

Toggle conditions have four data types:    

* Integet
* Floating point
* Vector
* Entity

The data used depends on the condition type. See the documenation for each type below.    
If a data type isn't used, it's current value doesn't matter.    

# Toggle Condition Types

Below is a list of toggle conditions. You can always get an up to date list using the `sm_nav_scripting_list_conditions` console command.    
Most editing commands requires a condition ID number, use the list conditions command to get their IDs.    

## TYPE_NOT_SET

No condition type set. Always returns true.    

## TYPE_ENTITY_EXISTS

Checks if the linked entity exists on the world.    

## TYPE_ENTITY_ALIVE

Checks if the linked entity is alive.    

## TYPE_ENTITY_ENABLED

Checks if the linked entity is enabled. Not compatible with every entity.    
This reads the entity's `m_bDisabled` property.    

## TYPE_ENTITY_LOCKED

Checks if the linked entity is locked. Not compatible with every entity.
This reads the entity's `m_bLocked` property.

## TYPE_ENTITY_TEAM

Checks if the linked entity assigned team index is equal to the stored integer data.    

## TYPE_ENTITY_TOGGLE_STATE

Checks if the linked entity toggle state is equal to the stored integer data.    
Toggle state is used by doors to determine if they're open or closed.    
This reads the entity's `m_toggle_state` property.    

## TYPE_ENTITY_DISTANCE

Checks the distance between the linked entity world space center and the stored vector data is less than the stored float data.    

## TYPE_ENTITY_VISIBLE

Checks if the linked entity is visible or not by checking if the `EF_NODRAW` flag is set.    

## TYPE_TRACEHULL_SOLIDWORLD

Peforms a trace hull and check for colisions with the world.    
Vector data sets the hull origin, float data sets the hull size.    

## TYPE_BASETOGGLE_POSITION

For entities that derives from `CBaseToggle` class, calculates the entity position and tests it against the passed float data.    
Float data should be in a 0 to 1 range.    
