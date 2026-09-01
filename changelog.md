# Changelog

# v2.0.7-beta

**New Features:**
- Added a Status Overlay (like Label from Mega Hack: HUD with 18 elements)
- Added automatic disabling of Click Between Frames (CBF) when recording macros
**Fixes:**
- Improved Show Trajectory
- Fixed an issue where the level would unintentionally resume when pressing the spacebar while typing text while paused
**GUI:**
- Added Blur to the GUI. - Renamed Trajectory Prediction to Show Trajectory (WIP)
- Moved Speedhack and Speedhack Audio to the Player tab
- Reworked the Global tab
- Fixed closing the GUI with Insert / Escape / Tab keys when input fields are active and pressed
**Lua:**
- Added Lua API (see Lua API: https://demaxi.gitbook.io/neverhook-lua-api)
- Expanded API hooks to 52 game events (PlayLayer, PlayerObject, GJBaseGameLayer, etc.)
- Added node.valid() to check node validity after level restart
- Added gd.frame_time() and gd.fps() functions
- Updated exports of menu states and Status Overlay variables for interaction with Lua scripts
- Added cls to Lua (direct access to classes and fields) Games: PlayLayer, PlayerObject, GameManager, etc.)
- Added asynchronous loading of sprites by URL and from disk (node.sprite_file, draw.image_path)