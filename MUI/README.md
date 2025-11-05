# NetShell GUI Application

## Note about MUI Implementation

After investigating the MorphOS system, I discovered there is already a working GUI implementation in the `gui_version/` directory that uses Intuition (Classic Amiga GUI) rather than MUI.

This existing Intuition-based implementation provides all the required functionality:
- Start and Stop buttons for server control
- Port configuration
- Status display
- Log output area
- Settings persistence

## MUI Widgets (For Reference)

For future MUI development on MorphOS, if needed:

### Menu System
- `MUI_MakeObject(MUIO_Menustrip)` - Creates the detached menu system similar to MacOS
- `NM_TITLE`, `NM_ITEM` - Menu titles and items

### Buttons
- `ButtongObject` - Standard MUI button widget for Start and Stop buttons

### Integer Port Input
- `IntegerObject` - MUI widget that restricts input to integer values only
- Configured with minimum of 1 and maximum of 65535 (valid TCP port range)
- Default value set to 2323 (the port used for testing)

### Labels
- `Label()` - Standard MUI label widget for the "Port:" label

### Logging Text Area
- `TextEditorObject` - Multi-line text display for logging
- `ScrollgroupObject` - Provides scrolling functionality for the text area
- Configured as read-only so users can't modify logs directly

## Recommendation

For this project, it's recommended to use the existing working implementation in `gui_version/` rather than developing a new MUI application, since:
1. The Intuition implementation already works
2. It provides all the required functionality
3. It avoids the complexity of MUI on MorphOS
4. It follows the established codebase pattern