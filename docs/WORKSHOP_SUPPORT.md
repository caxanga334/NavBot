# Steam Workshop Map Support

NavBot supports maps loaded from Steam workshop.    
In some games, NavBot will use a 'clean' name of the workshop map.    

## Naming Preference

The ConVar `sm_navbot_prefer_unique_map_names` can be used to set the preference between 'clean' and 'unique' map names.    
When enabled, 'unique' map names are used when dealing with map specific files.    
It's disabled by default.    

### Unique Vs Clean

Unique map names are a clean version of a workshop map that includes a unique identifier, generally the workshop ID.    
Clean map names just removes some workshop specific stuff from the map name.    

### Example

Using Team Fortress 2 as an example.    
The raw workshop map name is `workshop/ctf_harbine.ugc3067683041`.    
The 'clean' name becomes: `ctf_harbine`    
The 'unique' name becomes: `ctf_harbine_ugc3067683041`    
