# Nav Links

Links is a new form of connections between nav areas added to NavBot.

They allow two areas to be connected in ways that the standard connections doesn't allow.

# Commands

| Command | Arguments | Description |
|:---:|:---:|:---:|
| sm_nav_link_list_all | None | Lists all available link type. |
| sm_nav_link_connect | (Link type ID) | Creates a new nav link with the given type. |
| sm_nav_link_disconnect | (Link type ID) | Removes a nav link with the given type. |
| sm_nav_link_set_origin | None | Sets the starting point when creating new links. |
| sm_nav_link_warp_to_origin | None | Teleports you to the starting point used for new links. |

# Creating Links

First you must prepare the starting area and point.

Mark the starting area with `sm_nav_mark`, then go to where the link should start and use `sm_nav_link_set_origin` to mark the link starting point.

The area under your crosshair is selected as the destination area and your current position is used as the link end point.

Now use `sm_nav_link_connect <Link type ID>` to create the link.

# Removing Links

Mark the link's startng area, put your crosshair on the destination area and then use `sm_nav_link_disconnect <Link type ID>` to remove the link of given type.

# Current Link Types

Note: `sm_nav_link_list_all` will always list all available link types.

| Link Name | Link ID | Purpose |
|:---:|:---:|:---:|
| Ground | 1 | A simple ground link that tells the bots they can simply walk in a straight line. |
| Teleport | 2 | Bots can go to the other area via map based teleporters (trigger_teleport) entities. Very low cost to traverse. |
| Blast Jump | 3 | Bots can perform a blast jump (rocket jump/sticky jump/grenade jump) here. |
| Double Jump | 4 | Bots can perform a double jump here. |