# Action arguments

Send JSON to `POST /api/tool/<tool>` or use the same object as MCP tool
arguments. Read names and IDs from the corresponding list/describe operation.
IDs for UI controls and installation sessions belong to one MO2 process
lifetime.

## Mods and separators

| Action                          | Arguments beyond `action`                                           |
| ------------------------------- | ------------------------------------------------------------------- |
| `list`, `separators`            | None                                                                |
| `get`, `remove`                 | `name`                                                              |
| `create`, `createSeparator`     | `name`. Separator names gain `_separator` if absent                 |
| `rename`                        | `name`, `newName`                                                   |
| `setEnabled`                    | `name`, `enabled` boolean                                           |
| `setPriority`                   | `name`, zero-based `priority`                                       |
| `move`                          | `name`, `separator`. Moves the mod immediately after that separator |
| `metadata`                      | `name`, `values` object                                             |
| `addCategory`, `removeCategory` | `name`, `category` name                                             |

Writable metadata keys are `version`, `newestVersion`, `nexusId`, `game`, `url`,
`installationFile` and `comments` (the short text displayed in MO2's Notes
column). Reads also return notes, categories, archive IDs, flags, paths and
tracking/endorsement state exposed by MO2. Metadata edits update the live MO2
object. MO2 persists dirty metadata during its normal save/destruction
lifecycle, including normal shutdown.

Separator membership follows profile priority. Removing a separator leaves its
following mods installed. Use `scenario` for a series of moves. Moving each mod
immediately after a separator inserts it before the previous moved mod.

`remove` schedules MO2's native mod-list removal. Answer its confirmation
through `dialogs`, then poll `mods operationStatus`. The finished result
contains `name` and `removed`. Cancelling the confirmation returns
`removed: false`. Native removal handles profile indices and pending mod-list
writes before reporting completion.

`manage` takes a mod `name` and opens native mod information. Its operation
remains active until the editor closes. `editorState` returns the current mod,
HTML notes, short comments and the category tree with IDs, parent IDs and
assignments. `editorUpdate` accepts a `values` object with `notes` (HTML),
`comments`, `categories` (an ordered list of `{id, assigned}` changes) and
`primaryCategory` (an assigned category ID). MO2 applies these changes
immediately, and closing the editor persists them. There is no cancel/revert
operation in native mod information. `closeEditor` closes it and
`operationStatus` reports the final mod state. Normal mods and separators are
supported.

`setIgnoreUpdate` takes `name` and `enabled`. It invokes MO2's Ignore Update or
Un-ignore Update action and returns the resulting `ignoredVersion` through
`operationStatus`. MO2 offers Ignore Update only when a newer or older version
is known. Un-ignore is available while that version is ignored. An unavailable
action is reported as a failed operation. These context-menu operations select
and reveal the target row, so a mod excluded by active list filters must first
be made visible. `filters` returns current text, criteria and option choices.
`setFilters` takes a `filter` object with `text`, `mode` (`all`/`any`),
`grouping` and `separators` choice indexes, and `criteria` changes with each
reported `index` and `state` (`inactive`/`include`/`exclude`). `clearFilters`
invokes MO2's text and criteria reset. Separator visibility and grouping retain
their native settings.

## Category definitions

The `categories` tool manages the definitions shared by mod assignments. `list`
opens a temporary editor, reads the definitions and closes without applying
changes. `manage` opens the editor, and `editorState` reads staged rows. All
category operations are scheduled: poll `operationStatus` for their result.

`create` takes `values`. `update` takes a `row` from the latest editor response
and `values`. Writable fields are `id`, `name` and `parentId`. Numeric edits use
MO2's native validators. Use `parentId: 0` for a root category. Accept newly
created parents before assigning children, so the native editor's known-ID
validator includes them. Definition rows carry both `row` and `id`: MO2 can
contain duplicate category IDs, so definition edits use row positions. A
mutation returns fresh positions. Mod assignments use category IDs.

`remove` takes `row`. `setOrder` takes `rows`, containing every current row
position exactly once in the requested order. `setMappings` takes `row` and
Nexus `ids` discovered in `nexusCategories`. It replaces that row's mappings
through MO2's native removal and drop handlers. Nexus mappings must be enabled
in MO2 Settings. The native Refresh from Nexus and Import buttons are available
through `dialogs`.

`accept` commits and closes the editor. `cancel` discards staged changes.
Changing category IDs or removing definitions retains MO2's treatment of
existing mod assignments.

## Plugins

`list` returns every plugin, including disabled entries. `priority` is the
position in the full list. `loadOrder` is MO2's active index, with `-1` for
inactive entries. `forced` marks plugins whose state MO2 forces, such as the
game's own masters. `setEnabled` takes `name` and `enabled`, and fails for
forced plugins. `setPriority` takes `name` and `priority`. `setLoadOrder` takes
`names`, containing the entire current plugin set exactly once. MO2 still
applies game rules such as master ordering, so inspect the returned state.
Changes return after MO2 saves the profile's plugin list, so a following refresh
keeps them.

## Installers

`install start` takes either an absolute archive `path` or a `download` filename
returned by `downloads list`. Optional `name` suggests the mod name, and
optional `separator` selects final placement. It returns a running session
immediately, because an interactive native installer needs further requests
before it can finish. Only one installation session runs per process.

Use `dialogs describe` between installer steps. Its hierarchy includes control
IDs and parent IDs, page and group labels, option descriptions, image
references, checked state and enabled state. Operate returned IDs with `click`,
`setChecked` (`checked`), `setText` (`text`), `setValue` (`value`) or `select`
(`index`). `setValue` takes a boolean for a checkable button, a number for a
spin box, a choice's text for a combo box or text for a line edit. `cancel`
takes the top-level dialog `id` and rejects it. FOMOD choices use the
installer's own signals, dependency checks and conditional pages, and disabled
choices remain disabled.

Clicks on ordinary push buttons are scheduled so native child dialogs can open
without blocking the HTTP call. Inspect `dialogs` again after a scheduled click.

File dialogs also return `fileSelection`: directory, selected paths, filename
filters, selection mode and open/save mode. `selectFile` takes the dialog `id`
and `path`. Inspect the selected path before calling `accept` with the same
dialog ID. Acceptance is scheduled and preserves Qt's validation and overwrite
confirmation. `cancel` dismisses the picker.

API-driven editor operations and button clicks use Qt widget dialogs while their
native callbacks run. Windows-native file pickers expose no Qt child controls,
and programmatic acceptance closes such a picker without returning its selected
file to MO2. The scoped Qt preference keeps file selection automatable and
restores the previous dialog preference when the callback returns. Already-open
Windows-native pickers can be inspected and cancelled, but `selectFile` is
unavailable there.

`dialogs choices` groups selectable options by their native group and retains
navigation buttons. Optional `filter` matches group names, option text or
descriptions without changing selections. `page.index` is zero-based.
`definedPages` includes conditional pages that may be skipped. Selection rules
are included when exposed by the installer. `inViewport` distinguishes controls
below a scroll viewport, and `reveal` with a control `id` brings it into view.
`scroll` takes a scrollbar `id` and absolute `value` within its reported range.

Re-describe after navigation: controls from hidden or destroyed pages cannot be
operated. Check `install status` until it reports `completed`, `cancelled` or
`failed`. Completion includes the refresh after installation and separator
placement. Failed or cancelled installers may leave files according to the
native installer's merge, replacement and cancellation behavior. Inspect the
reported mod and MO2 state.

## Nexus content

`nexus description` takes an installed mod `name`, or a positive `modId` and
optional `game`. It retrieves the complete mod response through MO2, including
`description`, `summary`, author, version, image URL, timestamps, download
counts and endorsement data when returned by Nexus. Description markup is
preserved. `fileInfo` and `downloadUrls` additionally require `fileId`.

Network actions return `requestId` and `state: pending`. Poll `nexus status`
with that ID for `completed` and `result`, or `failed` and the repository error.
Omitting the ID lists requests in this process session. MO2 controls
authentication, queuing and rate limits. `nexus cached` with `name` reads all
installed `meta.ini` fields, current mod metadata and retained request results
without a network call. Use `downloads list` for downloaded file metadata,
including known Nexus file IDs.

MO2 2.5.2's bridge targets the managed game even when passed another game name,
so Dev Bench rejects that case. MO2 2.5.3beta12 supports the game argument. The
public file-list bridge in both releases emits expired file-object pointers, so
Dev Bench does not call it. Individual file details and the full mod description
use separate, working bridge responses.

## Captures

`capture kind=windows` lists visible windows in the selected MO2 process with
their title, owner ID and screen bounds. This includes installer windows with no
native owner and dialogs moved outside the main window. Windows belonging to
another MO2 process are excluded.

`capture kind=native checkpointId=<name>` captures the main window, or the
listed `window` id, as a PNG. Other plugins register
[capture providers](../plugin-authors/extensions.md#capture-providers).
`kind=<key>` asks that provider to write the image, and `kind=auto` uses the
only registered provider. `kind=providers` and `kind=extensions` list them.

Captures are written to
`<instance>/devbench/captures/<recording>/<variant>/<checkpointId>.png` with a
JSON sidecar of the same name, and publish `capture.saved`. `outDir`,
`recording` and `variant` choose the directory. Passing `golden` scores the
capture against a reference image with SSIM and adds
`{ssim, threshold, passed}`. `threshold` defaults to 0.98, and `regions` scores
0..1 UV rectangles independently. Native captures include MO2's UI, so they are
conclusive only with `excludeUi: false`.

MCP returns the image as native image content. HTTP returns the same `content`
array with base64 PNG data and `structuredContent` metadata. Minimized windows
must be restored before capture.

## Launch targets, downloads and files

`executables list` returns picker titles and an INI snapshot that may lag live
changes until MO2 writes its settings on shutdown. `snapshot` reads current live
configuration, including hidden entries, by briefly opening and closing the
native editor. Close any existing editor first. `manage` or `editorState` opens
the native editor and reads all entries, including hidden ones. `create` adds an
empty entry, `clone` copies the entry named by `name`, `remove` removes it, and
`update` changes its `values`. `addFromFile` opens MO2's file picker, including
its normal handling of Java executables. `reset` invokes MO2's reset operation.
`setOrder` takes `names` containing every editor title exactly once.

Editable `values` are `title`, `binary`, `workingDirectory`, raw `arguments`,
`overwriteSteamAppID`, `steamAppID`, `createFilesInMod`, `outputMod`,
`forceLoadLibraries`, `useApplicationIcon` and `hide`. MO2 2.5.3beta12 also
supports `minimizeToSystemTray`. It is omitted from 2.5.2 snapshots and cannot
be set there. Output-mod and library settings belong to the active profile.
Edits remain staged until `apply` or `accept` commits them to MO2's live
configuration, and `cancel` discards unapplied changes. Results include
`unappliedChanges` so a refused native commit is distinguishable from success.
Poll `operationStatus` after scheduled operations. `awaitingNativeDialog` means
the native operation is still running, possibly awaiting input, so inspect
`dialogs`. `finished` means the operation returned, not that staged changes were
saved.

The beta executable flags overlap: saving with `minimizeToSystemTray` enabled
also enables `useApplicationIcon`, and disabling it clears the icon flag. Editor
responses include this native constraint in `constraints`. Inspect returned
entries for MO2's resulting values.

`configureLibraries` opens the native library editor for `name`. `libraries`
reads saved rows and game-provided defaults for `name`. `setLibraries` replaces
the profile's saved rows with `libraries`, an array of
`{process, library, enabled}` objects, and optionally changes overall `enabled`.
Both actions briefly open the editor to validate the current live title, close
it, then read/write profile settings. Poll `operationStatus` for their result.
Apply or discard and close any existing executable editor first. Game-provided
rules remain supplied by MO2. A matching process/library row can override their
enabled state.

The supported MO2 releases leave the new native forced-library row's forced flag
uninitialized. `setLibraries` therefore writes MO2's profile QSettings schema
after live title validation instead of creating native rows. Existing rules
still use MO2's game-default merging behavior.

MO2 discards the force-load enabled flag when applying an executable with no
library rows. It then reads as enabled again, with no libraries to load. Read
`snapshot` after committing to inspect the resulting native configuration.
`setLibraries` can persist an explicit disabled flag with an empty list, but a
later native editor commit applies the same MO2 behavior.

`launch` takes `name` (configured title or executable path), optional `args`
string array, `cwd`, `profile` and `overwrite`, and returns the PID. `status`
reports running state and exit codes for tools launched by this plugin during
the current session. Each launch also emits `executables.started` and completion
emits `executables.finished` with its PID and exit code. Closing MO2 releases
their tracking handles and leaves the tools running.

`downloads list` reads downloaded archives and metadata. `start` takes `urls`,
`nexus` takes `modId` and `fileId`, and `path` takes a download manager `id`.
URL/Nexus operations use MO2's download manager and its existing authentication.

`files find` takes virtual `path` and optional `filters` globs. `directories`
takes `path`. `resolve` takes a virtual file `path` and returns its physical
path and provider mods.

## File conflicts

`mods list/get` includes `conflicts.flags`, `conflicts.redundant` and the native
Conflicts-column tooltip. `mods list` with `conflict: "redundant"` finds the
mods MO2 marks redundant. Other filter values identify loose, archive and mixed
conflict flags. Discover them in the tool schema.

These flags mirror MO2's cached left-pane classification. After hiding or
unhiding files, close the mod editor and use `settings refresh` before checking
the flags again. File-provider queries update during the editor operation,
before MO2 refreshes its conflict flags.

`conflicts inspect` schedules inspection of one or more `mods` by exact name, or
names matching `modFilter`. Omitting both selects the mod list. Inspect
`operationStatus` for the completed result. Each mod includes native conflict
paths, archive flags, provider text and a structured `providers` array with the
winner first. Other providers can be outside the selected set of mods.

`modOffset`/`modLimit` page the mod selection (10 by default). `offset`/`limit`
page each mod's files (100 by default). Returned next-offset fields are null on
the last page. `filter` uses MO2's native file filter. Views are `advanced`,
`winning`, `losing` and `unique`. Advanced inspection defaults to all providers
and conflicting files. `options` accepts `allProviders` and `includeUnique`.
Snapshots temporarily clear the main mod-list filters and restore them after
closing their editors.

For interactive resolution, open a mod with `mods manage`, then use
`conflicts list`. `hide` takes `paths` from that native view and invokes MO2's
Hide command. Archive members cannot be hidden individually. `preview` invokes
MO2's installed preview handler. Preview-specific automation belongs to the
preview plugin. Ordinary controls and window captures remain accessible through
Dev Bench. Poll `conflicts operationStatus` and answer any native dialogs before
continuing.

`directory` browses physical mod files using a relative `path`, including hidden
files and folders. `hideFiles` and `unhide` operate on those relative `paths`
through MO2's native file tree. They preserve native rename confirmations and
update the directory structure. Reinspect conflicts or `files resolve` to verify
the resulting winner. Changing `mods setPriority` changes loose-file priority.
Archive precedence follows the game's rules and plugin order.

`files info` pages one virtual directory and returns `path`, `resolvedPath`,
`archive` and `origins`. For archived files, `resolvedPath` can name a virtual
location inside a mod rather than an existing loose file. Enable archive parsing
through `settings` section `workarounds` and wait for the native refresh before
querying archive members.

## Profiles, settings and native commands

`profiles list` reads profile names, paths and flags. `select` takes an existing
profile `name`. `manage` opens the profile manager. `create`, `copy`, `rename`
and `remove` schedule the corresponding native operation, and `name` selects the
source profile for the latter three. Use `dialogs` for name entry, default-INI
selection and confirmation. Native restrictions on active profiles remain in
effect. `setLocalSaves` and `setLocalSettings` take `name` and `enabled` and
return the resulting checkbox state through `operationStatus`.

`transferSaves` with `name` opens MO2's native transfer dialog. Character and
save lists expose `items` and `index`, and `dialogs select` selects a character.
Its native copy/move buttons transfer all saves for that character between
global and profile storage, including associated files handled by MO2.
Confirmations and overwrite choices remain available through `dialogs`. Close
the profile manager to refresh the main profile picker after changes.

`settings list` at the root discovers sections: `general`, `theme`, `modList`,
`paths`, `nexus`, `plugins`, `workarounds` and `diagnostics`. Use separate
`section` and `action` arguments rather than compound action strings.

For native sections, `get` opens the section and returns its controls through
`operationStatus`. `set` takes a `values` object mapping discovered control
`objectName`s to scalar values, for example
`{"section":"workarounds","action":"set","values":{"enableArchiveParsingBox":true}}`.
Changes are staged until `apply`, which closes the dialog through MO2's native
acceptance flow. `cancel` discards staged changes. Inspect `dialogs` for native
confirmations and restart choices. Changing archive parsing triggers MO2's
directory refresh. It does not make archive analysis instantaneous. Theme and
path choices use the same native section workflow.

For `section: plugins`, `list` discovers plugin names, or setting keys when
`plugin` is provided. `get` takes `plugin` and `key`. `set` additionally takes a
JSON `value` and applies it through MO2's live plugin API. Results distinguish
`applied` and `staged`. Close the Settings dialog before a live plugin edit so
cached dialog values cannot overwrite it. Use `manage` with this section to
inspect MO2's native Plugins tab instead. `refresh` requests the native
asynchronous refresh.

`ui list` discovers native menu/toolbar actions, including enabled state and
shortcuts. `trigger` takes a returned `id` and schedules that native action. Use
`dialogs` for a resulting dialog. Actions can open editors, settings or other
tools. This does not provide arbitrary access to every custom third-party
widget.

## Logs

`logs list` returns available sources and log-file metadata without their
contents. `logs read` defaults to the selected process's live log and the last
10 entries. Set `lines` to any nonnegative count. There is no configured
maximum, and zero returns no entries. File reads take `source: file` and a
`file` name returned by `list`. They return the last requested lines and
truncation/count metadata. Files belong to the active MO2 instance directory and
can include earlier or concurrent processes. The live source belongs to the
selected process.

## Events and sequences

`events poll` takes optional `since`, and `wait` additionally accepts
`timeoutMs`. Advance the cursor to `headSeq`. `gap:true` means older events fell
out of the 1024-event history, so reread the affected state.

`scenario run` takes `steps`. Each step is either
`{"tool":"mods","args":{"action":"list"}}` or
`{"waitFor":"install.finished","timeoutMs":30000}`. Optional
`continueOnError:true` continues after failed steps. Results retain each step's
data/error and elapsed time. Native modal workflows still require dialog steps.
An installation start step alone does not finish an interactive installer.
