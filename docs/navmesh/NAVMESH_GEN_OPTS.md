# Nav Mesh Generation Options

Options/settings that controls nav mesh generation.

## ConVars

| Name | Description | Default Value |
|:---:|:---:|:---:|
| sm_nav_area_max_size | Maximum area size when generating nav areas. NavBot uses 10 as default while Valve uses 50. | 10 |
| sm_nav_generate_incremental_range | Maximum distance from walkable seeds to generate nodes when doing incremental generation. | 2000 |
| sm_nav_solid_props | If enabled, `prop_*` entities are solid to the nav mesh. | 0 |
| sm_nav_solid_func_brush | If enabled, `func_brush` entities are always solid to the nav mesh. | 1 |

## Console Commands

List of console commands that changes nav mesh generation behavior.    

### Changing Generation Trace Mask

The `sm_nav_change_generation_trace_mask` command cycles the trace mask used by the nav mesh.    
The default is `PLAYERSOLID_BRUSHONLY`.    
Changing it to `PLAYERSOLID` is required for some entities to be solid to the nav mesh.    
For example: `prop_*` entities won't be solid when using `PLAYERSOLID_BRUSHONLY` mask.    

### Forcing Entities To Be Solid

The `sm_nav_trace_make_entity_solid` command adds an entity to the forced solid list.    
It can be used without any arguments to add the entity you're aiming at or you can pass an entity index as the first argument to add a specific entity.    

### Clearing Forced Solid List

The `sm_nav_trace_clear_solid_entity_list` clears the forced solid list.    
