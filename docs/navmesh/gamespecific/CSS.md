# Counter-Strike: Source Nav Mesh

## CSS Attributes

CSS attributes can be toggled on nav areas using the `sm_css_nav_toggle_attribute <attribute>` command.    
The following attributes are available:    

* `no_hostages`: Bots escorting hostages cannot use this area.
* `no_ts`: Bots on the terrorist team cannot use this area.
* `no_cts`: Bots on the counter-terrorist team cannot use this area.

Use `sm_css_nav_wipe_attribute` to remove all CSS attributes from areas.    

## ConVars

* `sm_css_nav_show_attributes`: Controls if CSS attributes should be draw when editing the nav mesh.

## Notes

* Bots escorting hostages won't use ladders.

